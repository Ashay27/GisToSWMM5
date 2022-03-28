#include "Grid.h"

Grid::Grid()
{
    clear();
}

Grid::~Grid()
{
    clear();
}

void Grid::clear()
{
    cells = 0;
    nCols = 0;
    nRows = 0;
    cellSize = 0.0;
    xllCorner = 0.0;
    yllCorner = 0.0;
    urcorner[0] = urcorner[1] = 0.0;
    llcorner[0] = llcorner[1] = 0.0;
    gridType = -1;
    delete [] cells;
    cells = 0;
}

// Create normal grid by treating each grid cell as a subcatchment
int Grid::create(int gridTypeNew, Raster &landuseRaster, Raster &demRaster)
{
    if (gridTypeNew == 0)
    {
        clear();
        gridType = gridTypeNew; // gridType is set to -1 in clear() method
        nCols = landuseRaster.nCols;
        nRows = landuseRaster.nRows;
        cells = new Cell[ nCols * nRows ];

        return 0;
    }
    else
    {
        return 1;
    }
}

// (Legacy mode of creating N*(2^a) x N*(2^a) grid where a is the number of subdivisions)
// Create adaptive grid by subdividing cells with the same landuse
int Grid::create(int gridTypeNew, double cellSizeMax, int maxSubdivisions, Raster &landuseRaster, Raster &demRaster)
{
    if (gridTypeNew == 1 && cellSizeMax > 0.0 && maxSubdivisions >= 0)
    {
        clear(); // gridType is set to -1 in clear() method
        gridType = gridTypeNew;

        // Create cells.
        int numberOfCellsX;
        int numberOfCellsY;
        //double cellSizeMax = 16.0;
        numberOfCellsX = (int)((double)landuseRaster.nCols * landuseRaster.cellSize / cellSizeMax + 0.5);
        numberOfCellsY = (int)((double)landuseRaster.nRows * landuseRaster.cellSize / cellSizeMax + 0.5);
        double lowerLeftCornerX =  landuseRaster.xllCorner;
        double lowerLeftCornerY =  landuseRaster.yllCorner;
        int subdivisions = 0;
        //int maxSubdivisions = 3; // 16 -> 8 -> 4 -> 2
        std::vector<Cell> cellsAdaptive;

        for (int i = 0; i < numberOfCellsX * numberOfCellsY; i++)
        {
            subdivideCell(cellSizeMax, lowerLeftCornerX, lowerLeftCornerY, i, numberOfCellsX,
                          landuseRaster, subdivisions, maxSubdivisions, cellsAdaptive);
        }

        // Transfer adaptive cell data to the pointer array.
        nCols = (int)cellsAdaptive.size();
        nRows = 1;
        cells = new Cell[nRows * nCols];

        for (int i = 0; i < nRows * nCols; i++ )
        {
            cells[i].cellSize = cellsAdaptive[i].cellSize;
            cells[i].area = cells[i].cellSize * cells[i].cellSize; // Temp. place
            cells[i].centerCoordX = cellsAdaptive[i].centerCoordX;
            cells[i].centerCoordY = cellsAdaptive[i].centerCoordY;
            cells[i].landuse = cellsAdaptive[i].landuse;
        }

        return 0;
    }
    else
    {
        std::cout << "\n-> Error, when using the adaptive discretization algorithm ";
        std::cout << "check that the maximum cell width is above zero and maximum number of subdivisions ";
        std::cout << "is equal to or above zero.";

        return 1;
    }
}

void Grid::subdivideCell(double cellSize, double llcornerx, double llcornery, int index, int numberOfCells,
                         Raster &landuseRaster, int subdivisions, int maxSubdivisions, std::vector<Cell> &cellsAdaptive)
{
    int cellIndexX = index % numberOfCells;
    int cellIndexY = index / numberOfCells;
    llcornerx = llcornerx + cellIndexX * cellSize;
    llcornery = llcornery + cellIndexY * cellSize;

    if (isSingleLanduse(llcornerx, llcornery, cellSize, landuseRaster) || subdivisions >= maxSubdivisions)
    {
        Cell cell;
        cell.cellSize = cellSize;
        cell.centerCoordX = llcornerx + cell.cellSize * 0.5;
        cell.centerCoordY = llcornery + cell.cellSize * 0.5;
        std::string landuse = landuseRaster.getPixelValue(cell.centerCoordX, cell.centerCoordY);
        cell.landuse = atoi(landuse.c_str());
        cellsAdaptive.push_back(cell);
    }
    else
    {
        subdivisions++;

        for (int i = 0; i < 4; i++)
        {
            subdivideCell(0.5 * cellSize, llcornerx, llcornery, i, 2,
                          landuseRaster, subdivisions, maxSubdivisions, cellsAdaptive);
        }
    }
}

void Grid::computeHistogram(std::vector<std::string> &catLabels, std::vector<int> &catCount,
                            double lowerLeftCornerX, double lowerLeftCornerY, double cellSizeLoc, Raster &landuseRaster)
{
    catLabels.clear();
    catCount.clear();
    int indexMinX = (int)((lowerLeftCornerX - landuseRaster.xllCorner) / landuseRaster.cellSize + 0.5);
    int indexMaxX = (int)((lowerLeftCornerX + cellSizeLoc - landuseRaster.xllCorner) / landuseRaster.cellSize + 0.5);
    int indexMinY = (int)((lowerLeftCornerY - landuseRaster.yllCorner) / landuseRaster.cellSize + 0.5);
    int indexMaxY = (int)((lowerLeftCornerY + cellSizeLoc - landuseRaster.yllCorner) / landuseRaster.cellSize + 0.5);

    if (indexMinX < 0)
    {
        indexMinX = 0;
    }
    if (indexMinY < 0)
    {
        indexMinY = 0;
    }
    if (indexMaxX > landuseRaster.nCols)
    {
        indexMaxX = landuseRaster.nCols;
    }
    if (indexMaxY > landuseRaster.nRows)
    {
        indexMaxY = landuseRaster.nRows;
    }

    for (int j = indexMinY; j < indexMaxY; j++)
    {
        for (int i = indexMinX; i < indexMaxX; i++)
        {
            double coordinateX = landuseRaster.xllCorner + i * landuseRaster.cellSize + landuseRaster.cellSize * 0.5;
            double coordinateY = landuseRaster.yllCorner + j * landuseRaster.cellSize + landuseRaster.cellSize * 0.5;
            std::string landuse = landuseRaster.getPixelValue(coordinateX, coordinateY);
            bool landuseCatFound = false;

            for (int k = 0; k < (int)catLabels.size(); k++)
            {
                if (catLabels[k] == landuse)
                {
                    catCount[k]++;
                    landuseCatFound = true;
                }
            }

            if (landuseCatFound == false)
            {
                catLabels.push_back(landuse);
                catCount.push_back(1);
            }
        }
    }
}

