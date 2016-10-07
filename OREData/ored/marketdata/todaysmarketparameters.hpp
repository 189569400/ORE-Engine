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

/*! \file ored/marketdata/todaysmarketparameters.hpp
    \brief A class to hold todays market configuration(s)
    \ingroup curves
*/

#pragma once

#include <ored/marketdata/market.hpp>
#include <ored/utilities/xmlutils.hpp>
#include <ored/utilities/parsers.hpp>
#include <ored/utilities/log.hpp>

using std::vector;
using std::string;
using std::pair;
using ore::data::XMLSerializable;
using ore::data::XMLDocument;
using ore::data::XMLNode;
using ore::data::XMLUtils;

namespace ore {
namespace data {

//! Market Configuration structure
/*!
  The Market Configuration bundles configurations for each of the
  following
  - discount curves
  - yield curves
  - index forwarding curves
  - FX spots
  - FX volatilities
  - swaption volatilities
  - cap/floor volatilities
  - default curves
  - swap index forwarding curves
  and assigns a configuration ID.

  Several Market Configurations can be specified and held in a market object in parallel.
  Applications then need to specify the desired market configuration ID when making calls
  to any of the term structures provided by the market object.

  \ingroup curves
 */
struct MarketConfiguration {
    MarketConfiguration()
        : discountingCurvesId(Market::defaultConfiguration), yieldCurvesId(Market::defaultConfiguration),
          indexForwardingCurvesId(Market::defaultConfiguration), fxSpotsId(Market::defaultConfiguration),
          fxVolatilitiesId(Market::defaultConfiguration), swaptionVolatilitiesId(Market::defaultConfiguration),
          defaultCurvesId(Market::defaultConfiguration), swapIndexCurvesId(Market::defaultConfiguration),
          capFloorVolatilitiesId(Market::defaultConfiguration) {}
    string discountingCurvesId, yieldCurvesId, indexForwardingCurvesId, fxSpotsId, fxVolatilitiesId,
        swaptionVolatilitiesId, defaultCurvesId, swapIndexCurvesId, capFloorVolatilitiesId;
};

bool operator==(const MarketConfiguration& lhs, const MarketConfiguration& rhs);

//! Today's Market Parameters
/*!
  This class is a container of instructions (all text) for how to build
  a market object.

  An instance of this object is needed in order to call a TodaysMarket
  constructor.

  \ingroup curves
 */
class TodaysMarketParameters : public XMLSerializable {
public:
    //! Default constructor
    TodaysMarketParameters() {}

    //! \name Inspectors
    //@{
    const map<string, MarketConfiguration>& configurations() const;
    bool hasConfiguration(const string& configuration) const;

    //! EUR => Yield/EUR/EUR6M, USD => Yield/USD/USD3M etc.
    const map<string, string>& discountingCurves(const string& configuration) const;

    //! EUR => Yield/EUR/BANK_EUR_LEND, Yield/EUR/BANK_EUR_BORROW etc.
    const map<string, string>& yieldCurves(const string& configuration) const;

    //! EUR-EURIBOR-1M => Yield/EUR/EUR3M, EUR-EURIBOR-6M => Yield/EUR/EUR6M etc.
    const map<string, string>& indexForwardingCurves(const string& configuration) const;

    //! EUR-CMS-2Y => EUR-EONIA
    const map<string, string>& swapIndices(const string& configuration) const;

    //! EURUSD => FX/EUR/USD, EURGBP => FX/EUR/GBP etc.
    const map<string, string>& fxSpots(const string& configuration) const;

    //! EURUSD => FXVolatility/EUR/USD/EURUSD, EURGBP => FXVolatility/EUR/GBP/EURGBP etc.
    const map<string, string>& fxVolatilities(const string& configuration) const;

    //! EUR => SwaptionVolatility/EUR/EUR_SW_N, GBP => SwaptionVolatility/GBP/GBP_SW_SLN etc.
    const map<string, string>& swaptionVolatilities(const string& configuration) const;

    //! ENT_1 => Default/USD/ENT_1_SR_USD, ENT_2 => Default/USD/ENT_2_SR_USD etc.
    const map<string, string>& defaultCurves(const string& configuration) const;

    // EUR => CapFloorVolatility/EUR/EUR_CF_N,
    // GBP => CapFloorVolatility/GBP/GBP_CF_LN etc.
    const map<string, string>& capFloorVolatilities(const string& configuration) const;

    //! Build a vector of all the curve specs (may contain duplicates)
    vector<string> curveSpecs(const string& configuration) const;

    //! Individual term structure ids for a given configuration
    const string& discountingCurvesId(const string& configuration) const;
    const string& yieldCurvesId(const string& configuration) const;
    const string& indexForwardingCurvesId(const string& configuration) const;
    const string& swapIndexCurvesId(const string& configuration) const;
    const string& fxSpotsId(const string& configuration) const;
    const string& fxVolatilitiesId(const string& configuration) const;
    const string& swaptionVolatilitiesId(const string& configuration) const;
    const string& defaultCurvesId(const string& configuration) const;
    const string& capFloorVolatilitiesId(const string& configuration) const;
    //@}

