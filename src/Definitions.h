#define LANDUSE_NONE 0
// Old codes before 8 March 2018
//#define LANDUSE_BEDROCK 10
//#define LANDUSE_ROOF_CONNECTED 20
//#define LANDUSE_ROOF_UNCONNECTED 21
//#define LANDUSE_SAND 30
//#define LANDUSE_ASPHALT_STREET 40
//#define LANDUSE_TILES 50
//#define LANDUSE_VEGETATION 60
//#define LANDUSE_WATER 70
// New codes after 8 March 2018
#define ROOF_CONNECTED 10
#define ROOF_UNCONNECTED 20 // Has to be greater than ROOF_CONNECTED
#define BUILT_AREA 30       // Has to be greater than ROOF_UNCONNECTED
#define NATURAL_AREA 60     // Has to be greater than BUILT_AREA

#define MIN_WATER_USE 65    // For different water usages in the arealike river, lakes, canals, ponds etc.
#define MAX_WATER_USE 69    // Land uses (65,66,67,68,69) would not be connected to a junction