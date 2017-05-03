/*
 Copyright (C) 2016 Quaternion Risk Management Ltd
 All rights reserved.

 This file is part of ORE, a free-software/open-source library
 for transparent pricing and risk analysis - http://opensourcerisk.org

 ORE is free software: you can redistribute it and/or modify it
 under the terms of the Modified BSD License.  You should have received a
 copy of the license along with this program.
 The license is also available online at <http://opensourcerisk.org>

 This program is distributed on the basis that it will form a useful
 contribution to risk analytics and model standardisation, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. See the license for more details.
*/

/*! \file scenario/scenariosimmarket.cpp
    \brief A Market class that can be updated by Scenarios
    \ingroup
*/

#include <orea/scenario/scenariosimmarket.hpp>
#include <orea/engine/observationmode.hpp>
#include <ql/termstructures/volatility/swaption/swaptionvolstructure.hpp>
#include <ql/termstructures/volatility/swaption/swaptionvolmatrix.hpp>
#include <ql/termstructures/volatility/capfloor/capfloortermvolatilitystructure.hpp>
#include <ql/termstructures/volatility/capfloor/capfloortermvolsurface.hpp>
#include <ql/termstructures/volatility/optionlet/strippedoptionlet.hpp>
#include <ql/termstructures/volatility/optionlet/strippedoptionletadapter.hpp>
#include <ql/termstructures/defaulttermstructure.hpp>
#include <ql/termstructures/yield/discountcurve.hpp>
#include <ql/termstructures/credit/interpolatedsurvivalprobabilitycurve.hpp>
#include <ql/math/interpolations/loginterpolation.hpp>
#include <ql/termstructures/volatility/equityfx/blackvoltermstructure.hpp>
#include <ql/termstructures/volatility/equityfx/blackvariancecurve.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <ql/time/calendars/target.hpp>

#include <qle/termstructures/dynamicswaptionvolmatrix.hpp>
#include <qle/termstructures/dynamicblackvoltermstructure.hpp>
#include <qle/termstructures/swaptionvolatilityconverter.hpp>
#include <qle/termstructures/strippedoptionletadapter2.hpp>

#include <boost/timer.hpp>

#include <ored/utilities/log.hpp>

using namespace QuantLib;
using namespace QuantExt;
using namespace std;

