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

#include <ored/portfolio/legdata.hpp>
#include <ored/utilities/log.hpp>
#include <ored/portfolio/builders/capfloorediborleg.hpp>

#include <ql/errors.hpp>
#include <ql/cashflow.hpp>
#include <ql/cashflows/overnightindexedcoupon.hpp>
#include <ql/cashflows/iborcoupon.hpp>
#include <ql/cashflows/fixedratecoupon.hpp>
#include <ql/cashflows/simplecashflow.hpp>
#include <ql/cashflows/capflooredcoupon.hpp>

#include <boost/make_shared.hpp>

using namespace QuantLib;

namespace ore {
namespace data {

void CashflowData::fromXML(XMLNode* node) {
    XMLUtils::checkNode(node, "CashflowData");
    amounts_ = XMLUtils::getChildrenValuesAsDoublesWithAttributes(node, "Cashflow", "Amount", "Date", dates_);
}

XMLNode* CashflowData::toXML(XMLDocument& doc) {
    XMLNode* node = doc.allocNode("CashflowData");
    XMLUtils::addChildrenWithAttributes(doc, node, "Cashflow", "Amount", amounts_, "Date", dates_);
    return node;
}
void FixedLegData::fromXML(XMLNode* node) {
    XMLUtils::checkNode(node, "FixedLegData");
    rates_ = XMLUtils::getChildrenValuesAsDoublesWithAttributes(node, "Rates", "Rate", "startDate", rateDates_);
}

XMLNode* FixedLegData::toXML(XMLDocument& doc) {
    XMLNode* node = doc.allocNode("FixedLegData");
    XMLUtils::addChildrenWithAttributes(doc, node, "Rates", "Rate", rates_, "startDate", rateDates_);
    return node;
}

void FloatingLegData::fromXML(XMLNode* node) {
    XMLUtils::checkNode(node, "FloatingLegData");
    index_ = XMLUtils::getChildValue(node, "Index", true);
    spreads_ = XMLUtils::getChildrenValuesAsDoubles(node, "Spreads", "Spread", true);
    // These are all optional
    isInArrears_ = XMLUtils::getChildValueAsBool(node, "IsInArrears"); // defaults to true
    fixingDays_ = XMLUtils::getChildValueAsInt(node, "FixingDays");    // defaults to 0
    caps_ = XMLUtils::getChildrenValuesAsDoublesWithAttributes(node, "Caps", "Cap", "startDate", capDates_);
    floors_ = XMLUtils::getChildrenValuesAsDoublesWithAttributes(node, "Floors", "Floor", "startDate", floorDates_);
    gearings_ =
        XMLUtils::getChildrenValuesAsDoublesWithAttributes(node, "Gearings", "Gearing", "startDate", gearingDates_);
}

XMLNode* FloatingLegData::toXML(XMLDocument& doc) {
    XMLNode* node = doc.allocNode("FloatingLegData");
    XMLUtils::addChild(doc, node, "Index", index_);
    XMLUtils::addChildren(doc, node, "Spreads", "Spread", spreads_);
    XMLUtils::addChild(doc, node, "IsInArrears", isInArrears_);
    XMLUtils::addChild(doc, node, "FixingDays", fixingDays_);
    XMLUtils::addChildrenWithAttributes(doc, node, "Caps", "Cap", caps_, "startDate", capDates_);
    XMLUtils::addChildrenWithAttributes(doc, node, "Floors", "Floor", floors_, "startDate", floorDates_);
    XMLUtils::addChildrenWithAttributes(doc, node, "Gearings", "Gearing", gearings_, "startDate", gearingDates_);
    return node;
}

void LegData::fromXML(XMLNode* node) {
    XMLUtils::checkNode(node, "LegData");
    legType_ = XMLUtils::getChildValue(node, "LegType", true);
    isPayer_ = XMLUtils::getChildValueAsBool(node, "Payer");
    currency_ = XMLUtils::getChildValue(node, "Currency", true);
    dayCounter_ = XMLUtils::getChildValue(node, "DayCounter");
    paymentConvention_ = XMLUtils::getChildValue(node, "PaymentConvention");
    // if not given, default of getChildValueAsBool is true, which fits our needs here
    notionals_ =
        XMLUtils::getChildrenValuesAsDoublesWithAttributes(node, "Notionals", "Notional", "startDate", notionalDates_);
    XMLNode* tmp = XMLUtils::getChildNode(node, "Notionals");
    XMLNode* fxResetNode = XMLUtils::getChildNode(tmp, "FXReset");
    if (fxResetNode) {
        isNotResetXCCY_ = false;
        foreignCurrency_ = XMLUtils::getChildValue(fxResetNode, "ForeignCurrency");
        foreignAmount_ = XMLUtils::getChildValueAsDouble(fxResetNode, "ForeignAmount");
        fxIndex_ = XMLUtils::getChildValue(fxResetNode, "FXIndex");
        fixingDays_ = XMLUtils::getChildValueAsInt(fxResetNode, "FixingDays");
        // TODO add schedule
    } else {
        isNotResetXCCY_ = true;
    }
    XMLNode* exchangeNode = XMLUtils::getChildNode(tmp, "Exchanges");
    if (exchangeNode) {
        notionalInitialExchange_ = XMLUtils::getChildValueAsBool(exchangeNode, "NotionalInitialExchange");
        notionalFinalExchange_ = XMLUtils::getChildValueAsBool(exchangeNode, "NotionalFinalExchange");
        if (XMLUtils::getChildNode(exchangeNode, "NotionalAmortizingExchange"))
            notionalAmortizingExchange_ = XMLUtils::getChildValueAsBool(exchangeNode, "NotionalAmortizingExchange");
        else
            notionalAmortizingExchange_ = false;
    } else {
        notionalInitialExchange_ = false;
        notionalFinalExchange_ = false;
        notionalAmortizingExchange_ = false;
    }
    schedule_.fromXML(XMLUtils::getChildNode(node, "ScheduleData"));
    if (legType_ == "Fixed") {
        fixedLegData_.fromXML(XMLUtils::getChildNode(node, "FixedLegData"));
        floatingLegData_ = FloatingLegData();
        cashflowData_ = CashflowData();
    } else if (legType_ == "Floating") {
        floatingLegData_.fromXML(XMLUtils::getChildNode(node, "FloatingLegData"));
        fixedLegData_ = FixedLegData();
        cashflowData_ = CashflowData();
    } else if (legType_ == "Cashflow") {
        cashflowData_.fromXML(XMLUtils::getChildNode(node, "CashflowData"));
        fixedLegData_ = FixedLegData();
        floatingLegData_ = FloatingLegData();
    } else {
        QL_FAIL("Unkown legType :" << legType_);
    }
}

XMLNode* LegData::toXML(XMLDocument& doc) {
    XMLNode* node = doc.allocNode("LegData");
    QL_REQUIRE(node, "Failed to create LegData node");
    XMLUtils::addChild(doc, node, "LegType", legType_);
    XMLUtils::addChild(doc, node, "Payer", isPayer_);
    XMLUtils::addChild(doc, node, "Currency", currency_);
    XMLUtils::addChild(doc, node, "DayCounter", dayCounter_);
    XMLUtils::addChild(doc, node, "PaymentConvention", paymentConvention_);
    XMLUtils::addChildrenWithAttributes(doc, node, "Notionals", "Notional", notionals_, "startDate", notionalDates_);
    XMLUtils::addChild(doc, node, "NotionalInitialExchange", notionalInitialExchange_);
    XMLUtils::addChild(doc, node, "NotionalFinalExchange", notionalFinalExchange_);
    XMLUtils::addChild(doc, node, "NotionalAmortizingExchange", notionalAmortizingExchange_);
    XMLUtils::appendNode(node, schedule_.toXML(doc));
    // to do: Add toXML for reset
    if (legType_ == "Fixed") {
        XMLUtils::appendNode(node, fixedLegData_.toXML(doc));
    } else if (legType_ == "Floating") {
        XMLUtils::appendNode(node, floatingLegData_.toXML(doc));
    } else if (legType_ == "Cashflow") {
        XMLUtils::appendNode(node, cashflowData_.toXML(doc));
    } else {
        QL_FAIL("Unkown legType :" << legType_);
    }

    return node;
}

// Functions
Leg makeSimpleLeg(LegData& data) {
    const vector<double>& amounts = data.cashflowData().amounts();
    const vector<string>& dates = data.cashflowData().dates();
    QL_REQUIRE(amounts.size() == dates.size(), "Amounts / Date size mismatch in makeSimpleLeg."
                                                   << "Amounts:" << amounts.size() << ", Dates:" << dates.size());
    Leg leg;
    for (Size i = 0; i < dates.size(); i++) {
        Date d = parseDate(dates[i]);
        leg.push_back(boost::shared_ptr<CashFlow>(new SimpleCashFlow(amounts[i], d)));
    }
    return leg;
}

Leg makeFixedLeg(LegData& data) {
    Schedule schedule = makeSchedule(data.schedule());
    DayCounter dc = parseDayCounter(data.dayCounter());
    BusinessDayConvention bdc = parseBusinessDayConvention(data.paymentConvention());

    vector<double> notionals = buildScheduledVector(data.notionals(), data.notionalDates(), schedule);
    vector<double> rates = buildScheduledVector(data.fixedLegData().rates(), data.fixedLegData().rateDates(), schedule);

    Leg leg = FixedRateLeg(schedule).withNotionals(notionals).withCouponRates(rates, dc).withPaymentAdjustment(bdc);
    return leg;
}

Leg makeIborLeg(LegData& data, boost::shared_ptr<IborIndex> index,
                const boost::shared_ptr<EngineFactory>& engineFactory) {
    Schedule schedule = makeSchedule(data.schedule());
    DayCounter dc = parseDayCounter(data.dayCounter());
    BusinessDayConvention bdc = parseBusinessDayConvention(data.paymentConvention());
    FloatingLegData floatData = data.floatingLegData();
    bool hasCapsFloors = floatData.caps().size() > 0 || floatData.floors().size() > 0;

    vector<double> notionals = buildScheduledVector(data.notionals(), data.notionalDates(), schedule);
    vector<double> spreads = buildScheduledVector(floatData.spreads(), floatData.spreadDates(), schedule);

    IborLeg iborLeg = IborLeg(schedule, index)
                          .withNotionals(notionals)
                          .withSpreads(spreads)
                          .withPaymentDayCounter(dc)
                          .withPaymentAdjustment(bdc)
                          .withFixingDays(floatData.fixingDays());

    if (floatData.gearings().size() > 0)
        iborLeg.withGearings(buildScheduledVector(floatData.gearings(), floatData.gearingDates(), schedule));

    // If no caps or floors, return the leg
    if (!hasCapsFloors)
        return iborLeg;

    // If there are caps or floors, add them and set pricer
    if (floatData.caps().size() > 0)
        iborLeg.withCaps(buildScheduledVector(floatData.caps(), floatData.capDates(), schedule));

    if (floatData.floors().size() > 0)
        iborLeg.withFloors(buildScheduledVector(floatData.floors(), floatData.floorDates(), schedule));

    // Get a coupon pricer for the leg
    boost::shared_ptr<EngineBuilder> builder = engineFactory->builder("CapFlooredIborLeg");
    QL_REQUIRE(builder, "No builder found for CapFlooredIborLeg");
    boost::shared_ptr<CapFlooredIborLegEngineBuilder> cappedFlooredIborBuilder =
        boost::dynamic_pointer_cast<CapFlooredIborLegEngineBuilder>(builder);
    boost::shared_ptr<FloatingRateCouponPricer> couponPricer = cappedFlooredIborBuilder->engine(index->currency());

    // Loop over the coupons in the leg and set pricer
    Leg leg = iborLeg;
    for (const auto& cashflow : leg) {
        boost::shared_ptr<CappedFlooredIborCoupon> coupon =
            boost::dynamic_pointer_cast<CappedFlooredIborCoupon>(cashflow);
        QL_REQUIRE(coupon, "Expected a leg of coupons of type CappedFlooredIborCoupon");
        coupon->setPricer(couponPricer);
    }

    return leg;
}

Leg makeOISLeg(LegData& data, boost::shared_ptr<OvernightIndex> index) {

    FloatingLegData floatData = data.floatingLegData();
    if (floatData.caps().size() > 0 || floatData.floors().size() > 0)
        QL_FAIL("Caps and floors are not supported for OIS legs");

    Schedule schedule = makeSchedule(data.schedule());
    DayCounter dc = parseDayCounter(data.dayCounter());
    BusinessDayConvention bdc = parseBusinessDayConvention(data.paymentConvention());

    vector<double> notionals = buildScheduledVector(data.notionals(), data.notionalDates(), schedule);
    vector<double> spreads = buildScheduledVector(floatData.spreads(), floatData.spreadDates(), schedule);

    OvernightLeg leg = OvernightLeg(schedule, index)
                           .withNotionals(notionals)
                           .withSpreads(spreads)
                           .withPaymentDayCounter(dc)
                           .withPaymentAdjustment(bdc);

    if (floatData.gearings().size() > 0)
        leg.withGearings(buildScheduledVector(floatData.gearings(), floatData.gearingDates(), schedule));

    return leg;
}

Leg makeNotionalLeg(const Leg& refLeg, bool initNomFlow, bool finalNomFlow, bool amortNomFlow) {

    // Assumption - Cashflows on Input Leg are all coupons
    // This is the Leg to be populated
    Leg leg;

    // Initial Flow Amount
    if (initNomFlow) {
        double initFlowAmt = boost::dynamic_pointer_cast<QuantLib::Coupon>(refLeg[0])->nominal();
        Date initDate = boost::dynamic_pointer_cast<QuantLib::Coupon>(refLeg[0])->accrualStartDate();
        if (initFlowAmt != 0)
            leg.push_back(boost::shared_ptr<CashFlow>(new SimpleCashFlow(-initFlowAmt, initDate)));
    }

    // Amortization Flows
    if (amortNomFlow) {
        for (Size i = 1; i < refLeg.size(); i++) {
            Date flowDate = boost::dynamic_pointer_cast<QuantLib::Coupon>(refLeg[i])->accrualStartDate();
            Real initNom = boost::dynamic_pointer_cast<QuantLib::Coupon>(refLeg[i - 1])->nominal();
            Real newNom = boost::dynamic_pointer_cast<QuantLib::Coupon>(refLeg[i])->nominal();
            Real flow = initNom - newNom;
            if (flow != 0)
                leg.push_back(boost::shared_ptr<CashFlow>(new SimpleCashFlow(flow, flowDate)));
        }
    }

    // Final Nominal Return at Maturity
    if (finalNomFlow) {
        double finalNomFlow = boost::dynamic_pointer_cast<QuantLib::Coupon>(refLeg.back())->nominal();
        Date finalDate = boost::dynamic_pointer_cast<QuantLib::Coupon>(refLeg.back())->date();
        if (finalNomFlow != 0)
            leg.push_back(boost::shared_ptr<CashFlow>(new SimpleCashFlow(finalNomFlow, finalDate)));
    }

    return leg;
}

// e.g. node is Notionals, get all the children Notional
vector<double> buildScheduledVector(const vector<double>& values, const vector<string>& dates,
                                    const Schedule& schedule) {
    if (values.size() < 2 || dates.size() == 0)
        return values;

    QL_REQUIRE(values.size() == dates.size(), "Value / Date size mismatch in buildScheduledVector."
                                                  << "Value:" << values.size() << ", Dates:" << dates.size());

    // Need to use schedule logic
    // Length of data will be 1 less than schedule
    //
    // Notional 100
    // Notional {startDate 2015-01-01} 200
    // Notional {startDate 2016-01-01} 300
    //
    // Given schedule June, Dec from 2014 to 2016 (6 dates, 5 coupons)
    // we return 100, 100, 200, 200, 300

    // The first node must not have a date.
    // If the second one has a date, all the rest must have, and we process
    // If the second one does not have a date, none of them must have one
    // and we return the vector uneffected.
    QL_REQUIRE(dates[0] == "", "Invalid date " << dates[0] << " for first node");
    if (dates[1] == "") {
        // They must all be empty and then we return values
        for (Size i = 2; i < dates.size(); i++) {
            QL_REQUIRE(dates[i] == "", "Invalid date " << dates[i] << " for node " << i
                                                       << ". Cannot mix dates and non-dates attributes");
        }
        return values;
    }

    // We have nodes with date attributes now
    Size len = schedule.size() - 1;
    vector<double> data(len);
    Size j = 0, max_j = dates.size() - 1; // j is the index of date/value vector. 0 to max_j
    Date d = parseDate(dates[j + 1]);     // The first start date
    for (Size i = 0; i < len; i++) {      // loop over data vector and populate it.
        // If j == max_j we just fall through and take the final value
        while (schedule[i] > d && j < max_j) {
            j++;
            if (j < max_j) {
                QL_REQUIRE(dates[j + 1] != "", "Cannot have empty date attribute for node " << j + 1);
                d = parseDate(dates[j + 1]);
            }
        }
        data[i] = values[j];
    }

    return data;
}

} // namespace data
} // namespace ore
