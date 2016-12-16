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

/*! \file testsuite.cpp
    \brief wrapper calling all individual test cases
    \ingroup
*/

#include <iostream>
#include <iomanip>
using namespace std;

// Boost
#include <boost/timer.hpp>
using namespace boost;

// Boost.Test
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/parameterized_test.hpp>
using boost::unit_test::test_suite;

#ifdef BOOST_MSVC
#include <ql/auto_link.hpp>
#include <qle/auto_link.hpp>
#include <orea/auto_link.hpp>
#include <ored/auto_link.hpp>
#define BOOST_LIB_NAME boost_date_time
#include <boost/config/auto_link.hpp>
#define BOOST_LIB_NAME boost_serialization
#include <boost/config/auto_link.hpp>
#define BOOST_LIB_NAME boost_regex
#include <boost/config/auto_link.hpp>
#define BOOST_LIB_NAME boost_system
#include <boost/config/auto_link.hpp>
#define BOOST_LIB_NAME boost_filesystem
#include <boost/config/auto_link.hpp>
#endif

// Lib test suites
#include "aggregationscenariodata.hpp"
#include "cube.hpp"
#include "scenariosimmarket.hpp"
#include "scenariogenerator.hpp"
#include "swapperformance.hpp"
#include "observationmode.hpp"

namespace {

boost::timer t;

void startTimer() { t.restart(); }
void stopTimer() {
    double seconds = t.elapsed();
    int hours = int(seconds / 3600);
    seconds -= hours * 3600;
    int minutes = int(seconds / 60);
    seconds -= minutes * 60;
    std::cout << endl << " OREAnalytics tests completed in ";
    if (hours > 0)
        cout << hours << " h ";
    if (hours > 0 || minutes > 0)
        cout << minutes << " m ";
    cout << fixed << setprecision(0) << seconds << " s" << endl << endl;
}
}

test_suite* init_unit_test_suite(int, char* []) {

    test_suite* test = BOOST_TEST_SUITE("OREAnalyticsTestSuite");

    test->add(BOOST_TEST_CASE(startTimer));

    test->add(testsuite::AggregationScenarioDataTest::suite());
    test->add(testsuite::CubeTest::suite());
    test->add(testsuite::ScenarioSimMarketTest::suite());
    test->add(testsuite::ScenarioGeneratorTest::suite());
    test->add(testsuite::SwapPerformanceTest::suite());
    test->add(testsuite::ObservationModeTest::suite());

    // test->add(FXSwapTest::suite());
    test->add(BOOST_TEST_CASE(stopTimer));

    return test;
}
