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

#include <set>

#include <orea/scenario/scenariosimmarketparameters.hpp>
#include <ored/utilities/log.hpp>
#include <ored/utilities/xmlutils.hpp>

#include <boost/lexical_cast.hpp>

using namespace QuantLib;
using namespace ore::data;
using std::set;

namespace ore {
namespace analytics {

namespace {
const vector<Period>& returnTenors(const map<string, vector<Period>>& m, const string& k) {
    if (m.count(k) > 0) {
        return m.at(k);
    } else if (m.count("") > 0) {
        return m.at("");
    } else
        QL_FAIL("no period vector for key \"" << k << "\" found.");
}
const string& returnDayCounter(const map<string, string>& m, const string& k) {
    if (m.count(k) > 0) {
        return m.at(k);
    } else if (m.count("") > 0) {
        return m.at("");
    } else
        QL_FAIL("no dayCounter for key \"" << k << "\" found.");
}

} // namespace

vector<string> ScenarioSimMarketParameters::paramsLookup(RiskFactorKey::KeyType kt) const {
    vector<string> names;
    auto it = params_.find(kt);
    if (it != params_.end()) {
        for (auto n : it->second.second)
            names.push_back(n);
    }
    return names;
}

bool ScenarioSimMarketParameters::hasParamsName(RiskFactorKey::KeyType kt, string name) const {
    auto it = params_.find(kt);
    if (it != params_.end()) {
        return std::find(it->second.second.begin(), it->second.second.end(), name) == it->second.second.end() ? false
                                                                                                              : true;
    }
    return false;
}

void ScenarioSimMarketParameters::addParamsName(RiskFactorKey::KeyType kt, vector<string> names) {
    // check if key type exists - if doesn't exist set simulate to true first
    if (names.size() > 0) {
        auto it = params_.find(kt);
        if (it == params_.end())
            params_[kt].first = true;
        for (auto name : names) {
            if (!hasParamsName(kt, name))
                params_[kt].second.insert(name);
        }
    }
}

bool ScenarioSimMarketParameters::paramsSimulate(RiskFactorKey::KeyType kt) const {
    bool simulate = false;
    auto it = params_.find(kt);
    if (it != params_.end())
        simulate = it->second.first;
    return simulate;
}

void ScenarioSimMarketParameters::setParamsSimulate(RiskFactorKey::KeyType kt, bool simulate) {
    params_[kt].first = simulate;
}

void ScenarioSimMarketParameters::setDefaults() {
    // Set default simulate
    setSimulateDividendYield(false);
    setSimulateSwapVols(false);
    setSimulateYieldVols(false);
    setSimulateCapFloorVols(false);
    setSimulateYoYInflationCapFloorVols(false);
    setSimulateSurvivalProbabilities(false);
    setSimulateRecoveryRates(false);
    setSimulateCdsVols(false);
    setSimulateFXVols(false);
    setSimulateEquityVols(false);
    setSimulateBaseCorrelations(false);
    setCommodityCurveSimulate(false);
    setCommodityVolSimulate(false);
    setSecuritySpreadsSimulate(false);
    setSimulateFxSpots(true);
    setSimulateCorrelations(false);

    // Set default tenors (don't know why but keep it as is)
    capFloorVolExpiries_[""] = vector<Period>();
    yoyInflationCapFloorVolExpiries_[""] = vector<Period>();
    defaultTenors_[""] = vector<Period>();
    equityDividendTenors_[""] = vector<Period>();
    zeroInflationTenors_[""] = vector<Period>();
    yoyInflationTenors_[""] = vector<Period>();
    commodityCurveTenors_[""] = vector<Period>();
    // Default day counters
    yieldCurveDayCounters_[""] = "A365";
    swapVolDayCounters_[""] = "A365";
    yieldVolDayCounters_[""] = "A365";
    fxVolDayCounters_[""] = "A365";
    cdsVolDayCounters_[""] = "A365";
    equityVolDayCounters_[""] = "A365";
    capFloorVolDayCounters_[""] = "A365";
    yoyInflationCapFloorVolDayCounters_[""] = "A365";
    defaultCurveDayCounters_[""] = "A365";
    baseCorrelationDayCounters_[""] = "A365";
    zeroInflationDayCounters_[""] = "A365";
    yoyInflationDayCounters_[""] = "A365";
    commodityCurveDayCounters_[""] = "A365";
    commodityVolDayCounters_[""] = "A365";
    correlationDayCounters_[std::make_pair("", "")] = "A365";
    // Default calendars
    defaultCurveCalendars_[""] = "TARGET";
}

void ScenarioSimMarketParameters::reset() {
    ScenarioSimMarketParameters ssmp;
    std::swap(*this, ssmp);
}

const vector<Period>& ScenarioSimMarketParameters::yieldCurveTenors(const string& key) const {
    return returnTenors(yieldCurveTenors_, key);
}

const string& ScenarioSimMarketParameters::yieldCurveDayCounter(const string& key) const {
    return returnDayCounter(yieldCurveDayCounters_, key);
}

const vector<Period>& ScenarioSimMarketParameters::capFloorVolExpiries(const string& key) const {
    return returnTenors(capFloorVolExpiries_, key);
}

const vector<Period>& ScenarioSimMarketParameters::yoyInflationCapFloorVolExpiries(const string& key) const {
    return returnTenors(yoyInflationCapFloorVolExpiries_, key);
}

const vector<Period>& ScenarioSimMarketParameters::defaultTenors(const string& key) const {
    return returnTenors(defaultTenors_, key);
}

const string& ScenarioSimMarketParameters::defaultCurveDayCounter(const string& key) const {
    return returnDayCounter(defaultCurveDayCounters_, key);
}

const string& ScenarioSimMarketParameters::defaultCurveCalendar(const string& key) const {
    return returnDayCounter(defaultCurveCalendars_, key);
}

const string& ScenarioSimMarketParameters::swapVolDayCounter(const string& key) const {
    return returnDayCounter(swapVolDayCounters_, key);
}

const string& ScenarioSimMarketParameters::yieldVolDayCounter(const string& key) const {
    return returnDayCounter(yieldVolDayCounters_, key);
}

const string& ScenarioSimMarketParameters::fxVolDayCounter(const string& key) const {
    return returnDayCounter(fxVolDayCounters_, key);
}

const string& ScenarioSimMarketParameters::cdsVolDayCounter(const string& key) const {
    return returnDayCounter(cdsVolDayCounters_, key);
}

const string& ScenarioSimMarketParameters::equityVolDayCounter(const string& key) const {
    return returnDayCounter(equityVolDayCounters_, key);
}

const string& ScenarioSimMarketParameters::capFloorVolDayCounter(const string& key) const {
    return returnDayCounter(capFloorVolDayCounters_, key);
}

const string& ScenarioSimMarketParameters::yoyInflationCapFloorVolDayCounter(const string& key) const {
    return returnDayCounter(yoyInflationCapFloorVolDayCounters_, key);
}

const vector<Period>& ScenarioSimMarketParameters::equityDividendTenors(const string& key) const {
    return returnTenors(equityDividendTenors_, key);
}

const vector<Period>& ScenarioSimMarketParameters::zeroInflationTenors(const string& key) const {
    return returnTenors(zeroInflationTenors_, key);
}

const string& ScenarioSimMarketParameters::zeroInflationDayCounter(const string& key) const {
    return returnDayCounter(zeroInflationDayCounters_, key);
}

const vector<Period>& ScenarioSimMarketParameters::yoyInflationTenors(const string& key) const {
    return returnTenors(yoyInflationTenors_, key);
}

const string& ScenarioSimMarketParameters::yoyInflationDayCounter(const string& key) const {
    return returnDayCounter(yoyInflationDayCounters_, key);
}

const string& ScenarioSimMarketParameters::baseCorrelationDayCounter(const string& key) const {
    return returnDayCounter(baseCorrelationDayCounters_, key);
}

vector<string> ScenarioSimMarketParameters::commodityNames() const {
    return paramsLookup(RiskFactorKey::KeyType::CommoditySpot);
}

const vector<Period>& ScenarioSimMarketParameters::commodityCurveTenors(const string& commodityName) const {
    return returnTenors(commodityCurveTenors_, commodityName);
}

bool ScenarioSimMarketParameters::hasCommodityCurveTenors(const string& commodityName) const {
    return commodityCurveTenors_.count(commodityName) > 0;
}

const string& ScenarioSimMarketParameters::commodityCurveDayCounter(const string& commodityName) const {
    return returnDayCounter(commodityCurveDayCounters_, commodityName);
}

const vector<Period>& ScenarioSimMarketParameters::commodityVolExpiries(const string& commodityName) const {
    return returnTenors(commodityVolExpiries_, commodityName);
}

const vector<Real>& ScenarioSimMarketParameters::commodityVolMoneyness(const string& commodityName) const {
    if (commodityVolMoneyness_.count(commodityName) > 0) {
        return commodityVolMoneyness_.at(commodityName);
    } else {
        QL_FAIL("no moneyness for commodity \"" << commodityName << "\" found.");
    }
}

const string& ScenarioSimMarketParameters::correlationDayCounter(const string& index1, const string& index2) const {
    pair<string, string> p(index1, index2);

    if (correlationDayCounters_.count(p) > 0) {
        return correlationDayCounters_.at(p);
    } else if (correlationDayCounters_.count(std::make_pair("", "")) > 0) {
        return correlationDayCounters_.at(std::make_pair("", ""));
    } else
        QL_FAIL("no dayCounter for key \"" << index1 << ":" << index2 << "\" found.");
}

const string& ScenarioSimMarketParameters::commodityVolDayCounter(const string& commodityName) const {
    return returnDayCounter(commodityVolDayCounters_, commodityName);
}

void ScenarioSimMarketParameters::setYieldCurveTenors(const string& key, const std::vector<Period>& p) {
    yieldCurveTenors_[key] = p;
}

void ScenarioSimMarketParameters::setYieldCurveDayCounters(const string& key, const string& s) {
    yieldCurveDayCounters_[key] = s;
}

void ScenarioSimMarketParameters::setCapFloorVolExpiries(const string& key, const std::vector<Period>& p) {
    capFloorVolExpiries_[key] = p;
}

void ScenarioSimMarketParameters::setDefaultTenors(const string& key, const std::vector<Period>& p) {
    defaultTenors_[key] = p;
}

void ScenarioSimMarketParameters::setDefaultCurveDayCounters(const string& key, const string& s) {
    defaultCurveDayCounters_[key] = s;
}

void ScenarioSimMarketParameters::setDefaultCurveCalendars(const string& key, const string& s) {
    defaultCurveCalendars_[key] = s;
}

void ScenarioSimMarketParameters::setBaseCorrelationDayCounters(const string& key, const string& s) {
    baseCorrelationDayCounters_[key] = s;
}

void ScenarioSimMarketParameters::setEquityDividendTenors(const string& key, const std::vector<Period>& p) {
    equityDividendTenors_[key] = p;
}

void ScenarioSimMarketParameters::setZeroInflationTenors(const string& key, const std::vector<Period>& p) {
    zeroInflationTenors_[key] = p;
}

void ScenarioSimMarketParameters::setZeroInflationDayCounters(const string& key, const string& s) {
    zeroInflationDayCounters_[key] = s;
}

void ScenarioSimMarketParameters::setYoyInflationTenors(const string& key, const std::vector<Period>& p) {
    yoyInflationTenors_[key] = p;
}

void ScenarioSimMarketParameters::setYoyInflationDayCounters(const string& key, const string& s) {
    yoyInflationDayCounters_[key] = s;
}

void ScenarioSimMarketParameters::setFxVolDayCounters(const string& key, const string& s) {
    fxVolDayCounters_[key] = s;
}

void ScenarioSimMarketParameters::setSwapVolDayCounters(const string& key, const string& s) {
    swapVolDayCounters_[key] = s;
}

void ScenarioSimMarketParameters::setYieldVolDayCounters(const string& key, const string& s) {
    yieldVolDayCounters_[key] = s;
}

void ScenarioSimMarketParameters::setCdsVolDayCounters(const string& key, const string& s) {
    cdsVolDayCounters_[key] = s;
}

void ScenarioSimMarketParameters::setEquityVolDayCounters(const string& key, const string& s) {
    equityVolDayCounters_[key] = s;
}

void ScenarioSimMarketParameters::setCapFloorVolDayCounters(const string& key, const string& s) {
    capFloorVolDayCounters_[key] = s;
}

void ScenarioSimMarketParameters::setCommodityNames(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::CommoditySpot, names);
    setCommodityCurves(names);
}

void ScenarioSimMarketParameters::setCommodityCurveTenors(const string& commodityName, const vector<Period>& p) {
    commodityCurveTenors_[commodityName] = p;
}

void ScenarioSimMarketParameters::setCommodityCurveDayCounter(const string& commodityName, const string& d) {
    commodityCurveDayCounters_[commodityName] = d;
}

void ScenarioSimMarketParameters::setCommodityVolDayCounter(const string& commodityName, const string& d) {
    commodityVolDayCounters_[commodityName] = d;
}

void ScenarioSimMarketParameters::setDiscountCurveNames(vector<string> names) {
    ccys_ = names;
    addParamsName(RiskFactorKey::KeyType::DiscountCurve, names);
}

void ScenarioSimMarketParameters::setYieldCurveNames(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::YieldCurve, names);
}

