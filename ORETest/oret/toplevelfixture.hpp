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

/*! \file oret/toplevelfixture.hpp
    \brief Fixture that can be used at top level
*/

#pragma once

#include <ql/indexes/indexmanager.hpp>
#include <ql/settings.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

using QuantLib::IndexManager;
using QuantLib::SavedSettings;

namespace ore {
namespace test {

//! Top level fixture
class TopLevelFixture {
public:
    SavedSettings savedSettings;

    /*! Constructor
        Add things here that you want to happen at the start of every test case
    */
    TopLevelFixture() {}
    /*! Destructor
        Add things here that you want to happen after _every_ test case
    */
    virtual ~TopLevelFixture() {
        // Clear and fixings that have been added
        IndexManager::instance().clearHistories();
    }
};

}
}