    //! \name Setters
    //@{
    void addConfiguration(const string& name, const MarketConfiguration& configuration);
    void addDiscountingCurves(const string& id, const map<string, string>& assignments);
    void addYieldCurves(const string& id, const map<string, string>& assignments);
    void addIndexForwardingCurves(const string& id, const map<string, string>& assignments);
    void addSwapIndices(const string& id, const map<string, string>& assignments);
    void addFxSpots(const string& id, const map<string, string>& assignments);
    void addFxVolatilities(const string& id, const map<string, string>& assignemnts);
    void addSwaptionVolatilities(const string& id, const map<string, string>& assignments);
    void addDefaultCurves(const string& id, const map<string, string>& assignments);
    void addCapFloorVolatilities(const string& id, const map<string, string>& assignments);
    //@}

    //! \name Serialisation
    //@{
    void fromXML(XMLNode* node);
    XMLNode* toXML(XMLDocument& doc);
    //@}

    //! \Equality Operators
    //@{
    bool operator==(TodaysMarketParameters& rhs);
    bool operator!=(TodaysMarketParameters& rhs);
    //@}

private:
    // maps configuration name to id list
    map<string, MarketConfiguration> configurations_;
    // maps id to map (key,value)
    map<string, map<string, string>> discountingCurves_, yieldCurves_, indexForwardingCurves_, fxSpots_,
        fxVolatilities_, swaptionVolatilities_, defaultCurves_, capFloorVolatilities_;
    map<string, map<string, string>> swapIndices_;