void ScenarioSimMarketParameters::setIndices(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::IndexCurve, names);
}

void ScenarioSimMarketParameters::setFxCcyPairs(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::FXSpot, names);
}

void ScenarioSimMarketParameters::setSwapVolCcys(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::SwaptionVolatility, names);
}

void ScenarioSimMarketParameters::setYieldVolNames(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::YieldVolatility, names);
}

void ScenarioSimMarketParameters::setCapFloorVolCcys(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::OptionletVolatility, names);
}

void ScenarioSimMarketParameters::setYoYInflationCapFloorNames(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::YoYInflationCapFloorVolatility, names);
}

void ScenarioSimMarketParameters::setDefaultNames(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::SurvivalProbability, names);
    setRecoveryRates(names);
}

void ScenarioSimMarketParameters::setCdsVolNames(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::CDSVolatility, names);
}

void ScenarioSimMarketParameters::setEquityNames(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::EquitySpot, names);
    setEquityDividendCurves(names);
}

void ScenarioSimMarketParameters::setEquityDividendCurves(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::DividendYield, names);
}

void ScenarioSimMarketParameters::setFxVolCcyPairs(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::FXVolatility, names);
}

void ScenarioSimMarketParameters::setEquityVolNames(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::EquityVolatility, names);
}

void ScenarioSimMarketParameters::setSecurities(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::SecuritySpread, names);
}

void ScenarioSimMarketParameters::setRecoveryRates(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::RecoveryRate, names);
}

void ScenarioSimMarketParameters::setBaseCorrelationNames(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::BaseCorrelation, names);
}

void ScenarioSimMarketParameters::setCpiIndices(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::CPIIndex, names);
}

void ScenarioSimMarketParameters::setZeroInflationIndices(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::ZeroInflationCurve, names);
}

void ScenarioSimMarketParameters::setYoyInflationIndices(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::YoYInflationCurve, names);
}

void ScenarioSimMarketParameters::setCommodityVolNames(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::CommodityVolatility, names);
}

void ScenarioSimMarketParameters::setCommodityCurves(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::CommodityCurve, names);
}

void ScenarioSimMarketParameters::setCorrelationPairs(vector<string> names) {
    addParamsName(RiskFactorKey::KeyType::Correlation, names);
}

void ScenarioSimMarketParameters::setCprs(const vector<string>& names) {
    addParamsName(RiskFactorKey::KeyType::CPR, names);
}

void ScenarioSimMarketParameters::setSimulateDividendYield(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::DividendYield, simulate);
}

void ScenarioSimMarketParameters::setSimulateSwapVols(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::SwaptionVolatility, simulate);
}

void ScenarioSimMarketParameters::setSimulateYieldVols(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::YieldVolatility, simulate);
}

void ScenarioSimMarketParameters::setSimulateCapFloorVols(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::OptionletVolatility, simulate);
}

void ScenarioSimMarketParameters::setSimulateYoYInflationCapFloorVols(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::YoYInflationCapFloorVolatility, simulate);
}

void ScenarioSimMarketParameters::setSimulateSurvivalProbabilities(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::SurvivalProbability, simulate);
}

void ScenarioSimMarketParameters::setSimulateRecoveryRates(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::RecoveryRate, simulate);
}

void ScenarioSimMarketParameters::setSimulateCdsVols(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::CDSVolatility, simulate);
}

void ScenarioSimMarketParameters::setSimulateFXVols(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::FXVolatility, simulate);
}

void ScenarioSimMarketParameters::setSimulateEquityVols(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::EquityVolatility, simulate);
}

void ScenarioSimMarketParameters::setSimulateBaseCorrelations(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::BaseCorrelation, simulate);
}

void ScenarioSimMarketParameters::setCommodityCurveSimulate(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::CommodityCurve, simulate);
}

void ScenarioSimMarketParameters::setCommodityVolSimulate(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::CommodityVolatility, simulate);
}

void ScenarioSimMarketParameters::setSecuritySpreadsSimulate(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::SecuritySpread, simulate);
}

void ScenarioSimMarketParameters::setSimulateFxSpots(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::FXSpot, simulate);
}