namespace ore {
namespace analytics {

ReactionToTimeDecay parseDecayMode(const string& s) {
    static map<string, ReactionToTimeDecay> m = {{"ForwardVariance", ForwardForwardVariance},
                                                 {"ConstantVariance", ConstantVariance}};

    auto it = m.find(s);
    if (it != m.end()) {
        return it->second;
    } else {
        QL_FAIL("Decay mode \"" << s << "\" not recognized");
    }
}

ScenarioSimMarket::ScenarioSimMarket(boost::shared_ptr<ScenarioGenerator>& scenarioGenerator,
                                     boost::shared_ptr<Market>& initMarket,
                                     boost::shared_ptr<ScenarioSimMarketParameters>& parameters,
                                     Conventions conventions, const std::string& configuration)
    : SimMarket(conventions), scenarioGenerator_(scenarioGenerator), parameters_(parameters) {

    LOG("building ScenarioSimMarket...");
    asof_ = initMarket->asofDate();
    LOG("AsOf " << QuantLib::io::iso_date(asof_));

    // Build fixing manager
    fixingManager_ = boost::make_shared<FixingManager>(asof_);

    // constructing fxSpots_
    LOG("building FX triangulation..");
    for (const auto& ccyPair : parameters->fxCcyPairs()) {
        LOG("adding " << ccyPair << " FX rates");
        boost::shared_ptr<SimpleQuote> q(new SimpleQuote(initMarket->fxSpot(ccyPair, configuration)->value()));
        Handle<Quote> qh(q);
        fxSpots_[Market::defaultConfiguration].addQuote(ccyPair, qh);
        simData_.emplace(std::piecewise_construct, std::forward_as_tuple(RiskFactorKey::KeyType::FXSpot, ccyPair),
                         std::forward_as_tuple(q));
    }
    LOG("FX triangulation done");

    // constructing discount yield curves
    LOG("building discount yield curve times...");
    DayCounter dc = ActualActual();       // used to convert YieldCurve Periods to Times
    vector<Time> yieldCurveTimes(1, 0.0); // include today
    vector<Date> yieldCurveDates(1, asof_);
    QL_REQUIRE(parameters->yieldCurveTenors().front() > 0 * Days, "yield curve tenors must not include t=0");
    for (auto& tenor : parameters->yieldCurveTenors()) {
        LOG("Yield curve tenor " << tenor);
        yieldCurveTimes.push_back(dc.yearFraction(asof_, asof_ + tenor));
        yieldCurveDates.push_back(asof_ + tenor);
    }

    LOG("building discount yield curves...");
    for (const auto& ccy : parameters->ccys()) {
        LOG("building " << ccy << " discount yield curve..");
        Handle<YieldTermStructure> wrapper = initMarket->discountCurve(ccy, configuration);
        QL_REQUIRE(!wrapper.empty(), "discount curve for currency " << ccy << " not provided");
        // include today

        // constructing discount yield curves
        DayCounter dc = wrapper->dayCounter(); // used to convert YieldCurve Periods to Times
        vector<Time> yieldCurveTimes(1, 0.0);  // include today
        vector<Date> yieldCurveDates(1, asof_);
        QL_REQUIRE(parameters->yieldCurveTenors().front() > 0 * Days, "yield curve tenors must not include t=0");
        for (auto& tenor : parameters->yieldCurveTenors()) {
            yieldCurveTimes.push_back(dc.yearFraction(asof_, asof_ + tenor));
            yieldCurveDates.push_back(asof_ + tenor);
        }

        vector<Handle<Quote>> quotes;
        boost::shared_ptr<SimpleQuote> q(new SimpleQuote(1.0));
        quotes.push_back(Handle<Quote>(q));
        vector<Real> discounts(yieldCurveTimes.size());
        for (Size i = 0; i < yieldCurveTimes.size() - 1; i++) {
            boost::shared_ptr<SimpleQuote> q(new SimpleQuote(wrapper->discount(yieldCurveDates[i + 1])));
            Handle<Quote> qh(q);
            quotes.push_back(qh);

            simData_.emplace(std::piecewise_construct,
                             std::forward_as_tuple(RiskFactorKey::KeyType::DiscountCurve, ccy, i),
                             std::forward_as_tuple(q));

            LOG("SimMarket yield curve " << ccy << " discount[" << i << "]=" << q->value());
        }

        boost::shared_ptr<YieldTermStructure> discountCurve;

        if (ObservationMode::instance().mode() == ObservationMode::Mode::Unregister) {
            discountCurve = boost::shared_ptr<YieldTermStructure>(
                new QuantExt::InterpolatedDiscountCurve(yieldCurveTimes, quotes, 0, TARGET(), wrapper->dayCounter()));
        } else {
            discountCurve = boost::shared_ptr<YieldTermStructure>(
                new QuantExt::InterpolatedDiscountCurve2(yieldCurveTimes, quotes, wrapper->dayCounter()));
        }

        Handle<YieldTermStructure> dch(discountCurve);
        if (wrapper->allowsExtrapolation())
            dch->enableExtrapolation();
        discountCurves_.insert(
            pair<pair<string, string>, Handle<YieldTermStructure>>(make_pair(Market::defaultConfiguration, ccy), dch));
        LOG("building " << ccy << " discount yield curve done");
    }
    LOG("discount yield curves done");

    LOG("building benchmark yield curves...");
    for (const auto& name : parameters->yieldCurveNames()) {
        LOG("building benchmark yield curve name " << name);
        Handle<YieldTermStructure> wrapper = initMarket->yieldCurve(name, configuration);
        QL_REQUIRE(!wrapper.empty(), "yield curve for name " << name << " not provided");

        DayCounter dc = wrapper->dayCounter(); // used to convert YieldCurve Periods to Times
        vector<Time> yieldCurveTimes(1, 0.0);  // include today
        vector<Date> yieldCurveDates(1, asof_);
        QL_REQUIRE(parameters->yieldCurveTenors().front() > 0 * Days, "yield curve tenors must not include t=0");
        for (auto& tenor : parameters->yieldCurveTenors()) {
            yieldCurveTimes.push_back(dc.yearFraction(asof_, asof_ + tenor));
            yieldCurveDates.push_back(asof_ + tenor);
        }

        // include today
        vector<Handle<Quote>> quotes;
        boost::shared_ptr<SimpleQuote> q(new SimpleQuote(1.0));
        quotes.push_back(Handle<Quote>(q));
        vector<Real> discounts(yieldCurveTimes.size());
        for (Size i = 0; i < yieldCurveTimes.size() - 1; i++) {
            boost::shared_ptr<SimpleQuote> q(new SimpleQuote(wrapper->discount(yieldCurveDates[i + 1])));
            Handle<Quote> qh(q);
            quotes.push_back(qh);

            simData_.emplace(std::piecewise_construct,
                             std::forward_as_tuple(RiskFactorKey::KeyType::YieldCurve, name, i),
                             std::forward_as_tuple(q));

            LOG("SimMarket yield curve name " << name << " discount[" << i << "]=" << q->value());
        }

        boost::shared_ptr<YieldTermStructure> yieldCurve;

        if (ObservationMode::instance().mode() == ObservationMode::Mode::Unregister) {
            yieldCurve = boost::shared_ptr<YieldTermStructure>(
                new QuantExt::InterpolatedDiscountCurve(yieldCurveTimes, quotes, 0, TARGET(), wrapper->dayCounter()));
        } else {
            yieldCurve = boost::shared_ptr<YieldTermStructure>(
                new QuantExt::InterpolatedDiscountCurve2(yieldCurveTimes, quotes, wrapper->dayCounter()));
        }

        Handle<YieldTermStructure> dch(yieldCurve);
        if (wrapper->allowsExtrapolation())
            dch->enableExtrapolation();
        yieldCurves_.insert(
            pair<pair<string, string>, Handle<YieldTermStructure>>(make_pair(Market::defaultConfiguration, name), dch));
        LOG("building benchmark yield curve " << name << " done");
    }
    LOG("benchmark yield curves done");

    // building security spreads
    LOG("building security spreads...");
    for (const auto& name : parameters->securities()) {
        boost::shared_ptr<Quote> spreadQuote(new SimpleQuote(initMarket->securitySpread(name, configuration)->value()));
        securitySpreads_.insert(pair<pair<string, string>, Handle<Quote>>(make_pair(Market::defaultConfiguration, name),
                                                                          Handle<Quote>(spreadQuote)));
    }

    // constructing index curves
    LOG("building index curves...");
    for (const auto& ind : parameters->indices()) {
        LOG("building " << ind << " index curve");
        Handle<IborIndex> index = initMarket->iborIndex(ind, configuration);
        QL_REQUIRE(!index.empty(), "index object for " << ind << " not provided");
        Handle<YieldTermStructure> wrapperIndex = index->forwardingTermStructure();
        QL_REQUIRE(!wrapperIndex.empty(), "no termstructure for index " << ind);
        vector<string> keys(parameters->yieldCurveTenors().size());

        DayCounter dc = wrapperIndex->dayCounter(); // used to convert YieldCurve Periods to Times
        vector<Time> yieldCurveTimes(1, 0.0);       // include today
        vector<Date> yieldCurveDates(1, asof_);
        QL_REQUIRE(parameters->yieldCurveTenors().front() > 0 * Days, "yield curve tenors must not include t=0");
        for (auto& tenor : parameters->yieldCurveTenors()) {
            yieldCurveTimes.push_back(dc.yearFraction(asof_, asof_ + tenor));
            yieldCurveDates.push_back(asof_ + tenor);
        }

        // include today
        vector<Handle<Quote>> quotes;
        boost::shared_ptr<SimpleQuote> q(new SimpleQuote(1.0));
        quotes.push_back(Handle<Quote>(q));

        for (Size i = 0; i < yieldCurveTimes.size() - 1; i++) {
            boost::shared_ptr<SimpleQuote> q(new SimpleQuote(wrapperIndex->discount(yieldCurveDates[i + 1])));
            Handle<Quote> qh(q);
            quotes.push_back(qh);

            simData_.emplace(std::piecewise_construct,
                             std::forward_as_tuple(RiskFactorKey::KeyType::IndexCurve, ind, i),
                             std::forward_as_tuple(q));

            LOG("SimMarket index curve " << ind << " discount[" << i << "]=" << q->value());
        }
        // FIXME interpolation fixed to linear, added to xml??
        boost::shared_ptr<YieldTermStructure> indexCurve;
        if (ObservationMode::instance().mode() == ObservationMode::Mode::Unregister) {
            indexCurve = boost::shared_ptr<YieldTermStructure>(new QuantExt::InterpolatedDiscountCurve(
                yieldCurveTimes, quotes, 0, index->fixingCalendar(), wrapperIndex->dayCounter()));
        } else {
            indexCurve = boost::shared_ptr<YieldTermStructure>(
                new QuantExt::InterpolatedDiscountCurve2(yieldCurveTimes, quotes, wrapperIndex->dayCounter()));
        }

        // wrapped curve, is slower than a native curve
        // boost::shared_ptr<YieldTermStructure> correctedIndexCurve(
        //     new StaticallyCorrectedYieldTermStructure(
        //         discountCurves_[ccy], initMarket->discountCurve(ccy, configuration),
        //         wrapperIndex));

        Handle<YieldTermStructure> ich(indexCurve);
        // Handle<YieldTermStructure> ich(correctedIndexCurve);
        if (wrapperIndex->allowsExtrapolation())
            ich->enableExtrapolation();

        boost::shared_ptr<IborIndex> i(index->clone(ich));
        Handle<IborIndex> ih(i);
        iborIndices_.insert(
            pair<pair<string, string>, Handle<IborIndex>>(make_pair(Market::defaultConfiguration, ind), ih));
        LOG("building " << ind << " index curve done");
    }
    LOG("index curves done");

    // swap indices
    LOG("building swap indices...");
    for (const auto& it : parameters->swapIndices()) {
        const string& indexName = it.first;
        const string& discounting = it.second;
        LOG("Adding swap index " << indexName << " with discounting index " << discounting);

        addSwapIndex(indexName, discounting, Market::defaultConfiguration);
        LOG("Adding swap index " << indexName << " done.");
    }

    // constructing swaption volatility curves
    LOG("building swaption volatility curves...");
    for (const auto& ccy : parameters->swapVolCcys()) {
        LOG("building " << ccy << " swaption volatility curve...");
        RelinkableHandle<SwaptionVolatilityStructure> wrapper(*initMarket->swaptionVol(ccy, configuration));

        LOG("Initial market " << ccy << " swaption volatility type = " << wrapper->volatilityType());

        // Check that we have a swaption volatility matrix
        bool isMatrix = boost::dynamic_pointer_cast<SwaptionVolatilityMatrix>(*wrapper) != nullptr;

        // If swaption volatility type is not Normal, convert to Normal for the simulation
        if (wrapper->volatilityType() != Normal) {
            if (isMatrix) {
                // Get swap index associated with this volatility structure
                string swapIndexName = initMarket->swapIndexBase(ccy, configuration);
                Handle<SwapIndex> swapIndex = initMarket->swapIndex(swapIndexName, configuration);

                // Set up swaption volatility converter
                SwaptionVolatilityConverter converter(asof_, *wrapper, *swapIndex, Normal);
                wrapper.linkTo(converter.convert());

                LOG("Converting swaption volatilities in configuration " << configuration << " with currency " << ccy
                                                                         << " to normal swaption volatilities");
            } else {
                // Only support conversions for swaption volatility matrices
                LOG("Swaption volatility for ccy " << ccy << " is not a matrix so it is not converted to Normal");
            }
        }

        Handle<SwaptionVolatilityStructure> svp;
        if (parameters->simulateSwapVols()) {
            LOG("Simulating (normal) Swaption vols for ccy " << ccy);
            vector<Period> optionTenors = parameters->swapVolExpiries();
            vector<Period> swapTenors = parameters->swapVolTerms();
            vector<vector<Handle<Quote>>> quotes(optionTenors.size(),
                                                 vector<Handle<Quote>>(swapTenors.size(), Handle<Quote>()));
            vector<vector<Real>> shift(optionTenors.size(), vector<Real>(swapTenors.size(), 0.0));
            for (Size i = 0; i < optionTenors.size(); ++i) {
                for (Size j = 0; j < swapTenors.size(); ++j) {
                    Real strike = 0.0; // FIXME
                    Real vol = wrapper->volatility(optionTenors[i], swapTenors[j], strike);
                    boost::shared_ptr<SimpleQuote> q(new SimpleQuote(vol));
                    Size index = i * swapTenors.size() + j;
                    simData_.emplace(std::piecewise_construct,
                                     std::forward_as_tuple(RiskFactorKey::KeyType::SwaptionVolatility, ccy, index),
                                     std::forward_as_tuple(q));
                    quotes[i][j] = Handle<Quote>(q);
                    shift[i][j] = wrapper->shift(optionTenors[i], swapTenors[j]);
                }
            }
            bool flatExtrapolation = true; // FIXME: get this from curve configuration
            VolatilityType volType = wrapper->volatilityType();
            boost::shared_ptr<SwaptionVolatilityStructure> svolp(new SwaptionVolatilityMatrix(
                asof_, wrapper->calendar(), wrapper->businessDayConvention(), optionTenors, swapTenors, quotes,
                wrapper->dayCounter(), flatExtrapolation, volType, shift));
            svp = Handle<SwaptionVolatilityStructure>(svolp);
        } else {
            string decayModeString = parameters->swapVolDecayMode();
            ReactionToTimeDecay decayMode = parseDecayMode(decayModeString);
            boost::shared_ptr<QuantLib::SwaptionVolatilityStructure> svolp =
                boost::make_shared<QuantExt::DynamicSwaptionVolatilityMatrix>(*wrapper, 0, NullCalendar(), decayMode);
            svp = Handle<SwaptionVolatilityStructure>(svolp);
        }
        svp->enableExtrapolation(); // FIXME

        swaptionCurves_.insert(pair<pair<string, string>, Handle<SwaptionVolatilityStructure>>(
            make_pair(Market::defaultConfiguration, ccy), svp));

        LOG("Simulaton market " << ccy << " swaption volatility type = " << svp->volatilityType());

        string shortSwapIndexBase = initMarket->shortSwapIndexBase(ccy, configuration);
        string swapIndexBase = initMarket->swapIndexBase(ccy, configuration);
        swaptionIndexBases_.insert(pair<pair<string, string>, pair<string, string>>(
            make_pair(Market::defaultConfiguration, ccy), make_pair(shortSwapIndexBase, swapIndexBase)));
        swaptionIndexBases_.insert(pair<pair<string, string>, pair<string, string>>(
            make_pair(Market::defaultConfiguration, ccy), make_pair(swapIndexBase, swapIndexBase)));
    }

    LOG("swaption volatility curves done");

    // Constructing caplet/floorlet volatility surfaces
    LOG("building cap/floor volatility curves...");
    for (const auto& ccy : parameters->capFloorVolCcys()) {
        LOG("building " << ccy << " cap/floor volatility curve...");
        Handle<OptionletVolatilityStructure> wrapper = initMarket->capFloorVol(ccy, configuration);

        LOG("Initial market cap/floor volatility type = " << wrapper->volatilityType());

        Handle<OptionletVolatilityStructure> hCapletVol;

        if (parameters->simulateCapFloorVols()) {
            LOG("Simulating Cap/Floor Optionlet vols for ccy " << ccy);
            vector<Period> optionTenors = parameters->capFloorVolExpiries();
            vector<Date> optionDates(optionTenors.size());
            vector<Real> strikes = parameters->capFloorVolStrikes();
            vector<vector<Handle<Quote>>> quotes(optionTenors.size(),
                                                 vector<Handle<Quote>>(strikes.size(), Handle<Quote>()));
            for (Size i = 0; i < optionTenors.size(); ++i) {
                optionDates[i] = asof_ + optionTenors[i];
                for (Size j = 0; j < strikes.size(); ++j) {
                    Real vol = wrapper->volatility(optionTenors[i], strikes[j], wrapper->allowsExtrapolation());
                    boost::shared_ptr<SimpleQuote> q(new SimpleQuote(vol));
                    Size index = i * strikes.size() + j;
                    simData_.emplace(std::piecewise_construct,
                                     std::forward_as_tuple(RiskFactorKey::KeyType::OptionletVolatility, ccy, index),
                                     std::forward_as_tuple(q));
                    quotes[i][j] = Handle<Quote>(q);
                }
            }
            // FIXME: Works as of today only, i.e. for sensitivity/scenario analysis.
            // TODO: Build floating reference date StrippedOptionlet class for MC path generators
            boost::shared_ptr<StrippedOptionlet> optionlet = boost::make_shared<StrippedOptionlet>(
                0, // FIXME: settlement days
                wrapper->calendar(), wrapper->businessDayConvention(),
                boost::shared_ptr<IborIndex>(), // FIXME: required for ATM vol calculation
                optionDates, strikes, quotes, wrapper->dayCounter(), wrapper->volatilityType(),
                wrapper->displacement());
            boost::shared_ptr<StrippedOptionletAdapter2> adapter =
                boost::make_shared<StrippedOptionletAdapter2>(optionlet);
            hCapletVol = Handle<OptionletVolatilityStructure>(adapter);
        } else {
            string decayModeString = parameters->capFloorVolDecayMode();
            ReactionToTimeDecay decayMode = parseDecayMode(decayModeString);
            boost::shared_ptr<OptionletVolatilityStructure> capletVol =
                boost::make_shared<DynamicOptionletVolatilityStructure>(*wrapper, 0, NullCalendar(), decayMode);
            hCapletVol = Handle<OptionletVolatilityStructure>(capletVol);
        }

        capFloorCurves_.emplace(std::piecewise_construct, std::forward_as_tuple(Market::defaultConfiguration, ccy),
                                std::forward_as_tuple(hCapletVol));

        LOG("Simulaton market cap/floor volatility type = " << hCapletVol->volatilityType());
    }

    LOG("cap/floor volatility curves done");

    // building default curves
    LOG("building default curves...");
    for (const auto& name : parameters->defaultNames()) {
        LOG("building " << name << " default curve..");
        Handle<DefaultProbabilityTermStructure> wrapper = initMarket->defaultCurve(name, configuration);
        vector<Handle<Quote>> quotes;
        vector<Probability> probs;

        QL_REQUIRE(parameters->defaultTenors().front() > 0 * Days, "default curve tenors must not include t=0");

        vector<Date> dates(1, asof_);
        for (Size i = 0; i < parameters->defaultTenors().size(); i++) {
            dates.push_back(asof_ + parameters->defaultTenors()[i]);
        }

        for (const auto& date : dates) {
            Probability prob = wrapper->survivalProbability(date, true);
            boost::shared_ptr<SimpleQuote> q(new SimpleQuote(prob));
            Handle<Quote> qh(q);
            quotes.push_back(qh);
            probs.push_back(prob);
        }

        // FIXME riskmarket uses SurvivalProbabilityCurve but this isn't added to ore
        boost::shared_ptr<DefaultProbabilityTermStructure> defaultCurve(
            new InterpolatedSurvivalProbabilityCurve<Linear>(dates, probs, wrapper->dayCounter(), wrapper->calendar()));
        Handle<DefaultProbabilityTermStructure> dch(defaultCurve);
        if (wrapper->allowsExtrapolation())
            dch->enableExtrapolation();

        defaultCurves_.insert(pair<pair<string, string>, Handle<DefaultProbabilityTermStructure>>(
            make_pair(Market::defaultConfiguration, name), dch));

        // add recovery rate
        boost::shared_ptr<Quote> rrQuote(new SimpleQuote(initMarket->recoveryRate(name, configuration)->value()));
        recoveryRates_.insert(pair<pair<string, string>, Handle<Quote>>(make_pair(Market::defaultConfiguration, name),
                                                                        Handle<Quote>(rrQuote)));
    }
    LOG("default curves done");

    // building fx volatilities
    LOG("building fx volatilities...");
    for (const auto& ccyPair : parameters->fxVolCcyPairs()) {
        Handle<BlackVolTermStructure> wrapper = initMarket->fxVol(ccyPair, configuration);

        Handle<BlackVolTermStructure> fvh;

        if (parameters->simulateFXVols()) {
            LOG("Simulating FX Vols (BlackVarianceCurve3) for " << ccyPair);

            vector<Handle<Quote>> quotes;
            vector<Time> times;
            for (Size i = 0; i < parameters->fxVolExpiries().size(); i++) {
                Date date = asof_ + parameters->fxVolExpiries()[i];
                Volatility vol = wrapper->blackVol(date, Null<Real>(), true);
                times.push_back(wrapper->timeFromReference(date));
                boost::shared_ptr<SimpleQuote> q(new SimpleQuote(vol));
                simData_.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(RiskFactorKey::KeyType::FXVolatility, ccyPair, i),
                                 std::forward_as_tuple(q));
                quotes.emplace_back(q);
            }

            boost::shared_ptr<BlackVolTermStructure> fxVolCurve(new BlackVarianceCurve3(
                0, NullCalendar(), wrapper->businessDayConvention(), wrapper->dayCounter(), times, quotes));

            fvh = Handle<BlackVolTermStructure>(fxVolCurve);

        } else {
            string decayModeString = parameters->fxVolDecayMode();
            LOG("Deterministic FX Vols with decay mode " << decayModeString << " for " << ccyPair);
            ReactionToTimeDecay decayMode = parseDecayMode(decayModeString);

            // currently only curves (i.e. strike indepdendent) FX volatility structures are
            // supported, so we use a) the more efficient curve tag and b) a hard coded sticky
            // strike stickyness, since then no yield term structures and no fx spot are required
            // that define the ATM level - to be revisited when FX surfaces are supported
            fvh = Handle<BlackVolTermStructure>(boost::make_shared<QuantExt::DynamicBlackVolTermStructure<tag::curve>>(
                wrapper, 0, NullCalendar(), decayMode, StickyStrike));
        }

        if (wrapper->allowsExtrapolation())
            fvh->enableExtrapolation();
        fxVols_.insert(pair<pair<string, string>, Handle<BlackVolTermStructure>>(
            make_pair(Market::defaultConfiguration, ccyPair), fvh));

        // build inverted surface
        QL_REQUIRE(ccyPair.size() == 6, "Invalid Ccy pair " << ccyPair);
        string reverse = ccyPair.substr(3) + ccyPair.substr(0, 3);
        Handle<QuantLib::BlackVolTermStructure> ifvh(boost::make_shared<BlackInvertedVolTermStructure>(fvh));
        if (fvh->allowsExtrapolation())
            ifvh->enableExtrapolation();
        fxVols_.insert(pair<pair<string, string>, Handle<BlackVolTermStructure>>(
            make_pair(Market::defaultConfiguration, reverse), ifvh));
    }
    LOG("fx volatilities done");

