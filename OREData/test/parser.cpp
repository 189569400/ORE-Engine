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

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <ored/marketdata/marketdatumparser.hpp>
#include <ored/utilities/parsers.hpp>
#include <ored/utilities/strike.hpp>
#include <oret/toplevelfixture.hpp>
#include <ql/math/comparison.hpp>
#include <ql/time/daycounters/all.hpp>

using namespace QuantLib;
using namespace boost::unit_test_framework;
using namespace std;

namespace {

struct test_daycounter_data {
    const char* str;
    DayCounter dc;
};

static struct test_daycounter_data daycounter_data[] = {
    {"A360", Actual360()},
    {"Actual/360", Actual360()},
    {"ACT/360", Actual360()},
    {"A365", Actual365Fixed()},
    {"A365F", Actual365Fixed()},
    {"Actual/365 (Fixed)", Actual365Fixed()},
    {"ACT/365", Actual365Fixed()},
    {"T360", Thirty360(Thirty360::USA)},
    {"30/360", Thirty360(Thirty360::USA)},
    {"30/360 (Bond Basis)", Thirty360(Thirty360::USA)},
    {"ACT/nACT", Thirty360(Thirty360::USA)},
    {"30E/360 (Eurobond Basis)", Thirty360(Thirty360::European)},
    {"30E/360", Thirty360(Thirty360::European)},
    {"30/360 (Italian)", Thirty360(Thirty360::Italian)},
    {"ActActISDA", ActualActual(ActualActual::ISDA)},
    {"Actual/Actual (ISDA)", ActualActual(ActualActual::ISDA)},
    {"ACT/ACT", ActualActual(ActualActual::ISDA)},
    {"ACT29", ActualActual(ActualActual::AFB)},
    {"ACT", ActualActual(ActualActual::ISDA)},
    {"ActActISMA", ActualActual(ActualActual::ISMA)},
    {"Actual/Actual (ISMA)", ActualActual(ActualActual::ISMA)},
    {"ActActAFB", ActualActual(ActualActual::AFB)},
    {"Actual/Actual (AFB)", ActualActual(ActualActual::AFB)},
    {"1/1", OneDayCounter()},
    {"BUS/252", Business252()},
    {"Business/252", Business252()},
    {"Actual/365 (No Leap)", Actual365Fixed(Actual365Fixed::NoLeap)},
    {"Act/365 (NL)", Actual365Fixed(Actual365Fixed::NoLeap)},
    {"NL/365", Actual365Fixed(Actual365Fixed::NoLeap)},
    {"Actual/365 (JGB)", Actual365Fixed(Actual365Fixed::NoLeap)}};

struct test_freq_data {
    const char* str;
    Frequency freq;
};

static struct test_freq_data freq_data[] = {{"Z", Once},
                                            {"Once", Once},
                                            {"A", Annual},
                                            {"Annual", Annual},
                                            {"S", Semiannual},
                                            {"Semiannual", Semiannual},
                                            {"Q", Quarterly},
                                            {"Quarterly", Quarterly},
                                            {"B", Bimonthly},
                                            {"Bimonthly", Bimonthly},
                                            {"M", Monthly},
                                            {"Monthly", Monthly},
                                            {"L", EveryFourthWeek},
                                            {"Lunarmonth", EveryFourthWeek},
                                            {"W", Weekly},
                                            {"Weekly", Weekly},
                                            {"D", Daily},
                                            {"Daily", Daily}};

struct test_comp_data {
    const char* str;
    Compounding comp;
};

static struct test_comp_data comp_data[] = {
    {"Simple", Simple},
    {"Compounded", Compounded},
    {"Continuous", Continuous},
    {"SimpleThenCompounded", SimpleThenCompounded},
};

void checkStrikeParser(const std::string& s, const ore::data::Strike::Type expectedType, const Real expectedValue) {
    if (ore::data::parseStrike(s).type != expectedType) {
        BOOST_FAIL("unexpected strike type parsed from input string " << s);
    }
    if (!close_enough(ore::data::parseStrike(s).value, expectedValue)) {
        BOOST_FAIL("unexpected strike value parsed from input string " << s);
    }
    return;
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(OREDataTestSuite, ore::test::TopLevelFixture)

BOOST_AUTO_TEST_SUITE(ParserTests)

BOOST_AUTO_TEST_CASE(testDayCounterParsing) {

    BOOST_TEST_MESSAGE("Testing day counter parsing...");

    Size len = sizeof(daycounter_data) / sizeof(daycounter_data[0]);
    for (Size i = 0; i < len; ++i) {
        string str(daycounter_data[i].str);
        QuantLib::DayCounter d;

        try {
            d = ore::data::parseDayCounter(str);
        } catch (...) {
            BOOST_FAIL("Day Counter Parser failed to parse " << str);
        }
        if (d.empty() || d != daycounter_data[i].dc) {
            BOOST_FAIL("Day Counter Parser(" << str << ") returned day counter " << d << " expected "
                                             << daycounter_data[i].dc);

            BOOST_TEST_MESSAGE("Parsed \"" << str << "\" and got " << d);
        }
    }
}

BOOST_AUTO_TEST_CASE(testFrequencyParsing) {

    BOOST_TEST_MESSAGE("Testing frequency parsing...");

    Size len = sizeof(freq_data) / sizeof(freq_data[0]);
    for (Size i = 0; i < len; ++i) {
        string str(freq_data[i].str);

        try {
            QuantLib::Frequency f = ore::data::parseFrequency(str);
            if (f) {
                if (f != freq_data[i].freq)
                    BOOST_FAIL("Frequency Parser(" << str << ") returned frequency " << f << " expected "
                                                   << freq_data[i].freq);
                BOOST_TEST_MESSAGE("Parsed \"" << str << "\" and got " << f);
            }
        } catch (...) {
            BOOST_FAIL("Frequency Parser failed to parse " << str);
        }
    }
}

BOOST_AUTO_TEST_CASE(testCompoundingParsing) {

    BOOST_TEST_MESSAGE("Testing Compounding parsing...");

    Size len = sizeof(comp_data) / sizeof(comp_data[0]);
    for (Size i = 0; i < len; ++i) {
        string str(comp_data[i].str);

        try {
            QuantLib::Compounding c = ore::data::parseCompounding(str);
            if (c) {
                if (c != comp_data[i].comp)
                    BOOST_FAIL("Compounding Parser(" << str << ") returned Compounding " << c << " expected "
                                                     << comp_data[i].comp);
                BOOST_TEST_MESSAGE("Parsed \"" << str << "\" and got " << c);
            }
        } catch (...) {
            BOOST_FAIL("Compounding Parser failed to parse " << str);
        }
    }
}

BOOST_AUTO_TEST_CASE(testStrikeParsing) {

    BOOST_TEST_MESSAGE("Testing Strike parsing...");

    checkStrikeParser("ATM", ore::data::Strike::Type::ATM, 0.0);
    checkStrikeParser("atm", ore::data::Strike::Type::ATM, 0.0);
    checkStrikeParser("ATMF", ore::data::Strike::Type::ATMF, 0.0);
    checkStrikeParser("atmf", ore::data::Strike::Type::ATMF, 0.0);
    checkStrikeParser("ATM+0", ore::data::Strike::Type::ATM_Offset, 0.0);
    checkStrikeParser("ATM-1", ore::data::Strike::Type::ATM_Offset, -1.0);
    checkStrikeParser("ATM+1", ore::data::Strike::Type::ATM_Offset, 1.0);
    checkStrikeParser("ATM-0.01", ore::data::Strike::Type::ATM_Offset, -0.01);
    checkStrikeParser("ATM+0.01", ore::data::Strike::Type::ATM_Offset, 0.01);
    checkStrikeParser("atm+0", ore::data::Strike::Type::ATM_Offset, 0.0);
    checkStrikeParser("atm-1", ore::data::Strike::Type::ATM_Offset, -1.0);
    checkStrikeParser("atm+1", ore::data::Strike::Type::ATM_Offset, 1.0);
    checkStrikeParser("atm-0.01", ore::data::Strike::Type::ATM_Offset, -0.01);
    checkStrikeParser("atm+0.01", ore::data::Strike::Type::ATM_Offset, 0.01);
    checkStrikeParser("1", ore::data::Strike::Type::Absolute, 1.0);
    checkStrikeParser("0.01", ore::data::Strike::Type::Absolute, 0.01);
    checkStrikeParser("+0.01", ore::data::Strike::Type::Absolute, 0.01);
    checkStrikeParser("-0.01", ore::data::Strike::Type::Absolute, -0.01);
    checkStrikeParser("10d", ore::data::Strike::Type::Delta, 10.0);
    checkStrikeParser("10.0d", ore::data::Strike::Type::Delta, 10.0);
    checkStrikeParser("+10d", ore::data::Strike::Type::Delta, 10.0);
    checkStrikeParser("+10.0d", ore::data::Strike::Type::Delta, 10.0);
    checkStrikeParser("-25d", ore::data::Strike::Type::Delta, -25.0);
    checkStrikeParser("-25.0d", ore::data::Strike::Type::Delta, -25.0);
    checkStrikeParser("10D", ore::data::Strike::Type::Delta, 10.0);
    checkStrikeParser("10.0D", ore::data::Strike::Type::Delta, 10.0);
    checkStrikeParser("+10D", ore::data::Strike::Type::Delta, 10.0);
    checkStrikeParser("+10.0D", ore::data::Strike::Type::Delta, 10.0);
    checkStrikeParser("-25D", ore::data::Strike::Type::Delta, -25.0);
    checkStrikeParser("-25.0D", ore::data::Strike::Type::Delta, -25.0);

    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(testDatePeriodParsing) {

    BOOST_TEST_MESSAGE("Testing Date and Period parsing...");

    BOOST_CHECK_EQUAL(ore::data::parseDate("20170605"), Date(5, Jun, 2017));
    //
    BOOST_CHECK_EQUAL(ore::data::parseDate("2017-06-05"), Date(5, Jun, 2017));
    BOOST_CHECK_EQUAL(ore::data::parseDate("2017/06/05"), Date(5, Jun, 2017));
    BOOST_CHECK_EQUAL(ore::data::parseDate("2017.06.05"), Date(5, Jun, 2017));
    //
    BOOST_CHECK_EQUAL(ore::data::parseDate("05-06-2017"), Date(5, Jun, 2017));
    BOOST_CHECK_EQUAL(ore::data::parseDate("05/06/2017"), Date(5, Jun, 2017));
    BOOST_CHECK_EQUAL(ore::data::parseDate("05.06.2017"), Date(5, Jun, 2017));
    //
    BOOST_CHECK_EQUAL(ore::data::parseDate("05-06-17"), Date(5, Jun, 2017));
    BOOST_CHECK_EQUAL(ore::data::parseDate("05/06/17"), Date(5, Jun, 2017));
    BOOST_CHECK_EQUAL(ore::data::parseDate("05.06.17"), Date(5, Jun, 2017));
    //
    BOOST_CHECK_THROW(ore::data::parseDate("1Y"), QuantLib::Error);
    BOOST_CHECK_THROW(ore::data::parseDate("05-06-1Y"), QuantLib::Error);
    BOOST_CHECK_THROW(ore::data::parseDate("X5-06-17"), QuantLib::Error);
    BOOST_CHECK_THROW(ore::data::parseDate("2017-06-05-"), QuantLib::Error);
    BOOST_CHECK_THROW(ore::data::parseDate("-2017-06-05"), QuantLib::Error);
    BOOST_CHECK_THROW(ore::data::parseDate("xx17-06-05"), QuantLib::Error);

    BOOST_CHECK_EQUAL(ore::data::parsePeriod("3Y"), 3 * Years);
    BOOST_CHECK_EQUAL(ore::data::parsePeriod("3y"), 3 * Years);
    BOOST_CHECK_EQUAL(ore::data::parsePeriod("3M"), 3 * Months);
    BOOST_CHECK_EQUAL(ore::data::parsePeriod("3m"), 3 * Months);
    BOOST_CHECK_EQUAL(ore::data::parsePeriod("3W"), 3 * Weeks);
    BOOST_CHECK_EQUAL(ore::data::parsePeriod("3w"), 3 * Weeks);
    BOOST_CHECK_EQUAL(ore::data::parsePeriod("3D"), 3 * Days);
    BOOST_CHECK_EQUAL(ore::data::parsePeriod("3d"), 3 * Days);
    //
    BOOST_CHECK_EQUAL(ore::data::parsePeriod("1Y6M"), 1 * Years + 6 * Months);
    BOOST_CHECK_EQUAL(ore::data::parsePeriod("6M0W"), 6 * Months + 0 * Weeks);
    BOOST_CHECK_EQUAL(ore::data::parsePeriod("6M0D"), 6 * Months + 0 * Days);
    //
    BOOST_CHECK_THROW(ore::data::parsePeriod("20170605"), QuantLib::Error);
    BOOST_CHECK_THROW(ore::data::parsePeriod("3X"), QuantLib::Error);
    BOOST_CHECK_THROW(ore::data::parsePeriod("xY"), QuantLib::Error);
    BOOST_CHECK_THROW(ore::data::parsePeriod(".3M"), QuantLib::Error);
    BOOST_CHECK_THROW(ore::data::parsePeriod("3M."), QuantLib::Error);

    Date d;
    Period p;
    bool isDate;

    ore::data::parseDateOrPeriod("20170605", d, p, isDate);
    BOOST_CHECK(isDate && d == Date(5, Jun, 2017));
    ore::data::parseDateOrPeriod("3Y", d, p, isDate);
    BOOST_CHECK(!isDate && p == 3 * Years);
    ore::data::parseDateOrPeriod("3M", d, p, isDate);
    BOOST_CHECK(!isDate && p == 3 * Months);
    ore::data::parseDateOrPeriod("3W", d, p, isDate);
    BOOST_CHECK(!isDate && p == 3 * Weeks);
    ore::data::parseDateOrPeriod("3D", d, p, isDate);
    BOOST_CHECK(!isDate && p == 3 * Days);
    ore::data::parseDateOrPeriod("1Y6M", d, p, isDate);
    BOOST_CHECK(!isDate && p == 1 * Years + 6 * Months);
    ore::data::parseDateOrPeriod("20170605D", d, p, isDate);
    BOOST_CHECK(!isDate && p == 20170605 * Days);
    //
    BOOST_CHECK_THROW(ore::data::parseDateOrPeriod("5Y2017", d, p, isDate), QuantLib::Error);
    BOOST_CHECK_THROW(ore::data::parseDateOrPeriod("2017-06-05D", d, p, isDate), QuantLib::Error);
    BOOST_CHECK_THROW(ore::data::parseDateOrPeriod(".3M", d, p, isDate), QuantLib::Error);
    BOOST_CHECK_THROW(ore::data::parseDateOrPeriod("3M.", d, p, isDate), QuantLib::Error);
    BOOST_CHECK_THROW(ore::data::parseDateOrPeriod("xx17-06-05", d, p, isDate), QuantLib::Error);
}

BOOST_AUTO_TEST_CASE(testMarketDatumParsing) {

    BOOST_TEST_MESSAGE("Testing market datum parsing...");

    BOOST_TEST_MESSAGE("Testing correlation market datum parsing...");

    { // test rate quote
        Date d(1, Jan, 1990);
        Real value = 1;

        string input = "CORRELATION/RATE/INDEX1/INDEX2/1Y/ATM";

        boost::shared_ptr<ore::data::MarketDatum> datum = ore::data::parseMarketDatum(d, input, value);

        BOOST_CHECK(datum->asofDate() == d);
        BOOST_CHECK(datum->quote()->value() == value);
        BOOST_CHECK(datum->instrumentType() == ore::data::MarketDatum::InstrumentType::CORRELATION);
        BOOST_CHECK(datum->quoteType() == ore::data::MarketDatum::QuoteType::RATE);

        boost::shared_ptr<ore::data::CorrelationQuote> spreadDatum =
            boost::dynamic_pointer_cast<ore::data::CorrelationQuote>(datum);

        BOOST_CHECK(spreadDatum->index1() == "INDEX1");
        BOOST_CHECK(spreadDatum->index2() == "INDEX2");
        BOOST_CHECK(spreadDatum->expiry() == "1Y");
        BOOST_CHECK(spreadDatum->strike() == "ATM");
    }

    { // test price quote
        Date d(3, Mar, 2018);
        Real value = 10;

        string input = "CORRELATION/PRICE/INDEX1/INDEX2/1Y/0.1";

        boost::shared_ptr<ore::data::MarketDatum> datum = ore::data::parseMarketDatum(d, input, value);

        BOOST_CHECK(datum->asofDate() == d);
        BOOST_CHECK(datum->quote()->value() == value);
        BOOST_CHECK(datum->instrumentType() == ore::data::MarketDatum::InstrumentType::CORRELATION);
        BOOST_CHECK(datum->quoteType() == ore::data::MarketDatum::QuoteType::PRICE);

        boost::shared_ptr<ore::data::CorrelationQuote> spreadDatum =
            boost::dynamic_pointer_cast<ore::data::CorrelationQuote>(datum);

        BOOST_CHECK(spreadDatum->index1() == "INDEX1");
        BOOST_CHECK(spreadDatum->index2() == "INDEX2");
        BOOST_CHECK(spreadDatum->expiry() == "1Y");
        BOOST_CHECK(spreadDatum->strike() == "0.1");
    }

    { // test parsing throws
        Date d(3, Mar, 2018);
        Real value = 10;

        BOOST_CHECK_THROW(ore::data::parseMarketDatum(d, "CORRELATION/PRICE/INDEX1/INDEX2/1Y/SS", value),
                          QuantLib::Error);
        BOOST_CHECK_THROW(ore::data::parseMarketDatum(d, "CORRELATION/PRICE/INDEX1/INDEX2/6X/0.1", value),
                          QuantLib::Error);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
