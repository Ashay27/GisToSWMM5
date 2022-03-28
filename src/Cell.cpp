#include "Cell.h"

Cell::Cell()
{
	// Initialize variables.
	name = "empty";
	centerCoordX = 0.0;
	centerCoordY = 0.0;
	elevation = 0.0;
	flowdir = -1;
	cellSize = 0.0;
	slope = 0.0;
	area = 0.0;
	flowWidth = 0.0;
	landuse = 0;
	outletCoordX = 0.0;
	outletCoordY = 0.0;
	outletID = -1;
	subcatchmentID = -1;
	outlet = "*";
	raingage = "r1";
	imperv = "25.0";
	snowPack = "";
	N_Imperv = "0.01";
	N_Perv = "0.1";
	S_Imperv = "0.05";
	S_Perv = "0.05";
	PctZero = "0.0";
	RouteTo = "OUTLET";
	PctRouted = "100.0";
	Suction = "3.5";
	HydCon = "0.5";
	IMDmax = "0.25";
	isSink = 0;         // By default all cells are routed
	numElements = 0;
	hasInlet = 0;
	elevNoData = 0;
	tag = "None";
	LIDType = "NONE";

	//for (int i = 0; i < 8; i++)
	//{
	//	neighCellIndices[i] = -1;
	//}
	//flowDirection = -1;
}

Cell::~Cell()
{

}