    // building equity spots
    LOG("building equity spots...");
    for (const auto& eqName : parameters->equityNames()) {
        Real spotVal = initMarket->equitySpot(eqName, configuration)->value();
        DLOG("adding " << eqName << " equity spot price");
        boost::shared_ptr<SimpleQuote> q(new SimpleQuote(spotVal));
        Handle<Quote> qh(q);
        equitySpots_.insert(
            pair<pair<string, string>, Handle<Quote>>(make_pair(Market::defaultConfiguration, eqName), qh));
        simData_.emplace(std::piecewise_construct, std::forward_as_tuple(RiskFactorKey::KeyType::EQSpot, eqName),
                         std::forward_as_tuple(q));
    }
    LOG("equity spots done");

    vector<Time> equityCurveTimes(1, 0.0);
    vector<Date> equityCurveDates(1, asof_);
    if (parameters->equityNames().size() > 0) {
        QL_REQUIRE(parameters->equityTenors().size() > 0, "Equity curve tenor grid not defined");
        QL_REQUIRE(parameters->equityTenors().front() > 0 * Days, "equity curve tenors must not include t=0");
    }
    for (auto& tenor : parameters->equityTenors()) {
        equityCurveTimes.push_back(dc.yearFraction(asof_, asof_ + tenor));
        equityCurveDates.push_back(asof_ + tenor);
    }

