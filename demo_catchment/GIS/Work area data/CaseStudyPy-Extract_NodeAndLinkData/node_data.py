#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Apr 14 11:27:27 2020

@author: adi

File contains the script to build the data required to be passed to the pyINP
class SWMM model builder for building the model of the Rivierenbuurt.

This script compiles the nodes (manhole junctions) of the sewer system.

"""

import pandas as pd
import geopandas as gpd
import numpy as np
import rasterio as rio

from shapely.ops import nearest_points

# Import the .shp file of the riol CSRiool.shp and buurt
fp_riool = 'data/test/pipedata_234bottom_296top.shp'
data_riool = gpd.read_file(fp_riool)

fp_buurt = 'data/test/Working_area_final_clipped.shp'
buurt_bound = gpd.read_file(fp_buurt)

# File path for the cropped raster of the Riverenbuurt
fp_brt = 'data/test/raster_dem_exact_area2.tif'

# %% Cropping down the riool data to the area of the Rivierenbuurt with a buffer

# Make a spatial join to select the system contents within the bondary
temp = buurt_bound.copy()
temp['geometry'] = buurt_bound.geometry.buffer(20)
buurt_riool = gpd.sjoin(data_riool, temp, how="inner", op="within")

# Delete all temporary variables
del temp

# %% Bulding new riool dataframe with the required information
# Naming the new sewer pipe name as object name OBJNAAM
objn = []
typedict = {'Gemengd': 'CMB', 'DWA-Riool': 'DSF', 'Transportriool': 'TRR'}
for idx in buurt_riool.index.tolist():
    #sd = buurt_riool.at[idx, 'SD']
    #bc = buurt_riool.at[idx, 'BC']
    nt = str(idx)
    ty = typedict[buurt_riool.at[idx, 'TYPELEIDIN']]
    objn.append(ty+nt)

buurt_riool['OBJNAAM'] = objn

# Selecting on the required data columns from buurt_riool to new dataframe riool
riool = buurt_riool[['OBJNAAM', 'TYPELEIDIN', 'BEGINPUNT', 'EINDPUNT',
                     'BOB_BEGINP', 'BOB_EINDPU', 'LENGTE_M', 'BR_DIAMETE',
                     'HOOGTE_MM', 'VORM', 'MATERIAAL', 'geometry']]

# Create the dictionary with old and new names
new_col = {'TYPELEIDIN': 'PIPETYPE', 'BOB_EINDPU': 'BOB_ENDP',
             'LENGTE_M': 'LENGTH_M', 'BR_DIAMETE': 'BR_DIA_MM',
             'HOOGTE_MM': 'HEIGHT_MM', 'MATERIAAL': 'MATERIAL'}

# Rename the columns
riool = riool.rename(columns = new_col)

# Selecting the part of riool that is either 'Gemengd' or 'Transportriool'

# riool = riool[(riool['PIPETYPE']=='Gemengd') | (riool['PIPETYPE']== 'Transportriool')]

# Change the CRS of the riool that is as required as pySWMM

# Delete all temporary variables
del objn, nt, ty, new_col

# %% Extracting the required node info from the riool dataframe

riool['BEG_X'] = np.nan
riool['BEG_Y'] = np.nan
riool['END_X'] = np.nan
riool['END_Y'] = np.nan
riool['LEN'] = np.nan

for idx in riool.index.tolist():
    # Exracting the LineString into its coordinate Tuple
    lp = list(riool.at[idx, 'geometry'].coords)

    # Extracing the X,Y components from the coordinate tuple [BEG/END][X/Y]
    riool.at[idx, 'BEG_X'] = lp[0][0]
    riool.at[idx, 'BEG_Y'] = lp[0][1]
    riool.at[idx, 'END_X'] = lp[-1][0]
    riool.at[idx, 'END_Y'] = lp[-1][1]
    riool.at[idx, 'LEN'] = len(lp)

# # Check for Sewer pipes connections that have more pipes attached to the
# # nodes mentioned. After check it, it is found that these connections have
# # minor connections to them. So therefore the end and geing points are
# # sufficient to specify these connections and their middle points can
# # be ignored.
# riool_g3 = riool[riool.LEN > 2]
# riool_g3.to_file('checkriool.shp')

# Extracting all the Begin and Eind punten for all the nodes
nodes = riool.BEGINPUNT
nodes = nodes.append(riool.EINDPUNT)
node_id = nodes.unique() # All the Node Ids

# Delete all temporary variables
# del lp, nodes

# %% Parsing the information of the nodes as required by pySWMM
node_info = pd.DataFrame({'Name':node_id})
node_info['X_Coord'] = np.nan
node_info['Y_Coord'] = np.nan

# Obtaining the elevation from Spatial indexing of the node_info data from the raster file
node_info['elevation'] = np.nan # Absolute elevation of the manhole cover
node_info['inv_elev'] = np.nan # Invert elevation, BOB
node_info['MaxDepth'] = np.nan # Invert elevation

# Open raster data
brt_rstr = rio.open(fp_brt)
brt_band = brt_rstr.read(1, masked=True)

for idx in node_info.index.tolist():
    nid = node_info.at[idx, 'Name']

    # Extracting the coordinates
    xcrdall = riool.loc[riool['BEGINPUNT'] == nid , ['BEG_X']].BEG_X
    xcrdalle = riool.loc[riool['EINDPUNT'] == nid , ['END_X']].END_X
    xcrdall = xcrdall.append(xcrdalle)
    xcrd = xcrdall.unique().tolist()[0]
    node_info.at[idx, 'X_Coord'] = xcrd

    ycrdall = riool.loc[riool['BEGINPUNT'] == nid , ['BEG_Y']].BEG_Y
    ycrdalle = riool.loc[riool['EINDPUNT'] == nid , ['END_Y']].END_Y
    ycrdall = ycrdall.append(ycrdalle)
    ycrd = ycrdall.unique().tolist()[0]
    node_info.at[idx, 'Y_Coord'] = ycrd

    # Spatial indexing obejct .index produces (row, col) index value for the coordinate
    node_info.at[idx, 'elevation'] = brt_band[brt_rstr.index(xcrd, ycrd)]

    # Extracting the invert elevation BOB:Binnenonderkant buis
    # Hoogteligging t.o.v. NAP van de binnen-onderkant-buis van de leiding
    # van https://data.gwsw.nl/binnenonderkantbuis/
    boball = riool.loc[riool['BEGINPUNT'] == nid , ['BOB_BEGINP']].BOB_BEGINP
    boballe = riool.loc[riool['EINDPUNT'] == nid , ['BOB_ENDP']].BOB_ENDP
    boball = boball.append(boballe)
    node_info.at[idx, 'inv_elev'] = boball.min()

    # Calculating the Max depth from the ground surface till the invert elevation
    node_info.at[idx, 'MaxDepth'] = node_info.at[idx, 'elevation'] - \
        node_info.at[idx, 'inv_elev']

node_info = gpd.GeoDataFrame(node_info, geometry = \
                      gpd.points_from_xy(node_info.X_Coord, node_info.Y_Coord))

node_info.crs = riool.crs

# Close the connection to the raster data
brt_rstr.close()

# Delete all temporary variables
del idx, nid, xcrdall, xcrdalle, xcrd, ycrdall, ycrdalle, ycrd, boball, boballe

# %% Check for NaN in node_info
'''Check for NaN values in the columns and update the MaxDepth with the
nearest value'''

# all_node_n = node_info[node_info.MaxDepth.notna()].geometry.unary_union
# for idx in node_info[node_info.MaxDepth.isna()].index.tolist():
#     nrst_pt = nearest_points(node_info.at[idx, 'geometry'], all_node_n)[1]
#     node_info.at[idx, 'MaxDepth'] = node_info[node_info.geometry == nrst_pt].MaxDepth
#
# del all_node_n, idx, nrst_pt

# # %% Check for Outfalls and Junctions
#
# link_info = pd.read_pickle('data/link_info.pkl')
#
# dropnodeidx = []
#
# # Remove nodes that do not have a link
# for idx, row in node_info.iterrows():
#     fromLinks = link_info.loc[link_info.FromNode == row.Name]
#     toLinks = link_info.loc[link_info.ToNode == row.Name]
#     numLinks = len(fromLinks) + len(toLinks)
#     if numLinks < 1:
#         dropnodeidx.append(idx)
#
# node_info = node_info.drop(dropnodeidx)

# del dropnodeidx, idx, row, fromLinks, toLinks, numLinks

# %%

node_info['Type'] = 'JUNCTION'

# for idx, row in node_info.iterrows():
#     if buurt_bound.geometry[0].contains(row.geometry):
#         node_info.at[idx, 'Type'] = 'JUNCTION'
#     else:
#         fromLinks = link_info.loc[link_info.FromNode == row.Name]
#         toLinks = link_info.loc[link_info.ToNode == row.Name]
#         numLinks = len(fromLinks) + len(toLinks)
#         if numLinks < 2:
#             node_info.at[idx, 'Type'] = 'OUTFALL'
#         else:
#             node_info.at[idx, 'Type'] = 'JUNCTION'
#
# del idx, row, fromLinks, toLinks, numLinks

# %% Update some nodes to be OUTFALL

outfallidxadd = ['L34038', 'M39280', 'M34101', 'M34058', 'M18003']
for rowid in outfallidxadd:
    node_info.loc[node_info.Name == rowid, 'Type'] = 'OUTFALL'


# %% Saving the node_info dataframe file as pkl file

node_info.to_pickle('data/test/output/node_info.pkl')

node_info.to_file('data/test/output/nodes_withoutfall.shp')

# node_info.loc[node_info.Type == 'OUTFALL'].to_file('/Users/adi/PyWork/Hydro_GIS/SWMMnpy/B2T1/data/subareas_shp/outfall.shp')
