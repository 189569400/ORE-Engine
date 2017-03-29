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

#include <ored/configuration/equityvolcurveconfig.hpp>
#include <ql/errors.hpp>

using ore::data::XMLUtils;

namespace ore {
namespace data {

EquityVolatilityCurveConfig::EquityVolatilityCurveConfig(const string& curveID, const string& curveDescription,
                                                         const string& currency, const Dimension& dimension,
                                                         const vector<string>& expiries)
    : curveID_(curveID), curveDescription_(curveDescription), ccy_(currency), dimension_(dimension),
      expiries_(expiries) {}

void EquityVolatilityCurveConfig::fromXML(XMLNode* node) {
    XMLUtils::checkNode(node, "EquityVolatility");

    curveID_ = XMLUtils::getChildValue(node, "CurveId", true);
    curveDescription_ = XMLUtils::getChildValue(node, "CurveDescription", true);
    ccy_ = XMLUtils::getChildValue(node, "Currency", true);
    string dim = XMLUtils::getChildValue(node, "Dimension", true);
    if (dim == "ATM")
        dimension_ = Dimension::ATM;
    else {
        QL_FAIL("Dimension " << dim << " not supported yet");
    }
    expiries_ = XMLUtils::getChildrenValuesAsStrings(node, "Expiries", true);
}

XMLNode* EquityVolatilityCurveConfig::toXML(XMLDocument& doc) {
    XMLNode* node = doc.allocNode("EquityVolatility");

    XMLUtils::addChild(doc, node, "CurveId", curveID_);
    XMLUtils::addChild(doc, node, "CurveDescription", curveDescription_);
    XMLUtils::addChild(doc, node, "Currency", ccy_);
    if (dimension_ == Dimension::ATM) {
        XMLUtils::addChild(doc, node, "Dimension", "ATM");
    } else {
        QL_FAIL("Unkown Dimension in EquityVolatilityCurveConfig::toXML()");
    }
    XMLUtils::addGenericChildAsList(doc, node, "Expiries", expiries_);

    return node;
}
}
}
