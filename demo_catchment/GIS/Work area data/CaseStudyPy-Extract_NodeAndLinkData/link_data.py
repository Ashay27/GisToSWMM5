#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Apr 14 11:27:27 2020

@author: adi

File contains the script to build the data required to be passed to the pyINP
class SWMM model builder for building the model of the Rivierenbuurt.

This script compiles the links (sewer pipes) of the sewer system.
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
del temp, data_riool

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
del idx, objn, nt, ty, new_col, typedict


# %% Parsing the information of the nodes as required by pySWMM
link_info = gpd.GeoDataFrame({'Name': riool.OBJNAAM, 'FromNode': riool.BEGINPUNT,
                          'ToNode': riool.EINDPUNT, 'Length': riool.LENGTH_M,
                          'geometry': riool.geometry})

link_info.crs = riool.crs

# Adding roughness value
rough_val = {'Beton': 0.016, 'Ultra-Rib': 0.012, 'Ultra 3': 0.012,
             'PVC  SDR 34  PN 7.5': 0.012, 'Gietijzer': 0.014,
             'Metselwerk': 0.017, 'G.V.K.': 0.012}

link_info['Roughness'] = riool['MATERIAL'].map(rough_val)

# Adding Shape type
shape_type = {'Rond': 'CIRCULAR', 'Rechthoekig': 'RECT_CLOSED',
              'Eivormig': 'EGG'}

link_info['Shape'] = riool['VORM'].map(shape_type)

link_info['InOffset'] = np.nan
link_info['OutOffset'] = np.nan

link_info['Geom1'] = 0.0
link_info['Geom2'] = 0.0
link_info['Geom3'] = 0.0
link_info['Geom4'] = 0.0

link_info['x0'] = 0.0
link_info['y0'] = 0.0
link_info['x1'] = 0.0
link_info['y1'] = 0.0

link_info['elev_in'] = 0.0
link_info['elev_out'] = 0.0

node_info = pd.read_pickle('data/test/output/node_info.pkl')

for idx, row in link_info.iterrows():

    # Assessing the from node information
    fn_el = node_info.loc[node_info['Name'] == row['FromNode'],
                          'inv_elev'].tolist()[0] # From node elevation
    bp_el = riool.loc[riool['OBJNAAM'] == row['Name'],
                      'BOB_BEGINP'].tolist()[0] # Begin punt elevation

    if fn_el < bp_el: link_info.at[idx, 'InOffset'] = bp_el -fn_el
    else: link_info.at[idx, 'InOffset'] = 0

    # Assessing the to node information
    tn_el = node_info.loc[node_info['Name'] == row['ToNode'],
                          'inv_elev'].tolist()[0] # From node elevation
    ep_el = riool.loc[riool['OBJNAAM'] == row['Name'],
                      'BOB_ENDP'].tolist()[0] # Eind punt elevation

    if tn_el < ep_el: link_info.at[idx, 'OutOffset'] = ep_el - tn_el
    else: link_info.at[idx, 'OutOffset'] = 0

    # Assessing the Geom values
    if (row['Shape'] == 'CIRCULAR'):
        link_info.at[idx, 'Geom1'] = riool.loc[riool['OBJNAAM'] == row['Name'],
                                               'BR_DIA_MM'].tolist()[0]/1000

    if (row['Shape'] == 'RECT_CLOSED'):
        link_info.at[idx, 'Geom1'] = riool.loc[riool['OBJNAAM'] == row['Name'],
                                               'HEIGHT_MM'].tolist()[0]/1000
        link_info.at[idx, 'Geom2'] = riool.loc[riool['OBJNAAM'] == row['Name'],
                                               'BR_DIA_MM'].tolist()[0]/1000

    if (row['Shape'] == 'EGG'):
        link_info.at[idx, 'Geom1'] = riool.loc[riool['OBJNAAM'] == row['Name'],
                                               'HEIGHT_MM'].tolist()[0]/1000

    link_info.at[idx, 'x0'] = node_info.loc[node_info['Name'] == row['FromNode'],
                  'X_Coord'].tolist()[0]  # From node X-coordinate
    link_info.at[idx, 'y0'] = node_info.loc[node_info['Name'] == row['FromNode'],
                  'Y_Coord'].tolist()[0]  # From node Y-coordinate

    link_info.at[idx, 'x1'] = node_info.loc[node_info['Name'] == row['ToNode'],
                  'X_Coord'].tolist()[0]  # To node X-coordinate
    link_info.at[idx, 'y1'] = node_info.loc[node_info['Name'] == row['ToNode'],
                  'Y_Coord'].tolist()[0]  # To node Y-coordinate

    link_info.at[idx, 'elev_in'] = node_info.loc[node_info['Name'] == row['FromNode'],
                  'elevation'].tolist()[0]  # From node elevation
    link_info.at[idx, 'elev_out'] = node_info.loc[node_info['Name'] == row['ToNode'],
                  'elevation'].tolist()[0]  # To node elevation



del idx, row, fn_el, bp_el, tn_el, ep_el, node_info

# %% Check if the link is completely outside
buurt_poly = buurt_bound.geometry[0]
dropidx = []
for idx, row in link_info.iterrows():
    if not row.geometry.representative_point().within(buurt_poly):
        pt1 = row.geometry.boundary[0]
        pt2 = row.geometry.boundary[-1]
        if not (pt1.within(buurt_poly) and pt1.within(buurt_poly)):
            dropidx.append(idx)

link_info = link_info.drop(dropidx)
# %% Saving the link_info dataframe file as pkl file

link_info.to_pickle('data/test/output/link_info.pkl')

link_info.to_file('data/test/output/links.shp')

# %% Information about the characteristics

"""
Shape           Geom1           Geom2           Geom3       Geom4

CIRCULAR        Diameter
RECT_CLOSED     Full Height     Top Width
EGG             Full Height


Manning’s Roughness – Closed Conduits
Conduit Material: Manning n

Asbestos-cement pipe: 0.011 - 0.015
Brick: 0.013 - 0.017
Cast iron pipe
- Cement-lined & seal coated: 0.011 - 0.015
Concrete (monolithic)
- Smooth forms: 0.012 - 0.014
- Rough forms: 0.015 - 0.017
Concrete pipe: 0.011 - 0.015
Corrugated-metal pipe (1/2-in. x 2-2/3-in. corrugations)
- Plain: 0.022 - 0.026
- Paved invert: 0.018 - 0.022
- Spun asphalt lined: 0.011 - 0.015
Plastic pipe (smooth): 0.011 - 0.015
Vitrified clay
- Pipes: 0.011 - 0.013
- Liner plates: 0.015 0.017

"""
