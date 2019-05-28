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

/*! \file ored/utilities/indexparser.cpp
    \brief
    \ingroup utilities
*/

#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <map>
#include <ored/configuration/conventions.hpp>
#include <ored/utilities/indexparser.hpp>
#include <ored/utilities/parsers.hpp>
#include <ql/errors.hpp>
#include <ql/indexes/all.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/daycounters/all.hpp>
#include <qle/indexes/bmaindexwrapper.hpp>
#include <qle/indexes/dkcpi.hpp>
#include <qle/indexes/equityindex.hpp>
#include <qle/indexes/fxindex.hpp>
#include <qle/indexes/genericiborindex.hpp>
#include <qle/indexes/ibor/audbbsw.hpp>
#include <qle/indexes/ibor/brlcdi.hpp>
#include <qle/indexes/ibor/chfsaron.hpp>
#include <qle/indexes/ibor/chftois.hpp>
#include <qle/indexes/ibor/clpcamara.hpp>
#include <qle/indexes/ibor/copibr.hpp>
#include <qle/indexes/ibor/corra.hpp>
#include <qle/indexes/ibor/czkpribor.hpp>
#include <qle/indexes/ibor/demlibor.hpp>
#include <qle/indexes/ibor/dkkcibor.hpp>
#include <qle/indexes/ibor/dkkois.hpp>
#include <qle/indexes/ibor/hkdhibor.hpp>
#include <qle/indexes/ibor/hufbubor.hpp>
#include <qle/indexes/ibor/idridrfix.hpp>
#include <qle/indexes/ibor/idrjibor.hpp>
#include <qle/indexes/ibor/ilstelbor.hpp>
#include <qle/indexes/ibor/inrmifor.hpp>
#include <qle/indexes/ibor/krwcd.hpp>
#include <qle/indexes/ibor/krwkoribor.hpp>
#include <qle/indexes/ibor/mxntiie.hpp>
#include <qle/indexes/ibor/myrklibor.hpp>
#include <qle/indexes/ibor/noknibor.hpp>
#include <qle/indexes/ibor/nowa.hpp>
#include <qle/indexes/ibor/nzdbkbm.hpp>
#include <qle/indexes/ibor/plnpolonia.hpp>
#include <qle/indexes/ibor/phpphiref.hpp>
#include <qle/indexes/ibor/plnwibor.hpp>
#include <qle/indexes/ibor/rubmosprime.hpp>
#include <qle/indexes/ibor/saibor.hpp>
#include <qle/indexes/ibor/seksior.hpp>
#include <qle/indexes/ibor/sekstibor.hpp>
#include <qle/indexes/ibor/sgdsibor.hpp>
#include <qle/indexes/ibor/sgdsor.hpp>
#include <qle/indexes/ibor/skkbribor.hpp>
#include <qle/indexes/ibor/thbbibor.hpp>
#include <qle/indexes/ibor/tonar.hpp>
#include <qle/indexes/ibor/twdtaibor.hpp>
#include <qle/indexes/secpi.hpp>

using namespace QuantLib;
using namespace QuantExt;
using namespace std;
using ore::data::Convention;