    // building equity dividend yield curves
    LOG("building equity dividend yield curves...");
    for (const auto& eqName : parameters->equityNames()) {
        DLOG("building " << eqName << " equity dividend yield curve..");
        Handle<YieldTermStructure> wrapper = initMarket->equityDividendCurve(eqName, configuration);
        vector<Handle<Quote>> quotes;
        boost::shared_ptr<SimpleQuote> q(new SimpleQuote(1.0));
        quotes.push_back(Handle<Quote>(q));

        for (Size i = 0; i < equityCurveTimes.size() - 1; i++) {
            boost::shared_ptr<SimpleQuote> q(new SimpleQuote(wrapper->discount(equityCurveTimes[i + 1])));
            Handle<Quote> qh(q);
            quotes.push_back(qh);
        }
        boost::shared_ptr<YieldTermStructure> eqdivCurve;
        if (ObservationMode::instance().mode() == ObservationMode::Mode::Unregister) {
            eqdivCurve = boost::shared_ptr<YieldTermStructure>(new QuantExt::InterpolatedDiscountCurve(
                equityCurveTimes, quotes, 0, wrapper->calendar(), wrapper->dayCounter()));
        } else {
            eqdivCurve = boost::shared_ptr<YieldTermStructure>(
                new QuantExt::InterpolatedDiscountCurve2(equityCurveTimes, quotes, wrapper->dayCounter()));
        }
        Handle<YieldTermStructure> eqdiv_h(eqdivCurve);
        if (wrapper->allowsExtrapolation())
            eqdiv_h->enableExtrapolation();

        equityDividendCurves_.insert(pair<pair<string, string>, Handle<YieldTermStructure>>(
            make_pair(Market::defaultConfiguration, eqName), eqdiv_h));
        DLOG("building " << eqName << " equity dividend yield curve done");
    }
    LOG("equity dividend yield curves done");

