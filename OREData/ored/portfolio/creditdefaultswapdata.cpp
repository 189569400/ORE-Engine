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

#include <boost/lexical_cast.hpp>
#include <ored/portfolio/creditdefaultswapdata.hpp>
#include <ored/portfolio/legdata.hpp>
#include <ored/utilities/log.hpp>
#include <ored/utilities/parsers.hpp>

using namespace QuantLib;
using namespace QuantExt;

namespace ore {
namespace data {

void CreditDefaultSwapData::fromXML(XMLNode* node) {
    XMLUtils::checkNode(node, "CreditDefaultSwapData");
    issuerId_ = XMLUtils::getChildValue(node, "IssuerId");
    creditCurveId_ = XMLUtils::getChildValue(node, "CreditCurveId", true);
    settlesAccrual_ = XMLUtils::getChildValueAsBool(node, "SettlesAccrual", false);       // default = Y
    paysAtDefaultTime_ = XMLUtils::getChildValueAsBool(node, "PaysAtDefaultTime", false); // default = Y
    XMLNode* tmp = XMLUtils::getChildNode(node, "ProtectionStart");
    if (tmp)
        protectionStart_ = parseDate(XMLUtils::getNodeValue(tmp)); // null date if empty or missing
    else
        protectionStart_ = Date();
    tmp = XMLUtils::getChildNode(node, "UpfrontDate");
    if (tmp)
        upfrontDate_ = parseDate(XMLUtils::getNodeValue(tmp)); // null date if empty or mssing
    else
        upfrontDate_ = Date();

    // zero if empty or missing
    upfrontFee_ = 0.0;
    string strUpfrontFee = XMLUtils::getChildValue(node, "UpfrontFee", false);
    if (!strUpfrontFee.empty()) {
        upfrontFee_ = parseReal(strUpfrontFee);
    }

    if (upfrontDate_ == Date()) {
        QL_REQUIRE(close_enough(upfrontFee_, 0.0), "CreditDefaultSwapData::fromXML(): UpfronFee not zero ("
                                                       << upfrontFee_ << "), but no upfront date given");
        upfrontFee_ = Null<Real>();
    }

    // Recovery rate is Null<Real>() on a standard CDS i.e. if "FixedRecoveryRate" field is not populated.
    recoveryRate_ = Null<Real>();
    string strRecoveryRate = XMLUtils::getChildValue(node, "FixedRecoveryRate", false);
    if (!strRecoveryRate.empty()) {
        recoveryRate_ = parseReal(strRecoveryRate);
    }

    leg_.fromXML(XMLUtils::getChildNode(node, "LegData"));
}

XMLNode* CreditDefaultSwapData::toXML(XMLDocument& doc) {
    XMLNode* node = doc.allocNode("CreditDefaultSwapData");
    XMLUtils::addChild(doc, node, "IssuerId", issuerId_);
    XMLUtils::addChild(doc, node, "CreditCurveId", creditCurveId_);
    XMLUtils::addChild(doc, node, "SettlesAccrual", settlesAccrual_);
    XMLUtils::addChild(doc, node, "PaysAtDefaultTime", paysAtDefaultTime_);
    if (protectionStart_ != Date()) {
        std::ostringstream tmp;
        tmp << QuantLib::io::iso_date(protectionStart_);
        XMLUtils::addChild(doc, node, "ProtectionStart", tmp.str());
    }
    if (upfrontDate_ != Date()) {
        std::ostringstream tmp;
        tmp << QuantLib::io::iso_date(upfrontDate_);
        XMLUtils::addChild(doc, node, "UpfrontDate", tmp.str());
    }
    if (upfrontFee_ != Null<Real>())
        XMLUtils::addChild(doc, node, "UpfrontFee", upfrontFee_);
    
    if (recoveryRate_ != Null<Real>())
        XMLUtils::addChild(doc, node, "FixedRecoveryRate", recoveryRate_);

    XMLUtils::appendNode(node, leg_.toXML(doc));
    return node;
}
} // namespace data
} // namespace ore