void ScenarioSimMarketParameters::setSimulateCorrelations(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::Correlation, simulate);
}

void ScenarioSimMarketParameters::setSimulateCprs(bool simulate) {
    setParamsSimulate(RiskFactorKey::KeyType::CPR, simulate);
}

bool ScenarioSimMarketParameters::operator==(const ScenarioSimMarketParameters& rhs) {

    if (baseCcy_ != rhs.baseCcy_ || ccys_ != rhs.ccys_ || params_ != rhs.params_ ||
        yieldCurveDayCounters_ != rhs.yieldCurveDayCounters_ || yieldCurveCurrencies_ != rhs.yieldCurveCurrencies_ ||
        yieldCurveTenors_ != rhs.yieldCurveTenors_ || swapIndices_ != rhs.swapIndices_ ||
        interpolation_ != rhs.interpolation_ || extrapolate_ != rhs.extrapolate_ ||
        swapVolTerms_ != rhs.swapVolTerms_ || swapVolDayCounters_ != rhs.swapVolDayCounters_ ||
        swapVolIsCube_ != rhs.swapVolIsCube_ || swapVolSimulateATMOnly_ != rhs.swapVolSimulateATMOnly_ ||
        swapVolExpiries_ != rhs.swapVolExpiries_ || swapVolStrikeSpreads_ != rhs.swapVolStrikeSpreads_ ||
        swapVolDecayMode_ != rhs.swapVolDecayMode_ || capFloorVolDayCounters_ != rhs.capFloorVolDayCounters_ ||
        capFloorVolExpiries_ != rhs.capFloorVolExpiries_ || capFloorVolStrikes_ != rhs.capFloorVolStrikes_ ||
        capFloorVolDecayMode_ != rhs.capFloorVolDecayMode_ ||
        defaultCurveDayCounters_ != rhs.defaultCurveDayCounters_ ||
        defaultCurveCalendars_ != rhs.defaultCurveCalendars_ || defaultTenors_ != rhs.defaultTenors_ ||
        cdsVolExpiries_ != rhs.cdsVolExpiries_ || cdsVolDayCounters_ != rhs.cdsVolDayCounters_ ||
        cdsVolDecayMode_ != rhs.cdsVolDecayMode_ || equityDividendTenors_ != rhs.equityDividendTenors_ ||
        fxVolIsSurface_ != rhs.fxVolIsSurface_ ||
        fxVolExpiries_ != rhs.fxVolExpiries_ || fxVolDayCounters_ != rhs.fxVolDayCounters_ ||
        fxVolDecayMode_ != rhs.fxVolDecayMode_ || equityVolExpiries_ != rhs.equityVolExpiries_ ||
        equityVolDayCounters_ != rhs.equityVolDayCounters_ || equityVolDecayMode_ != rhs.equityVolDecayMode_ ||
        equityIsSurface_ != rhs.equityIsSurface_ || equityVolSimulateATMOnly_ != rhs.equityVolSimulateATMOnly_ ||
        equityMoneyness_ != rhs.equityMoneyness_ ||
        additionalScenarioDataIndices_ != rhs.additionalScenarioDataIndices_ ||
        additionalScenarioDataCcys_ != rhs.additionalScenarioDataCcys_ ||
        baseCorrelationTerms_ != rhs.baseCorrelationTerms_ ||
        baseCorrelationDayCounters_ != rhs.baseCorrelationDayCounters_ ||
        baseCorrelationDetachmentPoints_ != rhs.baseCorrelationDetachmentPoints_ ||
        zeroInflationDayCounters_ != rhs.zeroInflationDayCounters_ ||
        zeroInflationTenors_ != rhs.zeroInflationTenors_ || yoyInflationDayCounters_ != rhs.yoyInflationDayCounters_ ||
        yoyInflationTenors_ != rhs.yoyInflationTenors_ || commodityCurveTenors_ != rhs.commodityCurveTenors_ ||
        commodityCurveDayCounters_ != rhs.commodityCurveDayCounters_ ||
        commodityVolDecayMode_ != rhs.commodityVolDecayMode_ || commodityVolExpiries_ != rhs.commodityVolExpiries_ ||
        commodityVolMoneyness_ != rhs.commodityVolMoneyness_ ||
        commodityVolDayCounters_ != rhs.commodityVolDayCounters_ ||
        correlationDayCounters_ != rhs.correlationDayCounters_ || correlationIsSurface_ != rhs.correlationIsSurface_ ||
        correlationExpiries_ != rhs.correlationExpiries_ || correlationStrikes_ != rhs.correlationStrikes_ ||
        cprSimulate_ != rhs.cprSimulate_ || cprs_ != rhs.cprs_ || yieldVolTerms_ != rhs.yieldVolTerms_ ||
        yieldVolDayCounters_ != rhs.yieldVolDayCounters_ || yieldVolExpiries_ != rhs.yieldVolExpiries_ ||
        yieldVolDecayMode_ != rhs.yieldVolDecayMode_) {
        return false;
    } else {
        return true;
    }
}

bool ScenarioSimMarketParameters::operator!=(const ScenarioSimMarketParameters& rhs) { return !(*this == rhs); }

