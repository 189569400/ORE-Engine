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

#include <boost/algorithm/string.hpp>
#include <boost/timer.hpp>

#ifdef BOOST_MSVC
// disable warning C4503: '__LINE__Var': decorated name length exceeded, name was truncated
// This pragma statement needs to be at the top of the file - lower and it will not work:
// http://stackoverflow.com/questions/9673504/is-it-possible-to-disable-compiler-warning-c4503
// http://boost.2283326.n4.nabble.com/General-Warnings-and-pragmas-in-MSVC-td2587449.html
#pragma warning(disable : 4503)
#endif

#include <iostream>

#include <boost/filesystem.hpp>

#include <orea/orea.hpp>
#include <ored/ored.hpp>
#include <ql/cashflows/floatingratecoupon.hpp>
#include <ql/time/calendars/all.hpp>
#include <ql/time/daycounters/all.hpp>

#include <orea/app/oreapp.hpp>

using namespace std;
using namespace ore::data;
using namespace ore::analytics;

namespace ore {
namespace analytics {

void OREApp::run() {

    boost::timer timer;

    try {
        setupLog();
        std::cout << "ORE starting" << std::endl;
        LOG("ORE starting");
        readSetup();

        /*************
         * Load Conventions
         */
        cout << setw(tab_) << left << "Conventions... " << flush;
        getConventions();
        cout << "OK" << endl;

        /*********
         * Build Markets
         */
        cout << setw(tab_) << left << "Market... " << flush;
        getMarketParameters();
        buildMarket();
        cout << "OK" << endl;

        /************************
         *Build Pricing Engine Factory
         */
        cout << setw(tab_) << left << "Engine factory... " << flush;
        boost::shared_ptr<EngineFactory> factory = buildEngineFactory(market_);
        cout << "OK" << endl;

        /******************************
         * Load and Build the Portfolio
         */
        cout << setw(tab_) << left << "Portfolio... " << flush;
        portfolio_ = buildPortfolio(factory);
        cout << "OK" << endl;

        /******************************
         * Write initial reports
         */
        cout << setw(tab_) << left << "Write Reports... " << flush;
        writeInitialReports();
        cout << "OK" << endl;

        /******************************************
         * Simulation: Scenario and Cube Generation
         */

        fflush(stdout);
        if (simulate_) {
            generateNPVCube();
        } else {
            LOG("skip simulation");
            cout << setw(tab_) << left << "Simulation... ";
            cout << "SKIP" << endl;
        }

        /*****************************
         * Aggregation and XVA Reports
         */
        cout << setw(tab_) << left << "Aggregation and XVA Reports... " << flush;
        if (xva_) {

            // We reset this here because the date grid building below depends on it.
            Settings::instance().evaluationDate() = asof_;

            // Use pre-generated cube
            if (!cube_)
                loadCube();

            QL_REQUIRE(cube_->numIds() == portfolio_->size(),
                       "cube x dimension (" << cube_->numIds() << ") does not match portfolio size ("
                                            << portfolio_->size() << ")");

            // Use pre-generared scenarios
            if (!scenarioData_)
                loadScenarioData();

            QL_REQUIRE(scenarioData_->dimDates() == cube_->dates().size(),
                       "scenario dates do not match cube grid size");
            QL_REQUIRE(scenarioData_->dimSamples() == cube_->samples(),
                       "scenario sample size does not match cube sample size");

            runPostProcessor();
            cout << "OK" << endl;
            cout << setw(tab_) << left << "Write Reports... " << flush;
            writeXVAReports();
            if (writeDIMReport_)
                writeDIMReport();
            cout << "OK" << endl;
        } else {
            LOG("skip XVA reports");
            cout << "SKIP" << endl;
        }

    } catch (std::exception& e) {
        ALOG("Error: " << e.what());
        cout << "Error: " << e.what() << endl;
    }

    cout << "run time: " << setprecision(2) << timer.elapsed() << " sec" << endl;
    cout << "ORE done." << endl;

    LOG("ORE done.");
}

void OREApp::readSetup() {

    params_->log();

    if (params_->has("setup", "observationModel")) {
        string om = params_->get("setup", "observationModel");
        ObservationMode::instance().setMode(om);
        LOG("Observation Mode is " << om);
    }

    writeInitialReports_ = true;
    simulate_ = (params_->hasGroup("simulation") && params_->get("simulation", "active") == "Y") ? true : false;
    buildSimMarket_ = true;
    xva_ = (params_->hasGroup("xva") && params_->get("xva", "active") == "Y") ? true : false;
    writeDIMReport_ = (params_->has("xva", "dim") && parseBool(params_->get("xva", "dim"))) ? true : false;
}

void OREApp::setupLog() {
    string outputPath = params_->get("setup", "outputPath");
    string logFile = outputPath + "/" + params_->get("setup", "logFile");
    Size logMask = 15; // Default level

    // Get log mask if available
    if (params_->has("setup", "logMask")) {
        logMask = static_cast<Size>(parseInteger(params_->get("setup", "logMask")));
    }

    boost::filesystem::path p{outputPath};
    if (!boost::filesystem::exists(p)) {
        boost::filesystem::create_directories(p);
    }
    QL_REQUIRE(boost::filesystem::is_directory(p), "output path '" << outputPath << "' is not a directory.");

    Log::instance().registerLogger(boost::make_shared<FileLogger>(logFile));
    Log::instance().setMask(logMask);
    Log::instance().switchOn();
}

void OREApp::getConventions() {
    string inputPath = params_->get("setup", "inputPath");
    string conventionsFile = inputPath + "/" + params_->get("setup", "conventionsFile");
    conventions_.fromFile(conventionsFile);
}

void OREApp::buildMarket() {
    /*******************************
     * Market and fixing data loader
     */
    cout << endl << setw(tab_) << left << "Market data loader... " << flush;
    string inputPath = params_->get("setup", "inputPath");
    string marketFile = inputPath + "/" + params_->get("setup", "marketDataFile");
    string fixingFile = inputPath + "/" + params_->get("setup", "fixingDataFile");
    string implyTodaysFixingsString = params_->get("setup", "implyTodaysFixings");
    bool implyTodaysFixings = parseBool(implyTodaysFixingsString);
    CSVLoader loader(marketFile, fixingFile, implyTodaysFixings);
    cout << "OK" << endl;
    /**********************
     * Curve configurations
     */
    cout << setw(tab_) << left << "Curve configuration... " << flush;
    CurveConfigurations curveConfigs;
    string curveConfigFile = inputPath + "/" + params_->get("setup", "curveConfigFile");
    curveConfigs.fromFile(curveConfigFile);
    cout << "OK" << endl;

    market_ = boost::make_shared<TodaysMarket>(asof_, marketParameters_, loader, curveConfigs, conventions_);
}

void OREApp::getMarketParameters() {
    string inputPath = params_->get("setup", "inputPath");
    string marketConfigFile = inputPath + "/" + params_->get("setup", "marketConfigFile");
    marketParameters_.fromFile(marketConfigFile);
}

boost::shared_ptr<EngineFactory> OREApp::buildEngineFactory(const boost::shared_ptr<Market>& market) {
    string inputPath = params_->get("setup", "inputPath");
    string pricingEnginesFile = inputPath + "/" + params_->get("setup", "pricingEnginesFile");
    boost::shared_ptr<EngineData> engineData = boost::make_shared<EngineData>();
    engineData->fromFile(pricingEnginesFile);

    map<MarketContext, string> configurations;
    configurations[MarketContext::irCalibration] = params_->get("markets", "lgmcalibration");
    configurations[MarketContext::fxCalibration] = params_->get("markets", "fxcalibration");
    configurations[MarketContext::pricing] = params_->get("markets", "pricing");
    boost::shared_ptr<EngineFactory> factory = boost::make_shared<EngineFactory>(engineData, market, configurations);
    return factory;
}

boost::shared_ptr<TradeFactory> OREApp::buildTradeFactory() { return boost::make_shared<TradeFactory>(); }

boost::shared_ptr<Portfolio> OREApp::buildPortfolio(const boost::shared_ptr<EngineFactory>& factory) {
    string inputPath = params_->get("setup", "inputPath");
    string portfolioFile = inputPath + "/" + params_->get("setup", "portfolioFile");
    boost::shared_ptr<Portfolio> portfolio = boost::make_shared<Portfolio>();
    portfolio->load(portfolioFile, buildTradeFactory());
    portfolio->build(factory);
    return portfolio;
}

boost::shared_ptr<ScenarioSimMarketParameters> OREApp::getSimMarketData() {
    string inputPath = params_->get("setup", "inputPath");
    string simulationConfigFile = inputPath + "/" + params_->get("simulation", "simulationConfigFile");
    boost::shared_ptr<ScenarioSimMarketParameters> simMarketData(new ScenarioSimMarketParameters);
    simMarketData->fromFile(simulationConfigFile);
    return simMarketData;
}

boost::shared_ptr<ScenarioGeneratorData> OREApp::getScenarioGeneratorData() {
    string inputPath = params_->get("setup", "inputPath");
    string simulationConfigFile = inputPath + "/" + params_->get("simulation", "simulationConfigFile");
    boost::shared_ptr<ScenarioGeneratorData> sgd(new ScenarioGeneratorData);
    sgd->fromFile(simulationConfigFile);
    return sgd;
}
boost::shared_ptr<ScenarioGenerator>
OREApp::buildScenarioGenerator(boost::shared_ptr<Market> market,
                               boost::shared_ptr<ScenarioSimMarketParameters> simMarketData,
                               boost::shared_ptr<ScenarioGeneratorData> sgd) {
    LOG("Build Simulation Model");
    string inputPath = params_->get("setup", "inputPath");
    string simulationConfigFile = inputPath + "/" + params_->get("simulation", "simulationConfigFile");
    LOG("Load simulation model data from file: " << simulationConfigFile);
    boost::shared_ptr<CrossAssetModelData> modelData = boost::make_shared<CrossAssetModelData>();
    modelData->fromFile(simulationConfigFile);
    string lgmCalibrationMarketStr = Market::defaultConfiguration;
    if (params_->has("markets", "lgmcalibration"))
        lgmCalibrationMarketStr = params_->get("markets", "lgmcalibration");
    string fxCalibrationMarketStr = Market::defaultConfiguration;
    if (params_->has("markets", "fxcalibration"))
        fxCalibrationMarketStr = params_->get("markets", "fxcalibration");
    string eqCalibrationMarketStr = Market::defaultConfiguration;
    if (params_->has("markets", "eqcalibration"))
        eqCalibrationMarketStr = params_->get("markets", "eqcalibration");
    string simulationMarketStr = Market::defaultConfiguration;
    if (params_->has("markets", "simulation"))
        simulationMarketStr = params_->get("markets", "simulation");

    CrossAssetModelBuilder modelBuilder(market, lgmCalibrationMarketStr, fxCalibrationMarketStr, eqCalibrationMarketStr,
                                        simulationMarketStr);
    boost::shared_ptr<QuantExt::CrossAssetModel> model = modelBuilder.build(modelData);

    LOG("Load Simulation Parameters");
    ScenarioGeneratorBuilder sgb(sgd);
    boost::shared_ptr<ScenarioFactory> sf = boost::make_shared<SimpleScenarioFactory>();
    boost::shared_ptr<ScenarioGenerator> sg = sgb.build(
        model, sf, simMarketData, asof_, market, params_->get("markets", "simulation")); // pricing or simulation?
    // Optionally write out scenarios
    if (params_->has("simulation", "scenariodump")) {
        string outputPath = params_->get("setup", "outputPath");
        string filename = outputPath + "/" + params_->get("simulation", "scenariodump");
        sg = boost::make_shared<ScenarioWriter>(sg, filename);
    }
    return sg;
}

void OREApp::writeInitialReports() {

    string outputPath = params_->get("setup", "outputPath");
    string inputPath = params_->get("setup", "inputPath");

    /************
     * Curve dump
     */
    cout << endl << setw(tab_) << left << "Curve Report... " << flush;
    if (params_->hasGroup("curves") && params_->get("curves", "active") == "Y") {
        string fileName = outputPath + "/" + params_->get("curves", "outputFileName");
        CSVFileReport curvesReport(fileName);
        DateGrid grid(params_->get("curves", "grid"));
        ReportWriter::writeCurves(curvesReport, params_->get("curves", "configuration"), grid, marketParameters_,
                                  market_);
        cout << "OK" << endl;
    } else {
        LOG("skip curve report");
        cout << "SKIP" << endl;
    }

    /*********************
     * Portfolio valuation
     */
    cout << setw(tab_) << left << "NPV Report... " << flush;
    if (params_->hasGroup("npv") && params_->get("npv", "active") == "Y") {
        string fileName = outputPath + "/" + params_->get("npv", "outputFileName");
        CSVFileReport npvReport(fileName);
        ReportWriter::writeNpv(npvReport, params_->get("npv", "baseCurrency"), market_,
                               params_->get("markets", "pricing"), portfolio_);
        cout << "OK" << endl;
    } else {
        LOG("skip portfolio valuation");
        cout << "SKIP" << endl;
    }

    /**********************
     * Cash flow generation
     */
    cout << setw(tab_) << left << "Cashflow Report... " << flush;
    if (params_->hasGroup("cashflow") && params_->get("cashflow", "active") == "Y") {
        string fileName = outputPath + "/" + params_->get("cashflow", "outputFileName");
        CSVFileReport cashflowReport(fileName);
        ReportWriter::writeCashflow(cashflowReport, portfolio_);
        cout << "OK" << endl;
    } else {
        LOG("skip cashflow generation");
        cout << "SKIP" << endl;
    }
}

void OREApp::initAggregationScenarioData() {
    scenarioData_ = boost::make_shared<InMemoryAggregationScenarioData>(grid_->size(), samples_);
}

void OREApp::initCube() {
    if (cubeDepth_ == 1)
        cube_ = boost::make_shared<SinglePrecisionInMemoryCube>(asof_, simPortfolio_->ids(), grid_->dates(), samples_);
    else if (cubeDepth_ == 2)
        cube_ = boost::make_shared<SinglePrecisionInMemoryCubeN>(asof_, simPortfolio_->ids(), grid_->dates(), samples_,
                                                                 cubeDepth_);
    else {
        QL_FAIL("cube depth 1 or 2 expected");
    }
}

void OREApp::buildNPVCube() {
    LOG("Build valuation cube engine");
    // Valuation calculators
    string baseCurrency = params_->get("simulation", "baseCurrency");
    vector<boost::shared_ptr<ValuationCalculator>> calculators;
    calculators.push_back(boost::make_shared<NPVCalculator>(baseCurrency));
    if (cubeDepth_ > 1)
        calculators.push_back(boost::make_shared<CashflowCalculator>(baseCurrency, asof_, grid_, 1));
    LOG("Build cube");
    ValuationEngine engine(asof_, grid_, simMarket_);
    ostringstream o;
    o.str("");
    o << "Build Cube " << simPortfolio_->size() << " x " << grid_->size() << " x " << samples_ << "... ";

    auto progressBar = boost::make_shared<SimpleProgressBar>(o.str(), tab_);
    auto progressLog = boost::make_shared<ProgressLog>("Building cube...");
    engine.registerProgressIndicator(progressBar);
    engine.registerProgressIndicator(progressLog);
    engine.buildCube(simPortfolio_, cube_, calculators);
    cout << "OK" << endl;
}

void OREApp::generateNPVCube() {
    cout << setw(tab_) << left << "Simulation Setup... ";
    LOG("Load Simulation Market Parameters");
    boost::shared_ptr<ScenarioSimMarketParameters> simMarketData = getSimMarketData();
    boost::shared_ptr<ScenarioGeneratorData> sgd = getScenarioGeneratorData();
    grid_ = sgd->grid();
    samples_ = sgd->samples();
    boost::shared_ptr<ScenarioGenerator> sg = buildScenarioGenerator(market_, simMarketData, sgd);

    if (buildSimMarket_) {
        LOG("Build Simulation Market");
        simMarket_ = boost::make_shared<ScenarioSimMarket>(sg, market_, simMarketData, conventions_,
                                                           params_->get("markets", "simulation"));
        boost::shared_ptr<EngineFactory> simFactory = buildEngineFactory(simMarket_);

        LOG("Build portfolio linked to sim market");
        simPortfolio_ = buildPortfolio(simFactory);
        QL_REQUIRE(simPortfolio_->size() == portfolio_->size(),
                   "portfolio size mismatch, check simulation market setup");
        cout << "OK" << endl;
    }

    if (params_->has("simulation", "storeFlows") && params_->get("simulation", "storeFlows") == "Y")
        cubeDepth_ = 2; // NPV and FLOW
    else
        cubeDepth_ = 1; // NPV only

    ostringstream o;
    o << "Aggregation Scenario Data " << grid_->size() << " x " << samples_ << "... ";
    cout << setw(tab_) << o.str() << flush;

    initAggregationScenarioData();
    // Set AggregationScenarioData
    simMarket_->aggregationScenarioData() = scenarioData_;
    cout << "OK" << endl;

    initCube();
    buildNPVCube();
    writeCube();
    writeScenarioData();
}

void OREApp::writeCube() {
    cout << endl << setw(tab_) << left << "Write Cube... " << flush;
    LOG("Write cube");
    if (params_->has("simulation", "cubeFile")) {
        string outputPath = params_->get("setup", "outputPath");
        string cubeFileName = outputPath + "/" + params_->get("simulation", "cubeFile");
        cube_->save(cubeFileName);
        cout << "OK" << endl;
    } else
        cout << "SKIP" << endl;
}

void OREApp::writeScenarioData() {
    cout << endl << setw(tab_) << left << "Write Aggregation Scenario Data... " << flush;
    LOG("Write scenario data");
    if (params_->has("simulation", "additionalScenarioDataFileName")) {
        string outputPath = params_->get("setup", "outputPath");
        string outputFileNameAddScenData =
            outputPath + "/" + params_->get("simulation", "additionalScenarioDataFileName");
        scenarioData_->save(outputFileNameAddScenData);
        cout << "OK" << endl;
    } else
        cout << "SKIP" << endl;
}

void OREApp::loadScenarioData() {
    string outputPath = params_->get("setup", "outputPath");
    string scenarioFile = outputPath + "/" + params_->get("xva", "scenarioFile");
    scenarioData_ = boost::make_shared<InMemoryAggregationScenarioData>();
    scenarioData_->load(scenarioFile);
}

void OREApp::loadCube() {
    string outputPath = params_->get("setup", "outputPath");
    string cubeFile = outputPath + "/" + params_->get("xva", "cubeFile");
    cubeDepth_ = 1;
    if (params_->has("xva", "hyperCube"))
        cubeDepth_ = parseBool(params_->get("xva", "hyperCube")) ? 2 : 1;

    if (cubeDepth_ > 1)
        cube_ = boost::make_shared<SinglePrecisionInMemoryCubeN>();
    else
        cube_ = boost::make_shared<SinglePrecisionInMemoryCube>();
    LOG("Load cube from file " << cubeFile);
    cube_->load(cubeFile);
    LOG("Cube loading done");
}

boost::shared_ptr<NettingSetManager> OREApp::initNettingSetManager() {
    string inputPath = params_->get("setup", "inputPath");
    string csaFile = inputPath + "/" + params_->get("xva", "csaFile");
    boost::shared_ptr<NettingSetManager> netting = boost::make_shared<NettingSetManager>();
    netting->fromFile(csaFile);
    return netting;
}

void OREApp::runPostProcessor() {
    boost::shared_ptr<NettingSetManager> netting = initNettingSetManager();
    map<string, bool> analytics;
    analytics["exerciseNextBreak"] = parseBool(params_->get("xva", "exerciseNextBreak"));
    analytics["exposureProfiles"] = parseBool(params_->get("xva", "exposureProfiles"));
    analytics["cva"] = parseBool(params_->get("xva", "cva"));
    analytics["dva"] = parseBool(params_->get("xva", "dva"));
    analytics["fva"] = parseBool(params_->get("xva", "fva"));
    analytics["colva"] = parseBool(params_->get("xva", "colva"));
    analytics["collateralFloor"] = parseBool(params_->get("xva", "collateralFloor"));
    if (params_->has("xva", "mva"))
        analytics["mva"] = parseBool(params_->get("xva", "mva"));
    else
        analytics["mva"] = false;
    if (params_->has("xva", "dim"))
        analytics["dim"] = parseBool(params_->get("xva", "dim"));
    else
        analytics["dim"] = false;

    string baseCurrency = params_->get("xva", "baseCurrency");
    string calculationType = params_->get("xva", "calculationType");
    string allocationMethod = params_->get("xva", "allocationMethod");
    Real marginalAllocationLimit = parseReal(params_->get("xva", "marginalAllocationLimit"));
    Real quantile = parseReal(params_->get("xva", "quantile"));
    string dvaName = params_->get("xva", "dvaName");
    string fvaLendingCurve = params_->get("xva", "fvaLendingCurve");
    string fvaBorrowingCurve = params_->get("xva", "fvaBorrowingCurve");
    Real collateralSpread = parseReal(params_->get("xva", "collateralSpread"));

    Real dimQuantile = 0.99;
    Size dimHorizonCalendarDays = 14;
    Size dimRegressionOrder = 0;
    vector<string> dimRegressors;
    Real dimScaling = 1.0;
    Size dimLocalRegressionEvaluations = 0;
    Real dimLocalRegressionBandwidth = 0.25;

    if (analytics["mva"] || analytics["dim"]) {
        dimQuantile = parseReal(params_->get("xva", "dimQuantile"));
        dimHorizonCalendarDays = parseInteger(params_->get("xva", "dimHorizonCalendarDays"));
        dimRegressionOrder = parseInteger(params_->get("xva", "dimRegressionOrder"));
        string dimRegressorsString = params_->get("xva", "dimRegressors");
        dimRegressors = parseListOfValues(dimRegressorsString);
        dimScaling = parseReal(params_->get("xva", "dimScaling"));
        dimLocalRegressionEvaluations = parseInteger(params_->get("xva", "dimLocalRegressionEvaluations"));
        dimLocalRegressionBandwidth = parseReal(params_->get("xva", "dimLocalRegressionBandwidth"));
    }

    string marketConfiguration = params_->get("markets", "simulation");

    postProcess_ = boost::make_shared<PostProcess>(
        portfolio_, netting, market_, marketConfiguration, cube_, scenarioData_, analytics, baseCurrency,
        allocationMethod, marginalAllocationLimit, quantile, calculationType, dvaName, fvaBorrowingCurve,
        fvaLendingCurve, collateralSpread, dimQuantile, dimHorizonCalendarDays, dimRegressionOrder, dimRegressors,
        dimLocalRegressionEvaluations, dimLocalRegressionBandwidth, dimScaling);
}

void OREApp::writeXVAReports() {
    string outputPath = params_->get("setup", "outputPath");
    for (auto t : postProcess_->tradeIds()) {
        ostringstream o;
        o << outputPath << "/exposure_trade_" << t << ".csv";
        string tradeExposureFile = o.str();
        CSVFileReport tradeExposureReport(tradeExposureFile);
        ReportWriter::writeTradeExposures(tradeExposureReport, postProcess_, t);
    }
    for (auto n : postProcess_->nettingSetIds()) {
        ostringstream o1;
        o1 << outputPath << "/exposure_nettingset_" << n << ".csv";
        string nettingSetExposureFile = o1.str();
        CSVFileReport nettingSetExposureReport(nettingSetExposureFile);
        ReportWriter::writeNettingSetExposures(nettingSetExposureReport, postProcess_, n);

        ostringstream o2;
        o2 << outputPath << "/colva_nettingset_" << n << ".csv";
        string nettingSetColvaFile = o2.str();
        CSVFileReport nettingSetColvaReport(nettingSetColvaFile);
        ReportWriter::writeNettingSetColva(nettingSetColvaReport, postProcess_, n);
    }

    string XvaFile = outputPath + "/xva.csv";
    CSVFileReport xvaReport(XvaFile);
    ReportWriter::writeXVA(xvaReport, params_->get("xva", "allocationMethod"), portfolio_, postProcess_);

    string rawCubeOutputFile = params_->get("xva", "rawCubeOutputFile");
    CubeWriter cw1(outputPath + "/" + rawCubeOutputFile);
    map<string, string> nettingSetMap = portfolio_->nettingSetMap();
    cw1.write(cube_, nettingSetMap);

    string netCubeOutputFile = params_->get("xva", "netCubeOutputFile");
    CubeWriter cw2(outputPath + "/" + netCubeOutputFile);
    cw2.write(postProcess_->netCube(), nettingSetMap);
}

void OREApp::writeDIMReport() {
    string outputPath = params_->get("setup", "outputPath");
    string dimFile1 = outputPath + "/" + params_->get("xva", "dimEvolutionFile");
    vector<string> dimFiles2;
    for (auto f : parseListOfValues(params_->get("xva", "dimRegressionFiles")))
        dimFiles2.push_back(outputPath + "/" + f);
    string nettingSet = params_->get("xva", "dimOutputNettingSet");
    std::vector<Size> dimOutputGridPoints =
        parseListOfValues<Size>(params_->get("xva", "dimOutputGridPoints"), &parseInteger);
    postProcess_->exportDimEvolution(dimFile1, nettingSet);
    postProcess_->exportDimRegression(dimFiles2, nettingSet, dimOutputGridPoints);
}
}
}
