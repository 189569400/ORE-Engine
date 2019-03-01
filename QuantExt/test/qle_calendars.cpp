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

#include "toplevelfixture.hpp"
#include <boost/test/unit_test.hpp>
#include <ql/time/calendar.hpp>
#include <qle/calendars/chile.hpp>
#include <qle/calendars/colombia.hpp>
#include <qle/calendars/france.hpp>
#include <qle/calendars/malaysia.hpp>
#include <qle/calendars/netherlands.hpp>
#include <qle/calendars/peru.hpp>
#include <qle/calendars/philippines.hpp>
#include <qle/calendars/thailand.hpp>

using namespace QuantLib;
using namespace QuantExt;
using namespace boost::unit_test_framework;
using namespace std;

namespace check {

void checkCalendars(const std::vector<Date>& expectedHolidays, const std::vector<Date>& testHolidays) {

    for (Size i = 0; i < expectedHolidays.size(); i++) {
        if (testHolidays[i] != expectedHolidays[i])
            BOOST_FAIL("expected holiday was " << expectedHolidays[i] << " while calculated holiday is "
                                               << testHolidays[i]);
    }
}
} // namespace check

BOOST_FIXTURE_TEST_SUITE(QuantExtTestSuite, qle::test::TopLevelFixture)

BOOST_AUTO_TEST_SUITE(CalendarsTest)

BOOST_AUTO_TEST_CASE(testPeruvianCalendar) {

    BOOST_TEST_MESSAGE("Testing Peruvian holiday list");

    std::vector<Date> expectedHolidays;

    expectedHolidays.push_back(Date(1, January, 2018));
    expectedHolidays.push_back(Date(29, March, 2018));
    expectedHolidays.push_back(Date(30, March, 2018));
    expectedHolidays.push_back(Date(1, May, 2018));
    expectedHolidays.push_back(Date(29, June, 2018));
    expectedHolidays.push_back(Date(27, July, 2018));
    expectedHolidays.push_back(Date(30, August, 2018));
    expectedHolidays.push_back(Date(31, August, 2018));
    expectedHolidays.push_back(Date(8, October, 2018));
    expectedHolidays.push_back(Date(1, November, 2018));
    expectedHolidays.push_back(Date(2, November, 2018));
    expectedHolidays.push_back(Date(25, December, 2018));

    Calendar c = Peru();

    std::vector<Date> hol = Calendar::holidayList(c, Date(1, January, 2018), Date(31, December, 2018));

    BOOST_CHECK(hol.size() == expectedHolidays.size());

    check::checkCalendars(expectedHolidays, hol);
}

BOOST_AUTO_TEST_CASE(testColombianCalendar) {

    BOOST_TEST_MESSAGE("Testing Colombian holiday list");

    std::vector<Date> expectedHolidays;

    expectedHolidays.push_back(Date(1, January, 2018));
    expectedHolidays.push_back(Date(8, January, 2018));
    expectedHolidays.push_back(Date(19, March, 2018));
    expectedHolidays.push_back(Date(29, March, 2018));
    expectedHolidays.push_back(Date(30, March, 2018));
    expectedHolidays.push_back(Date(1, May, 2018));
    expectedHolidays.push_back(Date(14, May, 2018));
    expectedHolidays.push_back(Date(4, June, 2018));
    expectedHolidays.push_back(Date(11, June, 2018));
    expectedHolidays.push_back(Date(2, July, 2018));
    expectedHolidays.push_back(Date(20, July, 2018));
    expectedHolidays.push_back(Date(7, August, 2018));
    expectedHolidays.push_back(Date(20, August, 2018));
    expectedHolidays.push_back(Date(15, October, 2018));
    expectedHolidays.push_back(Date(5, November, 2018));
    expectedHolidays.push_back(Date(12, November, 2018));
    expectedHolidays.push_back(Date(25, December, 2018));

    Calendar c = Colombia();

    std::vector<Date> hol = Calendar::holidayList(c, Date(1, January, 2018), Date(31, December, 2018));

    BOOST_CHECK(hol.size() == expectedHolidays.size());

    check::checkCalendars(expectedHolidays, hol);
}

BOOST_AUTO_TEST_CASE(testPhilippineCalendar) {

    BOOST_TEST_MESSAGE("Testing Philippine holiday list");

    std::vector<Date> expectedHolidays;

    expectedHolidays.push_back(Date(1, January, 2018));
    expectedHolidays.push_back(Date(2, January, 2018));
    expectedHolidays.push_back(Date(29, March, 2018));
    expectedHolidays.push_back(Date(30, March, 2018));
    expectedHolidays.push_back(Date(9, April, 2018));
    expectedHolidays.push_back(Date(1, May, 2018));
    expectedHolidays.push_back(Date(12, June, 2018));
    expectedHolidays.push_back(Date(21, August, 2018));
    expectedHolidays.push_back(Date(27, August, 2018));
    expectedHolidays.push_back(Date(1, November, 2018));
    expectedHolidays.push_back(Date(30, November, 2018));
    expectedHolidays.push_back(Date(25, December, 2018));
    expectedHolidays.push_back(Date(31, December, 2018));

    Calendar c = Philippines();

    std::vector<Date> hol = Calendar::holidayList(c, Date(1, January, 2018), Date(31, December, 2018));

    check::checkCalendars(expectedHolidays, hol);
}