    void curveSpecs(const map<string, map<string, string>>&, const string&, vector<string>&) const;
};

// inline

inline const map<string, MarketConfiguration>& TodaysMarketParameters::configurations() const {
    return configurations_;
}

inline bool TodaysMarketParameters::hasConfiguration(const string& configuration) const {
    auto it = configurations_.find(configuration);
    return it != configurations_.end();
}

inline const string& TodaysMarketParameters::discountingCurvesId(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    return configurations_.at(configuration).discountingCurvesId;
}

inline const string& TodaysMarketParameters::yieldCurvesId(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    return configurations_.at(configuration).yieldCurvesId;
}

inline const string& TodaysMarketParameters::indexForwardingCurvesId(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    return configurations_.at(configuration).indexForwardingCurvesId;
}

inline const string& TodaysMarketParameters::swapIndexCurvesId(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    return configurations_.at(configuration).swapIndexCurvesId;
}

inline const string& TodaysMarketParameters::fxSpotsId(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    return configurations_.at(configuration).fxSpotsId;
}

inline const string& TodaysMarketParameters::fxVolatilitiesId(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    return configurations_.at(configuration).fxVolatilitiesId;
}

inline const string& TodaysMarketParameters::swaptionVolatilitiesId(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    return configurations_.at(configuration).swaptionVolatilitiesId;
}

inline const string& TodaysMarketParameters::defaultCurvesId(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    return configurations_.at(configuration).defaultCurvesId;
}

inline const string& TodaysMarketParameters::capFloorVolatilitiesId(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    return configurations_.at(configuration).capFloorVolatilitiesId;
}

inline const map<string, string>& TodaysMarketParameters::discountingCurves(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    auto it = discountingCurves_.find(discountingCurvesId(configuration));
    QL_REQUIRE(it != discountingCurves_.end(), "discounting curves with id " << discountingCurvesId(configuration)
                                                                             << " specified in configuration "
                                                                             << configuration << " not found");
    return it->second;
}

inline const map<string, string>& TodaysMarketParameters::yieldCurves(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    auto it = yieldCurves_.find(yieldCurvesId(configuration));
    QL_REQUIRE(it != yieldCurves_.end(), "yield curves with id " << yieldCurvesId(configuration)
                                                                 << " specified in configuration " << configuration
                                                                 << " not found");
    return it->second;
}

inline const map<string, string>& TodaysMarketParameters::indexForwardingCurves(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    auto it = indexForwardingCurves_.find(indexForwardingCurvesId(configuration));
    QL_REQUIRE(it != indexForwardingCurves_.end(),
               "index forwarding curves with id " << indexForwardingCurvesId(configuration)
                                                  << " specified in configuration " << configuration << " not found");
    return it->second;
}

inline const map<string, string>& TodaysMarketParameters::swapIndices(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    auto it = swapIndices_.find(swapIndexCurvesId(configuration));
    QL_REQUIRE(it != swapIndices_.end(), "swap index curves with id " << swapIndexCurvesId(configuration)
                                                                      << " specified in configuration " << configuration
                                                                      << " not found");
    return it->second;
}

inline const map<string, string>& TodaysMarketParameters::fxSpots(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    auto it = fxSpots_.find(fxSpotsId(configuration));
    QL_REQUIRE(it != fxSpots_.end(), "fx spots curves with id " << fxSpotsId(configuration)
                                                                << " specified in configuration " << configuration
                                                                << " not found");
    return it->second;
}

inline const map<string, string>& TodaysMarketParameters::fxVolatilities(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    auto it = fxVolatilities_.find(fxVolatilitiesId(configuration));
    QL_REQUIRE(it != fxVolatilities_.end(), "fx volatilities with id " << fxVolatilitiesId(configuration)
                                                                       << " specified in configuration "
                                                                       << configuration << " not found");
    return it->second;
}

inline const map<string, string>& TodaysMarketParameters::swaptionVolatilities(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    auto it = swaptionVolatilities_.find(swaptionVolatilitiesId(configuration));
    QL_REQUIRE(it != swaptionVolatilities_.end(),
               "swaption volatilities with id " << swaptionVolatilitiesId(configuration)
                                                << " specified in configuration " << configuration << " not found");
    return it->second;
}

inline const map<string, string>& TodaysMarketParameters::capFloorVolatilities(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    auto it = capFloorVolatilities_.find(capFloorVolatilitiesId(configuration));
    QL_REQUIRE(it != capFloorVolatilities_.end(),
               "cap/floor volatilities with id " << capFloorVolatilitiesId(configuration)
                                                 << " specified in configuration " << configuration << " not found");
    return it->second;
}

inline const map<string, string>& TodaysMarketParameters::defaultCurves(const string& configuration) const {
    QL_REQUIRE(hasConfiguration(configuration), "configuration " << configuration << " not found");
    auto it = defaultCurves_.find(defaultCurvesId(configuration));
    QL_REQUIRE(it != defaultCurves_.end(), "default curves with id " << defaultCurvesId(configuration)
                                                                     << " specified in configuration " << configuration
                                                                     << " not found");
    return it->second;
}

inline void TodaysMarketParameters::addConfiguration(const string& id, const MarketConfiguration& configuration) {
    configurations_[id] = configuration;
}

inline void TodaysMarketParameters::addDiscountingCurves(const string& id, const map<string, string>& assignments) {
    discountingCurves_[id] = assignments;
    for (auto s : assignments)
        DLOG("TodaysMarketParameters, add discounting curves: " << id << " " << s.first << " " << s.second);
}

inline void TodaysMarketParameters::addYieldCurves(const string& id, const map<string, string>& assignments) {
    yieldCurves_[id] = assignments;
    for (auto s : assignments)
        DLOG("TodaysMarketParameters, add yield curves: " << id << " " << s.first << " " << s.second);
}

inline void TodaysMarketParameters::addIndexForwardingCurves(const string& id, const map<string, string>& assignments) {
    indexForwardingCurves_[id] = assignments;
    for (auto s : assignments)
        DLOG("TodaysMarketParameters, add index forwarding curves: " << id << " " << s.first << " " << s.second);
}

inline void TodaysMarketParameters::addSwapIndices(const string& id, const map<string, string>& assignments) {
    swapIndices_[id] = assignments;
    for (auto s : assignments)
        DLOG("TodaysMarketParameters, add swap indexes: " << id << " " << s.first << " " << s.second);
}

inline void TodaysMarketParameters::addFxSpots(const string& id, const map<string, string>& assignments) {
    fxSpots_[id] = assignments;
    for (auto s : assignments)
        DLOG("TodaysMarketParameters, add fx spots: " << id << " " << s.first << " " << s.second);
}

inline void TodaysMarketParameters::addFxVolatilities(const string& id, const map<string, string>& assignments) {
    fxVolatilities_[id] = assignments;
    for (auto s : assignments)
        DLOG("TodaysMarketParameters, add fx volatilities: " << id << " " << s.first << " " << s.second);
}

inline void TodaysMarketParameters::addSwaptionVolatilities(const string& id, const map<string, string>& assignments) {
    swaptionVolatilities_[id] = assignments;
    for (auto s : assignments)
        DLOG("TodaysMarketParameters, add swaption volatilities: " << id << " " << s.first << " " << s.second);
}

inline void TodaysMarketParameters::addCapFloorVolatilities(const string& id, const map<string, string>& assignments) {
    capFloorVolatilities_[id] = assignments;
    for (auto s : assignments)
        DLOG("TodaysMarketParameters, add cap/floor volatilities: " << id << " " << s.first << " " << s.second);
}

inline void TodaysMarketParameters::addDefaultCurves(const string& id, const map<string, string>& assignments) {
    defaultCurves_[id] = assignments;
    for (auto s : assignments)
        DLOG("TodaysMarketParameters, add default curves: " << id << " " << s.first << " " << s.second);
}

} // namespace data
} // namesapce openriskengine