bool Grid::isSingleLanduse(double lowerLeftCornerX, double lowerLeftCornerY, double cellSizeLoc, Raster &landuseRaster)
{
    std::vector<std::string> catLabels;
    std::vector<int> catCount;
    computeHistogram(catLabels, catCount, lowerLeftCornerX, lowerLeftCornerY, cellSizeLoc, landuseRaster);

    int numberOfCategories = 0;

    for (int i = 0; i < (int)catCount.size(); i++)
    {
        if (catCount[i] > 0)
        {
            numberOfCategories++;
        }
    }

    if (numberOfCategories == 1)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void Grid::setCellNames()
{
    for (int i = 0; i < nCols * nRows; i++)
    {
        std::stringstream sstream("");
        sstream << "s" << i;
        cells[i].name = sstream.str();
    }
}

void Grid::computeCellDimensions(double cellSizeNew)
{
    cellSize = cellSizeNew;

    for (int i = 0; i < nCols * nRows; i++)
    {
        cells[i].area = cellSize * cellSize;
        cells[i].cellSize = cellSize;
    }
}

void Grid::computeCellCenterpoints(double xllCornerNew, double yllCornerNew)
{
    xllCorner = xllCornerNew; // is this needed anymore here?
    yllCorner = yllCornerNew; // is this needed anymore here?

    for (int j = 0; j < nRows; j++)
    {
        for (int i = 0; i < nCols; i++)
        {
            double centerCoordX = (double)i * cellSize + 0.5 * cellSize + xllCorner;
            double centerCoordY = (double)j * cellSize + 0.5 * cellSize + yllCorner;
            cells[ i + j * nCols ].centerCoordX = centerCoordX;
            cells[ i + j * nCols ].centerCoordY = centerCoordY;
        }
    }
}

void Grid::setCellElevations(Raster &demRaster)
{
    // Regular grid.
    if (gridType == 0)
    {
        for (int i = 0; i < nCols * nRows; i++)
        {
            std::string elevation = demRaster.getPixelValue( cells[i].centerCoordX, cells[i].centerCoordY );
            cells[i].elevation = atof(elevation.c_str());
            cells[i].elevNoData = std::stod(demRaster.noDataValue);
        }
    }
    // (Legacy mode of creating N*(2^a) x N*(2^a) grid where a is the number of subdivisions)
    // Adaptive grid.
    else if (gridType == 1)
    {
        for (int i = 0; i < nRows * nCols; i++ )
        {
            std::vector<std::string> catLabels;
            std::vector<int> catCount;
            computeHistogram(catLabels, catCount, cells[i].centerCoordX - 0.5 * cells[i].cellSize,
                             cells[i].centerCoordY - 0.5 * cells[i].cellSize, cells[i].cellSize, demRaster);
            double elevation = 0;
            int count = 0;

            for (int j = 0; j < (int)catLabels.size(); j++)
            {
                elevation += atof( catLabels[j].c_str() ) * (double)catCount[j];
                count += catCount[j];
            }

            if (count > 0)
            {
                elevation /= (double)count;
                cells[i].elevation = elevation;
            }
            else
            {
                std::cout << "\n-> Error, no elevation data was found for the cell " << i;
            }
        }
    }
}


void Grid::setCellFlowdirs(Raster &flowdirRaster)
{
    for (int i = 0; i < nCols * nRows; i++)
    {
        std::string flowdir = flowdirRaster.getPixelValue( cells[i].centerCoordX, cells[i].centerCoordY );
        cells[i].flowdir = atoi(flowdir.c_str());
    }
}

void Grid::setCellLanduse(Raster &landuseRaster)
{
    for (int j = 0; j < nRows; j++)
    {
        for (int i = 0; i < nCols; i++)
        {
            std::string landuseType = landuseRaster.getPixelValue( i, nRows - 1 - j );
            cells[ i + j * nCols ].landuse = atoi(landuseType.c_str());
        }
    }
}

void Grid::setCellLidType(Raster &lidRaster)
{
    for (int j = 0; j < nRows; j++)
    {
        for (int i = 0; i < nCols; i++)
        {
            int lidRasterValue = atoi(lidRaster.getPixelValue( i, nRows - 1 - j ).c_str());
            std::string lidTypeName = "NONE";
            int lidTypeId = lidRasterValue/100;
            std::string lidId; //= lidRasterValue%100;
            if (lidRasterValue != 0 && lidRasterValue%100 == 0){
                std::cout << "Invalid lid id in the raster file. ID should for each Lid type should be between 01 and 99";
            }else if (lidRasterValue%100 < 10){
                lidId = "0" + std::to_string(lidRasterValue%100);
            }else{
                 lidId = std::to_string(lidRasterValue%100);
            }

            if (lidTypeId == 100){
                    lidTypeName = "RB" + lidId;
            }else if (lidTypeId == 200){
                    lidTypeName = "GR" + lidId;
            }else if (lidTypeId == 300){
                    lidTypeName = "PP" + lidId;
            }else if (lidTypeId == 400){
                    lidTypeName = "RG" + lidId;
            }else if (lidTypeId == 500){
                    lidTypeName = "IT" + lidId;
            }else if (lidTypeId == 600){
                    lidTypeName = "BR" + lidId;
            }else if (lidTypeId == 700){
                    lidTypeName = "VS" + lidId;
            }else if (lidTypeId == 800){
                    lidTypeName = "RD" + lidId;
            }

            cells[ i + j * nCols ].LIDType = lidTypeName;
            if (lidTypeName != "NONE"){
                std::cout << "\n Lid type for cell at (" << i << "," << j << ") is:" << lidTypeName;
            }
        }
    }
}

void Grid::computeGridExtents()
{
    if (nCols * nRows > 0)
    {
        urcorner[0] = llcorner[0] = cells[0].centerCoordX;
        urcorner[1] = llcorner[1] = cells[0].centerCoordY;
    }

    for (int i = 0; i < nCols * nRows; i++)
    {
        if (cells[i].landuse != LANDUSE_NONE)
        {
            if (cells[i].centerCoordX < llcorner[0])
            {
                llcorner[0] = cells[i].centerCoordX;
            }

            if (cells[i].centerCoordX > urcorner[0])
            {
                urcorner[0] = cells[i].centerCoordX;
            }

            if (cells[i].centerCoordY < llcorner[1])
            {
                llcorner[1] = cells[i].centerCoordY;
            }

            if (cells[i].centerCoordY > urcorner[1])
            {
                urcorner[1] = cells[i].centerCoordY;
            }
        }
    }
}

void Grid::setSubcatchmentProperties(Table &catchPropTable)
{
    for (int i = 0; i < nCols * nRows; i++)
    {
        for (int j = 1; j < catchPropTable.nRows; j++)
        {
            if (cells[i].landuse == atoi(catchPropTable.data[j * catchPropTable.nCols].c_str()))
            {
                cells[i].raingage = catchPropTable.data[j * catchPropTable.nCols + 7];
                cells[i].imperv = catchPropTable.data[j * catchPropTable.nCols + 1].c_str();
                cells[i].N_Imperv = catchPropTable.data[j * catchPropTable.nCols + 3].c_str();
                cells[i].N_Perv = catchPropTable.data[j * catchPropTable.nCols + 5].c_str();
                cells[i].S_Imperv = catchPropTable.data[j * catchPropTable.nCols + 2].c_str();
                cells[i].S_Perv = catchPropTable.data[j * catchPropTable.nCols + 4].c_str();
                cells[i].PctZero = catchPropTable.data[j * catchPropTable.nCols + 6].c_str();
                cells[i].Suction = catchPropTable.data[j * catchPropTable.nCols + 10].c_str();
                cells[i].HydCon = catchPropTable.data[j * catchPropTable.nCols + 8].c_str();
                cells[i].IMDmax = catchPropTable.data[j * catchPropTable.nCols + 9].c_str();
                cells[i].snowPack = catchPropTable.data[j * catchPropTable.nCols + 11];
                if (catchPropTable.nCols > 12)
                    cells[i].tag = catchPropTable.data[j * catchPropTable.nCols + 12].c_str();

                break;
            }
        }
    }
}

void Grid::findCellNeighbours()
{
    // Regular grid.
    if (gridType == 0)
    {
        for (int j = 0; j < nRows; j++)
        {
            for (int i = 0; i < nCols; i++)
            {
                int currentCell = i + j * nCols;
                cells[currentCell].neighCellIndices.assign(8, -1);
                cells[currentCell].distanceToNeighbours.assign(8, 0.0 );

                // Change routing of cells so that flow directions go from 0-7
                // with 0 pointing Northeast and numbers increasing
                // in counterclockwise direction
                if (j < nRows - 1)
                {
                    cells[currentCell].neighCellIndices[1] = currentCell + nCols;
                }

                if (j < nRows - 1 && i > 0)
                {
                    cells[currentCell].neighCellIndices[2] = currentCell + nCols - 1;
                }

                if (i > 0)
                {
                    cells[currentCell].neighCellIndices[3] = currentCell - 1;
                }

                if (j > 0 && i > 0)
                {
                    cells[currentCell].neighCellIndices[4] = currentCell - nCols - 1;
                }

                if (j > 0)
                {
                    cells[currentCell].neighCellIndices[5] = currentCell - nCols;
                }

                if (i < nCols - 1 && j > 0)
                {
                    cells[currentCell].neighCellIndices[6] = currentCell + 1 - nCols;
                }

                if (i < nCols - 1)
                {
                    cells[currentCell].neighCellIndices[7] =  currentCell + 1;
                }

                if (i < nCols - 1 && j < nRows - 1)
                {
                    cells[currentCell].neighCellIndices[0] = currentCell + nCols + 1;
                }
            }
        }
    }
    // (Legacy mode of creating N*(2^a) x N*(2^a) grid where a is the number of subdivisions)
    // Adaptive grid. This section could be optimized.
    else if (gridType == 1)
    {
        double thresholdDistance = 0.5; // magic number !

        for (int i = 0; i < nRows * nCols; i++ )
        {
            std::vector<int> neighbours;

            for (int j = 0; j < nRows * nCols; j++ )
            {
                if
                (
                    // Corner points.
                    (
                        (
                            fabs((cells[i].centerCoordX + 0.5 * cells[i].cellSize) - (cells[j].centerCoordX - 0.5 * cells[j].cellSize)) < thresholdDistance
                            ||
                            fabs((cells[i].centerCoordX - 0.5 * cells[i].cellSize) - (cells[j].centerCoordX + 0.5 * cells[j].cellSize)) < thresholdDistance
                        )
                        &&
                        (
                            fabs((cells[i].centerCoordY + 0.5 * cells[i].cellSize) - (cells[j].centerCoordY - 0.5 * cells[j].cellSize)) < thresholdDistance
                            ||
                            fabs((cells[i].centerCoordY - 0.5 * cells[i].cellSize) - (cells[j].centerCoordY + 0.5 * cells[j].cellSize)) < thresholdDistance
                        )
                    )
                    ||
                    // Side points.
                    (
                        (
                            (
                                fabs((cells[i].centerCoordX + 0.5 * cells[i].cellSize) - (cells[j].centerCoordX - 0.5 * cells[j].cellSize)) < thresholdDistance
                                ||
                                fabs((cells[i].centerCoordX - 0.5 * cells[i].cellSize) - (cells[j].centerCoordX + 0.5 * cells[j].cellSize)) < thresholdDistance
                            )
                            &&
                            (
                                (cells[i].centerCoordY + 0.5 * cells[i].cellSize >= cells[j].centerCoordY && cells[i].centerCoordY - 0.5 * cells[i].cellSize <= cells[j].centerCoordY)
                                ||
                                (cells[j].centerCoordY + 0.5 * cells[j].cellSize >= cells[i].centerCoordY && cells[j].centerCoordY - 0.5 * cells[j].cellSize <= cells[i].centerCoordY)
                            )
                        )

                        ||

                        (
                            (
                                fabs((cells[i].centerCoordY + 0.5 * cells[i].cellSize) - (cells[j].centerCoordY - 0.5 * cells[j].cellSize)) < thresholdDistance
                                ||
                                fabs((cells[i].centerCoordY - 0.5 * cells[i].cellSize) - (cells[j].centerCoordY + 0.5 * cells[j].cellSize)) < thresholdDistance
                            )
                            &&
                            (
                                (cells[i].centerCoordX + 0.5 * cells[i].cellSize >= cells[j].centerCoordX && cells[i].centerCoordX - 0.5 * cells[i].cellSize <= cells[j].centerCoordX)
                                ||
                                (cells[j].centerCoordX + 0.5 * cells[j].cellSize >= cells[i].centerCoordX && cells[j].centerCoordX - 0.5 * cells[j].cellSize <= cells[i].centerCoordX)
                            )
                        )
                    )
                )
                {
                    if (i != j && cells[j].landuse != LANDUSE_NONE)
                    {
                        neighbours.push_back(j);
                    }
                }
            }

            cells[i].neighCellIndices = neighbours;
            cells[i].distanceToNeighbours.assign( (int)neighbours.size(), 0.0 );
        }
    }
}

// (Legacy mode of creating N*(2^a) x N*(2^a) grid where a is the number of subdivisions)
// Route cells in old adaptive grid
void Grid::routeCells()
{
    for (int i = 0; i < nCols * nRows; i++)
    {
        double slope = 0.0;
        double maxSlope = -1.0;
        double elevation = cells[i].elevation;
        int neighCellIndex = -1;
        cells[i].flowWidth = cells[i].cellSize;
        int flowDirection = -1;

        // Compute distances between neighbour cells.
        for (int j = 0; j < (int)cells[i].neighCellIndices.size(); j++)
        {
            if (cells[i].neighCellIndices[j] != -1)
            {
                double distance = sqrt( (cells[i].centerCoordX - cells[ cells[i].neighCellIndices[j] ].centerCoordX)
                                        * (cells[i].centerCoordX - cells[ cells[i].neighCellIndices[j] ].centerCoordX)
                                        + (cells[i].centerCoordY - cells[ cells[i].neighCellIndices[j] ].centerCoordY)
                                        * (cells[i].centerCoordY - cells[ cells[i].neighCellIndices[j] ].centerCoordY) );

                if (distance > 0.0)
                {
                    cells[i].distanceToNeighbours[j] = distance;
                }
                else
                {
                    std::cout << "\n-> Error, distance between cell " << i << " and cell " << j << " is zero or below zero";
                }
            }
        }

        // Find outlet.
        for (int k = 0; k < (int)cells[i].neighCellIndices.size(); k++)
        {
            if (cells[i].neighCellIndices[k] != -1 && cells[ cells[i].neighCellIndices[k] ].landuse >= BUILT_AREA)
            {
                double distance = cells[i].distanceToNeighbours[k];
                if (distance > 0.0)
                    slope = fabs((cells[i].elevation - cells[ cells[i].neighCellIndices[k] ].elevation) / distance);

                if (cells[ cells[i].neighCellIndices[k] ].elevation < elevation && slope > maxSlope)
                {
                    neighCellIndex = cells[i].neighCellIndices[k];
                    flowDirection = k;
                    maxSlope = slope;
                }
            }
        }

        // Save the outlet name and compute flow width.
        if (neighCellIndex != -1)
        {
            cells[i].outlet = cells[neighCellIndex].name;
            cells[i].outletCoordX = cells[neighCellIndex].centerCoordX;
            cells[i].outletCoordY = cells[neighCellIndex].centerCoordY;

            if (cells[i].distanceToNeighbours[flowDirection] > 0.0)
            {
                cells[i].flowWidth = cells[i].area / cells[i].distanceToNeighbours[flowDirection];
            }
        }
    }
}

// Use flow direction raster to route cells when using NxN or adap grid
void Grid::routeCellsReg()
{
    for (int i = 0; i < nCols * nRows; i++)
    {
        int neighCellIndex = -1;
        cells[i].flowWidth = cells[i].cellSize;
        int flowDirection = -1;

        // Compute distances between neighbour cells.
        for (int j = 0; j < (int)cells[i].neighCellIndices.size(); j++)
        {
            if (cells[i].neighCellIndices[j] != -1)
            {
                double distance = sqrt( (cells[i].centerCoordX - cells[ cells[i].neighCellIndices[j] ].centerCoordX)
                                        * (cells[i].centerCoordX - cells[ cells[i].neighCellIndices[j] ].centerCoordX)
                                        + (cells[i].centerCoordY - cells[ cells[i].neighCellIndices[j] ].centerCoordY)
                                        * (cells[i].centerCoordY - cells[ cells[i].neighCellIndices[j] ].centerCoordY) );

                if (distance > 0.0)
                {
                    cells[i].distanceToNeighbours[j] = distance;
                }
                else
                {
                    std::cout << "\n-> Error, distance between cell " << i << " and cell " << j << " is zero or below zero";
                }
            }
        }

        // Find outlet.
        flowDirection = cells[i].flowdir - 1;
        if (flowDirection != -1 &&
            cells[i].neighCellIndices[flowDirection] != -1 &&
            cells[ cells[i].neighCellIndices[flowDirection] ].landuse >= BUILT_AREA)
        {
            neighCellIndex = cells[i].neighCellIndices[flowDirection];
        }

        // Save the outlet name and compute flow width.
        if (neighCellIndex != -1)
        {
            cells[i].outletID = neighCellIndex;
            cells[i].outlet = cells[neighCellIndex].name;
            cells[i].outletCoordX = cells[neighCellIndex].centerCoordX;
            cells[i].outletCoordY = cells[neighCellIndex].centerCoordY;

            if (cells[i].distanceToNeighbours[flowDirection]  > 0.0)
            {
                cells[i].flowWidth = cells[i].area / cells[i].distanceToNeighbours[flowDirection] ;
            }
        }
    }
}

void Grid::computeCellSlopes()
{
    for (int i = 0; i < nCols * nRows; i++)
    {
        double slope = 0.0;
        int slopeCount = 0;

        for (int k = 0; k < (int)cells[i].neighCellIndices.size(); k++)
        {
            if (cells[i].neighCellIndices[k] != -1 && cells[ cells[i].neighCellIndices[k] ].landuse >= BUILT_AREA)
            {
                double distance = cells[i].distanceToNeighbours[k];

                if (distance > 0.0 && cells[i].elevation > cells[i].elevNoData && cells[ cells[i].neighCellIndices[k] ].elevation > cells[i].elevNoData)
                {
                    slope += fabs((cells[i].elevation - cells[ cells[i].neighCellIndices[k] ].elevation) / distance);
                    slopeCount++;
                }
                else
                {
                    std::cout << "\n-> Error, no data to calculate slope between cells " << i << " and " << cells[i].neighCellIndices[k] << ".";
                }
            }
        }

        // Save the average slope.
        if (slopeCount > 0)
        {
            cells[i].slope = slope / (double)slopeCount;
        }

        // Fix slope for the rooftops.
        if (cells[i].landuse >=ROOF_CONNECTED && cells[i].landuse < BUILT_AREA)
        {
            cells[i].slope = 0.05;
        }

        // Set minimum slope
        if (cells[i].slope < 0.001)
        {
            cells[i].slope = 0.001;
        }
    }
}


// This relies (implicitly) on the cell connected to junction being a local
// pit in the DEM, if it is not (and e.g. the next cell is) most of the water flows past
// the junction...
// 23 Oct 2017 UPDATE: Using 3x3 area surrounding junctions as the collecting area for water
void Grid::connectCellsToJunctions(Table &juncTable)
{
    for (int k = 1; k < juncTable.nRows; k++) // pass the header line
    {
        double juncPosX = atof( juncTable.data[k * juncTable.nCols].c_str() );
        double juncPosY = atof( juncTable.data[k * juncTable.nCols + 1].c_str() );
        int isOpen = atoi( juncTable.data[k * juncTable.nCols + 6].c_str() );

        // NxN or adap grid
        if (gridType == 0)
        {
            if (juncPosX >= xllCorner && juncPosX < xllCorner + nCols * cellSize
                    && juncPosY >= yllCorner && juncPosY < yllCorner + nRows * cellSize && isOpen == 1) // pass closed junctions
            {
                if (nCols > 0 && nRows > 0 && cellSize > 0.0)
                {
                    int col = (int)((juncPosX - xllCorner) / cellSize);
                    int row = (int)((juncPosY - yllCorner) / cellSize);

                    if (col + row * nCols >= 0 && col + row * nCols < nCols * nRows)
                    {
                        cells[ col + row * nCols ].outletID =  col + row * nCols;   // Set outletID to the outletID of junction cell
                        cells[ col + row * nCols ].outlet = juncTable.data[k * juncTable.nCols + 2];
                        cells[ col + row * nCols ].flowWidth = cells[ col + row * nCols ].cellSize;
                        cells[ col + row * nCols ].outletCoordX = stod(juncTable.data[k * juncTable.nCols + 0]);
                        cells[ col + row * nCols ].outletCoordY = stod(juncTable.data[k * juncTable.nCols + 1]);

                        // Use the 3x3 cell area surrounding each stormwater inlet as the collecting area for the inlet.
                        // This should help with misplacement of stormwater inlets when not just the cell with the inlet is used
                        // to collect water
                        for (int i = 0; i < (int)cells[ col + row * nCols ].neighCellIndices.size(); i++)
                        {
                            if (cells[ col + row * nCols ].neighCellIndices[i] != -1
                                    && cells[ cells[ col + row * nCols ].neighCellIndices[i] ].landuse >= BUILT_AREA)
                            {
                                cells[ cells[ col + row * nCols ].neighCellIndices[i] ].outletID =  col + row * nCols;   // Set outletID to the outletID of junction cell
                                cells[ cells[ col + row * nCols ].neighCellIndices[i] ].outlet = juncTable.data[k * juncTable.nCols + 2];
                                cells[ cells[ col + row * nCols ].neighCellIndices[i] ].flowWidth =  cells[ cells[ col + row * nCols ].neighCellIndices[i] ].cellSize; // This is unnecessary?
                                cells[ cells[ col + row * nCols ].neighCellIndices[i] ].outletCoordX = stod(juncTable.data[k * juncTable.nCols + 0]);
                                cells[ cells[ col + row * nCols ].neighCellIndices[i] ].outletCoordY = stod(juncTable.data[k * juncTable.nCols + 1]);
                            }
                        }
                    }
                }
            }
        }
        // (Legacy mode)
        // N*(2^a) x N*(2^a) grid where a is the number of subdivisions
        else if (gridType == 1)
        {
            for (int i = 0; i < nCols * nRows; i++)
            {
                if (cells[i].centerCoordX - 0.5 * cells[i].cellSize <= juncPosX
                        && cells[i].centerCoordX + 0.5 * cells[i].cellSize > juncPosX
                        && cells[i].centerCoordY - 0.5 * cells[i].cellSize <= juncPosY
                        && cells[i].centerCoordY + 0.5 * cells[i].cellSize > juncPosY
                        && isOpen == 1      // pass closed junctions
                   )
                {
                    cells[i].outlet = juncTable.data[k * juncTable.nCols + 2];
                    cells[i].flowWidth = cells[i].cellSize;
                    cells[i].outletCoordX = stod(juncTable.data[k * juncTable.nCols + 0]);
                    cells[i].outletCoordY = stod(juncTable.data[k * juncTable.nCols + 1]);
                }
            }
        }
    }
}

void Grid::routePavedPitAndRooftopCells(Table &juncTable)
{
    // Route paved pit cells and connected rooftop cells to junctions.
    double distanceMaxSquared = (urcorner[0] - llcorner[0]) * (urcorner[0] - llcorner[0]) +
                                (urcorner[1] - llcorner[1]) * (urcorner[1] - llcorner[1]);

    for (int i = 0; i < nCols * nRows; i++)
    {
        int flowDirection = -1;
        flowDirection = cells[i].flowdir - 1;

        // Route connected roofs to nearest junction
        if (cells[i].landuse >= ROOF_CONNECTED && cells[i].landuse < ROOF_UNCONNECTED) // should this be here?
        {
            double distanceSquared = distanceMaxSquared;

            for (int j = 1; j < juncTable.nRows; j++) // pass the header line
            {
                double juncPosX = atof( juncTable.data[j * juncTable.nCols].c_str() );
                double juncPosY = atof( juncTable.data[j * juncTable.nCols + 1].c_str() );
                double dx = juncPosX - cells[i].centerCoordX;
                double dy = juncPosY - cells[i].centerCoordY;
                int isroutable = atoi( juncTable.data[j * juncTable.nCols + 10].c_str() );

                int col = (int)((juncPosX - xllCorner) / cellSize);
                int row = (int)((juncPosY - yllCorner) / cellSize);

                if (dx * dx + dy * dy < distanceSquared && isroutable == 1)
                {
                    distanceSquared = dx * dx + dy * dy;
                    cells[i].outletID = col + row * nCols;   // Set outletID to the outletID of junction cell
                    cells[i].outlet = juncTable.data[j * juncTable.nCols + 2];
                    cells[i].flowWidth = cells[i].cellSize; //Modified 20160909, compute by area / sqrt (distanceSquared) ?
                    cells[i].outletCoordX = stod(juncTable.data[j * juncTable.nCols + 0]);
                    cells[i].outletCoordY = stod(juncTable.data[j * juncTable.nCols + 1]);
                }
            }

            // Mark local pits where water is routed forcefully
            cells[i].isSink = 2;
        }
        // Route unconnected rooftop cells to the nearest non-roof cell
        else if (cells[i].landuse >= ROOF_UNCONNECTED && cells[i].landuse < BUILT_AREA)
        {
            double distanceSquared = distanceMaxSquared;

            for (int j = 0; j < nCols * nRows; j++)
            {
                if (cells[j].landuse >= BUILT_AREA)
                {
                    double dx = cells[i].centerCoordX - cells[j].centerCoordX;
                    double dy = cells[i].centerCoordY - cells[j].centerCoordY;

                    if (dx * dx + dy * dy < distanceSquared)
                    {
                        distanceSquared = dx * dx + dy * dy;
                        cells[i].outletID = j;
                        cells[i].outlet = cells[j].name;
                        cells[i].flowWidth = cells[i].cellSize; //Modified 20160909, compute by area / sqrt (distanceSquared) ?
                        cells[i].outletCoordX = cells[j].centerCoordX;
                        cells[i].outletCoordY = cells[j].centerCoordY;
                    }
                }
            }
        }

        // Route isolated built and natural area to the nearest junction
        else if (flowDirection != -1 &&
            cells[i].landuse >= BUILT_AREA &&
            cells[i].neighCellIndices[flowDirection] != -1 &&
            //cells[ cells[i].neighCellIndices[flowDirection] ].landuse != LANDUSE_NONE &&
            cells[i].outlet == "*")
            //cells[ cells[i].neighCellIndices[flowDirection] ].landuse < BUILT_AREA)
        {
            if(cells[i].landuse < MIN_WATER_USE ||
            cells[i].landuse > MAX_WATER_USE){

            double distanceSquared = distanceMaxSquared;
            
            for (int j = 1; j < juncTable.nRows; j++) // pass the header line
            {
                double juncPosX = atof( juncTable.data[j * juncTable.nCols].c_str() );
                double juncPosY = atof( juncTable.data[j * juncTable.nCols + 1].c_str() );
                double dx = juncPosX - cells[i].centerCoordX;
                double dy = juncPosY - cells[i].centerCoordY;
                int isroutable = atoi( juncTable.data[j * juncTable.nCols + 10].c_str() );

                int col = (int)((juncPosX - xllCorner) / cellSize);
                int row = (int)((juncPosY - yllCorner) / cellSize);

                if (dx * dx + dy * dy < distanceSquared && isroutable == 1)
                {
                    distanceSquared = dx * dx + dy * dy;
                    cells[i].outletID = col + row * nCols;   // Set outletID to the outletID of junction cell
                    cells[i].outlet = juncTable.data[j * juncTable.nCols + 2];
                    cells[i].flowWidth = cells[i].cellSize; //Modified 20160909, compute by area / sqrt (distanceSquared) ?
                    cells[i].outletCoordX = stod(juncTable.data[j * juncTable.nCols + 0]);
                    cells[i].outletCoordY = stod(juncTable.data[j * juncTable.nCols + 1]);
                }
            }

            // Mark local pits where water is routed forcefully
            cells[i].isSink = 2;
            }
        }

        // Route pits in built areas to themselves, i.e. water is not routed downstream
        else if (cells[i].landuse >= BUILT_AREA && cells[i].landuse < NATURAL_AREA &&  cells[i].outlet ==  "*")
        {
            cells[i].outletID = i;
            cells[i].outlet = cells[i].name;
            cells[i].outletCoordX = cells[i].centerCoordX;
            cells[i].outletCoordY = cells[i].centerCoordY;

            // Mark local pits where water is not routed
            cells[i].isSink = 1;
        }
    }
}

// Route pits in natural areas to themselves, i.e. water is not routed downstream
void Grid::routePitCells()
{
    for (int i = 0; i < nCols * nRows; i++)
    {
        if (cells[i].outlet ==  "*" && cells[i].landuse >= NATURAL_AREA)
        {
            cells[i].outletID = i;
            cells[i].outlet = cells[i].name;

            // Set depression storage of pit cells in permeable areas to a very high value ...
            // ... to prevent loss of water from the system.
            cells[i].S_Imperv = "50000";
            cells[i].S_Perv = "50000";

            cells[i].outletCoordX = cells[i].centerCoordX;
            cells[i].outletCoordY = cells[i].centerCoordY;

            // Mark local pits where water is not routed
            cells[i].isSink = 1;
        }
    }
}

// Find cells routed to outlet and save routing and pipe information
std::vector<int> Grid::findRouted(Table &juncTable, std::string &path)
{
    // Set inletIDs contributing to each cell
    for (int i = 0; i < nCols*nRows; i++)
    {
        if ( cells[i].landuse >= ROOF_UNCONNECTED && i != cells[i].outletID )   // Connected roofs are treated separately
        {
            cells[cells[i].outletID].inletIDs.push_back(i);
        }
    }

    // Empty vector for final list of routed subcatchments ID's
    std::vector<int> final_IDs;

    // Go through junctions
    for (int i = 1; i < juncTable.nRows; i++)
    {
        double juncPosX = atof( juncTable.getData(i, 0).c_str() );
        double juncPosY = atof( juncTable.getData(i, 1).c_str() );
        int isOpen = atoi( juncTable.getData(i, 6).c_str() );

        // Add open cells with junctions to final list of subcatchments
        if (juncPosX >= xllCorner
                &&
                juncPosX < xllCorner + nCols * cellSize
                &&
                juncPosY >= yllCorner
                &&
                juncPosY < yllCorner + nRows * cellSize
                &&
                isOpen == 1) // pass closed junctions
        {
            if (nCols > 0 && nRows > 0 && cellSize > 0.0)
            {
                int col = (int)((juncPosX - xllCorner) / cellSize);
                int row = (int)((juncPosY - yllCorner) / cellSize);

                if (col + row * nCols >= 0 && col + row * nCols < nCols * nRows)
                {
                    std::vector<int> selection_IDs;
                    selection_IDs.reserve(nCols*nRows); // Is this necessary?

                    final_IDs.push_back(col + row * nCols);
                    selection_IDs.push_back(col + row * nCols);

                    // Iterate through cells with open jucntions and add cells as they flow towrads the opening
                    int counter = 0;
                    while (!selection_IDs.empty() && counter < nCols*nRows)
                    {
                        // Get all subcatchments routed into the current cell
                        std::vector<int> inSubs = cells[selection_IDs.front()].inletIDs;

                        // Go through the subcatchments routed into the current cell
                        for (auto it=inSubs.begin(); it != inSubs.end(); ++it)
                        {
                            final_IDs.push_back(*it);

                            if (!(std::find(selection_IDs.begin(), selection_IDs.end(), *it) != selection_IDs.end()))
                            {
                                // Add subcatchment to selected subactchments if it's not there yet
                                selection_IDs.push_back(*it);
                            }
                        }

                        // Remove the subcatchment from the list of selected ID's
                        selection_IDs.erase(selection_IDs.begin());
                        counter++;
                    }
                }
            }
        }
    }

    for (int i = 0; i < nCols*nRows; i++)
    {
        // Add connected roofs to final subcatchments
        if ( (cells[i].landuse >= ROOF_CONNECTED && cells[i].landuse < ROOF_UNCONNECTED) && (i != cells[i].outletID) )
        {
            final_IDs.push_back(i);
        }
    }

    // Create and save a WKT vector file of routed subcatchments
    saveSubcatchmentRouting(path, final_IDs);

    return final_IDs;
}

// Simplify computation grid based on common flow direction and landuse
void Grid::simplify(Table &juncTable, std::string &path)
{
    /*** NON-ROOF CELLS ***/
    // Ensure there are no inletIDs
    for (int i = 0; i < nCols*nRows; i++)
        cells[i].inletIDs.clear();

    // Set inletIDs contributing to each cell
    for (int i = 0; i < nCols*nRows; i++)
    {
        if ( cells[i].landuse >= BUILT_AREA && i != cells[i].outletID )
        {
            cells[cells[i].outletID].inletIDs.push_back(i);
        }
    }

    // Set the 3x3 cell area surrounding each stormwater inlet as the collecting area for the inlet.
    // This should help with misplacement of stormwater inlets when not just the cell with the inlet is used
    // to collect water
    for (int i = 1; i < juncTable.nRows; i++) // pass the header line
    {
        double juncPosX = atof( juncTable.data[i * juncTable.nCols].c_str() );
        double juncPosY = atof( juncTable.data[i * juncTable.nCols + 1].c_str() );
        int isOpen = atoi( juncTable.data[i * juncTable.nCols + 6].c_str() );

        if (juncPosX >= xllCorner && juncPosX < xllCorner + nCols * cellSize
                && juncPosY >= yllCorner && juncPosY < yllCorner + nRows * cellSize && isOpen == 1) // pass closed junctions
        {
            if (nCols > 0 && nRows > 0 && cellSize > 0.0)
            {
                int col = (int)((juncPosX - xllCorner) / cellSize);
                int row = (int)((juncPosY - yllCorner) / cellSize);

                if (col + row * nCols >= 0 && col + row * nCols < nCols * nRows)
                {
                    for (int j = 0; j < (int)cells[ col + row * nCols ].neighCellIndices.size(); j++)
                    {
                        if (cells[ col + row * nCols ].neighCellIndices[j] != -1
                                && cells[ cells[ col + row * nCols ].neighCellIndices[j] ].landuse >= BUILT_AREA)
                        {
                            cells[cells[ cells[ col + row * nCols ].neighCellIndices[j] ].outletID].inletIDs.push_back(cells[ col + row * nCols ].neighCellIndices[j]);
                        }
                    }
                }
            }
        }
    }

    // Empty vector for final list of routed cell ID's
    std::vector<int> final_IDs;

    std::vector<Cell> cellsAdaptive;
    std::unordered_map<int, std::vector<Cell>::size_type> adapID;  // A lookup table of subcatchmentID's in cellsAdaptive
    int subcatchmentID = -1;

    // Go through junctions
    for (int i = 1; i < juncTable.nRows; i++)
    {
        double juncPosX = atof( juncTable.getData(i, 0).c_str() );
        double juncPosY = atof( juncTable.getData(i, 1).c_str() );
        int isOpen = atoi( juncTable.getData(i, 6).c_str() );

        // Add cells with open junctions to final list of cells
        if (juncPosX >= xllCorner
                &&
                juncPosX < xllCorner + nCols * cellSize
                &&
                juncPosY >= yllCorner
                &&
                juncPosY < yllCorner + nRows * cellSize
                &&
                isOpen == 1) // pass closed junctions
        {
            if (nCols > 0 && nRows > 0 && cellSize > 0.0)
            {
                int col = (int)((juncPosX - xllCorner) / cellSize);
                int row = (int)((juncPosY - yllCorner) / cellSize);

                if (col + row * nCols >= 0 && col + row * nCols < nCols * nRows)
                {
                    std::vector<int> selection_IDs;
                    selection_IDs.reserve(nCols*nRows); // Is this necessary???

                    final_IDs.push_back(col + row * nCols);
                    selection_IDs.push_back(col + row * nCols);

                    // Update the subcatchmentID of current cell...
                    subcatchmentID++;
                    cells[col + row * nCols].subcatchmentID = subcatchmentID;

                    // ... create a new adaptive subcatchment...
                    Cell newCell;
                    newCell.cellSize = cellSize;
                    newCell.area += cells[col + row * nCols].area;
                    newCell.centerCoordX += cells[col + row * nCols].centerCoordX;
                    newCell.centerCoordY += cells[col + row * nCols].centerCoordY;
                    newCell.elevation += cells[col + row * nCols].elevation;
                    newCell.slope += cells[col + row * nCols].slope;
                    newCell.landuse = cells[col + row * nCols].landuse;
                    newCell.imperv = cells[col + row * nCols].imperv;
                    newCell.outlet = juncTable.getData(i, 2).c_str();
                    newCell.outletID = subcatchmentID;
                    newCell.outletCoordX = juncPosX;
                    newCell.outletCoordY = juncPosY;
                    newCell.subcatchmentID = subcatchmentID;
                    newCell.raingage = cells[col + row * nCols].raingage;
                    newCell.snowPack = cells[col + row * nCols].snowPack;
                    newCell.N_Imperv = cells[col + row * nCols].N_Imperv;
                    newCell.N_Perv = cells[col + row * nCols].N_Perv;
                    newCell.S_Imperv = cells[col + row * nCols].S_Imperv;
                    newCell.S_Perv = cells[col + row * nCols].S_Perv;
                    newCell.PctZero = cells[col + row * nCols].PctZero;
                    newCell.RouteTo = cells[col + row * nCols].RouteTo;
                    newCell.PctRouted = cells[col + row * nCols].PctRouted;
                    newCell.Suction = cells[col + row * nCols].Suction;
                    newCell.HydCon = cells[col + row * nCols].HydCon;
                    newCell.IMDmax = cells[col + row * nCols].IMDmax;
                    newCell.isSink = cells[col + row * nCols].isSink;
                    newCell.tag = cells[col + row * nCols].tag;
                    newCell.hasInlet = 1;
                    newCell.numElements++;
                    std::stringstream subcatchmentName("");
                    subcatchmentName << "s" << subcatchmentID + 1;
                    newCell.name = subcatchmentName.str();
                    cellsAdaptive.push_back(newCell);

                    // ... and update the lookup table of subcatchments and their indexes.
                    adapID[subcatchmentID] = cellsAdaptive.size()-1;

                    // Use the 3x3 cell area surrounding each stormwater inlet as the collecting area for the inlet.
                    // This should help with misplacement of stormwater inlets when not just the cell with the inlet is used
                    // to collect water
                    for (int j = 0; j < (int)cells[ col + row * nCols ].neighCellIndices.size(); j++)
                    {
                        if (cells[ col + row * nCols ].neighCellIndices[j] != -1
                        && cells[ cells[ col + row * nCols ].neighCellIndices[j] ].landuse >= BUILT_AREA
                        && cells[ cells[ col + row * nCols ].neighCellIndices[j] ].landuse != cells[ col + row * nCols ].landuse)   // Don't create new subcatchments if the landuse is the same as in inlet cell
                        {
                            final_IDs.push_back(cells[ col + row * nCols ].neighCellIndices[j]);
                            selection_IDs.push_back(cells[ col + row * nCols ].neighCellIndices[j]);

                            // Update the subcatchmentID of current cell...
                            subcatchmentID++;
                            cells[ cells[ col + row * nCols ].neighCellIndices[j]].subcatchmentID = subcatchmentID;

                            // ... create a new adaptive subcatchment...
                            Cell newCell;
                            newCell.cellSize = cellSize;
                            newCell.area += cells[ cells[ col + row * nCols ].neighCellIndices[j] ].area;
                            newCell.centerCoordX += cells[ cells[ col + row * nCols ].neighCellIndices[j] ].centerCoordX;
                            newCell.centerCoordY += cells[ cells[ col + row * nCols ].neighCellIndices[j] ].centerCoordY;
                            newCell.elevation += cells[ cells[ col + row * nCols ].neighCellIndices[j] ].elevation;
                            newCell.slope += cells[ cells[ col + row * nCols ].neighCellIndices[j] ].slope;
                            newCell.landuse = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].landuse;
                            newCell.imperv = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].imperv;
                            newCell.outlet = juncTable.getData(i, 2).c_str();
                            newCell.outletID = subcatchmentID;
                            newCell.outletCoordX = juncPosX;
                            newCell.outletCoordY = juncPosY;
                            newCell.subcatchmentID = subcatchmentID;
                            newCell.raingage = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].raingage;
                            newCell.snowPack = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].snowPack;
                            newCell.N_Imperv = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].N_Imperv;
                            newCell.N_Perv = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].N_Perv;
                            newCell.S_Imperv = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].S_Imperv;
                            newCell.S_Perv = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].S_Perv;
                            newCell.PctZero = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].PctZero;
                            newCell.RouteTo = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].RouteTo;
                            newCell.PctRouted = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].PctRouted;
                            newCell.Suction = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].Suction;
                            newCell.HydCon = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].HydCon;
                            newCell.IMDmax = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].IMDmax;
                            newCell.isSink = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].isSink;
                            newCell.tag = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].tag;
                            newCell.hasInlet = 1;
                            newCell.numElements++;
                            std::stringstream subcatchmentName("");
                            subcatchmentName << "s" << subcatchmentID + 1;
                            newCell.name = subcatchmentName.str();
                            cellsAdaptive.push_back(newCell);

                            // ... and update the lookup table of subcatchments and their indexes.
                            adapID[subcatchmentID] = cellsAdaptive.size()-1;
                        }
                    }

                    // Iterate through cells and add cells as they flow towards the inlet
                    int counter = 0;    // Failsafe to prevent infinite while loop
                    while (!selection_IDs.empty() && counter < nCols*nRows)
                    {
                        // Get all subcatchments routed into the current cell
                        std::vector<int> inSubs = cells[selection_IDs.front()].inletIDs;

                        // Go through the cells routed into the current cell
                        for (auto it=inSubs.begin(); it != inSubs.end(); ++it)
                        {
                            // Add cell to selected cells if it is not there or in final cells yet
                            if (!(std::find(selection_IDs.begin(), selection_IDs.end(), *it) != selection_IDs.end())
                                    &&
                                    !(std::find(final_IDs.begin(), final_IDs.end(), *it) != final_IDs.end()))
                            {
                                selection_IDs.push_back(*it);
                            }

                            // Add cell to final cells if it is not there yet
                            if (!(std::find(final_IDs.begin(), final_IDs.end(), *it) != final_IDs.end()))
                            {
                                final_IDs.push_back(*it);

                                // Add current cell to existing adaptive subctahment or create a new one if old doesn't exist
                                if (
                                    cells[cells[*it].outletID].subcatchmentID > -1   // adaptive subcatchment exists
                                    &&
                                    cells[*it].landuse == cells[cells[*it].outletID].landuse // cell has same landuse as the downstream subcatchment
                                )
                                {
                                    // Update subcatchmentID of current cell ...
                                    cells[*it].subcatchmentID = cells[cells[*it].outletID].subcatchmentID;

                                    // ... and add current cell to existing subcatchment
                                    int i = adapID[cells[cells[*it].outletID].subcatchmentID];
                                    cellsAdaptive[i].area += cells[*it].area;
                                    cellsAdaptive[i].centerCoordX += cells[*it].centerCoordX;
                                    cellsAdaptive[i].centerCoordY += cells[*it].centerCoordY;
                                    cellsAdaptive[i].elevation += cells[*it].elevation;
                                    cellsAdaptive[i].slope += cells[*it].slope;
                                    cellsAdaptive[i].numElements++;
                                }
                                else
                                {
                                    // Update the subcatchmentID of current cell...
                                    subcatchmentID++;
                                    cells[*it].subcatchmentID = subcatchmentID;

                                    // Get index of downstream subcatchment
                                    int i = adapID[cells[cells[*it].outletID].subcatchmentID];

                                    // ... and create a new adaptive subcatchment...
                                    Cell newCell;
                                    newCell.cellSize = cellSize;
                                    newCell.area += cells[*it].area;
                                    newCell.centerCoordX += cells[*it].centerCoordX;
                                    newCell.centerCoordY += cells[*it].centerCoordY;
                                    newCell.elevation += cells[*it].elevation;
                                    newCell.slope += cells[*it].slope;
                                    newCell.landuse = cells[*it].landuse;
                                    newCell.imperv = cells[*it].imperv;
                                    newCell.outlet = cellsAdaptive[i].name;
                                    newCell.outletID = cells[cells[*it].outletID].subcatchmentID;
                                    newCell.subcatchmentID = subcatchmentID;
                                    newCell.raingage = cells[*it].raingage;
                                    newCell.snowPack = cells[*it].snowPack;
                                    newCell.N_Imperv = cells[*it].N_Imperv;
                                    newCell.N_Perv = cells[*it].N_Perv;
                                    newCell.S_Imperv = cells[*it].S_Imperv;
                                    newCell.S_Perv = cells[*it].S_Perv;
                                    newCell.PctZero = cells[*it].PctZero;
                                    newCell.RouteTo = cells[*it].RouteTo;
                                    newCell.PctRouted = cells[*it].PctRouted;
                                    newCell.Suction = cells[*it].Suction;
                                    newCell.HydCon = cells[*it].HydCon;
                                    newCell.IMDmax = cells[*it].IMDmax;
                                    newCell.isSink = cells[*it].isSink;
                                    newCell.tag = cells[*it].tag;
                                    newCell.numElements++;
                                    std::stringstream subcatchmentName("");
                                    subcatchmentName << "s" << subcatchmentID + 1;
                                    newCell.name = subcatchmentName.str();
                                    cellsAdaptive.push_back(newCell);

                                    // ... and update the lookup table of subcatchments and their indexes.
                                    adapID[subcatchmentID] = cellsAdaptive.size()-1;
                                }
                            }
                        }
                        // Remove the cell ID from the list of selected ID's
                        selection_IDs.erase(selection_IDs.begin());
                        counter++;
                    }
                }
            }
        }
    }

    /*** UNCONNECTED ROOFS ***/
    // Ensure the are no earlier inletIDs
    for (int i = 0; i < (int)cellsAdaptive.size(); ++i)
        cellsAdaptive[i].inletIDs.clear();

    // Find outlets of unconnected roof cells
    for (int i = 0; i < nCols*nRows; i++)
    {
        if ( (cells[i].landuse >= ROOF_UNCONNECTED && cells[i].landuse < BUILT_AREA)
                &&
                (cells[cells[i].outletID].subcatchmentID > -1)  // Check that the roof is connected to routed cell
                &&
                (i != cells[i].outletID) )
        {
            // Get index of downstream subcatchment
            int j = adapID[cells[cells[i].outletID].subcatchmentID];
            cellsAdaptive[j].inletIDs.push_back(i);
        }
    }
    std::vector<int> roofOutlets;   // Temp vector of subcatchments where unconnnectd roofs are connected
    for (int i = 0; i < (int)cellsAdaptive.size(); ++i)
    {
        if (!cellsAdaptive[i].inletIDs.empty())
            roofOutlets.push_back(i);
    }

    // Combine unconnected roof cells with the same outlet into one subcatchment
    for (auto it=roofOutlets.begin(); it != roofOutlets.end(); ++it)
    {
        if (!cellsAdaptive[*it].inletIDs.empty())
        {
            // Get all roof cells routed into the current cell
            std::vector<int> roofCells = cellsAdaptive[*it].inletIDs;

            // Update the subcatchmentID of current cell...
            subcatchmentID++;
            cells[roofCells.front()].subcatchmentID = subcatchmentID;

            // ... and create a new adaptive subcatchment.
            Cell newCell;
            newCell.cellSize = cellSize;
            newCell.area += cells[roofCells.front()].area;
            newCell.centerCoordX += cells[roofCells.front()].centerCoordX;
            newCell.centerCoordY += cells[roofCells.front()].centerCoordY;
            newCell.elevation += cells[roofCells.front()].elevation;
            newCell.slope += cells[roofCells.front()].slope;
            newCell.landuse = cells[roofCells.front()].landuse;
            newCell.imperv = cells[roofCells.front()].imperv;
            newCell.outlet = cellsAdaptive[*it].name;
            newCell.outletID = *it;
            newCell.subcatchmentID = subcatchmentID;
            newCell.raingage = cells[roofCells.front()].raingage;
            newCell.snowPack = cells[roofCells.front()].snowPack;
            newCell.N_Imperv = cells[roofCells.front()].N_Imperv;
            newCell.N_Perv = cells[roofCells.front()].N_Perv;
            newCell.S_Imperv = cells[roofCells.front()].S_Imperv;
            newCell.S_Perv = cells[roofCells.front()].S_Perv;
            newCell.PctZero = cells[roofCells.front()].PctZero;
            newCell.RouteTo = cells[roofCells.front()].RouteTo;
            newCell.PctRouted = cells[roofCells.front()].PctRouted;
            newCell.Suction = cells[roofCells.front()].Suction;
            newCell.HydCon = cells[roofCells.front()].HydCon;
            newCell.IMDmax = cells[roofCells.front()].IMDmax;
            newCell.isSink = cells[roofCells.front()].isSink;
            newCell.tag = cells[roofCells.front()].tag;
            newCell.numElements++;
            std::stringstream subcatchmentName("");
            subcatchmentName << "s" << subcatchmentID + 1;
            newCell.name = subcatchmentName.str();

            // Remove the current cell ID from the list of rooCell ID's
            roofCells.erase(roofCells.begin());

            // Go through rest of the roof cells and add to the current subcatchment
            for (auto it=roofCells.begin(); it != roofCells.end(); ++it)
            {
                // Update subcatchmentID of current cell ...
                cells[*it].subcatchmentID = subcatchmentID;

                // ... and add current cell to existing subcatchment
                newCell.area += cells[*it].area;
                newCell.centerCoordX += cells[*it].centerCoordX;
                newCell.centerCoordY += cells[*it].centerCoordY;
                newCell.elevation += cells[*it].elevation;
                newCell.slope += cells[*it].slope;
                newCell.numElements++;
            }
            // Save current subcatchment
            cellsAdaptive.push_back(newCell);

            // ... and update the lookup table of subcatchments and their indexes.
            adapID[subcatchmentID] = cellsAdaptive.size()-1;
        }
    }

    /*** CONNECTED ROOFS ***/
    // Ensure the are no earlier inletIDs
    for (int i = 0; i < nCols*nRows; ++i)
        cells[i].inletIDs.clear();

    // Find outlet cells of connected roof cells
    for (int i = 0; i < nCols*nRows; i++)
    {
        if ( (cells[i].landuse >= ROOF_CONNECTED && cells[i].landuse < ROOF_UNCONNECTED)
                &&
                (i != cells[i].outletID) )
        {
            cells[cells[i].outletID].inletIDs.push_back(i);
        }
    }

    // Go through junctions
    for (int i = 1; i < juncTable.nRows; i++)
    {
        double juncPosX = atof( juncTable.getData(i, 0).c_str() );
        double juncPosY = atof( juncTable.getData(i, 1).c_str() );
        int isRoutable = atoi( juncTable.getData(i, 10).c_str() );

        // Check that cell is routable and within catchment
        if (juncPosX >= xllCorner
                &&
                juncPosX < xllCorner + nCols * cellSize
                &&
                juncPosY >= yllCorner
                &&
                juncPosY < yllCorner + nRows * cellSize
                &&
                isRoutable == 1) // pass closed junctions
        {
            if (nCols > 0 && nRows > 0 && cellSize > 0.0)
            {
                int col = (int)((juncPosX - xllCorner) / cellSize);
                int row = (int)((juncPosY - yllCorner) / cellSize);

                int j = col + row * nCols;  // Cell index of junction i

                if (j >= 0 && j < nCols * nRows)
                {
                    std::vector<int> selection_IDs;
                    selection_IDs.reserve(nCols*nRows); // Is this necessary???

                    for (auto it = cells[j].inletIDs.begin(); it !=  cells[j].inletIDs.end(); ++it) // Inlet roof cell to junction i in cell j
                    {
                        // Add cell to final cells if it is not there yet
                        if (!(std::find(final_IDs.begin(), final_IDs.end(), *it) != final_IDs.end()))
                        {
                            final_IDs.push_back(*it);
                            selection_IDs.push_back(*it);

                            // Update the subcatchmentID of current cell...
                            subcatchmentID++;
                            cells[*it].subcatchmentID = subcatchmentID;

                            // ... create a new adaptive subcatchment...
                            Cell newCell;
                            newCell.cellSize = cellSize;
                            newCell.area += cells[*it].area;
                            newCell.centerCoordX += cells[*it].centerCoordX;
                            newCell.centerCoordY += cells[*it].centerCoordY;
                            newCell.elevation += cells[*it].elevation;
                            newCell.slope += cells[*it].slope;
                            newCell.landuse = cells[*it].landuse;
                            newCell.imperv = cells[*it].imperv;
                            newCell.outlet = juncTable.getData(i, 2).c_str();
                            newCell.outletID = j;
                            newCell.outletCoordX = juncPosX;
                            newCell.outletCoordY = juncPosY;
                            newCell.subcatchmentID = subcatchmentID;
                            newCell.raingage = cells[*it].raingage;
                            newCell.snowPack = cells[*it].snowPack;
                            newCell.N_Imperv = cells[*it].N_Imperv;
                            newCell.N_Perv = cells[*it].N_Perv;
                            newCell.S_Imperv = cells[*it].S_Imperv;
                            newCell.S_Perv = cells[*it].S_Perv;
                            newCell.PctZero = cells[*it].PctZero;
                            newCell.RouteTo = cells[*it].RouteTo;
                            newCell.PctRouted = cells[*it].PctRouted;
                            newCell.Suction = cells[*it].Suction;
                            newCell.HydCon = cells[*it].HydCon;
                            newCell.IMDmax = cells[*it].IMDmax;
                            newCell.isSink = cells[*it].isSink;
                            newCell.tag = cells[*it].tag;
                            newCell.hasInlet = 1;
                            newCell.numElements++;
                            std::stringstream subcatchmentName("");
                            subcatchmentName << "s" << subcatchmentID + 1;
                            newCell.name = subcatchmentName.str();
                            cellsAdaptive.push_back(newCell);

                            // ... and update the lookup table of subcatchments and their indexes.
                            adapID[subcatchmentID] = cellsAdaptive.size()-1;

                            // Iterate through neighbouring roof cells and add cells connected to same junction
                            int counter = 0;    // Failsafe to prevent infinite while loop
                            while (!selection_IDs.empty() && counter < nCols*nRows)
                            {
                                std::vector<int> neighCells = cells[selection_IDs.front()].neighCellIndices;
                                // Go through the neighbour cells routed into the same junction
                                for (int k = 0; k < (int) neighCells.size(); k++)
                                {
                                    if (neighCells[k] != -1
                                            &&
                                            cells[neighCells[k]].landuse >= ROOF_CONNECTED && cells[neighCells[k]].landuse < ROOF_UNCONNECTED // Neighbouring cell is connected roof
                                            &&
                                            cells[neighCells[k]].outletID == cells[selection_IDs.front()].outletID    // Neighbouring cell is connected to same junction
                                       )
                                    {
                                        // Add cell to selected cells if it is not there or in final cells yet
                                        if (!(std::find(selection_IDs.begin(), selection_IDs.end(), neighCells[k]) != selection_IDs.end())
                                                &&
                                                !(std::find(final_IDs.begin(), final_IDs.end(), neighCells[k]) != final_IDs.end()))
                                        {
                                            selection_IDs.push_back(neighCells[k]);
                                        }

                                        // Add cell to final cells if it is not there yet
                                        if (!(std::find(final_IDs.begin(), final_IDs.end(), neighCells[k]) != final_IDs.end()))
                                        {
                                            final_IDs.push_back(neighCells[k]);

                                            // Update subcatchmentID of current cell ...
                                            cells[neighCells[k]].subcatchmentID = cells[selection_IDs.front()].subcatchmentID;

                                            // ... and add current cell to existing subcatchment
                                            int i = adapID[cells[selection_IDs.front()].subcatchmentID];
                                            cellsAdaptive[i].area += cells[neighCells[k]].area;
                                            cellsAdaptive[i].centerCoordX += cells[neighCells[k]].centerCoordX;
                                            cellsAdaptive[i].centerCoordY += cells[neighCells[k]].centerCoordY;
                                            cellsAdaptive[i].elevation += cells[neighCells[k]].elevation;
                                            cellsAdaptive[i].slope += cells[neighCells[k]].slope;
                                            cellsAdaptive[i].numElements++;
                                        }
                                    }
                                }

                                // Remove the cell ID from the list of selected ID's
                                selection_IDs.erase(selection_IDs.begin());
                                counter++;
                            }
                        }
                    }
                }
            }
        }
    }

    // Create and save a raster of adaptive grid for inspection before destroying the cells
    std::cout << "\n-> Creating an output raster of adaptive grid for inspection";
    saveRaster(path);

    // Save grid extent before destroying the cells
    double urcornerOld[2];
    double llcornerOld[2];
    urcornerOld[0] = urcorner[0];
    urcornerOld[1] = urcorner[1];
    llcornerOld[0] = llcorner[0];
    llcornerOld[1] = llcorner[1];


    clear();
    nCols = (int)cellsAdaptive.size();
    nRows = 1;
    cells = new Cell[nRows * nCols];
    // Copy grid extent after destroying the cells
    urcorner[0] = urcornerOld[0];
    urcorner[1] = urcornerOld[1];
    llcorner[0] = llcornerOld[0];
    llcorner[1] = llcornerOld[1];

    // Compute adaptive subcatchment averages
    for (int i = 0; i < nRows * nCols; i++ )
    {
        if (cellsAdaptive[i].numElements > 0)
        {
            cellsAdaptive[i].centerCoordX /= (double) cellsAdaptive[i].numElements;
            cellsAdaptive[i].centerCoordY /= (double) cellsAdaptive[i].numElements;
            cellsAdaptive[i].elevation /= (double) cellsAdaptive[i].numElements;
            cellsAdaptive[i].slope /= (double) cellsAdaptive[i].numElements;
        }
        else
            std::cout << "\nError: No cells in adaptive subcatchment " << i << "!";
    }

    // Transfer adaptive cell data to the pointer array.
    for (int i = 0; i < nRows * nCols; i++ )
    {
        cells[i].name = cellsAdaptive[i].name;
        cells[i].centerCoordX = cellsAdaptive[i].centerCoordX;
        cells[i].centerCoordY = cellsAdaptive[i].centerCoordY;
        cells[i].elevation = cellsAdaptive[i].elevation;
        cells[i].cellSize = cellsAdaptive[i].cellSize;
        cells[i].slope = cellsAdaptive[i].slope;
        cells[i].area = cellsAdaptive[i].area;
        cells[i].flowWidth = 0.7 * std::sqrt(cellsAdaptive[i].area);    // Krebs et al. (2014). Journal of Hydrology.
        cells[i].landuse = cellsAdaptive[i].landuse;
        if (cellsAdaptive[i].hasInlet == 1) // Outlet is junction
        {
            cells[i].outletCoordX = cellsAdaptive[i].outletCoordX;
            cells[i].outletCoordY = cellsAdaptive[i].outletCoordY;
        }
        else    // outlet is another subcatchment
        {
            cells[i].outletCoordX = cellsAdaptive[cellsAdaptive[i].outletID].centerCoordX;
            cells[i].outletCoordY = cellsAdaptive[cellsAdaptive[i].outletID].centerCoordY;
        }
        cells[i].outletID = cellsAdaptive[i].outletID;
        cells[i].outlet = cellsAdaptive[i].outlet;
        cells[i].raingage = cellsAdaptive[i].raingage;
        cells[i].imperv = cellsAdaptive[i].imperv;
        cells[i].snowPack = cellsAdaptive[i].snowPack;
        cells[i].N_Imperv = cellsAdaptive[i].N_Imperv;
        cells[i].N_Perv = cellsAdaptive[i].N_Perv;
        cells[i].S_Imperv = cellsAdaptive[i].S_Imperv;
        cells[i].S_Perv = cellsAdaptive[i].S_Perv;
        cells[i].PctZero = cellsAdaptive[i].PctZero;
        cells[i].RouteTo = cellsAdaptive[i].RouteTo;
        cells[i].PctRouted = cellsAdaptive[i].PctRouted;
        cells[i].Suction = cellsAdaptive[i].Suction;
        cells[i].HydCon = cellsAdaptive[i].HydCon;
        cells[i].IMDmax = cellsAdaptive[i].IMDmax;
        cells[i].isSink = cellsAdaptive[i].isSink;
        cells[i].tag = cellsAdaptive[i].tag;
    }
}

