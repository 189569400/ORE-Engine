/*
 Copyright (C) 2017 Quaternion Risk Management Ltd
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

/*! \file orea/cube/sensitivitycube.hpp
    \brief holds a grid of NPVs for a list of trades under various scenarios
    \ingroup Cube
*/

#pragma once

#include <boost/bimap.hpp>
#include <boost/shared_ptr.hpp>
#include <orea/cube/npvsensicube.hpp>
#include <orea/scenario/shiftscenariogenerator.hpp>
#include <ql/errors.hpp>
#include <ql/time/date.hpp>
#include <ql/types.hpp>
#include <vector>

namespace ore {
namespace analytics {

//! SensitivityCube is a wrapper for an npvCube that gives easier access to the underlying cube elements
class SensitivityCube {

public:
    typedef std::pair<RiskFactorKey, RiskFactorKey> crossPair;
    typedef ShiftScenarioGenerator::ScenarioDescription ShiftScenarioDescription;

    //! Constructor using a vector of scenario descriptions
    SensitivityCube(const boost::shared_ptr<NPVSensiCube>& cube,
                    const std::vector<ShiftScenarioDescription>& scenarioDescriptions,
                    const std::map<RiskFactorKey, QuantLib::Real>& shiftSizes);

    //! Constructor using a vector of scenario description strings
    SensitivityCube(const boost::shared_ptr<NPVSensiCube>& cube, const std::vector<std::string>& scenarioDescriptions,
                    const std::map<RiskFactorKey, QuantLib::Real>& shiftSizes);

    //! \name Inspectors
    //@{
    const boost::shared_ptr<NPVSensiCube>& npvCube() const { return cube_; }
    const std::vector<ShiftScenarioDescription>& scenarioDescriptions() const { return scenarioDescriptions_; }
    const std::vector<string>& tradeIds() const { return cube_->ids(); }
    //@}

    //! Check if the cube has scenario NPVs for trade with ID \p tradeId
    bool hasTrade(const std::string& tradeId) const;

    /*! Return factor for given up/down scenario index or None if given index
      is not an up/down scenario (to be reviewed) */
    RiskFactorKey upDownFactor(const Size upDownIndex) const;

    //! Check if the cube has scenario NPVs for scenario with description \p scenarioDescription
    bool hasScenario(const ShiftScenarioDescription& scenarioDescription) const;

    //! Get the description for the risk factor key \p riskFactorKey
    //! Returns the result of ShiftScenarioGenerator::ScenarioDescription::factor1()
    std::string factorDescription(const RiskFactorKey& riskFactorKey) const;

    //! Returns the set of risk factor keys for which a delta and gamma can be calculated
    const std::set<RiskFactorKey>& factors() const;

    //! Returns the set of pairs of risk factor keys for which a cross gamma is available
    const std::set<crossPair>& crossFactors() const;

    //! Returns the absolute shift size for given risk factor \p key
    QuantLib::Real shiftSize(const RiskFactorKey& riskFactorKey) const;

    //! Get the base NPV for trade with ID \p tradeId
    QuantLib::Real npv(const std::string& tradeId) const;

    //! Get the NPV with scenario description \p scenarioDescription for trade with ID \p tradeId
    QuantLib::Real npv(const std::string& tradeId, const ShiftScenarioDescription& scenarioDescription) const;

    //! Get the trade delta for trade with ID \p tradeId and for the given risk factor key \p riskFactorKey
    QuantLib::Real delta(const std::string& tradeId, const RiskFactorKey& riskFactorKey) const;

    //! Get the trade gamma for trade with ID \p tradeId and for the given risk factor key \p riskFactorKey
    QuantLib::Real gamma(const std::string& tradeId, const RiskFactorKey& riskFactorKey) const;

    //! Get the trade cross gamma for trade with ID \p tradeId and for the given risk factor key pair \p
    //! riskFactorKeyPair
    QuantLib::Real crossGamma(const std::string& tradeId, const crossPair& riskFactorKeyPair) const;

private:
    //! Initialise method used by the constructors
    void initialise();

    boost::shared_ptr<NPVSensiCube> cube_;
    std::vector<ShiftScenarioDescription> scenarioDescriptions_;
    std::map<RiskFactorKey, QuantLib::Real> shiftSizes_;

    // Duplication between map keys below and these sets but trade-off
    // Means that we can return by reference in public inspector methods
    std::set<RiskFactorKey> factors_;
    std::set<crossPair> crossPairs_;

    // Maps for faster lookup of cube entries. They are populated in the constructor
    // TODO: Review this i.e. could it be done better / using less memory
    std::map<std::string, QuantLib::Size> tradeIdx_;
    std::map<ShiftScenarioDescription, QuantLib::Size> scenarioIdx_;
    boost::bimap<RiskFactorKey, QuantLib::Size> upFactors_;
    std::map<RiskFactorKey, QuantLib::Size> downFactors_;
    std::map<crossPair, QuantLib::Size> crossFactors_;
};

} // namespace analytics
} // namespace ore
