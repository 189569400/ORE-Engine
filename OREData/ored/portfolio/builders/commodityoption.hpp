/*
 Copyright (C) 2018 Quaternion Risk Management Ltd
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

/*! \file portfolio/builders/commodityoption.hpp
    \brief Engine builder for commodity option
    \ingroup builders
*/

#pragma once

#include <ored/portfolio/builders/cachingenginebuilder.hpp>
#include <ored/portfolio/enginefactory.hpp>

namespace ore {
namespace data {

//! Engine builder for commodity option
/*! Pricing engines are cached by commodity and currency

    \ingroup builders
 */
class CommodityOptionEngineBuilder
    : public CachingPricingEngineBuilder<std::string, const std::string&, const QuantLib::Currency&> {
public:
    CommodityOptionEngineBuilder()
        : CachingEngineBuilder("BlackScholes", "AnalyticEuropeanEngine", {"CommodityOption"}) {}

protected:
    virtual std::string keyImpl(const std::string& commodityName, const QuantLib::Currency& ccy) override {
        return commodityName + "/" + ccy.code();
    }

    virtual boost::shared_ptr<QuantLib::PricingEngine> engineImpl(const std::string& commodityName,
                                                                  const QuantLib::Currency& ccy) override;
};

} // namespace data
} // namespace ore