// Simplify computation grid based on common flow direction and landuse
void Grid::simplifyLID(Table &juncTable, std::string &path)
{
    /*** NON-ROOF CELLS ***/
    // Ensure there are no inletIDs
    for (int i = 0; i < nCols*nRows; i++)
        cells[i].inletIDs.clear();

    // Set inletIDs contributing to each cell
    for (int i = 0; i < nCols*nRows; i++)
    {
        if ( cells[i].landuse >= BUILT_AREA && i != cells[i].outletID )
        {
            cells[cells[i].outletID].inletIDs.push_back(i);
        }
    }

    // Set the 3x3 cell area surrounding each stormwater inlet as the collecting area for the inlet.
    // This should help with misplacement of stormwater inlets when not just the cell with the inlet is used
    // to collect water
    for (int i = 1; i < juncTable.nRows; i++) // pass the header line
    {
        double juncPosX = atof( juncTable.data[i * juncTable.nCols].c_str() );
        double juncPosY = atof( juncTable.data[i * juncTable.nCols + 1].c_str() );
        int isOpen = atoi( juncTable.data[i * juncTable.nCols + 6].c_str() );

        if (juncPosX >= xllCorner && juncPosX < xllCorner + nCols * cellSize
                && juncPosY >= yllCorner && juncPosY < yllCorner + nRows * cellSize && isOpen == 1) // pass closed junctions
        {
            if (nCols > 0 && nRows > 0 && cellSize > 0.0)
            {
                int col = (int)((juncPosX - xllCorner) / cellSize);
                int row = (int)((juncPosY - yllCorner) / cellSize);

                if (col + row * nCols >= 0 && col + row * nCols < nCols * nRows)
                {
                    for (int j = 0; j < (int)cells[ col + row * nCols ].neighCellIndices.size(); j++)
                    {
                        if (cells[ col + row * nCols ].neighCellIndices[j] != -1
                                && cells[ cells[ col + row * nCols ].neighCellIndices[j] ].landuse >= BUILT_AREA)
                        {
                            cells[cells[ cells[ col + row * nCols ].neighCellIndices[j] ].outletID].inletIDs.push_back(cells[ col + row * nCols ].neighCellIndices[j]);
                        }
                    }
                }
            }
        }
    }

    // Empty vector for final list of routed cell ID's
    std::vector<int> final_IDs;

    std::vector<Cell> cellsAdaptive;
    std::unordered_map<int, std::vector<Cell>::size_type> adapID;  // A lookup table of subcatchmentID's in cellsAdaptive
    int subcatchmentID = -1;

    // Go through junctions
    for (int i = 1; i < juncTable.nRows; i++)
    {
        double juncPosX = atof( juncTable.getData(i, 0).c_str() );
        double juncPosY = atof( juncTable.getData(i, 1).c_str() );
        int isOpen = atoi( juncTable.getData(i, 6).c_str() );

        // Add cells with open junctions to final list of cells
        if (juncPosX >= xllCorner
                &&
                juncPosX < xllCorner + nCols * cellSize
                &&
                juncPosY >= yllCorner
                &&
                juncPosY < yllCorner + nRows * cellSize
                &&
                isOpen == 1) // pass closed junctions
        {
            if (nCols > 0 && nRows > 0 && cellSize > 0.0)
            {
                int col = (int)((juncPosX - xllCorner) / cellSize);
                int row = (int)((juncPosY - yllCorner) / cellSize);

                if (col + row * nCols >= 0 && col + row * nCols < nCols * nRows)
                {
                    std::vector<int> selection_IDs;
                    selection_IDs.reserve(nCols*nRows); // Is this necessary???

                    final_IDs.push_back(col + row * nCols);
                    selection_IDs.push_back(col + row * nCols);

                    // Update the subcatchmentID of current cell...
                    subcatchmentID++;
                    cells[col + row * nCols].subcatchmentID = subcatchmentID;

                    // ... create a new adaptive subcatchment...
                    Cell newCell;
                    newCell.cellSize = cellSize;
                    newCell.area += cells[col + row * nCols].area;
                    newCell.centerCoordX += cells[col + row * nCols].centerCoordX;
                    newCell.centerCoordY += cells[col + row * nCols].centerCoordY;
                    newCell.elevation += cells[col + row * nCols].elevation;
                    newCell.slope += cells[col + row * nCols].slope;
                    newCell.landuse = cells[col + row * nCols].landuse;
                    newCell.imperv = cells[col + row * nCols].imperv;
                    newCell.outlet = juncTable.getData(i, 2).c_str();
                    newCell.outletID = subcatchmentID;
                    newCell.outletCoordX = juncPosX;
                    newCell.outletCoordY = juncPosY;
                    newCell.subcatchmentID = subcatchmentID;
                    newCell.raingage = cells[col + row * nCols].raingage;
                    newCell.snowPack = cells[col + row * nCols].snowPack;
                    newCell.N_Imperv = cells[col + row * nCols].N_Imperv;
                    newCell.N_Perv = cells[col + row * nCols].N_Perv;
                    newCell.S_Imperv = cells[col + row * nCols].S_Imperv;
                    newCell.S_Perv = cells[col + row * nCols].S_Perv;
                    newCell.PctZero = cells[col + row * nCols].PctZero;
                    newCell.RouteTo = cells[col + row * nCols].RouteTo;
                    newCell.PctRouted = cells[col + row * nCols].PctRouted;
                    newCell.Suction = cells[col + row * nCols].Suction;
                    newCell.HydCon = cells[col + row * nCols].HydCon;
                    newCell.IMDmax = cells[col + row * nCols].IMDmax;
                    newCell.isSink = cells[col + row * nCols].isSink;
                    newCell.tag = cells[col + row * nCols].tag;
                    newCell.LIDType = cells[col + row * nCols].LIDType;
                    newCell.hasInlet = 1;
                    newCell.numElements++;
                    std::stringstream subcatchmentName("");
                    subcatchmentName << "s" << subcatchmentID + 1;
                    newCell.name = subcatchmentName.str();
                    cellsAdaptive.push_back(newCell);

                    // ... and update the lookup table of subcatchments and their indexes.
                    adapID[subcatchmentID] = cellsAdaptive.size()-1;

                    // Use the 3x3 cell area surrounding each stormwater inlet as the collecting area for the inlet.
                    // This should help with misplacement of stormwater inlets when not just the cell with the inlet is used
                    // to collect water
                    for (int j = 0; j < (int)cells[ col + row * nCols ].neighCellIndices.size(); j++)
                    {
                        if (cells[ col + row * nCols ].neighCellIndices[j] != -1
                        && cells[ cells[ col + row * nCols ].neighCellIndices[j] ].landuse >= BUILT_AREA
                        && (cells[ cells[ col + row * nCols ].neighCellIndices[j] ].landuse != cells[ col + row * nCols ].landuse ||
                            cells[ cells[ col + row * nCols ].neighCellIndices[j] ].LIDType != cells[ col + row * nCols ].LIDType))  // Don't create new subcatchments if the landuse and LID type is the same as in inlet cell
                        {
                            final_IDs.push_back(cells[ col + row * nCols ].neighCellIndices[j]);
                            selection_IDs.push_back(cells[ col + row * nCols ].neighCellIndices[j]);

                            // Update the subcatchmentID of current cell...
                            subcatchmentID++;
                            cells[ cells[ col + row * nCols ].neighCellIndices[j]].subcatchmentID = subcatchmentID;

                            // ... create a new adaptive subcatchment...
                            Cell newCell;
                            newCell.cellSize = cellSize;
                            newCell.area += cells[ cells[ col + row * nCols ].neighCellIndices[j] ].area;
                            newCell.centerCoordX += cells[ cells[ col + row * nCols ].neighCellIndices[j] ].centerCoordX;
                            newCell.centerCoordY += cells[ cells[ col + row * nCols ].neighCellIndices[j] ].centerCoordY;
                            newCell.elevation += cells[ cells[ col + row * nCols ].neighCellIndices[j] ].elevation;
                            newCell.slope += cells[ cells[ col + row * nCols ].neighCellIndices[j] ].slope;
                            newCell.landuse = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].landuse;
                            newCell.imperv = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].imperv;
                            newCell.outlet = juncTable.getData(i, 2).c_str();
                            newCell.outletID = subcatchmentID;
                            newCell.outletCoordX = juncPosX;
                            newCell.outletCoordY = juncPosY;
                            newCell.subcatchmentID = subcatchmentID;
                            newCell.raingage = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].raingage;
                            newCell.snowPack = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].snowPack;
                            newCell.N_Imperv = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].N_Imperv;
                            newCell.N_Perv = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].N_Perv;
                            newCell.S_Imperv = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].S_Imperv;
                            newCell.S_Perv = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].S_Perv;
                            newCell.PctZero = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].PctZero;
                            newCell.RouteTo = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].RouteTo;
                            newCell.PctRouted = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].PctRouted;
                            newCell.Suction = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].Suction;
                            newCell.HydCon = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].HydCon;
                            newCell.IMDmax = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].IMDmax;
                            newCell.isSink = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].isSink;
                            newCell.tag = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].tag;
                            newCell.LIDType = cells[ cells[ col + row * nCols ].neighCellIndices[j] ].LIDType;
                            newCell.hasInlet = 1;
                            newCell.numElements++;
                            std::stringstream subcatchmentName("");
                            subcatchmentName << "s" << subcatchmentID + 1;
                            newCell.name = subcatchmentName.str();
                            cellsAdaptive.push_back(newCell);

                            // ... and update the lookup table of subcatchments and their indexes.
                            adapID[subcatchmentID] = cellsAdaptive.size()-1;
                        }
                    }

                    // Iterate through cells and add cells as they flow towards the inlet
                    int counter = 0;    // Failsafe to prevent infinite while loop
                    while (!selection_IDs.empty() && counter < nCols*nRows)
                    {
                        // Get all subcatchments routed into the current cell
                        std::vector<int> inSubs = cells[selection_IDs.front()].inletIDs;

                        // Go through the cells routed into the current cell
                        for (auto it=inSubs.begin(); it != inSubs.end(); ++it)
                        {
                            // Add cell to selected cells if it is not there or in final cells yet
                            if (!(std::find(selection_IDs.begin(), selection_IDs.end(), *it) != selection_IDs.end())
                                    &&
                                    !(std::find(final_IDs.begin(), final_IDs.end(), *it) != final_IDs.end()))
                            {
                                selection_IDs.push_back(*it);
                            }

                            // Add cell to final cells if it is not there yet
                            if (!(std::find(final_IDs.begin(), final_IDs.end(), *it) != final_IDs.end()))
                            {
                                final_IDs.push_back(*it);

                                // Add current cell to existing adaptive subctahment or create a new one if old doesn't exist
                                if (
                                    cells[cells[*it].outletID].subcatchmentID > -1   // adaptive subcatchment exists
                                    &&
                                    cells[*it].landuse == cells[cells[*it].outletID].landuse // cell has same landuse as the downstream subcatchment
                                    &&
                                    cells[*it].LIDType == cells[cells[*it].outletID].LIDType // cell has same LID type as the downstream subcatchment
                                )
                                {
                                    // Update subcatchmentID of current cell ...
                                    cells[*it].subcatchmentID = cells[cells[*it].outletID].subcatchmentID;

                                    // ... and add current cell to existing subcatchment
                                    int i = adapID[cells[cells[*it].outletID].subcatchmentID];
                                    cellsAdaptive[i].area += cells[*it].area;
                                    cellsAdaptive[i].centerCoordX += cells[*it].centerCoordX;
                                    cellsAdaptive[i].centerCoordY += cells[*it].centerCoordY;
                                    cellsAdaptive[i].elevation += cells[*it].elevation;
                                    cellsAdaptive[i].slope += cells[*it].slope;
                                    cellsAdaptive[i].numElements++;
                                }
                                else
                                {
                                    // Update the subcatchmentID of current cell...
                                    subcatchmentID++;
                                    cells[*it].subcatchmentID = subcatchmentID;

                                    // Get index of downstream subcatchment
                                    int i = adapID[cells[cells[*it].outletID].subcatchmentID];

                                    // ... and create a new adaptive subcatchment...
                                    Cell newCell;
                                    newCell.cellSize = cellSize;
                                    newCell.area += cells[*it].area;
                                    newCell.centerCoordX += cells[*it].centerCoordX;
                                    newCell.centerCoordY += cells[*it].centerCoordY;
                                    newCell.elevation += cells[*it].elevation;
                                    newCell.slope += cells[*it].slope;
                                    newCell.landuse = cells[*it].landuse;
                                    newCell.imperv = cells[*it].imperv;
                                    newCell.outlet = cellsAdaptive[i].name;
                                    newCell.outletID = cells[cells[*it].outletID].subcatchmentID;
                                    newCell.subcatchmentID = subcatchmentID;
                                    newCell.raingage = cells[*it].raingage;
                                    newCell.snowPack = cells[*it].snowPack;
                                    newCell.N_Imperv = cells[*it].N_Imperv;
                                    newCell.N_Perv = cells[*it].N_Perv;
                                    newCell.S_Imperv = cells[*it].S_Imperv;
                                    newCell.S_Perv = cells[*it].S_Perv;
                                    newCell.PctZero = cells[*it].PctZero;
                                    newCell.RouteTo = cells[*it].RouteTo;
                                    newCell.PctRouted = cells[*it].PctRouted;
                                    newCell.Suction = cells[*it].Suction;
                                    newCell.HydCon = cells[*it].HydCon;
                                    newCell.IMDmax = cells[*it].IMDmax;
                                    newCell.isSink = cells[*it].isSink;
                                    newCell.tag = cells[*it].tag;
                                    newCell.LIDType = cells[*it].LIDType;
                                    newCell.numElements++;
                                    std::stringstream subcatchmentName("");
                                    subcatchmentName << "s" << subcatchmentID + 1;
                                    newCell.name = subcatchmentName.str();
                                    cellsAdaptive.push_back(newCell);

                                    // ... and update the lookup table of subcatchments and their indexes.
                                    adapID[subcatchmentID] = cellsAdaptive.size()-1;
                                }
                            }
                        }
                        // Remove the cell ID from the list of selected ID's
                        selection_IDs.erase(selection_IDs.begin());
                        counter++;
                    }
                }
            }
        }
    }

    /*** UNCONNECTED ROOFS ***/
    // Ensure the are no earlier inletIDs
    for (int i = 0; i < (int)cellsAdaptive.size(); ++i)
        cellsAdaptive[i].inletIDs.clear();

    // Find outlets of unconnected roof cells
    for (int i = 0; i < nCols*nRows; i++)
    {
        if ( (cells[i].landuse >= ROOF_UNCONNECTED && cells[i].landuse < BUILT_AREA)
                &&
                (cells[cells[i].outletID].subcatchmentID > -1)  // Check that the roof is connected to routed cell
                &&
                (i != cells[i].outletID) )
        {
            // Get index of downstream subcatchment
            int j = adapID[cells[cells[i].outletID].subcatchmentID];
            cellsAdaptive[j].inletIDs.push_back(i);   // set the unconnected roof cell as inlet for the subcatchment
        }
    }
    std::vector<int> roofOutlets;   // Temp vector of subcatchments where unconnnectd roofs are connected
    for (int i = 0; i < (int)cellsAdaptive.size(); ++i)
    {
        if (!cellsAdaptive[i].inletIDs.empty())
            roofOutlets.push_back(i);
    }

    // Combine unconnected roof cells with the same outlet into one subcatchment
    for (auto it=roofOutlets.begin(); it != roofOutlets.end(); ++it)
    {
        if (!cellsAdaptive[*it].inletIDs.empty())
        {
            // Get all roof cells routed into the current cell
            std::vector<int> roofCells = cellsAdaptive[*it].inletIDs;       //All the unconnected roof cells that are connected to same outlet into one subcatchment

            int counter = 0;    // Failsafe to prevent infinite while loop
            while (!roofCells.empty() && counter < roofCells.size())
            {
                // Update the subcatchmentID of current cell...
                subcatchmentID++;
                cells[roofCells.front()].subcatchmentID = subcatchmentID;

                // ... and create a new adaptive subcatchment.
                Cell newCell;
                newCell.cellSize = cellSize;
                newCell.area += cells[roofCells.front()].area;
                newCell.centerCoordX += cells[roofCells.front()].centerCoordX;
                newCell.centerCoordY += cells[roofCells.front()].centerCoordY;
                newCell.elevation += cells[roofCells.front()].elevation;
                newCell.slope += cells[roofCells.front()].slope;
                newCell.landuse = cells[roofCells.front()].landuse;
                newCell.imperv = cells[roofCells.front()].imperv;
                newCell.outlet = cellsAdaptive[*it].name;
                newCell.outletID = *it;
                newCell.subcatchmentID = subcatchmentID;
                newCell.raingage = cells[roofCells.front()].raingage;
                newCell.snowPack = cells[roofCells.front()].snowPack;
                newCell.N_Imperv = cells[roofCells.front()].N_Imperv;
                newCell.N_Perv = cells[roofCells.front()].N_Perv;
                newCell.S_Imperv = cells[roofCells.front()].S_Imperv;
                newCell.S_Perv = cells[roofCells.front()].S_Perv;
                newCell.PctZero = cells[roofCells.front()].PctZero;
                newCell.RouteTo = cells[roofCells.front()].RouteTo;
                newCell.PctRouted = cells[roofCells.front()].PctRouted;
                newCell.Suction = cells[roofCells.front()].Suction;
                newCell.HydCon = cells[roofCells.front()].HydCon;
                newCell.IMDmax = cells[roofCells.front()].IMDmax;
                newCell.isSink = cells[roofCells.front()].isSink;
                newCell.tag = cells[roofCells.front()].tag;
                newCell.LIDType = cells[roofCells.front()].LIDType;
                newCell.numElements++;
                std::stringstream subcatchmentName("");
                subcatchmentName << "s" << subcatchmentID + 1;
                newCell.name = subcatchmentName.str();

                // Remove the current cell ID from the list of rooCell ID's
                roofCells.erase(roofCells.begin());

                //MAKE changes HERE
                auto it = roofCells.begin();
                while (it != roofCells.end())
                {  
                    if(newCell.LIDType == cells[*it].LIDType)
                    {    
                        // Update subcatchmentID of current cell ...
                        cells[*it].subcatchmentID = subcatchmentID;

                        // ... and add current cell to existing subcatchment
                        newCell.area += cells[*it].area;
                        newCell.centerCoordX += cells[*it].centerCoordX;
                        newCell.centerCoordY += cells[*it].centerCoordY;
                        newCell.elevation += cells[*it].elevation;
                        newCell.slope += cells[*it].slope;
                        newCell.numElements++;

                        roofCells.erase(it);
                    }
                    else{
                        ++it;
                    }
                }
                
                /*
                // Go through rest of the roof cells and add to the current subcatchment
                for (auto it=roofCells.begin(); it != roofCells.end(); ++it)
                {
                    // Update subcatchmentID of current cell ...
                    cells[*it].subcatchmentID = subcatchmentID;

                    // ... and add current cell to existing subcatchment
                    newCell.area += cells[*it].area;
                    newCell.centerCoordX += cells[*it].centerCoordX;
                    newCell.centerCoordY += cells[*it].centerCoordY;
                    newCell.elevation += cells[*it].elevation;
                    newCell.slope += cells[*it].slope;
                    newCell.numElements++;
                }
                */

                // Save current subcatchment
                cellsAdaptive.push_back(newCell);

                // ... and update the lookup table of subcatchments and their indexes.
                adapID[subcatchmentID] = cellsAdaptive.size()-1;
                counter++;
            }
        }
    }

    /*** CONNECTED ROOFS ***/
    // Ensure the are no earlier inletIDs
    for (int i = 0; i < nCols*nRows; ++i)
        cells[i].inletIDs.clear();

    // Find outlet cells of connected roof cells
    for (int i = 0; i < nCols*nRows; i++)
    {
        if ( (cells[i].landuse >= ROOF_CONNECTED && cells[i].landuse < ROOF_UNCONNECTED)
                &&
                (i != cells[i].outletID) )
        {
            cells[cells[i].outletID].inletIDs.push_back(i);
        }
    }

    // Go through junctions
    for (int i = 1; i < juncTable.nRows; i++)
    {
        double juncPosX = atof( juncTable.getData(i, 0).c_str() );
        double juncPosY = atof( juncTable.getData(i, 1).c_str() );
        int isRoutable = atoi( juncTable.getData(i, 10).c_str() );

        // Check that cell is routable and within catchment
        if (juncPosX >= xllCorner
                &&
                juncPosX < xllCorner + nCols * cellSize
                &&
                juncPosY >= yllCorner
                &&
                juncPosY < yllCorner + nRows * cellSize
                &&
                isRoutable == 1) // pass closed junctions
        {
            if (nCols > 0 && nRows > 0 && cellSize > 0.0)
            {
                int col = (int)((juncPosX - xllCorner) / cellSize);
                int row = (int)((juncPosY - yllCorner) / cellSize);

                int j = col + row * nCols;  // Cell index of junction i

                if (j >= 0 && j < nCols * nRows)
                {
                    std::vector<int> selection_IDs;
                    selection_IDs.reserve(nCols*nRows); // Is this necessary???

                    for (auto it = cells[j].inletIDs.begin(); it !=  cells[j].inletIDs.end(); ++it) // Inlet roof cell to junction i in cell j
                    {
                        // Add cell to final cells if it is not there yet
                        if (!(std::find(final_IDs.begin(), final_IDs.end(), *it) != final_IDs.end()))
                        {
                            final_IDs.push_back(*it);
                            selection_IDs.push_back(*it);

                            // Update the subcatchmentID of current cell...
                            subcatchmentID++;
                            cells[*it].subcatchmentID = subcatchmentID;

                            // ... create a new adaptive subcatchment...
                            Cell newCell;
                            newCell.cellSize = cellSize;
                            newCell.area += cells[*it].area;
                            newCell.centerCoordX += cells[*it].centerCoordX;
                            newCell.centerCoordY += cells[*it].centerCoordY;
                            newCell.elevation += cells[*it].elevation;
                            newCell.slope += cells[*it].slope;
                            newCell.landuse = cells[*it].landuse;
                            newCell.imperv = cells[*it].imperv;
                            newCell.outlet = juncTable.getData(i, 2).c_str();
                            newCell.outletID = j;
                            newCell.outletCoordX = juncPosX;
                            newCell.outletCoordY = juncPosY;
                            newCell.subcatchmentID = subcatchmentID;
                            newCell.raingage = cells[*it].raingage;
                            newCell.snowPack = cells[*it].snowPack;
                            newCell.N_Imperv = cells[*it].N_Imperv;
                            newCell.N_Perv = cells[*it].N_Perv;
                            newCell.S_Imperv = cells[*it].S_Imperv;
                            newCell.S_Perv = cells[*it].S_Perv;
                            newCell.PctZero = cells[*it].PctZero;
                            newCell.RouteTo = cells[*it].RouteTo;
                            newCell.PctRouted = cells[*it].PctRouted;
                            newCell.Suction = cells[*it].Suction;
                            newCell.HydCon = cells[*it].HydCon;
                            newCell.IMDmax = cells[*it].IMDmax;
                            newCell.isSink = cells[*it].isSink;
                            newCell.tag = cells[*it].tag;
                            newCell.LIDType = cells[*it].LIDType;
                            newCell.hasInlet = 1;
                            newCell.numElements++;
                            std::stringstream subcatchmentName("");
                            subcatchmentName << "s" << subcatchmentID + 1;
                            newCell.name = subcatchmentName.str();
                            cellsAdaptive.push_back(newCell);

                            // ... and update the lookup table of subcatchments and their indexes.
                            adapID[subcatchmentID] = cellsAdaptive.size()-1;

                            // Iterate through neighbouring roof cells and add cells connected to same junction
                            int counter = 0;    // Failsafe to prevent infinite while loop
                            while (!selection_IDs.empty() && counter < nCols*nRows)
                            {
                                std::vector<int> neighCells = cells[selection_IDs.front()].neighCellIndices;
                                // Go through the neighbour cells routed into the same junction
                                for (int k = 0; k < (int) neighCells.size(); k++)
                                {
                                    if (neighCells[k] != -1
                                            &&
                                            cells[neighCells[k]].landuse >= ROOF_CONNECTED && cells[neighCells[k]].landuse < ROOF_UNCONNECTED // Neighbouring cell is connected roof
                                            &&
                                            cells[neighCells[k]].outletID == cells[selection_IDs.front()].outletID    // Neighbouring cell is connected to same junction
                                            &&
                                            cells[neighCells[k]].LIDType == cells[selection_IDs.front()].LIDType      // Neighbouring cell has same LID type
                                       )
                                    {
                                        // Add cell to selected cells if it is not there or in final cells yet
                                        if (!(std::find(selection_IDs.begin(), selection_IDs.end(), neighCells[k]) != selection_IDs.end())
                                                &&
                                                !(std::find(final_IDs.begin(), final_IDs.end(), neighCells[k]) != final_IDs.end()))
                                        {
                                            selection_IDs.push_back(neighCells[k]);
                                        }

                                        // Add cell to final cells if it is not there yet
                                        if (!(std::find(final_IDs.begin(), final_IDs.end(), neighCells[k]) != final_IDs.end()))
                                        {
                                            final_IDs.push_back(neighCells[k]);

                                            // Update subcatchmentID of current cell ...
                                            cells[neighCells[k]].subcatchmentID = cells[selection_IDs.front()].subcatchmentID;

                                            // ... and add current cell to existing subcatchment
                                            int i = adapID[cells[selection_IDs.front()].subcatchmentID];
                                            cellsAdaptive[i].area += cells[neighCells[k]].area;
                                            cellsAdaptive[i].centerCoordX += cells[neighCells[k]].centerCoordX;
                                            cellsAdaptive[i].centerCoordY += cells[neighCells[k]].centerCoordY;
                                            cellsAdaptive[i].elevation += cells[neighCells[k]].elevation;
                                            cellsAdaptive[i].slope += cells[neighCells[k]].slope;
                                            cellsAdaptive[i].numElements++;
                                        }
                                    }
                                }

                                // Remove the cell ID from the list of selected ID's
                                selection_IDs.erase(selection_IDs.begin());
                                counter++;
                            }
                        }
                    }
                }
            }
        }
    }

    // Create and save a raster of adaptive grid for inspection before destroying the cells
    std::cout << "\n-> Creating an output raster of adaptive grid for inspection";
    saveRaster(path);

    // Save grid extent before destroying the cells
    double urcornerOld[2];
    double llcornerOld[2];
    urcornerOld[0] = urcorner[0];
    urcornerOld[1] = urcorner[1];
    llcornerOld[0] = llcorner[0];
    llcornerOld[1] = llcorner[1];


    clear();
    nCols = (int)cellsAdaptive.size();
    nRows = 1;
    cells = new Cell[nRows * nCols];
    // Copy grid extent after destroying the cells
    urcorner[0] = urcornerOld[0];
    urcorner[1] = urcornerOld[1];
    llcorner[0] = llcornerOld[0];
    llcorner[1] = llcornerOld[1];

    // Compute adaptive subcatchment averages
    for (int i = 0; i < nRows * nCols; i++ )
    {
        if (cellsAdaptive[i].numElements > 0)
        {
            cellsAdaptive[i].centerCoordX /= (double) cellsAdaptive[i].numElements;
            cellsAdaptive[i].centerCoordY /= (double) cellsAdaptive[i].numElements;
            cellsAdaptive[i].elevation /= (double) cellsAdaptive[i].numElements;
            cellsAdaptive[i].slope /= (double) cellsAdaptive[i].numElements;
        }
        else
            std::cout << "\nError: No cells in adaptive subcatchment " << i << "!";
    }

    // Transfer adaptive cell data to the pointer array.
    for (int i = 0; i < nRows * nCols; i++ )
    {
        cells[i].name = cellsAdaptive[i].name;
        cells[i].centerCoordX = cellsAdaptive[i].centerCoordX;
        cells[i].centerCoordY = cellsAdaptive[i].centerCoordY;
        cells[i].elevation = cellsAdaptive[i].elevation;
        cells[i].cellSize = cellsAdaptive[i].cellSize;
        cells[i].slope = cellsAdaptive[i].slope;
        cells[i].area = cellsAdaptive[i].area;
        cells[i].flowWidth = 0.7 * std::sqrt(cellsAdaptive[i].area);    // Krebs et al. (2014). Journal of Hydrology.
        cells[i].landuse = cellsAdaptive[i].landuse;
        if (cellsAdaptive[i].hasInlet == 1) // Outlet is junction
        {
            cells[i].outletCoordX = cellsAdaptive[i].outletCoordX;
            cells[i].outletCoordY = cellsAdaptive[i].outletCoordY;
        }
        else    // outlet is another subcatchment
        {
            cells[i].outletCoordX = cellsAdaptive[cellsAdaptive[i].outletID].centerCoordX;
            cells[i].outletCoordY = cellsAdaptive[cellsAdaptive[i].outletID].centerCoordY;
        }
        cells[i].outletID = cellsAdaptive[i].outletID;
        cells[i].outlet = cellsAdaptive[i].outlet;
        cells[i].raingage = cellsAdaptive[i].raingage;
        cells[i].imperv = cellsAdaptive[i].imperv;
        cells[i].snowPack = cellsAdaptive[i].snowPack;
        cells[i].N_Imperv = cellsAdaptive[i].N_Imperv;
        cells[i].N_Perv = cellsAdaptive[i].N_Perv;
        cells[i].S_Imperv = cellsAdaptive[i].S_Imperv;
        cells[i].S_Perv = cellsAdaptive[i].S_Perv;
        cells[i].PctZero = cellsAdaptive[i].PctZero;
        cells[i].RouteTo = cellsAdaptive[i].RouteTo;
        cells[i].PctRouted = cellsAdaptive[i].PctRouted;
        cells[i].Suction = cellsAdaptive[i].Suction;
        cells[i].HydCon = cellsAdaptive[i].HydCon;
        cells[i].IMDmax = cellsAdaptive[i].IMDmax;
        cells[i].isSink = cellsAdaptive[i].isSink;
        cells[i].tag = cellsAdaptive[i].tag;
        cells[i].LIDType = cellsAdaptive[i].LIDType;
    }
}