void ScenarioSimMarketParameters::fromXML(XMLNode* root) {

    // fromXML always uses a "clean" object
    reset();

    DLOG("ScenarioSimMarketParameters::fromXML()");

    XMLNode* sim = XMLUtils::locateNode(root, "Simulation");
    XMLNode* node = XMLUtils::getChildNode(sim, "Market");
    XMLUtils::checkNode(node, "Market");

    // TODO: add in checks (checkNode or QL_REQUIRE) on mandatory nodes
    DLOG("Loading Currencies");
    baseCcy_ = XMLUtils::getChildValue(node, "BaseCurrency");
    setDiscountCurveNames(XMLUtils::getChildrenValues(node, "Currencies", "Currency"));

    DLOG("Loading BenchmarkCurve");
    XMLNode* nodeChild = XMLUtils::getChildNode(node, "BenchmarkCurves");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        vector<string> yields;
        for (XMLNode* n = XMLUtils::getChildNode(nodeChild, "BenchmarkCurve"); n != nullptr;
             n = XMLUtils::getNextSibling(n, "BenchmarkCurve")) {
            yields.push_back(XMLUtils::getChildValue(n, "Name", true));
            yieldCurveCurrencies_[XMLUtils::getChildValue(n, "Name", true)] =
                XMLUtils::getChildValue(n, "Currency", true);
        }
        setYieldCurveNames(yields);
    }

    DLOG("Loading YieldCurves");
    nodeChild = XMLUtils::getChildNode(node, "YieldCurves");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        for (XMLNode* child = XMLUtils::getChildNode(nodeChild, "Configuration"); child;
             child = XMLUtils::getNextSibling(child)) {

            // If there is no attribute "curve", this returns "" i.e. the default
            string label = XMLUtils::getAttribute(child, "curve");
            if (label == "") {
                interpolation_ = XMLUtils::getChildValue(child, "Interpolation", true);
                extrapolate_ = XMLUtils::getChildValueAsBool(child, "Extrapolate");
                yieldCurveTenors_[label] = XMLUtils::getChildrenValuesAsPeriods(child, "Tenors", true);
            } else {
                if (XMLUtils::getChildNode(child, "Interpolation")) {
                    WLOG("Only one default interpolation value is allowed for yield curves");
                }
                if (XMLUtils::getChildNode(child, "Extrapolate")) {
                    WLOG("Only one default extrapolation value is allowed for yield curves");
                }
                if (XMLUtils::getChildNode(child, "Tenors")) {
                    yieldCurveTenors_[label] = XMLUtils::getChildrenValuesAsPeriods(child, "Tenors", true);
                }
            }

            if (XMLUtils::getChildNode(child, "DayCounter")) {
                yieldCurveDayCounters_[label] = XMLUtils::getChildValue(child, "DayCounter", true);
            }
        }
    }

    DLOG("Loading Libor indices");
    setIndices(XMLUtils::getChildrenValues(node, "Indices", "Index"));

    DLOG("Loading swap indices");
    nodeChild = XMLUtils::getChildNode(node, "SwapIndices");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        for (XMLNode* n = XMLUtils::getChildNode(nodeChild, "SwapIndex"); n != nullptr;
             n = XMLUtils::getNextSibling(n, "SwapIndex")) {
            string name = XMLUtils::getChildValue(n, "Name");
            string disc = XMLUtils::getChildValue(n, "DiscountingIndex");
            swapIndices_[name] = disc;
        }
    }

    DLOG("Loading FX Rates");
    nodeChild = XMLUtils::getChildNode(node, "FxRates");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        XMLNode* fxSpotSimNode = XMLUtils::getChildNode(nodeChild, "Simulate");
        if (fxSpotSimNode)
            setSimulateFxSpots(ore::data::parseBool(XMLUtils::getNodeValue(fxSpotSimNode)));
        // if currency pairs are specified load these, otherwise infer from currencies list and base currency
        XMLNode* ccyPairsNode = XMLUtils::getChildNode(nodeChild, "CurrencyPairs");
        if (ccyPairsNode) {
            setFxCcyPairs(XMLUtils::getChildrenValues(nodeChild, "CurrencyPairs", "CurrencyPair", true));
        } else {
            vector<string> ccys;
            for (auto ccy : ccys_) {
                if (ccy != baseCcy_)
                    ccys.push_back(ccy + baseCcy_);
            }
            setFxCcyPairs(ccys);
        }
    } else {
        // spot simulation turned on by default
        setSimulateFxSpots(true);
        vector<string> ccys;
        for (auto ccy : ccys_) {
            if (ccy != baseCcy_)
                ccys.push_back(ccy + baseCcy_);
        }
        setFxCcyPairs(ccys);
    }

    DLOG("Loading SwaptionVolatilities");
    nodeChild = XMLUtils::getChildNode(node, "SwaptionVolatilities");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        XMLNode* swapVolSimNode = XMLUtils::getChildNode(nodeChild, "Simulate");
        if (swapVolSimNode)
            setSimulateSwapVols(ore::data::parseBool(XMLUtils::getNodeValue(swapVolSimNode)));
        swapVolTerms_ = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "Terms", true);
        swapVolExpiries_ = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "Expiries", true);
        setSwapVolCcys(XMLUtils::getChildrenValues(nodeChild, "Currencies", "Currency", true));
        swapVolDecayMode_ = XMLUtils::getChildValue(nodeChild, "ReactionToTimeDecay");
        XMLNode* cubeNode = XMLUtils::getChildNode(nodeChild, "Cube");
        if (cubeNode) {
            swapVolIsCube_ = true;
            XMLNode* atmOnlyNode = XMLUtils::getChildNode(cubeNode, "SimulateATMOnly");
            if (atmOnlyNode) {
                swapVolSimulateATMOnly_ = XMLUtils::getChildValueAsBool(cubeNode, "SimulateATMOnly", true);
            } else {
                swapVolSimulateATMOnly_ = false;
            }
            if (!swapVolSimulateATMOnly_)
                swapVolStrikeSpreads_ = XMLUtils::getChildrenValuesAsDoublesCompact(cubeNode, "StrikeSpreads", true);
        } else {
            swapVolIsCube_ = false;
        }
        XMLNode* dc = XMLUtils::getChildNode(nodeChild, "DayCounters");
        if (dc) {
            for (XMLNode* child = XMLUtils::getChildNode(dc, "DayCounter"); child;
                 child = XMLUtils::getNextSibling(child)) {
                string label = XMLUtils::getAttribute(child, "ccy");
                swapVolDayCounters_[label] = XMLUtils::getNodeValue(child);
            }
        }
        QL_REQUIRE(swapVolDayCounters_.find("") != swapVolDayCounters_.end(),
                   "default daycounter is not set for swapVolSurfaces");
    }

    DLOG("Loading YieldVolatilities");
    nodeChild = XMLUtils::getChildNode(node, "YieldVolatilities");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        XMLNode* yieldVolSimNode = XMLUtils::getChildNode(nodeChild, "Simulate");
        if (yieldVolSimNode) {
            setSimulateYieldVols(ore::data::parseBool(XMLUtils::getNodeValue(yieldVolSimNode)));
            yieldVolTerms_ = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "Terms", true);
            yieldVolExpiries_ = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "Expiries", true);
            setYieldVolNames(XMLUtils::getChildrenValues(nodeChild, "Names", "Name", true));
            yieldVolDecayMode_ = XMLUtils::getChildValue(nodeChild, "ReactionToTimeDecay");
            XMLNode* dc = XMLUtils::getChildNode(nodeChild, "DayCounters");
            if (dc) {
                for (XMLNode* child = XMLUtils::getChildNode(dc, "DayCounter"); child;
                    child = XMLUtils::getNextSibling(child)) {
                    string label = XMLUtils::getAttribute(child, "ccy");
                    yieldVolDayCounters_[label] = XMLUtils::getNodeValue(child);
                }
            }
            QL_REQUIRE(yieldVolDayCounters_.find("") != yieldVolDayCounters_.end(),
                "default daycounter is not set for yieldVolSurfaces");
        }
    }

    DLOG("Loading Correlations");
    nodeChild = XMLUtils::getChildNode(node, "Correlations");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        XMLNode* pn = XMLUtils::getChildNode(nodeChild, "Pairs");
        vector<string> pairs;
        if (pn) {
            for (XMLNode* child = XMLUtils::getChildNode(pn, "Pair"); child; child = XMLUtils::getNextSibling(child)) {
                string p = XMLUtils::getNodeValue(child);
                vector<string> tokens;
                boost::split(tokens, p, boost::is_any_of(",:"));
                QL_REQUIRE(tokens.size() == 2, "not a valid correlation pair: " << p);
                pairs.push_back(tokens[0] + ":" + tokens[1]);
            }
        }
        setCorrelationPairs(pairs);
        XMLNode* correlSimNode = XMLUtils::getChildNode(nodeChild, "Simulate");
        if (correlSimNode) {
            setSimulateCorrelations(ore::data::parseBool(XMLUtils::getNodeValue(correlSimNode)));
            correlationExpiries_ = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "Expiries", true);

            XMLNode* surfaceNode = XMLUtils::getChildNode(nodeChild, "Surface");
            if (surfaceNode) {
                correlationIsSurface_ = true;
                correlationStrikes_ = XMLUtils::getChildrenValuesAsDoublesCompact(surfaceNode, "Strikes", true);
            } else {
                correlationIsSurface_ = false;
            }
            XMLNode* dc = XMLUtils::getChildNode(nodeChild, "DayCounters");
            if (dc) {
                for (XMLNode* child = XMLUtils::getChildNode(dc, "DayCounter"); child;
                     child = XMLUtils::getNextSibling(child)) {
                    string label1 = XMLUtils::getAttribute(child, "index1");
                    string label2 = XMLUtils::getAttribute(child, "index2");
                    correlationDayCounters_[std::make_pair(label1, label2)] = XMLUtils::getNodeValue(child);
                }
            }
            QL_REQUIRE(correlationDayCounters_.find(pair<string, string>()) != correlationDayCounters_.end(),
                       "default daycounter is not set for correlationSurfaces");
        }
    }

    DLOG("Loading CapFloorVolatilities");
    nodeChild = XMLUtils::getChildNode(node, "CapFloorVolatilities");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        setSimulateCapFloorVols(false);
        XMLNode* capVolSimNode = XMLUtils::getChildNode(nodeChild, "Simulate");
        if (capVolSimNode)
            setSimulateCapFloorVols(ore::data::parseBool(XMLUtils::getNodeValue(capVolSimNode)));
        capFloorVolExpiries_[""] = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "Expiries", true);
        // TODO read other keys
        capFloorVolStrikes_ = XMLUtils::getChildrenValuesAsDoublesCompact(nodeChild, "Strikes", true);
        setCapFloorVolCcys(XMLUtils::getChildrenValues(nodeChild, "Currencies", "Currency", true));
        capFloorVolDecayMode_ = XMLUtils::getChildValue(nodeChild, "ReactionToTimeDecay");
        XMLNode* dc = XMLUtils::getChildNode(nodeChild, "DayCounters");
        if (dc) {
            for (XMLNode* child = XMLUtils::getChildNode(dc, "DayCounter"); child;
                 child = XMLUtils::getNextSibling(child)) {
                string label = XMLUtils::getAttribute(child, "ccy");
                capFloorVolDayCounters_[label] = XMLUtils::getNodeValue(child);
            }
        }
        QL_REQUIRE(capFloorVolDayCounters_.find("") != capFloorVolDayCounters_.end(),
                   "default daycounter is not set for capFloorVolSurfaces");
    }

    DLOG("Loading YYCapFloorVolatilities");
    nodeChild = XMLUtils::getChildNode(node, "YYCapFloorVolatilities");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        setSimulateYoYInflationCapFloorVols(false);
        XMLNode* yoyCapVolSimNode = XMLUtils::getChildNode(nodeChild, "Simulate");
        if (yoyCapVolSimNode)
            setSimulateYoYInflationCapFloorVols(ore::data::parseBool(XMLUtils::getNodeValue(yoyCapVolSimNode)));
        yoyInflationCapFloorVolExpiries_[""] = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "Expiries", true);
        yoyInflationCapFloorVolStrikes_ = XMLUtils::getChildrenValuesAsDoublesCompact(nodeChild, "Strikes", true);
        setYoYInflationCapFloorNames(XMLUtils::getChildrenValues(nodeChild, "Names", "Name", true));
        yoyInflationCapFloorVolDecayMode_ = XMLUtils::getChildValue(nodeChild, "ReactionToTimeDecay");
    }

    DLOG("Loading DefaultCurves Rates");
    nodeChild = XMLUtils::getChildNode(node, "DefaultCurves");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        setDefaultNames(XMLUtils::getChildrenValues(nodeChild, "Names", "Name", true));
        defaultTenors_[""] = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "Tenors", true);
        // TODO read other keys
        XMLNode* survivalProbabilitySimNode = XMLUtils::getChildNode(nodeChild, "SimulateSurvivalProbabilities");
        if (survivalProbabilitySimNode)
            setSimulateSurvivalProbabilities(ore::data::parseBool(XMLUtils::getNodeValue(survivalProbabilitySimNode)));
        XMLNode* recoveryRateSimNode = XMLUtils::getChildNode(nodeChild, "SimulateRecoveryRates");
        if (recoveryRateSimNode)
            setSimulateRecoveryRates(ore::data::parseBool(XMLUtils::getNodeValue(recoveryRateSimNode)));

        XMLNode* dc = XMLUtils::getChildNode(nodeChild, "DayCounters");
        if (dc) {
            for (XMLNode* child = XMLUtils::getChildNode(dc, "DayCounter"); child;
                 child = XMLUtils::getNextSibling(child)) {
                string label = XMLUtils::getAttribute(child, "name");
                defaultCurveDayCounters_[label] = XMLUtils::getNodeValue(child);
            }
        }
        QL_REQUIRE(defaultCurveDayCounters_.find("") != defaultCurveDayCounters_.end(),
                   "default daycounter is not set  for defaultCurves");

        XMLNode* cal = XMLUtils::getChildNode(nodeChild, "Calendars");
        if (cal) {
            for (XMLNode* child = XMLUtils::getChildNode(cal, "Calendar"); child;
                 child = XMLUtils::getNextSibling(child)) {
                string label = XMLUtils::getAttribute(child, "name");
                defaultCurveCalendars_[label] = XMLUtils::getNodeValue(child);
            }
        }
        QL_REQUIRE(defaultCurveCalendars_.find("") != defaultCurveCalendars_.end(),
                   "default calendar is not set for defaultCurves");
    }

    DLOG("Loading Equities Rates");
    nodeChild = XMLUtils::getChildNode(node, "Equities");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        XMLNode* dividendYieldSimNode = XMLUtils::getChildNode(nodeChild, "SimulateDividendYield");
        if (dividendYieldSimNode)
            setSimulateDividendYield(ore::data::parseBool(XMLUtils::getNodeValue(dividendYieldSimNode)));
        else
            setSimulateDividendYield(false);
        vector<string> equityNames = XMLUtils::getChildrenValues(nodeChild, "Names", "Name", true);
        setEquityNames(equityNames);
        equityDividendTenors_[""] = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "DividendTenors", true);
    }

    DLOG("Loading CDSVolatilities Rates");
    nodeChild = XMLUtils::getChildNode(node, "CDSVolatilities");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        XMLNode* cdsVolSimNode = XMLUtils::getChildNode(nodeChild, "Simulate");
        if (cdsVolSimNode)
            setSimulateCdsVols(ore::data::parseBool(XMLUtils::getNodeValue(cdsVolSimNode)));
        cdsVolExpiries_ = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "Expiries", true);
        setCdsVolNames(XMLUtils::getChildrenValues(nodeChild, "Names", "Name", true));
        cdsVolDecayMode_ = XMLUtils::getChildValue(nodeChild, "ReactionToTimeDecay");
        XMLNode* dc = XMLUtils::getChildNode(nodeChild, "DayCounters");
        if (dc) {
            for (XMLNode* child = XMLUtils::getChildNode(dc, "DayCounter"); child;
                 child = XMLUtils::getNextSibling(child)) {
                string label = XMLUtils::getAttribute(child, "name");
                cdsVolDayCounters_[label] = XMLUtils::getNodeValue(child);
            }
        }
        QL_REQUIRE(cdsVolDayCounters_.find("") != cdsVolDayCounters_.end(),
                   "default daycounter is not set for cdsVolSurfaces");
    }

    DLOG("Loading FXVolatilities");
    nodeChild = XMLUtils::getChildNode(node, "FxVolatilities");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        setSimulateFXVols(false);
        XMLNode* fxVolSimNode = XMLUtils::getChildNode(nodeChild, "Simulate");
        if (fxVolSimNode)
            setSimulateFXVols(ore::data::parseBool(XMLUtils::getNodeValue(fxVolSimNode)));
        fxVolExpiries_ = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "Expiries", true);
        fxVolDecayMode_ = XMLUtils::getChildValue(nodeChild, "ReactionToTimeDecay");
        setFxVolCcyPairs(XMLUtils::getChildrenValues(nodeChild, "CurrencyPairs", "CurrencyPair", true));
        XMLNode* fxSurfaceNode = XMLUtils::getChildNode(nodeChild, "Surface");
        if (fxSurfaceNode) {
            fxVolIsSurface_ = true;
            fxMoneyness_ = XMLUtils::getChildrenValuesAsDoublesCompact(fxSurfaceNode, "Moneyness", true);
        } else {
            fxVolIsSurface_ = false;
            fxMoneyness_ = {0.0};
        }
        XMLNode* dc = XMLUtils::getChildNode(nodeChild, "DayCounters");
        if (dc) {
            for (XMLNode* child = XMLUtils::getChildNode(dc, "DayCounter"); child;
                 child = XMLUtils::getNextSibling(child)) {
                string label = XMLUtils::getAttribute(child, "ccyPair");
                fxVolDayCounters_[label] = XMLUtils::getNodeValue(child);
            }
        }
        QL_REQUIRE(fxVolDayCounters_.find("") != fxVolDayCounters_.end(),
                   "default daycounter is not set for fxVolSurfaces");
    }

    DLOG("Loading EquityVolatilities");
    nodeChild = XMLUtils::getChildNode(node, "EquityVolatilities");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        setSimulateEquityVols(XMLUtils::getChildValueAsBool(nodeChild, "Simulate", true));
        equityVolExpiries_ = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "Expiries", true);
        equityVolDecayMode_ = XMLUtils::getChildValue(nodeChild, "ReactionToTimeDecay");
        setEquityVolNames(XMLUtils::getChildrenValues(nodeChild, "Names", "Name", true));
        XMLNode* eqSurfaceNode = XMLUtils::getChildNode(nodeChild, "Surface");
        if (eqSurfaceNode) {
            equityIsSurface_ = true;
            XMLNode* atmOnlyNode = XMLUtils::getChildNode(eqSurfaceNode, "SimulateATMOnly");
            if (atmOnlyNode) {
                equityVolSimulateATMOnly_ = XMLUtils::getChildValueAsBool(eqSurfaceNode, "SimulateATMOnly", true);
            } else {
                equityVolSimulateATMOnly_ = false;
            }
            if (!equityVolSimulateATMOnly_)
                equityMoneyness_ = XMLUtils::getChildrenValuesAsDoublesCompact(eqSurfaceNode, "Moneyness", true);
        } else {
            equityIsSurface_ = false;
        }
        XMLNode* dc = XMLUtils::getChildNode(nodeChild, "DayCounters");
        if (dc) {
            for (XMLNode* child = XMLUtils::getChildNode(dc, "DayCounter"); child;
                 child = XMLUtils::getNextSibling(child)) {
                string label = XMLUtils::getAttribute(child, "name");
                equityVolDayCounters_[label] = XMLUtils::getNodeValue(child);
            }
        }
        QL_REQUIRE(equityVolDayCounters_.find("") != equityVolDayCounters_.end(),
                   "default daycounter is not set for equityVolSurfaces");
    }

    DLOG("Loading CpiInflationIndexCurves");
    setCpiIndices(XMLUtils::getChildrenValues(node, "CpiIndices", "Index", false));

    DLOG("Loading ZeroInflationIndexCurves");
    nodeChild = XMLUtils::getChildNode(node, "ZeroInflationIndexCurves");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        setZeroInflationIndices(XMLUtils::getChildrenValues(nodeChild, "Names", "Name", true));
        zeroInflationTenors_[""] = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "Tenors", true);

        XMLNode* dc = XMLUtils::getChildNode(nodeChild, "DayCounters");
        if (dc) {
            for (XMLNode* child = XMLUtils::getChildNode(dc, "DayCounter"); child;
                 child = XMLUtils::getNextSibling(child)) {
                string label = XMLUtils::getAttribute(child, "name");
                zeroInflationDayCounters_[label] = XMLUtils::getNodeValue(child);
            }
        }
        QL_REQUIRE(zeroInflationDayCounters_.find("") != zeroInflationDayCounters_.end(),
                   "default daycounter is not set for zeroInflation Surfaces");
    }

    DLOG("Loading YYInflationIndexCurves");

    nodeChild = XMLUtils::getChildNode(node, "YYInflationIndexCurves");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        setYoyInflationIndices(XMLUtils::getChildrenValues(nodeChild, "Names", "Name", true));
        yoyInflationTenors_[""] = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "Tenors", true);
        XMLNode* dc = XMLUtils::getChildNode(nodeChild, "DayCounters");
        if (dc) {
            for (XMLNode* child = XMLUtils::getChildNode(dc, "DayCounter"); child;
                 child = XMLUtils::getNextSibling(child)) {
                string label = XMLUtils::getAttribute(child, "name");
                yoyInflationDayCounters_[label] = XMLUtils::getNodeValue(child);
            }
        }
        QL_REQUIRE(yoyInflationDayCounters_.find("") != yoyInflationDayCounters_.end(),
                   "default daycounter is not set for yoyInflation Surfaces");
    }

    DLOG("Loading AggregationScenarioDataIndices");
    if (XMLUtils::getChildNode(node, "AggregationScenarioDataIndices")) {
        additionalScenarioDataIndices_ = XMLUtils::getChildrenValues(node, "AggregationScenarioDataIndices", "Index");
    }

    DLOG("Loading AggregationScenarioDataCurrencies");
    if (XMLUtils::getChildNode(node, "AggregationScenarioDataCurrencies")) {
        additionalScenarioDataCcys_ =
            XMLUtils::getChildrenValues(node, "AggregationScenarioDataCurrencies", "Currency", true);
    }

    DLOG("Loading Securities");
    nodeChild = XMLUtils::getChildNode(node, "Securities");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        // TODO 1) this should be renamed to SimulateSpread?
        //      2) add security recovery rates here separate from default curves?
        setSecuritySpreadsSimulate(XMLUtils::getChildValueAsBool(nodeChild, "Simulate", false));
        vector<string> securities = XMLUtils::getChildrenValues(nodeChild, "Names", "Name");
        setSecurities(securities);
    }

    DLOG("Loading CPRs");
    nodeChild = XMLUtils::getChildNode(node, "CPRs");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        setSimulateCprs(XMLUtils::getChildValueAsBool(nodeChild, "Simulate", false));
        setCprs(XMLUtils::getChildrenValues(nodeChild, "Names", "Name"));
    }

    DLOG("Loading BaseCorrelations");
    nodeChild = XMLUtils::getChildNode(node, "BaseCorrelations");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        setSimulateBaseCorrelations(XMLUtils::getChildValueAsBool(nodeChild, "Simulate", true));
        setBaseCorrelationNames(XMLUtils::getChildrenValues(nodeChild, "IndexNames", "IndexName", true));
        baseCorrelationTerms_ = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "Terms", true);
        baseCorrelationDetachmentPoints_ =
            XMLUtils::getChildrenValuesAsDoublesCompact(nodeChild, "DetachmentPoints", true);

        XMLNode* dc = XMLUtils::getChildNode(nodeChild, "DayCounters");
        if (dc) {
            for (XMLNode* child = XMLUtils::getChildNode(dc, "DayCounter"); child;
                 child = XMLUtils::getNextSibling(child)) {
                string label = XMLUtils::getAttribute(child, "name");
                baseCorrelationDayCounters_[label] = XMLUtils::getNodeValue(child);
            }
        }
        QL_REQUIRE(baseCorrelationDayCounters_.find("") != baseCorrelationDayCounters_.end(),
                   "default daycounter is not set for baseCorrelation Surfaces");
    }

    DLOG("Loading commodities data");
    nodeChild = XMLUtils::getChildNode(node, "Commodities");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        XMLNode* commoditySimNode = XMLUtils::getChildNode(nodeChild, "Simulate");
        setCommodityCurveSimulate(commoditySimNode ? parseBool(XMLUtils::getNodeValue(commoditySimNode)) : false);

        vector<string> commodityNames = XMLUtils::getChildrenValues(nodeChild, "Names", "Name", true);
        setCommodityNames(commodityNames);
        commodityCurveTenors_[""] = XMLUtils::getChildrenValuesAsPeriods(nodeChild, "Tenors", true);

        // If present, override DayCounter for _all_ commodity price curves
        XMLNode* commodityDayCounterNode = XMLUtils::getChildNode(nodeChild, "DayCounter");
        if (commodityDayCounterNode) {
            commodityCurveDayCounters_[""] = XMLUtils::getNodeValue(commodityDayCounterNode);
        }
    }

    DLOG("Loading commodity volatility data");
    nodeChild = XMLUtils::getChildNode(node, "CommodityVolatilities");
    if (nodeChild && XMLUtils::getChildNode(nodeChild)) {
        setCommodityVolSimulate(XMLUtils::getChildValueAsBool(nodeChild, "Simulate", true));
        commodityVolDecayMode_ = XMLUtils::getChildValue(nodeChild, "ReactionToTimeDecay");

        vector<string> names;
        XMLNode* namesNode = XMLUtils::getChildNode(nodeChild, "Names");
        if (namesNode) {
            for (XMLNode* child = XMLUtils::getChildNode(namesNode, "Name"); child;
                 child = XMLUtils::getNextSibling(child)) {
                // Get the vol configuration for each commodity name
                string name = XMLUtils::getAttribute(child, "id");
                names.push_back(name);
                commodityVolExpiries_[name] = XMLUtils::getChildrenValuesAsPeriods(child, "Expiries", true);
                vector<Real> moneyness = XMLUtils::getChildrenValuesAsDoublesCompact(child, "Moneyness", false);
                if (moneyness.empty())
                    moneyness = {1.0};
                commodityVolMoneyness_[name] = moneyness;
            }
        }
        setCommodityVolNames(names);

        // If present, override DayCounter for _all_ commodity volatilities
        XMLNode* dayCounterNode = XMLUtils::getChildNode(nodeChild, "DayCounter");
        if (dayCounterNode) {
            commodityVolDayCounters_[""] = XMLUtils::getNodeValue(dayCounterNode);
        }
    }

    DLOG("Loaded ScenarioSimMarketParameters");
}

