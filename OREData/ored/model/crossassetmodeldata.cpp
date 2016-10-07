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

#include <ored/model/crossassetmodeldata.hpp>
#include <ored/utilities/parsers.hpp>
#include <ored/utilities/correlationmatrix.hpp>
#include <ored/utilities/log.hpp>

#include <qle/models/irlgm1fpiecewiseconstantparametrization.hpp>
#include <qle/models/irlgm1fpiecewiselinearparametrization.hpp>
#include <qle/models/irlgm1fconstantparametrization.hpp>
#include <qle/models/irlgm1fpiecewiseconstanthullwhiteadaptor.hpp>
#include <qle/models/fxbsconstantparametrization.hpp>
#include <qle/models/fxbspiecewiseconstantparametrization.hpp>
// #include <qle/pricingengines/analyticlgmswaptionengine.hpp>
// #include <qle/pricingengines/analyticcclgmfxoptionengine.hpp>
//#include <qle/models/fxoptionhelper.hpp>

#include <ql/math/optimization/levenbergmarquardt.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/models/shortrate/calibrationhelpers/swaptionhelper.hpp>
#include <ql/utilities/dataformatters.hpp>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/lexical_cast.hpp>

namespace ore {
namespace data {

bool CrossAssetModelData::operator==(const CrossAssetModelData& rhs) {

    if (domesticCurrency_ != rhs.domesticCurrency_ || currencies_ != rhs.currencies_ ||
        correlations_ != rhs.correlations_ || bootstrapTolerance_ != rhs.bootstrapTolerance_ ||
        irConfigs_.size() != rhs.irConfigs_.size() || fxConfigs_.size() != rhs.fxConfigs_.size()) {
        return false;
    }

    for (int i = 0; i < irConfigs_.size(); i++) {
        if (*irConfigs_[i] != *(rhs.irConfigs_[i])) {
            return false;
        }
    }

    for (int i = 0; i < fxConfigs_.size(); i++) {
        if (*fxConfigs_[i] != *(rhs.fxConfigs_[i])) {
            return false;
        }
    }

    return true;
}

bool CrossAssetModelData::operator!=(const CrossAssetModelData& rhs) { return !(*this == rhs); }

void CrossAssetModelData::clear() {
    currencies_.clear();
    irConfigs_.clear();
    fxConfigs_.clear();
    correlations_.clear();
}

void CrossAssetModelData::validate() {
    QL_REQUIRE(irConfigs_.size() > 0, "no IR data provided");
    QL_REQUIRE(fxConfigs_.size() == irConfigs_.size() - 1, "inconsistent number of FX data provided");
    for (Size i = 0; i < fxConfigs_.size(); ++i)
        QL_REQUIRE(fxConfigs_[i]->foreignCcy() == irConfigs_[i + 1]->ccy(),
                   "currency mismatch betwee IR and FX config vectors");
}

std::vector<std::string> pairToStrings(std::pair<std::string, std::string> p) {
    std::vector<std::string> pair = {p.first, p.second};
    for (int i = 0; i < 2; i++) {
        if (pair[i].substr(0, 4) == "INF:") {
            vector<string> tmp;
            boost::split(tmp, pair[i], boost::is_any_of(":/"));
            pair[i] = tmp[0] + ":" + tmp[1] + "/Index/" + tmp[2];
        }
    }
    return pair;
}

void CrossAssetModelData::fromXML(XMLNode* root) {
    clear();

    XMLNode* sim = XMLUtils::locateNode(root, "Simulation");
    XMLNode* modelNode = XMLUtils::getChildNode(sim, "CrossAssetModel");
    XMLUtils::checkNode(modelNode, "CrossAssetModel");

    domesticCurrency_ = XMLUtils::getChildValue(modelNode, "DomesticCcy", true); // mandatory
    LOG("CrossAssetModelData: domesticCcy " << domesticCurrency_);

    currencies_ = XMLUtils::getChildrenValues(modelNode, "Currencies", "Currency", true);
    for (auto ccy : currencies_) {
        LOG("CrossAssetModelData: ccy " << ccy);
    }

    bootstrapTolerance_ = XMLUtils::getChildValueAsDouble(modelNode, "BootstrapTolerance", true);
    LOG("CrossAssetModelData: bootstrap tolerance = " << bootstrapTolerance_);

    // Configure IR model components

    std::map<std::string, boost::shared_ptr<LgmData>> irDataMap;
    XMLNode* irNode = XMLUtils::getChildNode(modelNode, "InterestRateModels");
    if (irNode) {
        for (XMLNode* child = XMLUtils::getChildNode(irNode, "LGM"); child;
             child = XMLUtils::getNextSibling(child, "LGM")) {

            boost::shared_ptr<LgmData> config(new LgmData());
            config->fromXML(child);

            for (Size i = 0; i < config->swaptionExpiries().size(); i++) {
                LOG("LGM calibration swaption " << config->swaptionExpiries()[i] << " x " << config->swaptionTerms()[i]
                                                << " " << config->swaptionStrikes()[i]);
            }

            irDataMap[config->ccy()] = config;

            LOG("CrossAssetModelData: IR config built for key " << config->ccy());

        } // end of  for (XMLNode* child = XMLUtils::getChildNode(irNode, "LGM"); child;
    }     // end of if (irNode)
    else {
        LOG("No IR model section found");
    }

    buildIrConfigs(irDataMap);

    for (Size i = 0; i < irConfigs_.size(); i++)
        LOG("CrossAssetModelData: IR config currency " << i << " = " << irConfigs_[i]->ccy());

    // Configure FX model components

    std::map<std::string, boost::shared_ptr<FxBsData>> fxDataMap;
    XMLNode* fxNode = XMLUtils::getChildNode(modelNode, "ForeignExchangeModels");
    if (fxNode) {
        for (XMLNode* child = XMLUtils::getChildNode(fxNode, "CrossCcyLGM"); child;
             child = XMLUtils::getNextSibling(child, "CrossCcyLGM")) {

            boost::shared_ptr<FxBsData> config(new FxBsData());
            config->fromXML(child);

            for (Size i = 0; i < config->optionExpiries().size(); i++) {
                LOG("CC-LGM calibration option " << config->optionExpiries()[i] << " " << config->optionStrikes()[i]);
            }

            fxDataMap[config->foreignCcy()] = config;

            LOG("CrossAssetModelData: FX config built with key (foreign ccy) " << config->foreignCcy());
        }
    } else {
        LOG("No FX Models section found");
    }

    buildFxConfigs(fxDataMap);

    for (Size i = 0; i < fxConfigs_.size(); i++)
        LOG("CrossAssetModelData: FX config currency " << i << " = " << fxConfigs_[i]->foreignCcy());

    // Configure correlation structure

    XMLNode* correlationNode = XMLUtils::getChildNode(modelNode, "InstantaneousCorrelations");
    CorrelationMatrixBuilder cmb;
    if (correlationNode) {
        vector<XMLNode*> nodes = XMLUtils::getChildrenNodes(correlationNode, "Correlation");
        for (Size i = 0; i < nodes.size(); ++i) {
            string f1 = XMLUtils::getAttribute(nodes[i], "factor1");
            string f2 = XMLUtils::getAttribute(nodes[i], "factor2");
            string v = XMLUtils::getNodeValue(nodes[i]);
            if (f1 != "" && f2 != "" && v != "") {
                cmb.addCorrelation(f1, f2, boost::lexical_cast<Real>(v));
                LOG("CrossAssetModelData: add correlation " << f1 << " " << f2 << " " << v);
            }
        }
    } else {
        QL_FAIL("No InstantaneousCorrelations found in model configuration XML");
    }

    correlations_ = cmb.data();

    validate();

    LOG("CrossAssetModelData loading from XML done");
}

void CrossAssetModelData::buildIrConfigs(std::map<std::string, boost::shared_ptr<LgmData>>& irDataMap) {
    // Append IR configurations into the irConfigs vector in the order of the currencies
    // in the currencies vector.
    // If there is an IR configuration for any of the currencies missing, then we will
    // look up the configuration with key "default" and use this instead. If this is
    // not provided either we will throw an exception.
    irConfigs_.resize(currencies_.size());
    for (Size i = 0; i < currencies_.size(); i++) {
        string ccy = currencies_[i];
        if (irDataMap.find(ccy) != irDataMap.end())
            irConfigs_[i] = irDataMap[ccy];
        else { // copy from default
            LOG("IR configuration missing for currency " << ccy << ", using default");
            if (irDataMap.find("default") == irDataMap.end()) {
                ALOG("Both default IR and " << ccy << " IR configuration missing");
                QL_FAIL("Both default IR and " << ccy << " IR configuration missing");
            }
            boost::shared_ptr<LgmData> def = irDataMap["default"];
            irConfigs_[i] = boost::make_shared<LgmData>(
                ccy, // overwrite this and keep the others
                def->calibrationType(), def->reversionType(), def->volatilityType(), def->calibrateH(),
                def->hParamType(), def->hTimes(), def->hValues(), def->calibrateA(), def->aParamType(), def->aTimes(),
                def->aValues(), def->shiftHorizon(), def->scaling(), def->swaptionExpiries(), def->swaptionTerms(),
                def->swaptionStrikes());
        }
        LOG("CrossAssetModelData: IR config added for ccy " << ccy << " " << irConfigs_[i]->ccy());
    }
}

void CrossAssetModelData::buildFxConfigs(std::map<std::string, boost::shared_ptr<FxBsData>>& fxDataMap) {
    // Append FX configurations into the fxConfigs vector in the order of the foreign
    // currencies in the currencies vector.
    // If there is an FX configuration for any of the foreign currencies missing,
    // then we will look up the configuration with key "default" and use this instead.
    // If this is not provided either we will throw an exception.
    for (Size i = 0; i < currencies_.size(); i++) {
        string ccy = currencies_[i];
        if (ccy == domesticCurrency_)
            continue;
        if (fxDataMap.find(ccy) != fxDataMap.end())
            fxConfigs_.push_back(fxDataMap[ccy]);
        else { // copy from default
            LOG("FX configuration missing for foreign currency " << ccy << ", using default");
            if (fxDataMap.find("default") == fxDataMap.end()) {
                ALOG("Both default FX and " << ccy << " FX configuration missing");
                QL_FAIL("Both default FX and " << ccy << " FX configuration missing");
            }
            boost::shared_ptr<FxBsData> def = fxDataMap["default"];
            boost::shared_ptr<FxBsData> fxData = boost::make_shared<FxBsData>(
                ccy, def->domesticCcy(), def->calibrationType(), def->calibrateSigma(), def->sigmaParamType(),
                def->sigmaTimes(), def->sigmaValues(), def->optionExpiries(), def->optionStrikes());

            fxConfigs_.push_back(fxData);
        }
        LOG("CrossAssetModelData: FX config added for foreign ccy " << ccy);
    }
}

XMLNode* CrossAssetModelData::toXML(XMLDocument& doc) {

    XMLNode* crossAssetModelNode = doc.allocNode("CrossAssetModel");

    XMLUtils::addChild(doc, crossAssetModelNode, "DomesticCcy", domesticCurrency_);
    XMLUtils::addChildren(doc, crossAssetModelNode, "Currencies", "Currency", currencies_);
    XMLUtils::addChild(doc, crossAssetModelNode, "BootstrapTolerance", bootstrapTolerance_);

    XMLNode* interestRateModelsNode = XMLUtils::addChild(doc, crossAssetModelNode, "InterestRateModels");
    for (int irConfigs_Iterator = 0; irConfigs_Iterator < irConfigs_.size(); irConfigs_Iterator++) {
        XMLNode* lgmNode = irConfigs_[irConfigs_Iterator]->toXML(doc);
        XMLUtils::appendNode(interestRateModelsNode, lgmNode);
    }

    XMLNode* foreignExchangeModelsNode = XMLUtils::addChild(doc, crossAssetModelNode, "ForeignExchangeModels");
    for (int fxConfigs_Iterator = 0; fxConfigs_Iterator < fxConfigs_.size(); fxConfigs_Iterator++) {
        XMLNode* crossCcyLgmNode = fxConfigs_[fxConfigs_Iterator]->toXML(doc);
        XMLUtils::appendNode(foreignExchangeModelsNode, crossCcyLgmNode);
    }

    XMLNode* instantaneousCorrelationsNode = doc.allocNode("InstantaneousCorrelations");
    XMLUtils::appendNode(crossAssetModelNode, instantaneousCorrelationsNode);

    for (auto correlationIterator = correlations_.begin(); correlationIterator != correlations_.end();
         correlationIterator++) {
        XMLNode* node = doc.allocNode("Correlation", std::to_string(correlationIterator->second));
        XMLUtils::appendNode(instantaneousCorrelationsNode, node);
        std::vector<std::string> factors = pairToStrings(correlationIterator->first);
        XMLUtils::addAttribute(doc, node, "factor1", factors[0]);
        XMLUtils::addAttribute(doc, node, "factor2", factors[1]);
    }

    return crossAssetModelNode;
}
}
}