    // building eq volatilities
    LOG("building eq volatilities...");
    for (const auto& eqName : parameters->eqVolNames()) {
        Handle<BlackVolTermStructure> wrapper = initMarket->equityVol(eqName, configuration);

        Handle<BlackVolTermStructure> evh;

        if (parameters->simulateEQVols()) {
            LOG("Simulating EQ Vols (BlackVarianceCurve3) for " << eqName);
            vector<Handle<Quote>> quotes;
            vector<Time> times;
            for (Size i = 0; i < parameters->eqVolExpiries().size(); i++) {
                Date date = asof_ + parameters->eqVolExpiries()[i];
                Volatility vol = wrapper->blackVol(date, Null<Real>(), true);
                times.push_back(wrapper->timeFromReference(date));
                boost::shared_ptr<SimpleQuote> q(new SimpleQuote(vol));
                simData_.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(RiskFactorKey::KeyType::EQVolatility, eqName, i),
                                 std::forward_as_tuple(q));
                quotes.emplace_back(q);
            }
            boost::shared_ptr<BlackVolTermStructure> eqVolCurve(new BlackVarianceCurve3(
                0, NullCalendar(), wrapper->businessDayConvention(), wrapper->dayCounter(), times, quotes));
            evh = Handle<BlackVolTermStructure>(eqVolCurve);
        } else {
            string decayModeString = parameters->eqVolDecayMode();
            DLOG("Deterministic EQ Vols with decay mode " << decayModeString << " for " << eqName);
            ReactionToTimeDecay decayMode = parseDecayMode(decayModeString);

            // currently only curves (i.e. strike indepdendent) EQ volatility structures are
            // supported, so we use a) the more efficient curve tag and b) a hard coded sticky
            // strike stickyness, since then no yield term structures and no EQ spot are required
            // that define the ATM level - to be revisited when EQ surfaces are supported
            evh = Handle<BlackVolTermStructure>(boost::make_shared<QuantExt::DynamicBlackVolTermStructure<tag::curve>>(
                wrapper, 0, NullCalendar(), decayMode, StickyStrike));
        }
        if (wrapper->allowsExtrapolation())
            evh->enableExtrapolation();
        equityVols_.insert(pair<pair<string, string>, Handle<BlackVolTermStructure>>(
            make_pair(Market::defaultConfiguration, eqName), evh));
        DLOG("EQ volatility curve built for " << eqName);
    }
    LOG("equity volatilities done");
}