void Grid::saveRaster(std::string path)
{
    path += ".asc";

    Raster outputRaster;
    outputRaster.pathName = path;
    outputRaster.nCols = nCols;
    outputRaster.nRows = nRows;
    outputRaster.xllCorner = xllCorner;
    outputRaster.yllCorner = yllCorner;
    outputRaster.cellSize = cellSize;
    std::string noValue = "-1";
    outputRaster.noDataValue = noValue;
    outputRaster.data = new std::string[outputRaster.nCols * outputRaster.nRows];

    for (int j = 0; j < outputRaster.nRows; j++)
    {
        for (int i = 0; i < outputRaster.nCols; i++)
        {
            std::stringstream sstream;
//            sstream << std::fixed;
//            sstream.precision(8);

            if (cells[ i + j * nCols ].landuse != LANDUSE_NONE)
            {
                sstream << cells[ i + j * nCols ].subcatchmentID + 1;
            }
            else
            {
                sstream << noValue.c_str();
            }

            int flippedIndex = outputRaster.nRows - 1 - j;
            outputRaster.data[ i + flippedIndex * outputRaster.nCols ] = sstream.str();
        }
    }

    outputRaster.save( outputRaster.pathName );
}

int Grid::saveSubcatchmentPolygon(std::string path)
{
    std::stringstream sstream;
    std::stringstream sstream_csvt;
    sstream << std::fixed;

    sstream << "id;";
    sstream << "wkt;";          // Polygon defining the shape of the  subcatchment
    sstream << "name;";         // Name of the  subcatchment
    sstream << "outlet;";       // Name of node or another subcatchment that receives runoff
    sstream << "area_m2;";      // Area of subcatchment (m2)
    sstream << "slope_pct;";    // Average surface slope (%)
    sstream << "elevation;";    // Elevation of the subcatchment
    sstream << "landuse;";      // Code for landuse type
    sstream << "imp_pct;";      // Percent of impervious area (%)
    sstream << "n_imp;";        // Mannings N for impervious area (-)
    sstream << "n_per;";        // Mannings N for pervious area (-)
    sstream << "S_imp_mm;";     // Depth of depression storage on impervious area (mm)
    sstream << "S_per_mm;";     // Depth of depression storage on pervious area (mm)
    sstream << "suct_mm;";      // Soil capillary suction head (mm)
    sstream << "Ksat_mmhr;";    // Soil saturated hydraulic conductivity (mm/hr)
    sstream << "IMDmax;";       // Difference between soil porosity and initial moisture content (a fraction)
    sstream << "isSink;";       // Cell is a local sink
    sstream << "Tag;";           // Optional tag sepcifying e.g. cell landuse in string format
    sstream << "LIDType";       // LID Type of the subcatchment

    // Create a .csvt file defining the field types of the .wkt file for ogr2ogr conversion to shapefile
    sstream_csvt << "Integer,";
    sstream_csvt << "WKT,";
    sstream_csvt << "String,";
    sstream_csvt << "String,";
    sstream_csvt << "Real,";
    sstream_csvt << "Real,";
    sstream_csvt << "Real,";
    sstream_csvt << "Real,";
    sstream_csvt << "Real,";
    sstream_csvt << "Real,";
    sstream_csvt << "Real,";
    sstream_csvt << "Real,";
    sstream_csvt << "Real,";
    sstream_csvt << "Real,";
    sstream_csvt << "Real,";
    sstream_csvt << "Real,";
    sstream_csvt << "Integer,";
    sstream_csvt << "String,";
    sstream_csvt << "String";


    // Write polygon vertex coordinates.
    int polyId = 1;
    for (int i = 0; i < nRows * nCols; i++)
    {
        if (cells[i].landuse != LANDUSE_NONE)
        {
            sstream << "\n" << polyId << ";POLYGON((";
            sstream.precision(2);
            sstream << std::fixed << cells[i].centerCoordX - 0.5 * cells[i].cellSize << " ";
            sstream << std::fixed << cells[i].centerCoordY + 0.5 * cells[i].cellSize;
            sstream << ",";
            sstream << std::fixed << cells[i].centerCoordX - 0.5 * cells[i].cellSize << " ";
            sstream << std::fixed << cells[i].centerCoordY - 0.5 * cells[i].cellSize;
            sstream << ",";
            sstream << std::fixed << cells[i].centerCoordX + 0.5 * cells[i].cellSize << " ";
            sstream << std::fixed << cells[i].centerCoordY - 0.5 * cells[i].cellSize;
            sstream << ",";
            sstream << std::fixed << cells[i].centerCoordX + 0.5 * cells[i].cellSize << " ";
            sstream << std::fixed << cells[i].centerCoordY + 0.5 * cells[i].cellSize;
            sstream << ",";
            sstream << std::fixed << cells[i].centerCoordX - 0.5 * cells[i].cellSize << " ";
            sstream << std::fixed << cells[i].centerCoordY + 0.5 * cells[i].cellSize;
            sstream << "));" << cells[i].name;
            sstream <<  ";" << cells[i].outlet;
            sstream.precision(5);
            sstream <<  ";" << cells[i].area;
            sstream <<  ";" << cells[i].slope * 100.0; // convert fraction to percentage * 100.0
            sstream <<  ";" << cells[i].elevation;
            sstream <<  ";" << cells[i].landuse;
            sstream <<  ";" << cells[i].imperv;
            sstream.precision(3);
            sstream <<  ";" << cells[i].N_Imperv;
            sstream <<  ";" << cells[i].N_Perv;
            sstream.precision(2);
            sstream <<  ";" << cells[i].S_Imperv;
            sstream <<  ";" << cells[i].S_Perv;
            sstream <<  ";" << cells[i].Suction;
            sstream <<  ";" << cells[i].HydCon;
            sstream <<  ";" << cells[i].IMDmax;
            sstream <<  ";" << cells[i].isSink;
            sstream <<  ";" << cells[i].tag;
            sstream <<  ";" << cells[i].LIDType;

            polyId++;
        }
    }

    // Write files to disk.
    FileIO fileio;
    std::string path_csvt = path + ".csvt";
    path += ".wkt";
    int res = fileio.saveAsciiFile( path, sstream.str() );
    res = std::min(res, fileio.saveAsciiFile( path_csvt, sstream_csvt.str() ));

    return res;
}

