@echo off
REM This is an example script to run adap2shp.py in order to produce a shapefile of
REM SWMM subcatchments.
REM Syntax:
REM 	python adap2shp.py [INFILE_subcatchments.asc] [INFILE.wkt] [OUTFILE.shp] 'epsg:XXXX'

REM Activate conda environment (with necessary packages installed)
REM (here called 'gistoswmm5')
call conda.bat activate gistoswmm5

REM Example of running adap2shp.py
REM '^' is used only to split long lines here
python "../utils/adap2shp.py"^
 "../demo_catchment/out/SWMM_in/demo_catchment_adap_subcatchments.asc"^
 "../demo_catchment/out/SWMM_in/demo_catchment_adap_subcatchments_attr.wkt"^
 "../demo_catchment/out/SWMM_in/shp/demo_catchment_adap_subcatchments.shp"^
 "epsg:3879"

REM Deactivate conda environment
call conda deactivate

REM Wait for user input before closing terminal
REM pause
