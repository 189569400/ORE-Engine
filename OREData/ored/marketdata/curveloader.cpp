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

#include <ored/marketdata/curveloader.hpp>
#include <ored/marketdata/structuredcurveerror.hpp>
#include <ored/utilities/log.hpp>
#include <ql/errors.hpp>

using namespace boost;
using QuantLib::Size;

namespace ore {
namespace data {

// Returns true if we can build this YieldCurveSpec with the given curve specs
static bool canBuild(boost::shared_ptr<YieldCurveSpec>& ycs, vector<boost::shared_ptr<YieldCurveSpec>>& specs,
                     const CurveConfigurations& curveConfigs, map<string, string>& missingDependents,
                     map<string, string>& errors, bool continueOnError) {

    string yieldCurveID = ycs->curveConfigID();
    if (!curveConfigs.hasYieldCurveConfig(yieldCurveID)) {
        string errMsg = "Can't get yield curve configuration for " + yieldCurveID;
        if (continueOnError) {
            errors[ycs->name()] = errMsg;
            TLOG(errMsg);
            return false;
        } else {
            QL_FAIL(errMsg);
        }
    }

    boost::shared_ptr<YieldCurveConfig> curveConfig = curveConfigs.yieldCurveConfig(yieldCurveID);
    set<string> requiredYieldCurveIDs = curveConfig->requiredYieldCurveIDs();
    for (auto it : requiredYieldCurveIDs) {
        // search for this name in the vector specs, return false if not found, otherwise move to next required id
        bool ok = false;
        for (Size i = 0; i < specs.size(); i++) {
            if (specs[i]->curveConfigID() == it)
                ok = true;
        }
        if (!ok) {
            DLOG("required yield curve " << it << " for " << yieldCurveID << " not (yet) available");
            missingDependents[yieldCurveID] = it;
            return false;
        }
    }

    // We can build everything required
    missingDependents[yieldCurveID] = "";
    return true;
}

void order(vector<boost::shared_ptr<CurveSpec>>& curveSpecs, const CurveConfigurations& curveConfigs,
           std::map<std::string, std::string>& errors, bool continueOnError) {

    /* Order the curve specs and remove duplicates (i.e. those with same name).
     * The sort() call relies on CurveSpec::operator< which ensures a few properties:
     * - FX loaded before FXVol
     * - Eq loaded before EqVol
     * - Inf loaded before InfVol
     * - rate curves, swap indices, swaption vol surfaces before correlation curves
     */
    sort(curveSpecs.begin(), curveSpecs.end());
    auto itSpec = unique(curveSpecs.begin(), curveSpecs.end());
    curveSpecs.resize(std::distance(curveSpecs.begin(), itSpec));

    /* remove the YieldCurveSpecs from curveSpecs
     */
    vector<boost::shared_ptr<YieldCurveSpec>> yieldCurveSpecs;
    itSpec = curveSpecs.begin();
    while (itSpec != curveSpecs.end()) {
        if ((*itSpec)->baseType() == CurveSpec::CurveType::Yield) {
            boost::shared_ptr<YieldCurveSpec> spec = boost::dynamic_pointer_cast<YieldCurveSpec>(*itSpec);
            yieldCurveSpecs.push_back(spec);
            itSpec = curveSpecs.erase(itSpec);
        } else
            ++itSpec;
    }

    /* Now sort the yieldCurveSpecs, store them in sortedYieldCurveSpecs  */
    vector<boost::shared_ptr<YieldCurveSpec>> sortedYieldCurveSpecs;

    /* Map to sort the missing dependencies */
    map<string, string> missingDependents;

    /* Loop over yieldCurveSpec, remove all curvespecs that we can build by checking sortedYieldCurveSpecs
     * Repeat until yieldCurveSpec is empty
     */
    while (yieldCurveSpecs.size() > 0) {
        Size n = yieldCurveSpecs.size();

        auto it = yieldCurveSpecs.begin();
        while (it != yieldCurveSpecs.end()) {
            if (canBuild(*it, sortedYieldCurveSpecs, curveConfigs, missingDependents, errors, continueOnError)) {
                DLOG("can build " << (*it)->curveConfigID());
                sortedYieldCurveSpecs.push_back(*it);
                it = yieldCurveSpecs.erase(it);
            } else {
                DLOG("can not (yet) build " << (*it)->curveConfigID());
                ++it;
            }
        }

        if (n == yieldCurveSpecs.size()) {
            for (auto ycs : yieldCurveSpecs) {
                if (errors.count(ycs->name()) > 0) {
                    WLOG("Cannot build curve " << ycs->curveConfigID() << " due to error: " << errors.at(ycs->name()));
                } else {
                    WLOG("Cannot build curve " << ycs->curveConfigID() << ", dependent curves missing");
                    errors[ycs->name()] = "dependent curves missing - " + missingDependents[ycs->curveConfigID()];
                }
                ALOG(StructuredCurveErrorMessage(ycs->curveConfigID(), "Cannot build curve", errors.at(ycs->name())));
            }
            break;
        }
    }

    /* Now put them into the front of curveSpecs */
    curveSpecs.insert(curveSpecs.begin(), sortedYieldCurveSpecs.begin(), sortedYieldCurveSpecs.end());

    DLOG("Ordered Curves (" << curveSpecs.size() << ")")
    for (Size i = 0; i < curveSpecs.size(); ++i)
        DLOG(std::setw(2) << i << " " << curveSpecs[i]->name());
}
} // namespace data
} // namespace ore