void Grid::saveSubcatchmentRouting(std::string path, std::vector<int> cellIDs)
{
    std::stringstream sstream;
    std::stringstream sstream_csvt;
    sstream << std::fixed;

    sstream << "id;";
    sstream << "wkt;";      // Line object defining the route between "from" and "to" subcatchments
    sstream << "from;";     // Name of the origin subcatchment
    sstream << "to";        // Name of the target subcatchment

    // Create a .csvt file defining the field types of the .wkt file for ogr2ogr conversion to shapefile
    sstream_csvt << "Integer,";
    sstream_csvt << "WKT,";
    sstream_csvt << "String,";
    sstream_csvt << "String,";

    // Write polygon vertex coordinates.
    int lineId = 1;
    for (auto i : cellIDs)
    {
        if (cells[i].landuse != LANDUSE_NONE)
        {
            sstream << "\n" << lineId << ";LINESTRING(";
            sstream.precision(2);
            sstream << std::fixed << cells[i].centerCoordX << " ";
            sstream << std::fixed << cells[i].centerCoordY;
            sstream << ",";
            sstream << std::fixed << cells[i].outletCoordX << " ";
            sstream << std::fixed << cells[i].outletCoordY;
            sstream << ");" << cells[i].name;
            sstream <<  ";" << cells[i].outlet;
            lineId++;
        }
    }

    // Write files to disk.
    FileIO fileio;
    std::string path_csvt = path + ".csvt";
    path += ".wkt";
    int res = fileio.saveAsciiFile( path, sstream.str() );
    res = fileio.saveAsciiFile( path_csvt, sstream_csvt.str() );
}