namespace ore {
namespace data {

// Helper build classes for static map

class IborIndexParser {
public:
    virtual ~IborIndexParser() {}
    virtual boost::shared_ptr<IborIndex> build(Period p, const Handle<YieldTermStructure>& h) const = 0;
};

template <class T> class IborIndexParserWithPeriod : public IborIndexParser {
public:
    boost::shared_ptr<IborIndex> build(Period p, const Handle<YieldTermStructure>& h) const override {
        QL_REQUIRE(p != 1 * Days, "must have a period longer than 1D");
        return boost::make_shared<T>(p, h);
    }
};

// Specialise for MXN-TIIE. If tenor equates to 28 Days, i.e. tenor is 4W or 28D, ensure that the index is created
// with a tenor of 4W under the hood. Things work better this way especially cap floor stripping.
template <> class IborIndexParserWithPeriod<MXNTiie> : public IborIndexParser {
public:
    boost::shared_ptr<IborIndex> build(Period p, const Handle<YieldTermStructure>& h) const override {
        QL_REQUIRE(p != 1 * Days, "must have a period longer than 1D");
        if (p.units() == Days && p.length() == 28) {
            return boost::make_shared<MXNTiie>(4 * Weeks, h);
        } else {
            return boost::make_shared<MXNTiie>(p, h);
        }
    }
};

template <class T> class IborIndexParserOIS : public IborIndexParser {
public:
    boost::shared_ptr<IborIndex> build(Period p, const Handle<YieldTermStructure>& h) const override {
        QL_REQUIRE(p == 1 * Days, "must have period 1D");
        return boost::make_shared<T>(h);
    }
};

template <class T> class IborIndexParserBMA : public IborIndexParser {
public:
    boost::shared_ptr<IborIndex> build(Period p, const Handle<YieldTermStructure>& h) const override {
        QL_REQUIRE((p.length() == 7 && p.units() == Days) || (p.length() == 1 && p.units() == Weeks),
                   "BMA indexes are uniquely available with a tenor of 1 week.");
        const boost::shared_ptr<BMAIndex> bma = boost::make_shared<BMAIndex>(h);
        return boost::make_shared<T>(bma);
    }
};

boost::shared_ptr<FxIndex> parseFxIndex(const string& s) {
    std::vector<string> tokens;
    split(tokens, s, boost::is_any_of("-"));
    QL_REQUIRE(tokens.size() == 4, "four tokens required in " << s << ": FX-TAG-CCY1-CCY2");
    QL_REQUIRE(tokens[0] == "FX", "expected first token to be FX");
    return boost::make_shared<FxIndex>(tokens[0] + "/" + tokens[1], 0, parseCurrency(tokens[2]),
                                       parseCurrency(tokens[3]), NullCalendar());
}

boost::shared_ptr<EquityIndex> parseEquityIndex(const string& s) {
    std::vector<string> tokens;
    split(tokens, s, boost::is_any_of("-"));
    QL_REQUIRE(tokens.size() == 2, "two tokens required in " << s << ": EQ-NAME");
    QL_REQUIRE(tokens[0] == "EQ", "expected first token to be EQ");
    if (tokens.size() == 2) {
        return boost::make_shared<EquityIndex>(tokens[1], NullCalendar(), Currency());
    } else {
        QL_FAIL("Error parsing equity string " + s);
    }
}

bool tryParseIborIndex(const string& s, boost::shared_ptr<IborIndex>& index) {
    try {
        index = parseIborIndex(s);
    } catch (...) {
        return false;
    }
    return true;
}

boost::shared_ptr<IborIndex> parseIborIndex(const string& s, const Handle<YieldTermStructure>& h) {
    string dummy;
    return parseIborIndex(s, dummy, h);
}

boost::shared_ptr<IborIndex> parseIborIndex(const string& s, string& tenor, const Handle<YieldTermStructure>& h) {

    std::vector<string> tokens;
    split(tokens, s, boost::is_any_of("-"));

    QL_REQUIRE(tokens.size() == 2 || tokens.size() == 3,
               "Two or three tokens required in " << s << ": CCY-INDEX or CCY-INDEX-TERM");

    Period p;
    if (tokens.size() == 3) {
        tenor = tokens[2];
        p = parsePeriod(tokens[2]);
    } else {
        tenor = "";
        p = 1 * Days;
    }

    static map<string, boost::shared_ptr<IborIndexParser>> m = {
        {"EUR-EONIA", boost::make_shared<IborIndexParserOIS<Eonia>>()},
        {"GBP-SONIA", boost::make_shared<IborIndexParserOIS<Sonia>>()},
        {"JPY-TONAR", boost::make_shared<IborIndexParserOIS<Tonar>>()},
        {"CHF-TOIS", boost::make_shared<IborIndexParserOIS<CHFTois>>()},
        {"CHF-SARON", boost::make_shared<IborIndexParserOIS<CHFSaron>>()},
        {"USD-FedFunds", boost::make_shared<IborIndexParserOIS<FedFunds>>()},
        {"AUD-AONIA", boost::make_shared<IborIndexParserOIS<Aonia>>()},
        {"CAD-CORRA", boost::make_shared<IborIndexParserOIS<CORRA>>()},
        {"DKK-DKKOIS", boost::make_shared<IborIndexParserOIS<DKKOis>>()},
        {"DKK-TNR", boost::make_shared<IborIndexParserOIS<DKKOis>>()},
        {"SEK-SIOR", boost::make_shared<IborIndexParserOIS<SEKSior>>()},
        {"AUD-BBSW", boost::make_shared<IborIndexParserWithPeriod<AUDbbsw>>()},
        {"AUD-LIBOR", boost::make_shared<IborIndexParserWithPeriod<AUDLibor>>()},
        {"EUR-EURIBOR", boost::make_shared<IborIndexParserWithPeriod<Euribor>>()},
        {"EUR-EURIB", boost::make_shared<IborIndexParserWithPeriod<Euribor>>()},
        {"CAD-CDOR", boost::make_shared<IborIndexParserWithPeriod<Cdor>>()},
        {"CAD-BA", boost::make_shared<IborIndexParserWithPeriod<Cdor>>()},
        {"CZK-PRIBOR", boost::make_shared<IborIndexParserWithPeriod<CZKPribor>>()},
        {"EUR-LIBOR", boost::make_shared<IborIndexParserWithPeriod<EURLibor>>()},
        {"USD-LIBOR", boost::make_shared<IborIndexParserWithPeriod<USDLibor>>()},
        {"GBP-LIBOR", boost::make_shared<IborIndexParserWithPeriod<GBPLibor>>()},
        {"JPY-LIBOR", boost::make_shared<IborIndexParserWithPeriod<JPYLibor>>()},
        {"JPY-TIBOR", boost::make_shared<IborIndexParserWithPeriod<Tibor>>()},
        {"CAD-LIBOR", boost::make_shared<IborIndexParserWithPeriod<CADLibor>>()},
        {"CHF-LIBOR", boost::make_shared<IborIndexParserWithPeriod<CHFLibor>>()},
        {"SEK-LIBOR", boost::make_shared<IborIndexParserWithPeriod<SEKLibor>>()},
        {"SEK-STIBOR", boost::make_shared<IborIndexParserWithPeriod<SEKStibor>>()},
        {"NOK-NIBOR", boost::make_shared<IborIndexParserWithPeriod<NOKNibor>>()},
        {"HKD-HIBOR", boost::make_shared<IborIndexParserWithPeriod<HKDHibor>>()},
        {"SAR-SAIBOR", boost::make_shared<IborIndexParserWithPeriod<SAibor>>()},
        {"SGD-SIBOR", boost::make_shared<IborIndexParserWithPeriod<SGDSibor>>()},
        {"SGD-SOR", boost::make_shared<IborIndexParserWithPeriod<SGDSor>>()},
        {"DKK-CIBOR", boost::make_shared<IborIndexParserWithPeriod<DKKCibor>>()},
        {"DKK-LIBOR", boost::make_shared<IborIndexParserWithPeriod<DKKLibor>>()},
        {"HUF-BUBOR", boost::make_shared<IborIndexParserWithPeriod<HUFBubor>>()},
        {"IDR-IDRFIX", boost::make_shared<IborIndexParserWithPeriod<IDRIdrfix>>()},
        {"IDR-JIBOR", boost::make_shared<IborIndexParserWithPeriod<IDRJibor>>()},
        {"ILS-TELBOR", boost::make_shared<IborIndexParserWithPeriod<ILSTelbor>>()},
        {"INR-MIFOR", boost::make_shared<IborIndexParserWithPeriod<INRMifor>>()},
        {"MXN-TIIE", boost::make_shared<IborIndexParserWithPeriod<MXNTiie>>()},
        {"PLN-WIBOR", boost::make_shared<IborIndexParserWithPeriod<PLNWibor>>()},
        {"SKK-BRIBOR", boost::make_shared<IborIndexParserWithPeriod<SKKBribor>>()},
        {"NZD-BKBM", boost::make_shared<IborIndexParserWithPeriod<NZDBKBM>>()},
        {"TRY-TRLIBOR", boost::make_shared<IborIndexParserWithPeriod<TRLibor>>()},
        {"TWD-TAIBOR", boost::make_shared<IborIndexParserWithPeriod<TWDTaibor>>()},
        {"MYR-KLIBOR", boost::make_shared<IborIndexParserWithPeriod<MYRKlibor>>()},
        {"KRW-CD", boost::make_shared<IborIndexParserWithPeriod<KRWCd>>()},
        {"KRW-KORIBOR", boost::make_shared<IborIndexParserWithPeriod<KRWKoribor>>()},
        {"ZAR-JIBAR", boost::make_shared<IborIndexParserWithPeriod<Jibar>>()},
        {"RUB-MOSPRIME", boost::make_shared<IborIndexParserWithPeriod<RUBMosprime>>()},
        {"USD-SIFMA", boost::make_shared<IborIndexParserBMA<BMAIndexWrapper>>()},
        {"THB-BIBOR", boost::make_shared<IborIndexParserWithPeriod<THBBibor>>()},
        {"PHP-PHIREF", boost::make_shared<IborIndexParserWithPeriod<PHPPhiref>>()},
        {"COP-IBR", boost::make_shared<IborIndexParserOIS<COPIbr>>()},
        {"DEM-LIBOR", boost::make_shared<IborIndexParserWithPeriod<DEMLibor>>()},
        {"BRL-CDI", boost::make_shared<IborIndexParserOIS<BRLCdi>>()},
        {"NOK-NOWA", boost::make_shared<IborIndexParserOIS<Nowa>>()},
        {"CLP-CAMARA", boost::make_shared<IborIndexParserOIS<CLPCamara>>()},
        {"NZD-OCR", boost::make_shared<IborIndexParserOIS<Nzocr>>()},
        {"PLN-POLONIA", boost::make_shared<IborIndexParserOIS<PLNPolonia>>()}};

    auto it = m.find(tokens[0] + "-" + tokens[1]);
    if (it != m.end()) {
        return it->second->build(p, h);
    } else if (tokens[1] == "GENERIC") {
        // We have a generic index
        auto ccy = parseCurrency(tokens[0]);
        return boost::make_shared<GenericIborIndex>(p, ccy, h);
    } else {
        QL_FAIL("parseIborIndex \"" << s << "\" not recognized");
    }
}

bool isGenericIndex(const string& indexName) { 
    return indexName.find("-GENERIC-") != string::npos;
}

bool isInflationIndex(const string& indexName) {
    try {
        // Currently, only way to have an inflation index is to have a ZeroInflationIndex
        parseZeroInflationIndex(indexName);
    } catch (...) {
        return false;
    }
    return true;
}

// Swap Index Parser base
class SwapIndexParser {
public:
    virtual ~SwapIndexParser() {}
    virtual boost::shared_ptr<SwapIndex> build(Period p, const Handle<YieldTermStructure>& f,
                                               const Handle<YieldTermStructure>& d) const = 0;
};

// We build with both a forwarding and discounting curve
template <class T> class SwapIndexParserDualCurve : public SwapIndexParser {
public:
    boost::shared_ptr<SwapIndex> build(Period p, const Handle<YieldTermStructure>& f,
                                       const Handle<YieldTermStructure>& d) const override {
        return boost::make_shared<T>(p, f, d);
    }
};

boost::shared_ptr<SwapIndex> parseSwapIndex(const string& s, const Handle<YieldTermStructure>& f,
                                            const Handle<YieldTermStructure>& d,
                                            boost::shared_ptr<data::IRSwapConvention> convention) {

    std::vector<string> tokens;
    split(tokens, s, boost::is_any_of("-"));

    QL_REQUIRE(tokens.size() == 3, "three tokens required in " << s << ": CCY-CMS-TENOR");
    QL_REQUIRE(tokens[0].size() == 3, "invalid currency code in " << s);
    QL_REQUIRE(tokens[1] == "CMS", "expected CMS as middle token in " << s);

    Period p = parsePeriod(tokens[2]);

    string familyName = tokens[0] + "LiborSwapIsdaFix";
    Currency ccy = parseCurrency(tokens[0]);

    boost::shared_ptr<IborIndex> index =
        f.empty() || !convention ? boost::shared_ptr<IborIndex>() : convention->index()->clone(f);
    QuantLib::Natural settlementDays = index ? index->fixingDays() : 0;
    QuantLib::Calendar calender = convention ? convention->fixedCalendar() : NullCalendar();
    Period fixedLegTenor = convention ? Period(convention->fixedFrequency()) : Period(1, Months);
    BusinessDayConvention fixedLegConvention = convention ? convention->fixedConvention() : ModifiedFollowing;
    DayCounter fixedLegDayCounter = convention ? convention->fixedDayCounter() : ActualActual();

    if (d.empty())
        return boost::make_shared<SwapIndex>(familyName, p, settlementDays, ccy, calender, fixedLegTenor,
                                             fixedLegConvention, fixedLegDayCounter, index);
    else
        return boost::make_shared<SwapIndex>(familyName, p, settlementDays, ccy, calender, fixedLegTenor,
                                             fixedLegConvention, fixedLegDayCounter, index, d);
}

// Zero Inflation Index Parser
class ZeroInflationIndexParserBase {
public:
    virtual ~ZeroInflationIndexParserBase() {}
    virtual boost::shared_ptr<ZeroInflationIndex> build(bool isInterpolated,
                                                        const Handle<ZeroInflationTermStructure>& h) const = 0;
};

template <class T> class ZeroInflationIndexParser : public ZeroInflationIndexParserBase {
public:
    boost::shared_ptr<ZeroInflationIndex> build(bool isInterpolated,
                                                const Handle<ZeroInflationTermStructure>& h) const override {
        return boost::make_shared<T>(isInterpolated, h);
    }
};

boost::shared_ptr<ZeroInflationIndex> parseZeroInflationIndex(const string& s, bool isInterpolated,
                                                              const Handle<ZeroInflationTermStructure>& h) {

    static map<string, boost::shared_ptr<ZeroInflationIndexParserBase>> m = {
        {"EUHICP", boost::make_shared<ZeroInflationIndexParser<EUHICP>>()},
        {"EU HICP", boost::make_shared<ZeroInflationIndexParser<EUHICP>>()},
        {"EUHICPXT", boost::make_shared<ZeroInflationIndexParser<EUHICPXT>>()},
        {"EU HICPXT", boost::make_shared<ZeroInflationIndexParser<EUHICPXT>>()},
        {"FRHICP", boost::make_shared<ZeroInflationIndexParser<FRHICP>>()},
        {"FR HICP", boost::make_shared<ZeroInflationIndexParser<FRHICP>>()},
        {"UKRPI", boost::make_shared<ZeroInflationIndexParser<UKRPI>>()},
        {"UK RPI", boost::make_shared<ZeroInflationIndexParser<UKRPI>>()},
        {"USCPI", boost::make_shared<ZeroInflationIndexParser<USCPI>>()},
        {"US CPI", boost::make_shared<ZeroInflationIndexParser<USCPI>>()},
        {"ZACPI", boost::make_shared<ZeroInflationIndexParser<ZACPI>>()},
        {"ZA CPI", boost::make_shared<ZeroInflationIndexParser<ZACPI>>()},
        {"SECPI", boost::make_shared<ZeroInflationIndexParser<SECPI>>()},
        {"DKCPI", boost::make_shared<ZeroInflationIndexParser<DKCPI>>()}};

    auto it = m.find(s);
    if (it != m.end()) {
        return it->second->build(isInterpolated, h);
    } else {
        QL_FAIL("parseZeroInflationIndex: \"" << s << "\" not recognized");
    }
}

boost::shared_ptr<Index> parseIndex(const string& s, const data::Conventions& conventions) {
    boost::shared_ptr<QuantLib::Index> ret_idx;
    try {
        ret_idx = parseIborIndex(s);
    } catch (...) {
    }
    if (!ret_idx) {
        try {
            auto c = boost::dynamic_pointer_cast<SwapIndexConvention>(conventions.get(s));
            QL_REQUIRE(c, "no swap index convention");
            auto c2 = boost::dynamic_pointer_cast<IRSwapConvention>(conventions.get(c->conventions()));
            QL_REQUIRE(c2, "no swap convention");
            ret_idx = parseSwapIndex(s, Handle<YieldTermStructure>(), Handle<YieldTermStructure>(), c2);
        } catch (...) {
        }
    }
    if (!ret_idx) {
        try {
            ret_idx = parseZeroInflationIndex(s);
        } catch (...) {
        }
    }
    if (!ret_idx) {
        try {
            ret_idx = parseFxIndex(s);
        } catch (...) {
        }
    }
    if (!ret_idx) {
        try {
            ret_idx = parseEquityIndex(s);
        } catch (...) {
        }
    }
    QL_REQUIRE(ret_idx, "parseIndex \"" << s << "\" not recognized");
    return ret_idx;
}

bool isOvernightIndex(const string& indexName) {

    boost::shared_ptr<IborIndex> index;
    if (tryParseIborIndex(indexName, index)) {
        auto onIndex = boost::dynamic_pointer_cast<OvernightIndex>(index);
        if (onIndex)
            return true;
    }

    return false;
}

} // namespace data
} // namespace ore