BOOST_AUTO_TEST_CASE(testThaiCalendar) {

    BOOST_TEST_MESSAGE("Testing Thai holiday list");

    std::vector<Date> expectedHolidays;

    expectedHolidays.push_back(Date(1, January, 2018));
    expectedHolidays.push_back(Date(2, January, 2018));
    expectedHolidays.push_back(Date(6, April, 2018));
    expectedHolidays.push_back(Date(13, April, 2018));
    expectedHolidays.push_back(Date(16, April, 2018));
    expectedHolidays.push_back(Date(1, May, 2018));
    expectedHolidays.push_back(Date(30, July, 2018));
    expectedHolidays.push_back(Date(13, August, 2018));
    expectedHolidays.push_back(Date(15, October, 2018));
    expectedHolidays.push_back(Date(23, October, 2018));
    expectedHolidays.push_back(Date(5, December, 2018));
    expectedHolidays.push_back(Date(10, December, 2018));
    expectedHolidays.push_back(Date(31, December, 2018));

    Calendar c = Thailand();

    std::vector<Date> hol = Calendar::holidayList(c, Date(1, January, 2018), Date(31, December, 2018));

    BOOST_CHECK(hol.size() == expectedHolidays.size());

    check::checkCalendars(expectedHolidays, hol);
}

BOOST_AUTO_TEST_CASE(testMalaysianCalendar) {

    BOOST_TEST_MESSAGE("Testing Malaysian holiday list");

    std::vector<Date> expectedHolidays;

    expectedHolidays.push_back(Date(1, January, 2018));
    expectedHolidays.push_back(Date(1, February, 2018));
    expectedHolidays.push_back(Date(1, May, 2018));
    expectedHolidays.push_back(Date(31, August, 2018));
    expectedHolidays.push_back(Date(17, September, 2018));
    expectedHolidays.push_back(Date(25, December, 2018));

    Calendar c = Malaysia();

    std::vector<Date> hol = Calendar::holidayList(c, Date(1, January, 2018), Date(31, December, 2018));

    BOOST_CHECK(hol.size() == expectedHolidays.size());

    check::checkCalendars(expectedHolidays, hol);
}

BOOST_AUTO_TEST_CASE(testChileanCalendar) {

    BOOST_TEST_MESSAGE("Testing Chilean holiday list");

    std::vector<Date> expectedHolidays;

    expectedHolidays.push_back(Date(1, January, 2018));
    expectedHolidays.push_back(Date(30, March, 2018));
    expectedHolidays.push_back(Date(1, May, 2018));
    expectedHolidays.push_back(Date(21, May, 2018));
    expectedHolidays.push_back(Date(16, July, 2018));
    expectedHolidays.push_back(Date(15, August, 2018));
    expectedHolidays.push_back(Date(1, November, 2018));
    expectedHolidays.push_back(Date(25, December, 2018));

    Calendar c = Chile();

    std::vector<Date> hol = Calendar::holidayList(c, Date(1, January, 2018), Date(31, December, 2018));

    BOOST_CHECK(hol.size() == expectedHolidays.size());

    check::checkCalendars(expectedHolidays, hol);
}

BOOST_AUTO_TEST_CASE(testNetherlandianCalendar) {

    BOOST_TEST_MESSAGE("Testing Netherlandian holiday list");

    std::vector<Date> expectedHolidays;

    expectedHolidays.push_back(Date(1, January, 2018));
    expectedHolidays.push_back(Date(30, March, 2018));
    expectedHolidays.push_back(Date(2, April, 2018));
    expectedHolidays.push_back(Date(27, April, 2018));
    expectedHolidays.push_back(Date(10, May, 2018));
    expectedHolidays.push_back(Date(21, May, 2018));
    expectedHolidays.push_back(Date(25, December, 2018));
    expectedHolidays.push_back(Date(26, December, 2018));

    Calendar c = Netherlands();

    std::vector<Date> hol = Calendar::holidayList(c, Date(1, January, 2018), Date(31, December, 2018));

    BOOST_CHECK(hol.size() == expectedHolidays.size());

    check::checkCalendars(expectedHolidays, hol);
}

BOOST_AUTO_TEST_CASE(testFrenchCalendar) {

    BOOST_TEST_MESSAGE("Testing French holiday list");

    std::vector<Date> expectedHolidays;

    expectedHolidays.push_back(Date(1, January, 2018));
    expectedHolidays.push_back(Date(30, March, 2018));
    expectedHolidays.push_back(Date(2, April, 2018));
    expectedHolidays.push_back(Date(1, May, 2018));
    expectedHolidays.push_back(Date(8, May, 2018));
    expectedHolidays.push_back(Date(10, May, 2018));
    expectedHolidays.push_back(Date(21, May, 2018));
    expectedHolidays.push_back(Date(15, August, 2018));
    expectedHolidays.push_back(Date(1, November, 2018));
    expectedHolidays.push_back(Date(25, December, 2018));
    expectedHolidays.push_back(Date(26, December, 2018));

    Calendar c = France();

    std::vector<Date> hol = Calendar::holidayList(c, Date(1, January, 2018), Date(31, December, 2018));

    BOOST_CHECK(hol.size() == expectedHolidays.size());

    check::checkCalendars(expectedHolidays, hol);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