void Grid::saveSubcatchmentRouting(std::string path)
{
    std::vector<int> ind(nCols*nRows);
    std::iota (std::begin(ind), std::end(ind), 0); // Fill with 0, 1, ..., (nCols*nROws-1)

    saveSubcatchmentRouting(path, ind);
}

void Grid::saveNetworkRouting(std::string path, Table &condTable)
{
    std::stringstream sstream;
    std::stringstream sstream_csvt;
    sstream << std::fixed;

    sstream << "id;";
    sstream << "wkt;";      // Line object defining the route between "from" and "to" junctions
    sstream << "name;";     // Name of the conduit
    sstream << "from;";     // Name of the origin junction
    sstream << "to";        // Name of the target junction

    // Create a .csvt file defining the field types of the .wkt file for ogr2ogr conversion to shapefile
    sstream_csvt << "Integer,";
    sstream_csvt << "WKT,";
    sstream_csvt << "String,";
    sstream_csvt << "String,";
    sstream_csvt << "String,";

    // Write polyline vertex coordinates.
    int lineId = 0;
    for (int i = 1; i < condTable.nRows; i++)
    {
        sstream << "\n" << lineId << ";LINESTRING(";
        sstream.precision(2);
        sstream << std::fixed << condTable.getData(i, 0) << " ";
        sstream << std::fixed << condTable.getData(i, 1);
        sstream << ",";
        sstream << std::fixed << condTable.getData(i, 2) << " ";
        sstream << std::fixed << condTable.getData(i, 3);
        sstream << ");" << condTable.getData(i, 4);
        sstream <<  ";" << condTable.getData(i, 8);
        sstream <<  ";" << condTable.getData(i, 9);
        lineId++;
    }

    // Write files to disk.
    FileIO fileio;
    std::string path_csvt = path + "_network_routing.csvt";
    path += "_network_routing.wkt";
    int res = fileio.saveAsciiFile( path, sstream.str() );
    res = fileio.saveAsciiFile( path_csvt, sstream_csvt.str() );
}