XMLNode* ScenarioSimMarketParameters::toXML(XMLDocument& doc) {

    XMLNode* marketNode = doc.allocNode("Market");

    // currencies
    XMLUtils::addChild(doc, marketNode, "BaseCurrency", baseCcy_);
    XMLUtils::addChildren(doc, marketNode, "Currencies", "Currency", ccys_);

    // yield curves
    DLOG("Writing yield curves data");
    XMLNode* yieldCurvesNode = XMLUtils::addChild(doc, marketNode, "YieldCurves");

    // Take the keys from the yieldCurveDayCounters_ and yieldCurveTenors_ maps
    set<string> keys;
    for (const auto& kv : yieldCurveTenors_) {
        keys.insert(kv.first);
    }
    for (const auto& kv : yieldCurveDayCounters_) {
        keys.insert(kv.first);
    }
    QL_REQUIRE(keys.count("") > 0, "There is no default yield curve configuration in simulation parameters");

    // Add the yield curve configuration nodes
    for (const auto& key : keys) {
        XMLNode* configNode = doc.allocNode("Configuration");
        XMLUtils::addAttribute(doc, configNode, "curve", key);
        if (yieldCurveTenors_.count(key) > 0) {
            XMLUtils::addGenericChildAsList(doc, configNode, "Tenors", yieldCurveTenors_.at(key));
        }
        if (key == "") {
            XMLUtils::addChild(doc, configNode, "Interpolation", interpolation_);
            XMLUtils::addChild(doc, configNode, "Extrapolation", extrapolate_);
        }
        if (yieldCurveDayCounters_.count(key) > 0) {
            XMLUtils::addChild(doc, configNode, "DayCounter", yieldCurveDayCounters_.at(key));
        }
        XMLUtils::appendNode(yieldCurvesNode, configNode);
    }

    // fx rates
    if (fxCcyPairs().size() > 0) {
        DLOG("Writing FX rates");
        XMLNode* fxRatesNode = XMLUtils::addChild(doc, marketNode, "FxRates");
        XMLUtils::addChildren(doc, fxRatesNode, "CurrencyPairs", "CurrencyPair", fxCcyPairs());
    }

    // indices
    if (indices().size() > 0) {
        DLOG("Writing libor indices");
        XMLUtils::addChildren(doc, marketNode, "Indices", "Index", indices());
    }

    // swap indices
    if (swapIndices_.size() > 0) {
        DLOG("Writing swap indices");
        XMLNode* swapIndicesNode = XMLUtils::addChild(doc, marketNode, "SwapIndices");
        for (auto kv : swapIndices_) {
            XMLNode* swapIndexNode = XMLUtils::addChild(doc, swapIndicesNode, "SwapIndex");
            XMLUtils::addChild(doc, swapIndexNode, "Name", kv.first);
            XMLUtils::addChild(doc, swapIndexNode, "DiscountingIndex", kv.second);
        }
    }

    // default curves
    if (!defaultNames().empty()) {
        DLOG("Writing default curves");
        XMLNode* defaultCurvesNode = XMLUtils::addChild(doc, marketNode, "DefaultCurves");
        XMLUtils::addChildren(doc, defaultCurvesNode, "Names", "Name", defaultNames());
        XMLUtils::addGenericChildAsList(doc, defaultCurvesNode, "Tenors", returnTenors(defaultTenors_, ""));
        XMLUtils::addChild(doc, defaultCurvesNode, "SimulateSurvivalProbabilities", simulateSurvivalProbabilities());
        XMLUtils::addChild(doc, defaultCurvesNode, "SimulateRecoveryRates", simulateRecoveryRates());

        if (defaultCurveDayCounters_.size() > 0) {
            XMLNode* node = XMLUtils::addChild(doc, defaultCurvesNode, "DayCounters");
            for (auto dc : defaultCurveDayCounters_) {
                XMLNode* c = doc.allocNode("DayCounter", dc.second);
                XMLUtils::addAttribute(doc, c, "name", dc.first);
                XMLUtils::appendNode(node, c);
            }
        }

        if (defaultCurveCalendars_.size() > 0) {
            XMLNode* node = XMLUtils::addChild(doc, defaultCurvesNode, "Calendars");
            for (auto dc : defaultCurveCalendars_) {
                XMLNode* c = doc.allocNode("Calendar", dc.second);
                XMLUtils::addAttribute(doc, c, "name", dc.first);
                XMLUtils::appendNode(node, c);
            }
        }
    }

    // equities
    if (!equityNames().empty()) {
        DLOG("Writing equities");
        XMLNode* equitiesNode = XMLUtils::addChild(doc, marketNode, "Equities");
        XMLUtils::addChildren(doc, equitiesNode, "Names", "Name", equityNames());
        XMLUtils::addGenericChildAsList(doc, equitiesNode, "DividendTenors", returnTenors(equityDividendTenors_, ""));
        XMLUtils::addChild(doc, equitiesNode, "SimulateDividendYield", simulateDividendYield());
    }

    // swaption volatilities
    if (!swapVolCcys().empty()) {
        DLOG("Writing swaption volatilities");
        XMLNode* swaptionVolatilitiesNode = XMLUtils::addChild(doc, marketNode, "SwaptionVolatilities");
        XMLUtils::addChild(doc, swaptionVolatilitiesNode, "Simulate", simulateSwapVols());
        XMLUtils::addChild(doc, swaptionVolatilitiesNode, "ReactionToTimeDecay", swapVolDecayMode_);
        XMLUtils::addChildren(doc, swaptionVolatilitiesNode, "Currencies", "Currency", swapVolCcys());
        XMLUtils::addGenericChildAsList(doc, swaptionVolatilitiesNode, "Expiries", swapVolExpiries_);
        XMLUtils::addGenericChildAsList(doc, swaptionVolatilitiesNode, "Terms", swapVolTerms_);
        if (swapVolIsCube_) {
            XMLNode* swapVolNode = XMLUtils::addChild(doc, swaptionVolatilitiesNode, "Cube");
            XMLUtils::addChild(doc, swapVolNode, "SimulateATMOnly", swapVolSimulateATMOnly_);
            XMLUtils::addGenericChildAsList(doc, swapVolNode, "StrikeSpreads", swapVolStrikeSpreads_);
        }
        if (swapVolDayCounters_.size() > 0) {
            XMLNode* node = XMLUtils::addChild(doc, swaptionVolatilitiesNode, "DayCounters");
            for (auto dc : swapVolDayCounters_) {
                XMLNode* c = doc.allocNode("DayCounter", dc.second);
                XMLUtils::addAttribute(doc, c, "ccy", dc.first);
                XMLUtils::appendNode(node, c);
            }
        }
    }

    // yield volatilities
    if (!yieldVolNames().empty()) {
        DLOG("Writing yield volatilities");
        XMLNode* yieldVolatilitiesNode = XMLUtils::addChild(doc, marketNode, "YieldVolatilities");
        XMLUtils::addChild(doc, yieldVolatilitiesNode, "Simulate", simulateYieldVols());
        XMLUtils::addChild(doc, yieldVolatilitiesNode, "ReactionToTimeDecay", yieldVolDecayMode_);
        XMLUtils::addChildren(doc, yieldVolatilitiesNode, "Names", "Name", yieldVolNames());
        XMLUtils::addGenericChildAsList(doc, yieldVolatilitiesNode, "Expiries", yieldVolExpiries_);
        XMLUtils::addGenericChildAsList(doc, yieldVolatilitiesNode, "Terms", yieldVolTerms_);
    }

    // cap/floor volatilities
    if (!capFloorVolCcys().empty()) {
        DLOG("Writing cap/floor volatilities");
        XMLNode* capFloorVolatilitiesNode = XMLUtils::addChild(doc, marketNode, "CapFloorVolatilities");
        XMLUtils::addChild(doc, capFloorVolatilitiesNode, "Simulate", simulateCapFloorVols());
        XMLUtils::addChild(doc, capFloorVolatilitiesNode, "ReactionToTimeDecay", capFloorVolDecayMode_);
        XMLUtils::addChildren(doc, capFloorVolatilitiesNode, "Currencies", "Currency", capFloorVolCcys());
        XMLUtils::addGenericChildAsList(doc, capFloorVolatilitiesNode, "Expiries",
                                        returnTenors(capFloorVolExpiries_, ""));
        // TODO write other keys
        XMLUtils::addGenericChildAsList(doc, capFloorVolatilitiesNode, "Strikes", capFloorVolStrikes_);
        if (capFloorVolDayCounters_.size() > 0) {
            XMLNode* node = XMLUtils::addChild(doc, capFloorVolatilitiesNode, "DayCounters");
            for (auto dc : capFloorVolDayCounters_) {
                XMLNode* c = doc.allocNode("DayCounter", dc.second);
                XMLUtils::addAttribute(doc, c, "ccy", dc.first);
                XMLUtils::appendNode(node, c);
            }
        }
    }

    if (!cdsVolNames().empty()) {
        DLOG("Writing CDS volatilities");
        XMLNode* cdsVolatilitiesNode = XMLUtils::addChild(doc, marketNode, "CDSVolatilities");
        XMLUtils::addChild(doc, cdsVolatilitiesNode, "Simulate", simulateCdsVols());
        XMLUtils::addChild(doc, cdsVolatilitiesNode, "ReactionToTimeDecay", cdsVolDecayMode_);
        XMLUtils::addChildren(doc, cdsVolatilitiesNode, "Names", "Name", cdsVolNames());
        XMLUtils::addGenericChildAsList(doc, cdsVolatilitiesNode, "Expiries", cdsVolExpiries_);
    }

    // fx volatilities
    if (!fxVolCcyPairs().empty()) {
        DLOG("Writing FX volatilities");
        XMLNode* fxVolatilitiesNode = XMLUtils::addChild(doc, marketNode, "FxVolatilities");
        XMLUtils::addChild(doc, fxVolatilitiesNode, "Simulate", simulateFXVols());
        XMLUtils::addChild(doc, fxVolatilitiesNode, "ReactionToTimeDecay", fxVolDecayMode_);
        XMLUtils::addChildren(doc, fxVolatilitiesNode, "CurrencyPairs", "CurrencyPair", fxVolCcyPairs());
        XMLUtils::addGenericChildAsList(doc, fxVolatilitiesNode, "Expiries", fxVolExpiries_);
        if (fxVolDayCounters_.size() > 0) {
            XMLNode* node = XMLUtils::addChild(doc, fxVolatilitiesNode, "DayCounters");
            for (auto dc : fxVolDayCounters_) {
                XMLNode* c = doc.allocNode("DayCounter", dc.second);
                XMLUtils::addAttribute(doc, c, "ccyPair", dc.first);
                XMLUtils::appendNode(node, c);
            }
        }
    }

    // eq volatilities
    if (!equityVolNames().empty()) {
        DLOG("Writing equity volatilities");
        XMLNode* eqVolatilitiesNode = XMLUtils::addChild(doc, marketNode, "EquityVolatilities");
        XMLUtils::addChild(doc, eqVolatilitiesNode, "Simulate", simulateEquityVols());
        XMLUtils::addChild(doc, eqVolatilitiesNode, "ReactionToTimeDecay", equityVolDecayMode_);
        XMLUtils::addChildren(doc, eqVolatilitiesNode, "Names", "Name", equityVolNames());
        XMLUtils::addGenericChildAsList(doc, eqVolatilitiesNode, "Expiries", equityVolExpiries_);
        if (equityIsSurface_) {
            XMLNode* eqSurfaceNode = XMLUtils::addChild(doc, eqVolatilitiesNode, "Surface");
            XMLUtils::addGenericChildAsList(doc, eqSurfaceNode, "Moneyness", equityMoneyness_);
        }
    }

    // benchmark yield curves
    for (Size i = 0; i < yieldCurveNames().size(); ++i) {
        DLOG("Writing benchmark yield curves data");
        XMLNode* benchmarkCurvesNode = XMLUtils::addChild(doc, marketNode, "BenchmarkCurves");
        XMLNode* benchmarkCurveNode = XMLUtils::addChild(doc, benchmarkCurvesNode, "BenchmarkCurve");
        XMLUtils::addChild(doc, benchmarkCurveNode, "Currency", yieldCurveCurrencies_[yieldCurveNames()[i]]);
        XMLUtils::addChild(doc, benchmarkCurveNode, "Name", yieldCurveNames()[i]);
    }

    // securities
    if (!securities().empty()) {
        DLOG("Writing securities");
        XMLNode* secNode = XMLUtils::addChild(doc, marketNode, "Securities");
        XMLUtils::addChild(doc, secNode, "Simulate", securitySpreadsSimulate());
        XMLUtils::addChildren(doc, secNode, "Securities", "Security", securities());
    }

    // cprs
    if (!cprs().empty()) {
        DLOG("Writing cprs");
        XMLNode* cprNode = XMLUtils::addChild(doc, marketNode, "CPRs");
        XMLUtils::addChild(doc, cprNode, "Simulate", simulateCprs());
        XMLUtils::addChildren(doc, cprNode, "Names", "Name", cprs());
    }

    // inflation indices
    if (!cpiIndices().empty()) {
        DLOG("Writing inflation indices");
        XMLNode* cpiNode = XMLUtils::addChild(doc, marketNode, "CpiInflationIndices");
        XMLUtils::addChildren(doc, cpiNode, "CpiIndices", "Index", cpiIndices());
    }

    // zero inflation
    if (!zeroInflationIndices().empty()) {
        DLOG("Writing zero inflation");
        XMLNode* zeroNode = XMLUtils::addChild(doc, marketNode, "ZeroInflationIndexCurves");
        XMLUtils::addChildren(doc, zeroNode, "Names", "Name", zeroInflationIndices());
        XMLUtils::addGenericChildAsList(doc, zeroNode, "Tenors", returnTenors(zeroInflationTenors_, ""));
        if (zeroInflationDayCounters_.size() > 0) {
            XMLNode* node = XMLUtils::addChild(doc, zeroNode, "DayCounters");
            for (auto dc : zeroInflationDayCounters_) {
                XMLNode* c = doc.allocNode("DayCounter", dc.second);
                XMLUtils::addAttribute(doc, c, "name", dc.first);
                XMLUtils::appendNode(node, c);
            }
        }
    }

    // yoy inflation
    if (!yoyInflationIndices().empty()) {
        DLOG("Writing year-on-year inflation");
        XMLNode* yoyNode = XMLUtils::addChild(doc, marketNode, "YYInflationIndexCurves");
        XMLUtils::addChildren(doc, yoyNode, "Names", "Name", yoyInflationIndices());
        XMLUtils::addGenericChildAsList(doc, yoyNode, "Tenors", returnTenors(yoyInflationTenors_, ""));

        if (yoyInflationDayCounters_.size() > 0) {
            XMLNode* node = XMLUtils::addChild(doc, yoyNode, "DayCounters");
            for (auto dc : yoyInflationDayCounters_) {
                XMLNode* c = doc.allocNode("DayCounter", dc.second);
                XMLUtils::addAttribute(doc, c, "name", dc.first);
                XMLUtils::appendNode(node, c);
            }
        }
    }

    // Commodity price curves
    if (!commodityNames().empty()) {
        DLOG("Writing commodity price curves");
        XMLNode* commodityPriceNode = XMLUtils::addChild(doc, marketNode, "Commodities");
        XMLUtils::addChild(doc, commodityPriceNode, "Simulate", commodityCurveSimulate());
        XMLUtils::addChildren(doc, commodityPriceNode, "Names", "Name", commodityNames());
        XMLUtils::addGenericChildAsList(doc, commodityPriceNode, "Tenors", commodityCurveTenors_.at(""));
        XMLUtils::addChild(doc, commodityPriceNode, "DayCounter", commodityCurveDayCounters_.at(""));
    }

    // Commodity volatilities
    if (!commodityVolNames().empty()) {
        DLOG("Writing commodity volatilities");
        XMLNode* commodityVolatilitiesNode = XMLUtils::addChild(doc, marketNode, "CommodityVolatilities");
        XMLUtils::addChild(doc, commodityVolatilitiesNode, "Simulate", commodityVolSimulate());
        XMLUtils::addChild(doc, commodityVolatilitiesNode, "ReactionToTimeDecay", commodityVolDecayMode_);
        XMLNode* namesNode = XMLUtils::addChild(doc, commodityVolatilitiesNode, "Names");
        for (const auto& name : commodityVolNames()) {
            XMLNode* nameNode = doc.allocNode("Name");
            XMLUtils::addAttribute(doc, nameNode, "id", name);
            XMLUtils::addGenericChildAsList(doc, nameNode, "Expiries", commodityVolExpiries_[name]);
            XMLUtils::addGenericChildAsList(doc, nameNode, "Moneyness", commodityVolMoneyness_[name]);
            XMLUtils::appendNode(namesNode, nameNode);
        }
        XMLUtils::addChild(doc, commodityVolatilitiesNode, "DayCounter", commodityVolDayCounters_.at(""));
    }

    // additional scenario data currencies
    if (!additionalScenarioDataCcys_.empty()) {
        DLOG("Writing aggregation scenario data currencies");
        XMLUtils::addChildren(doc, marketNode, "AggregationScenarioDataCurrencies", "Currency",
            additionalScenarioDataCcys_);
    }

    // additional scenario data indices
    if (!additionalScenarioDataIndices_.empty()) {
        DLOG("Writing aggregation scenario data indices");
        XMLUtils::addChildren(doc, marketNode, "AggregationScenarioDataIndices", "Index",
            additionalScenarioDataIndices_);
    }

    // base correlations
    if (!baseCorrelationNames().empty()) {
        DLOG("Writing base correlations");
        XMLNode* bcNode = XMLUtils::addChild(doc, marketNode, "BaseCorrelations");
        XMLUtils::addChild(doc, bcNode, "Simulate", simulateBaseCorrelations());
        XMLUtils::addChildren(doc, bcNode, "IndexNames", "IndexName", baseCorrelationNames());
        XMLUtils::addGenericChildAsList(doc, bcNode, "Terms", baseCorrelationTerms_);
        XMLUtils::addGenericChildAsList(doc, bcNode, "DetachmentPoints", baseCorrelationDetachmentPoints_);
        if (yoyInflationDayCounters_.size() > 0) {
            XMLNode* node = XMLUtils::addChild(doc, bcNode, "DayCounters");
            for (auto dc : baseCorrelationDayCounters_) {
                XMLNode* c = doc.allocNode("DayCounter", dc.second);
                XMLUtils::addAttribute(doc, c, "name", dc.first);
                XMLUtils::appendNode(node, c);
            }
        }
    }

    // correlations
    if (!correlationPairs().empty()) {
        DLOG("Writing correlation");
        XMLNode* correlationsNode = XMLUtils::addChild(doc, marketNode, "Correlations");
        XMLUtils::addChild(doc, correlationsNode, "Simulate", simulateCorrelations());
        XMLUtils::addChildren(doc, correlationsNode, "Pairs", "Pair", correlationPairs());

        XMLUtils::addGenericChildAsList(doc, correlationsNode, "Expiries", correlationExpiries_);
        if (correlationDayCounters_.size() > 0) {
            XMLNode* node = XMLUtils::addChild(doc, correlationsNode, "DayCounters");
            for (auto dc : correlationDayCounters_) {
                XMLNode* c = doc.allocNode("DayCounter", dc.second);
                XMLUtils::addAttribute(doc, c, "index1", dc.first.first);
                XMLUtils::addAttribute(doc, c, "index2", dc.first.second);
                XMLUtils::appendNode(node, c);
            }
        }
    }

    return marketNode;
}
} // namespace analytics
} // namespace ore