void ScenarioSimMarket::update(const Date& d) {
    // DLOG("ScenarioSimMarket::update called with Date " << QuantLib::io::iso_date(d));

    ObservationMode::Mode om = ObservationMode::instance().mode();
    if (om == ObservationMode::Mode::Disable)
        ObservableSettings::instance().disableUpdates(false);
    else if (om == ObservationMode::Mode::Defer)
        ObservableSettings::instance().disableUpdates(true);

    boost::shared_ptr<Scenario> scenario = scenarioGenerator_->next(d);

    numeraire_ = scenario->getNumeraire();

    if (d != Settings::instance().evaluationDate())
        Settings::instance().evaluationDate() = d;
    else if (om == ObservationMode::Mode::Unregister) {
        // Due to some of the notification chains having been unregistered,
        // it is possible that some lazy objects might be missed in the case
        // that the evaluation date has not been updated. Therefore, we
        // manually kick off an observer notification from this level.
        // We have unit regression tests in OREAnalyticsTestSuite to ensure
        // the various ObservationMode settings return the anticipated results.
        boost::shared_ptr<QuantLib::Observable> obs = QuantLib::Settings::instance().evaluationDate();
        obs->notifyObservers();
    }

    const vector<RiskFactorKey>& keys = scenario->keys();

    Size count = 0;
    bool missingPoint = false;
    for (const auto& key : keys) {
        // TODO: Is this really an error?
        auto it = simData_.find(key);
        if (it == simData_.end()) {
            ALOG("simulation data point missing for key " << key);
            missingPoint = true;
        } else {
            // LOG("simulation data point found for key " << key);
            it->second->setValue(scenario->get(key));
            count++;
        }
    }
    QL_REQUIRE(!missingPoint, "simulation data points missing from scenario, exit.");

    if (count != simData_.size()) {
        ALOG("mismatch between scenario and sim data size, " << count << " vs " << simData_.size());
        for (auto it : simData_) {
            if (!scenario->has(it.first))
                ALOG("Key " << it.first << " missing in scenario");
        }
        QL_FAIL("mismatch between scenario and sim data size, exit.");
    }

    // Observation Mode - key to update these before fixings are set
    if (om == ObservationMode::Mode::Disable) {
        refresh();
        ObservableSettings::instance().enableUpdates();
    } else if (om == ObservationMode::Mode::Defer) {
        ObservableSettings::instance().enableUpdates();
    }

    // Apply fixings as historical fixings. Must do this before we populate ASD
    fixingManager_->update(d);

    if (asd_) {
        // add additional scenario data to the given container, if required
        for (auto i : parameters_->additionalScenarioDataIndices())
            asd_->set(iborIndex(i)->fixing(d), AggregationScenarioDataType::IndexFixing, i);

        for (auto c : parameters_->additionalScenarioDataCcys()) {
            if (c != parameters_->baseCcy())
                asd_->set(fxSpot(c + parameters_->baseCcy())->value(), AggregationScenarioDataType::FXSpot, c);
        }

        asd_->set(numeraire_, AggregationScenarioDataType::Numeraire);

        asd_->next();
    }

    // DLOG("ScenarioSimMarket::update done");
}
}
}