void Grid::saveSWMM5File(Table &headerTable, Table &catchPropTable, Table &evaporationTable, Table &temperatureTable,
                         Table &inflowsTable, Table &timeseriesTable, Table &reportTable, Table &snowpacksTable,
                         Table &raingagesTable, Table &symbolsTable, Table &juncTable, Table &outfallsTable,
                         Table &condTable, Table &pumpsTable, Table &pumpCurvesTable, Table &dwfTable, Table &patternsTable,
                         Table &lossesTable, Table &storageTable, Table &xsectionTable, Table &lidControlsTable, std::string path)
{
    std::stringstream sstream;
    sstream << std::fixed;

    // If the output file is a PEST template file, write an identifier (ptf #) into the begining of the file
    if (path.substr ((int)path.length() - 3, 3) == "TPL")
    {
        std::cout << "\n-> test print: " << path.substr ((int)path.length() - 3, 3);
        sstream << "ptf #\n";
    }

    // Write the header table.
    std::cout << "\n-> Writing header table";
    sstream << "[TITLE]";
    sstream << "\n;;Project Title/Notes";
    sstream << "\n;;Generated with the GisToSWMM5 program";
    sstream << "\n";
    sstream << "\n[OPTIONS]";
    sstream << "\n;Option Value";
    headerTable.writeToStringStream(sstream);
    sstream << "\n";

    // Write the evaporation table.
    std::cout << "\n-> Writing evaporation table";
    sstream << "\n[EVAPORATION]";
    sstream << "\n;;Evap Data      Parameters";
    sstream << "\n;;-------------- ----------------";
    evaporationTable.writeToStringStream(sstream);
    sstream << "\n";

    // Write the temperature table.
    std::cout << "\n-> Writing temperature table";
    sstream << "\n[TEMPERATURE]";
    sstream << "\n;;Temp/Wind/Snow   Source/Data";
    temperatureTable.writeToStringStream(sstream);
    sstream << "\n";

    // Write raingage properties.
    std::cout << "\n-> Writing raingages";
    sstream << "\n[RAINGAGES]";
    sstream << "\n;;Gage           Format    Interval SCF      Source";
    sstream << "\n;;-------------- --------- ------ ------ ----------";
    raingagesTable.writeToStringStream(sstream);
    sstream << "\n";

    // Write subcatchment properties.
    std::cout << "\n-> Writing subcatchment properties";
    sstream << "\n[SUBCATCHMENTS]";
    sstream << "\n;;Subcatchment   Rain Gage        Outlet           Area     %Imperv  Width    %Slope   CurbLen  Snow Pack       ";
    sstream << "\n;;-------------- ---------------- ---------------- -------- -------- -------- -------- -------- ----------------";

    for (int i = 0; i < nRows * nCols; i++)
    {
        if (cells[i].landuse != LANDUSE_NONE)
        {
            sstream << "\n;" << cells[i].landuse;
            sstream << "\n" << cells[i].name << "   ";
            sstream << cells[i].raingage << "   ";
            sstream << cells[i].outlet << "   ";
            sstream.precision(5);
            sstream << cells[i].area / 10000.0 << "   "; // convert m2 to ha
            sstream.precision(2);
            sstream << cells[i].imperv << "   ";
            sstream << cells[i].flowWidth << "   ";
            sstream.precision(4);
            sstream << cells[i].slope * 100.0 << "   "; // convert fraction to percentage * 100.0
            sstream.precision(2);
            sstream << cells[i].cellSize << "   ";
            sstream << cells[i].snowPack << "   ";
        }
    }

    sstream << "\n";

    // Write subarea properties.
    std::cout << "\n-> Writing subarea properties";
    sstream << "\n[SUBAREAS]";
    sstream << "\n;;Subcatchment   N-Imperv   N-Perv     S-Imperv   S-Perv     PctZero    RouteTo    PctRouted ";
    sstream << "\n;;-------------- ---------- ---------- ---------- ---------- ---------- ---------- ----------";

    for (int i = 0; i < nRows * nCols; i++)
    {
        if (cells[i].landuse != LANDUSE_NONE)
        {
            sstream << "\n" << cells[i].name << "   ";
            //sstream.precision(3);
            sstream << cells[i].N_Imperv << "   ";
            sstream << cells[i].N_Perv << "   ";
            //sstream.precision(2);
            sstream << cells[i].S_Imperv << "   ";
            sstream << cells[i].S_Perv << "   ";
            sstream << cells[i].PctZero << "   ";
            sstream << cells[i].RouteTo << "   ";
            sstream << cells[i].PctRouted << "   ";
        }
    }

    sstream << "\n";

    // Write infiltration properties.
    std::cout << "\n-> Writing infiltration properties";
    sstream << "\n[INFILTRATION]";
    sstream << "\n;;Subcatchment   Suction    HydCon     IMDmax    ";
    sstream << "\n;;-------------- ---------- ---------- ----------";

    for (int i = 0; i < nRows * nCols; i++)
    {
        if (cells[i].landuse != LANDUSE_NONE)
        {
            sstream << "\n" << cells[i].name << "   ";
            sstream << cells[i].Suction << "   ";
            sstream << cells[i].HydCon << "   ";
            sstream << cells[i].IMDmax << "   ";
        }
    }

    sstream << "\n\n";

    // Write the LID controls table.
    std::cout << "\n-> Writing LID controls table.";
    
    std::string line,filePath;
    std::ifstream inp_file;
    std::cout << " Loading data from lid controls file path";
    filePath = lidControlsTable.getData(0,0);
    inp_file.open(filePath);
    
    if(inp_file){
        while(getline(inp_file,line)){
            sstream << line << "\n";
        }
    }else {
        printf("\nCannot read LID controls file");
    }

    inp_file.close();

    sstream << "\n";


    // Write the LID Usage properties.
    std::cout << "\n-> Writing the LID Usage properties";
    sstream << "\n[LID_USAGE]";
    sstream << "\n;;Subcatchment   LID Process      Number  Area       Width      InitSat    FromImp    ToPerv     RptFile                  DrainTo  ";
    sstream << "\n;;-------------- ---------------- ------- ---------- ---------- ---------- ---------- ---------- ------------------------ ----------------";

    for (int i = 0; i < nRows * nCols; i++)
    {
        if (cells[i].LIDType != "NONE")
        {
            std::string lid = cells[i].LIDType.c_str();
            lid = lid.substr(0,lid.size()-2);
            
            sstream << "\n" << cells[i].name << "   ";
            sstream << cells[i].LIDType << "   ";
            sstream << "1" << "   ";
            sstream << cells[i].area << "   ";
            if (lid == "PP" || lid == "GR"){
                sstream << cells[i].flowWidth << "   ";
            }else{
                sstream << "0" << "   ";
            }
            sstream << "0" << "   ";
            sstream << "0" << "   ";
            sstream << "0" << "   ";
            sstream << "*" << "   ";
            sstream << "*" << "   ";
        }
    }

    sstream << "\n";

    // Write the snowpacks table.
    std::cout << "\n-> Writing snowpacks table";
    sstream << "\n[SNOWPACKS]";
    sstream << "\n;;Name           Surface    Parameters";
    sstream << "\n;;-------------- ---------- ----------";
    snowpacksTable.writeToStringStream(sstream);
    sstream << "\n";

    // Write junction properties.
    std::cout << "\n-> Writing junction properties";
    sstream << "\n[JUNCTIONS]";
    sstream << "\n;;Junction       Invert     MaxDepth   InitDepth  SurDepth   Aponded   ";
    sstream << "\n;;-------------- ---------- ---------- ---------- ---------- ----------";

    for (int i = 1; i < juncTable.nRows; i++)
    {
        sstream << "\n" << juncTable.getData(i, 2) << "   ";        // Junction name
        sstream << juncTable.getData(i, 4) << "   ";                // Invert elevation
        sstream << juncTable.getData(i, 5) << "   ";                // Max depth
        sstream << juncTable.getData(i, 7) << "   ";                // Initial depth
        sstream << juncTable.getData(i, 8) << "   ";                // Surcharge dept
        sstream << juncTable.getData(i, 9) << "   ";                // Ponded area
    }

    sstream << "\n";

    // Write outfall properties.
    std::cout << "\n-> Writing outfalls properties";
    sstream << "\n[OUTFALLS]";
    sstream << "\n;;Outfall        Invert     Type       Stage Data       Gated   ";
    sstream << "\n;;-------------- ---------- ---------- ---------------- --------";

    for (int i = 1; i < outfallsTable.nRows; i++)
    {
        sstream << "\n" << outfallsTable.data[i * outfallsTable.nCols + 2] << "   ";    // Outfall name
        sstream << outfallsTable.data[i * outfallsTable.nCols + 3] << "   ";            // Invert height
        sstream << outfallsTable.data[i * outfallsTable.nCols + 4] << "   ";            // Type
        sstream << outfallsTable.data[i * outfallsTable.nCols + 5] << "   ";            // Stage data
        sstream << outfallsTable.data[i * outfallsTable.nCols + 6] << "   ";            // Gated
    }

    sstream << "\n";

    // Write storage properties.
    // TODO: This is outdated formatting and needs to be updated
    std::cout << "\n-> Writing storage properties";
    sstream << "\n[STORAGE]";
    sstream << "\n;;Name           Elev.    MaxDepth   InitDepth  Shape      Curve Name/Params                     Fevap    Psi      Ksat     IMD    ";
    sstream << "\n;;-------------- -------- ---------- ----------- ---------- ---------------------------- -------- --------          -------- --------";

    for (int i = 1; i < storageTable.nRows; i++)
    {
        sstream << "\n" << storageTable.getData(i, 2) << "   ";
        sstream << storageTable.getData(i, 3) << "   ";
        sstream << storageTable.getData(i, 4) << "   ";
        sstream << storageTable.getData(i, 5) << "   ";
        sstream << storageTable.getData(i, 6) << "   ";
        sstream << storageTable.getData(i, 7) << "   ";
        sstream << storageTable.getData(i, 8) << "   ";
    }

    sstream << "\n";

    // Write conduits properties.
    std::cout << "\n-> Writing conduits properties";
    sstream << "\n[CONDUITS]";
    sstream << "\n;;Conduit        From Node        To Node          Length     Roughness  InOffset   OutOffset  InitFlow   MaxFlow   ";
    sstream << "\n;;-------------- ---------------- ---------------- ---------- ---------- ---------- ---------- ---------- ----------";

    for (int i = 1; i < condTable.nRows; i++)
    {
        sstream << "\n" << condTable.data[i * condTable.nCols + 4] << "   ";    // Conduit name
        sstream << condTable.data[i * condTable.nCols + 8] << "   ";            // From node
        sstream << condTable.data[i * condTable.nCols + 9] << "   ";            // To node
        sstream << condTable.data[i * condTable.nCols + 7] << "   ";            // Length
        sstream << condTable.data[i * condTable.nCols + 10] << "   ";           // roughness
        sstream << condTable.data[i * condTable.nCols + 11] << "   ";           // InOffset
        sstream << condTable.data[i * condTable.nCols + 12] << "   ";           // OutOffset
        sstream << 0.0 << "   ";        // InitFlow
        sstream << 0.0 << "   ";        // MaxFlow
    }

    sstream << "\n";

    // Write pump properties.
    std::cout << "\n-> Writing pumps";
    sstream << "\n[PUMPS]";
    sstream << "\n;;Name	From node	To node	Pump curve	Status	Startup	Shutoff";
    sstream << "\n;;------	------	------	------	------	------	------";
    pumpsTable.writeToStringStream(sstream);
    sstream << "\n";

    // Write conduit cross section properties.
    std::cout << "\n-> Writing conduit cross section properties";
    sstream << "\n[XSECTIONS]";
    sstream << "\n;;Link           Shape        Geom1            Geom2      Geom3      Geom4      Barrels    Culvert   ";
    sstream << "\n;;-------------- ------------ ---------------- ---------- ---------- ---------- ---------- ----------";
    xsectionTable.writeToStringStream(sstream);
    sstream << "\n";

    // Write losses table.
    std::cout << "\n-> Writing losses table";
    sstream << "\n[LOSSES]";
    sstream << "\n;;Link           Kentry     Kexit      Kavg       Flap Gate  Seepage   ";
    sstream << "\n;;-------------- ---------- ---------- ---------- ---------- ----------";
    lossesTable.writeToStringStream(sstream);
    sstream << "\n";

    // Write pump curve properties.
    std::cout << "\n-> Writing pump curves";
    sstream << "\n[CURVES]";
    sstream << "\n;;Name	Type	X-value	Y-value";
    sstream << "\n;;------	------	------	------";
    pumpCurvesTable.writeToStringStream(sstream);
    sstream << "\n";

    // Write inflows table.
    std::cout << "\n-> Writing inflows table";
    sstream << "\n[INFLOWS]";
    sstream << "\n;;Node           Constituent      Time Series      Type     Mfactor  Sfactor  Baseline Pattern";
    sstream << "\n;;-------------- ---------------- ---------------- -------- -------- -------- -------- --------";
    inflowsTable.writeToStringStream(sstream);
    sstream << "\n";

    // Write time series table.
    std::cout << "\n-> Writing time series table";
    sstream << "\n[TIMESERIES]";
    sstream << "\n;;Name           Date       Time       Value";
    sstream << "\n;;-------------- ---------- ---------- ----------";
    timeseriesTable.writeToStringStream(sstream);
    sstream << "\n";

    // Write dry weather flow table.
    std::cout << "\n-> Writing dry weather flow table";
    sstream << "\n[DWF]";
    sstream << "\n;;Node           Constituent      Baseline   Patterns ";
    sstream << "\n;;-------------- ---------------- ---------- ----------";
    dwfTable.writeToStringStream(sstream);
    sstream << "\n";

    // Write patterns table.
    std::cout << "\n-> Writing patterns table";
    sstream << "\n[PATTERNS]";
    sstream << "\n;;Name           Type       Multipliers";
    sstream << "\n;;-------------- ---------- -----------";
    patternsTable.writeToStringStream(sstream);
    sstream << "\n";

    // Write reporting settings.
    std::cout << "\n-> Writing reporting settings";
    sstream << "\n[REPORT]";
    sstream << "\n;;Reporting Options";
    reportTable.writeToStringStream(sstream);
    sstream << "\n";

    // Write tags.
    sstream << "\n[TAGS]";
    if (catchPropTable.nCols > 12)
    {
        for (int i = 0; i < nRows * nCols; i++)
        {
            if (cells[i].landuse != LANDUSE_NONE)
            {
                sstream << "\nSubcatch   " << cells[i].name ;
                sstream << "   " << cells[i].tag;
            }
        }
    }
    sstream << "\n";

    // Write map settings.
    std::cout << "\n-> Writing map settings";
    sstream << "\n[MAP]";
    sstream << "\nDIMENSIONS ";
    sstream.precision(2);
    sstream << llcorner[0] * 0.99999 << " ";
    sstream << llcorner[1] * 0.99999 << " ";
    sstream << urcorner[0] * 1.00001 << " ";
    sstream << urcorner[1] * 1.00001;
    sstream << "\nUnits      Meters";
    sstream << "\n";

    // Write node coordinates.
    std::cout << "\n-> Writing node coordinates";
    sstream << "\n[COORDINATES]";
    sstream << "\n;;Node           X-Coord            Y-Coord           ";
    sstream << "\n;;-------------- ------------------ ------------------";

    for (int i = 1; i < juncTable.nRows; i++)
    {
        sstream << "\n" << juncTable.data[i * juncTable.nCols + 2] << "   ";
        sstream << juncTable.data[i * juncTable.nCols + 0] << "   ";
        sstream << juncTable.data[i * juncTable.nCols + 1] << "   ";
    }

    for (int i = 1; i < outfallsTable.nRows; i++)
    {
        sstream << "\n" << outfallsTable.data[i * outfallsTable.nCols + 2] << "   ";
        sstream << outfallsTable.data[i * outfallsTable.nCols] << "   ";
        sstream << outfallsTable.data[i * outfallsTable.nCols + 1] << "   ";
    }

    for (int i = 1; i < storageTable.nRows; i++)
    {
        sstream << "\n" << storageTable.getData(i, 2) << "   ";
        sstream << storageTable.getData(i, 0) << "   ";
        sstream << storageTable.getData(i, 1) << "   ";
    }

    sstream << "\n";
    sstream << "\n[VERTICES]";
    sstream << "\n;;Link           X-Coord            Y-Coord           ";
    sstream << "\n;;-------------- ------------------ ------------------";
    sstream << "\n";

    // Write polygon vertex coordinates.
    std::cout << "\n-> Writing polygon vertex coordinates";
    sstream << "\n[Polygons]";
    sstream << "\n;;Subcatchment   X-Coord            Y-Coord           ";
    sstream << "\n;;-------------- ------------------ ------------------";

    for (int i = 0; i < nRows * nCols; i++)
    {
        if (cells[i].landuse != LANDUSE_NONE)
        {
            sstream.precision(2);
            sstream << "\n" << cells[i].name << "   ";
            sstream << std::fixed << cells[i].centerCoordX - 0.5 * cells[i].cellSize << "   ";
            sstream << std::fixed << cells[i].centerCoordY + 0.5 * cells[i].cellSize;
            sstream << "\n" << cells[i].name << "   ";
            sstream << std::fixed << cells[i].centerCoordX - 0.5 * cells[i].cellSize << "   ";
            sstream << std::fixed << cells[i].centerCoordY - 0.5 * cells[i].cellSize;
            sstream << "\n" << cells[i].name << "   ";
            sstream << std::fixed << cells[i].centerCoordX + 0.5 * cells[i].cellSize << "   ";
            sstream << std::fixed << cells[i].centerCoordY - 0.5 * cells[i].cellSize;
            sstream << "\n" << cells[i].name << "   ";
            sstream << std::fixed << cells[i].centerCoordX + 0.5 * cells[i].cellSize << "   ";
            sstream << std::fixed << cells[i].centerCoordY + 0.5 * cells[i].cellSize;
            sstream << "\n" << cells[i].name << "   ";
            sstream << std::fixed << cells[i].centerCoordX - 0.5 * cells[i].cellSize << "   ";
            sstream << std::fixed << cells[i].centerCoordY + 0.5 * cells[i].cellSize;
        }
    }

    sstream << "\n";

    // Write symbols.
    std::cout << "\n-> Writing symbols";
    sstream << "\n[SYMBOLS]";
    sstream << "\n;;Gage           X-Coord            Y-Coord";
    sstream << "\n;;-------------- ------------------ ------------------";
    symbolsTable.writeToStringStream(sstream);
    sstream << "\n";
    sstream << "\n";

    // Write the file to disk.
    std::cout << "\n-> Writing the file to disk";
    FileIO fileio;
    path += ".inp";
    int res = fileio.saveAsciiFile( path, sstream.str() );
}

