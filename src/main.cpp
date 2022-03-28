#ifndef _MAIN_H_
#define _MAIN_H_
#include "main.h"
#endif

// Number of command line parameters.
const int regGridParams = 25;
const int adapGridParams = 26;
const int adapGridParamsLID = 28;
const int adapGridParamsOld = 27;

int main (int argc, char* cArgv[])
{
    // Introduce timing related objects.
    int iStartClock = clock(); // For Windows version

    // Print header.
    std::cout << "\n\n************************************************************";
    std::cout << "\n*                       GisToSWMM5                         *";
    std::cout << "\n*           Creates a SWMM5 computational model.           *";
    std::cout << "\n*     The program is developed under the MIT license.      *";
    std::cout << "\n*                (C) Aalto University 2016.                *";
    std::cout << "\n************************************************************";

    // Print commandline arguments.
    std::cout << "\n\nCommand line arguments:";

    for (int i = 0; i < argc; i++)
    {
        std::cout << "\n-> Argument " << i << ": " << cArgv[i];
    }

    if (argc == regGridParams || argc == adapGridParams || argc == adapGridParamsOld || argc == adapGridParamsLID)
    {
        std::cout << "\n\nLoading data:";

        // Load the digital elevation map raster file.
        std::cout << "\n-> Loading digital elevation map raster";
        Raster demRaster;
        int resDem = demRaster.load( cArgv[1] );

        // Load the flow direction raster file.
        std::cout << "\n-> Loading flow direction raster";
        Raster flowdirRaster;
        int resFlowdirRaster = flowdirRaster.load( cArgv[2] );

        // Load the landuse raster file
        std::cout << "\n-> Loading landuse raster";
        Raster landuseRaster;
        int resLanduse = landuseRaster.load( cArgv[3] );

        // Load catchment properties table.
        std::cout << "\n-> Loading catchment properties table";
        Table catchPropTable;
        int resCatchProp = catchPropTable.load( cArgv[4] );

        // Load network junctions table.
        std::cout << "\n-> Loading network junctions table";
        Table juncTable;
        int resJunc = juncTable.load( cArgv[5] );

        // Load network conduits table.
        std::cout << "\n-> Loading network conduits table";
        Table condTable;
        int resCond = condTable.load( cArgv[6] );

        // Load network outfalls table.
        std::cout << "\n-> Loading network outfalls table";
        Table outfallsTable;
        int resOutfalls = outfallsTable.load( cArgv[7] );

        // Load input file header.
        std::cout << "\n-> Loading input file header table";
        Table headerTable;
        int resHeader = headerTable.load( cArgv[8] );

        // Load input file evaporation settings.
        std::cout << "\n-> Loading input file evaporation table";
        Table evaporationTable;
        int resEvaporation = evaporationTable.load( cArgv[9] );

        // Load input file temperature settings.
        std::cout << "\n-> Loading input file temperature table";
        Table temperatureTable;
        int resTemperature = temperatureTable.load( cArgv[10] );

        // Load snowpacks table.
        std::cout << "\n-> Loading input file snowpacks table";
        Table snowpacksTable;
        int resSnowpacks = snowpacksTable.load( cArgv[11] );

        // Load raingage table.
        std::cout << "\n-> Loading input file raingages table";
        Table raingagesTable;
        int resRaingages = raingagesTable.load( cArgv[12] );

        // Load inflows table.
        std::cout << "\n-> Loading inflows table";
        Table inflowsTable;
        int resInflows = inflowsTable.load(cArgv[13]);

        // Load time series table.
        std::cout << "\n-> Loading time series table";
        Table timeseriesTable;
        int resTimeseries = timeseriesTable.load(cArgv[14]);

        // Load pump table.
        std::cout << "\n-> Loading pump table";
        Table pumpsTable;
        int resPumps = pumpsTable.load(cArgv[15]);

        // Load pump curves table.
        std::cout << "\n-> Loading pump curves table";
        Table pumpCurvesTable;
        int resPumpCurves = pumpCurvesTable.load(cArgv[16]);

        // Load storage table.
        std::cout << "\n-> Loading storage table";
        Table storageTable;
        int resStorages = storageTable.load(cArgv[17]);

        // Load dwf table.
        std::cout << "\n-> Loading dwf table";
        Table dwfTable;
        int resDwf = dwfTable.load(cArgv[18]);

        // Load patterns table.
        std::cout << "\n-> Loading patterns table";
        Table patternsTable;
        int resPatterns = patternsTable.load(cArgv[19]);

        // Load losses table.
        std::cout << "\n-> Loading losses table";
        Table lossesTable;
        int resLosses = lossesTable.load(cArgv[20]);

        // Load xsections table.
        std::cout << "\n-> Loading xsection table";
        Table xsectionTable;
        int resXsections = xsectionTable.load(cArgv[21]);

        // Load input file report settings.
        std::cout << "\n-> Loading input file report settings table";
        Table reportTable;
        int resReport = reportTable.load( cArgv[22] );

        // Load input file symbols.
        std::cout << "\n-> Loading input file symbols table";
        Table symbolsTable;
        int resSymbols = symbolsTable.load( cArgv[23] );

        Table lidControlsTable;
        Raster lidRaster;
        int resLidControls = 1, resLidType = 1;
        if (argc == adapGridParamsLID){
            // Load LID controls table.
            std::cout << "\n-> Loading input file LID controls table";
            resLidControls = lidControlsTable.loadFileName( cArgv[26] );

            // Load the LID type raster file.
            std::cout << "\n-> Loading LID type raster";
            resLidType = lidRaster.load( cArgv[27] );
        }

        if (resDem == 0 && resLanduse == 0 && resCatchProp == 0 && resCatchProp == 0 &&
                resCond == 0 && resOutfalls == 0 && resHeader == 0 && resEvaporation == 0 &&
                resTemperature == 0 && resRaingages == 0 && resInflows == 0 && resTimeseries == 0
                && resPumps == 0 && resPumpCurves == 0 && resStorages == 0 && resDwf == 0
                && resPatterns == 0 && resLosses == 0 && resXsections == 0 && resReport == 0
                && resSymbols == 0 && resSnowpacks == 0)
        {
            // Create the computational grid.
            std::cout << "\n\nCreating the computational grid:";
            int gridType = -1;
            Grid grid;
            int resDiscretization = 1;

            if (argc == regGridParams || argc == adapGridParams || argc == adapGridParamsLID)
            {
                // Print discretization method.
                std::cout << "\n-> Discretizing the area with a regular grid";
                gridType = 0;
                resDiscretization = grid.create(gridType, landuseRaster, demRaster);
            }
            else if (argc == adapGridParamsOld)
            {
                // Print discretization method.
                std::cout << "\n-> Discretizing the area with a rectangular adaptive grid:";
                std::cout << "\n-> Initial grid cell size: " << cArgv[25] << "x" << cArgv[25] << "m^2";
                std::cout << "\n-> Number of subdivisions: " << cArgv[26];
                gridType = 1;
                resDiscretization = grid.create(gridType, atof(cArgv[25]), atoi(cArgv[26]),
                                                landuseRaster, demRaster);
            }

            if (resDiscretization != 0)
            {
                std::cout << "\n-> Error in the discretization stage of the grid creation.";

                return 1;
            }

            // Set cell names.
            std::cout << "\n-> Setting cell names";
            grid.setCellNames();

            if (gridType == 0)  // NxN or adap grid
            {
                // Set cell dimensions.
                std::cout << "\n-> Setting cell dimensions";
                grid.computeCellDimensions(landuseRaster.cellSize);

                // Compute cell centerpoints.
                std::cout << "\n-> Computing cell centerpoint coordinates";
                grid.computeCellCenterpoints(landuseRaster.xllCorner, landuseRaster.yllCorner);
            }

            // Set cell elevations.
            std::cout << "\n-> Setting cell elevations";
            grid.setCellElevations(demRaster);

            if (gridType == 0)  // NxN or adap grid
            {
                // Set cell flow directions
                std::cout << "\n-> Setting cell flow directions";
                grid.setCellFlowdirs(flowdirRaster);

                // Set cell landuse.
                std::cout << "\n-> Setting cell landuses";
                grid.setCellLanduse(landuseRaster);

                if (argc == adapGridParamsLID && resLidType == 0){
                    // Set cell LID type.
                    std::cout << "\n-> Setting cell LID type";
                    grid.setCellLidType(lidRaster);
                } else{
                    std::cout << "\nAll LID types are set to 'NONE' by default";
                }
            }

            // Compute active grid extents.
            std::cout << "\n-> Computing active grid extents";
            grid.computeGridExtents();

            // Save cell properties.
            std::cout << "\n-> Saving cell properties";
            grid.setSubcatchmentProperties(catchPropTable);

            // Find cell neighbours.
            std::cout << "\n-> Finding cell neighbours";
            grid.findCellNeighbours();

            // Route cells to a neighbour cells. Also update flow width parameter of cells.
            if (gridType == 0)  // NxN or adap grid
            {
                std::cout << "\n-> Routing cells to neighbour cells using flow direction raster and updating flow width parameter";
                grid.routeCellsReg();
            }
            else if (gridType == 1)     // N*(2^a) x N*(2^a) grid where a is the number of subdivisions
            {
                std::cout << "\n-> Routing cells to neighbour cells and updating flow width parameter";
                grid.routeCells();
            }

            // Compute cell slopes.
            std::cout << "\n-> Computing cell slopes";
            grid.computeCellSlopes();

            // Set the outlets of cells with junctions to the junctions.
            std::cout << "\n-> Connecting cells to junctions";
            grid.connectCellsToJunctions(juncTable);

            // Set outlet of rooftops cells and cells withouth outlets to the nearest junction.
            std::cout << "\n-> Routing rooftop cells and other artificial impermeable pit cells to the nearest junctions";
            grid.routePavedPitAndRooftopCells(juncTable);

            // Set outlets of cells without outlets to themselves.
            std::cout << "\n-> Routing natural (rock, vegetation and sand) pit cells to themselves";
            grid.routePitCells();

            // Create and save a WKT vector file of all individual subcatchment cells in the study area
            std::cout << "\n-> Creating a subcatchment polygon file for inspection";
            std::ostringstream strs;
            strs << cArgv[24] << "_subcatchments_" << (int) landuseRaster.cellSize << "x" << (int) landuseRaster.cellSize << "m";
            std::string outNameSubcatchments = strs.str();
            grid.saveSubcatchmentPolygon(outNameSubcatchments);

            // Create and save a WKT vector file of subcatchment routing between individual subcatchment cells in study area
            std::cout << "\n-> Creating a subcatchment routing file for inspection";
            std::string outNameSubcatchmentRouting = strs.str() + "_routing";
            grid.saveSubcatchmentRouting(outNameSubcatchmentRouting);

            // Find routed subcatchments cells and save to wkt-file
            std::cout << "\n-> Creating a file of routed subcatchments for inspection";
            std::string outNameSubcatchmentRouted = strs.str() + "_routed";
            std::vector<int> routedIDs = grid.findRouted(juncTable, outNameSubcatchmentRouted);

            // Create and save a WKT vector file of network routing
            std::cout << "\n-> Creating a network routing file for inspection";
            grid.saveNetworkRouting(cArgv[24], condTable);

            // Simplify subcatchments based on common landuse and routing
            if (argc == adapGridParams)
            {
                std::cout << "\n-> Creating adaptive subcatchments based on landuse and routing";
                std::string outNameAdapSubcatchments(cArgv[24]);
                outNameAdapSubcatchments += "_subcatchments";
                grid.simplify(juncTable, outNameAdapSubcatchments);

                // Create and save a WKT vector file of subcatchment routing in adaptive grid
                std::cout << "\n-> Creating adaptive subcatchment routing file for inspection";
                std::string outNameSubcatchmentRouting(cArgv[24]);
                outNameSubcatchmentRouting += "_subcatchment_routing";
                grid.saveSubcatchmentRouting(outNameSubcatchmentRouting);

                std::cout << "\n-> Creating a file of routed adaptive subcatchments for inspection";
                std::string outNameAdapSubcatchmentAttr(cArgv[24]);
                outNameAdapSubcatchmentAttr += "_subcatchments_attr";
                int resSaveSimple = grid.saveSubcatchmentPolygon(outNameAdapSubcatchmentAttr);
                if (resSaveSimple != 0)
                {
                    std::cout << "\n-> Error in creating an adaptive subcatchment routing file for inspection.";

                    return 1;
                }
            }

            // Simplify subcatchments based on common landuse, routing and LID type
            if (argc == adapGridParamsLID)
            {
                std::cout << "\n-> Creating adaptive subcatchments based on landuse, routing and LID type";
                std::string outNameAdapSubcatchments(cArgv[24]);
                outNameAdapSubcatchments += "_subcatchments";
                grid.simplifyLID(juncTable, outNameAdapSubcatchments);

                // Create and save a WKT vector file of subcatchment routing in adaptive grid
                std::cout << "\n-> Creating adaptive subcatchment routing file for inspection";
                std::string outNameSubcatchmentRouting(cArgv[24]);
                outNameSubcatchmentRouting += "_subcatchment_routing";
                grid.saveSubcatchmentRouting(outNameSubcatchmentRouting);

                std::cout << "\n-> Creating a file of routed adaptive subcatchments for inspection";
                std::string outNameAdapSubcatchmentAttr(cArgv[24]);
                outNameAdapSubcatchmentAttr += "_subcatchments_attr";
                int resSaveSimple = grid.saveSubcatchmentPolygon(outNameAdapSubcatchmentAttr);
                if (resSaveSimple != 0)
                {
                    std::cout << "\n-> Error in creating an adaptive subcatchment routing file for inspection.";

                    return 1;
                }
            }

            // Create the SWMM5 file.
            std::cout << "\n\nCreating the SWMM5 model input file:";
            grid.saveSWMM5File(headerTable, catchPropTable, evaporationTable, temperatureTable,
                               inflowsTable, timeseriesTable, reportTable, snowpacksTable,
                               raingagesTable, symbolsTable, juncTable, outfallsTable, condTable,
                               pumpsTable, pumpCurvesTable, dwfTable, patternsTable, lossesTable,
                               storageTable, xsectionTable, lidControlsTable, cArgv[24]);

            // Print report.
            std::cout << "\n\nReport:";
            std::cout << "\n-> Running time: ";
            std::cout << (double)(clock() - iStartClock)/(double)(CLOCKS_PER_SEC * 60) << " minutes";
            if (argc == adapGridParams)
            {
                grid.printReport(catchPropTable);
            }
            else if (argc == adapGridParamsLID)
            {
                // Todo: Add details about LID in the printReport
                grid.printReport(catchPropTable);
               
            }
            else
            {
                grid.printReport(catchPropTable, routedIDs);
            }
            std::cout << "\n\n";
        }
        else
        {
            if (resDem > 0)
            {
                std::cout << "\nError, digital elevation map raster was not succesfully loaded.";
            }

            if (resFlowdirRaster > 0)
            {
                std::cout << "\nError, flow direction raster was not succesfully loaded.";
            }

            if (resLanduse > 0)
            {
                std::cout << "\nError, land use raster was not succesfully loaded.";
            }

            if (resCatchProp > 0)
            {
                std::cout << "\nError, catchment properties table was not succesfully loaded.";
            }

            if (resJunc > 0)
            {
                std::cout << "\nError, network junctions table was not succesfully loaded.";
            }

            if (resCond > 0)
            {
                std::cout << "\nError, network conduits table was not succesfully loaded.";
            }

            if (resOutfalls > 0)
            {
                std::cout << "\nError, network outfalls table was not succesfully loaded.";
            }

            if (resHeader > 0)
            {
                std::cout << "\nError, input file header table was not succesfully loaded.";
            }

            if (resEvaporation > 0)
            {
                std::cout << "\nError, input file evaporation table was not succesfully loaded.";
            }

            if (resTemperature > 0)
            {
                std::cout << "\nError, input file temperature table was not succesfully loaded.";
            }

            if (resSnowpacks > 0)
            {
                std::cout << "\nError, input file snowpacks table was not succesfully loaded.";
            }

            if (resRaingages > 0)
            {
                std::cout << "\nError, input file raingages table was not succesfully loaded.";
            }

            if (resInflows > 0)
            {
                std::cout << "\nError, inflows table was not succesfully loaded.";
            }

            if (resTimeseries > 0)
            {
                std::cout << "\nError, time series table was not succesfully loaded.";
            }

            if (resPumps > 0)
            {
                std::cout << "\nError, pump table was not succesfully loaded.";
            }

            if (resPumpCurves > 0)
            {
                std::cout << "\nError, pump curve table was not succesfully loaded.";
            }

            if (resStorages > 0)
            {
                std::cout << "\nError, storage table was not succesfully loaded.";
            }

            if (resDwf > 0)
            {
                std::cout << "\nError, dwf table was not succesfully loaded.";
            }

            if (resPatterns > 0)
            {
                std::cout << "\nError, patterns table was not succesfully loaded.";
            }

            if (resLosses > 0)
            {
                std::cout << "\nError, losses table was not succesfully loaded.";
            }

            if (resXsections > 0)
            {
                std::cout << "\nError, xsection table was not succesfully loaded.";
            }

            if (resReport > 0)
            {
                std::cout << "\nError, input file report table was not succesfully loaded.";
            }

            if (resSymbols > 0)
            {
                std::cout << "\nError, input file symbols table was not succesfully loaded.";
            }

            return 1;
        }
    }
    else
    {
        std::cout << "\nError, number of command line arguments should be " << regGridParams << " or " << adapGridParams << " or " << adapGridParamsOld << " or " << adapGridParamsLID;

        return 1;
    }

    return 0;
}