void Grid::printReport(Table &catchPropTable, std::vector<int> routedIDs)
{
    double elevationAverage = 0.0;
    double slopeAverage = 0.0;
    int numOfLandUseClasses = catchPropTable.nRows - 1;

    if (numOfLandUseClasses < 0)
    {
        numOfLandUseClasses = 0;
    }

    std::vector<int> cellsInlandUseClass;
    cellsInlandUseClass.assign(numOfLandUseClasses, 0);
    std::vector<int> landUseClassIds;
    landUseClassIds.assign(numOfLandUseClasses, 0);
    std::vector<double> areasInlandUseClass;
    areasInlandUseClass.assign(numOfLandUseClasses, 0.0);

    for (int i = 0; i < (int)landUseClassIds.size(); i++)
    {
        landUseClassIds[i] = atoi(catchPropTable.data[(i + 1) * catchPropTable.nCols].c_str());
    }

    int numOfActiveCells = 0;
    double areaOfActiveCellsNoRoofs = 0.0;

    for (auto i : routedIDs)
    {
        if (cells[i].landuse != LANDUSE_NONE)
        {
            numOfActiveCells += 1;

            // Don't use roofs and noData areas for catchment mean slope and elevation calculations
            if ((cells[i].landuse >= BUILT_AREA) && cells[i].elevation > cells[i].elevNoData)
            {
                elevationAverage += cells[i].elevation * cells[i].area;
                slopeAverage += cells[i].slope * cells[i].area;
                areaOfActiveCellsNoRoofs += cells[i].area;
            }

            for (int j = 0; j < (int)landUseClassIds.size(); j++)
            {
                if (landUseClassIds[j] == cells[i].landuse)
                {
                    cellsInlandUseClass[j] += 1;
                    areasInlandUseClass[j] += cells[i].area;
                    break;
                }
            }
        }
    }

    double area = 0.0;

    for (int i = 0; i < numOfLandUseClasses; i++)
    {
        area += areasInlandUseClass[i];
    }

    if (areaOfActiveCellsNoRoofs > 0.0)
    {
        elevationAverage /= areaOfActiveCellsNoRoofs;
        slopeAverage /= areaOfActiveCellsNoRoofs;
    }

    std::cout << "\n-> Total number of cells: " << nCols * nRows;
    std::cout << "\n-> Number of active cells: " << numOfActiveCells;
    std::cout << "\n-> Catchment area: " << area / 10000.0 << " ha";
    std::cout << "\n-> Catchment average elevation: " << elevationAverage << " m";
    std::cout << "\n-> Catchment average slope: " << slopeAverage;
    std::cout << "\n-> Landuse information (code, number of cells, area in ha, % of catchment):";

    for (int i = 0; i < (int)cellsInlandUseClass.size(); i++)
    {
        std::cout << "\n"
                  << "\t" << landUseClassIds[i]
                  << "\t" << cellsInlandUseClass[i]
                  << "\t" << areasInlandUseClass[i] / 10000.0
                  << "\t" << areasInlandUseClass[i] / area * 100.0;
    }
}

void Grid::printReport(Table &catchPropTable)
{
    std::vector<int> ind(nCols*nRows);
    std::iota (std::begin(ind), std::end(ind), 0); // Fill with 0, 1, ..., (nCols*nROws-1)

    printReport(catchPropTable, ind);
}

