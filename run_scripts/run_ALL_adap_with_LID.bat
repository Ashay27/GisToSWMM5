@echo off
REM This is an example script to demonstrate use of GisToSWMM5, SWMM and utility scripts in adaptive mode on Windows


REM Create SWMM5 model using GisToSWMM5
echo Running GisToSWMM5
call run_GisToSWMM5_adap_with_LID.bat


REM Convert the subcatchment GIS files from .asc and .wkt into shapefiles.
REM echo.
echo Running adap2shp.py
call run_adap2shp.bat

REM Create routing shapefiles. 
REM Note that we use inp2gis.py to create the routing shapefile, but because the .inp file only has 
REM catchment center points for adap model, we cannot use it to create the subcatchment shapefile.
REM run_inp2gis.bat shows an example of using inp2gis to create both subcatchment and routing shapefiles.
REM echo.
echo Running inp2gis.py
call conda activate gistoswmm5
python "../utils/inp2gis.py" "../demo_catchment/out/SWMM_in/demo_catchment_adap.inp" "EPSG:28992"
call conda deactivate
REM Remove the inp2gis created subcatchments-shapefile and move the created routing shapefile to
REM folder SWMM_in/shp
REM echo.
echo Moving and deleting unnecessary files
FOR %%F IN (".prj",".cpg",".shx",".shp",".dbf") DO (
	del ..\demo_catchment\out\SWMM_in\demo_catchment_adap_subcatchments%%F /f /q
	move ..\demo_catchment\out\SWMM_in\demo_catchment_adap_subcatchment_routing%%F ..\demo_catchment\out\SWMM_in\shp\ )
REM Wait for user input 
REM pause
	
REM Run SWMM using the created inp-file
REM echo.
echo Running SWMM
"C:/Program Files (x86)/EPA SWMM 5.1.015/swmm5.exe" "../demo_catchment/out/SWMM_in/demo_catchment_adap.inp" "../demo_catchment/out/SWMM_out/demo_catchment_adap.rpt"

REM pause
REM Collect subcatchment results from the SWMM report and save as shapefile
REM echo.
echo Running ExtractSubcatchmentResults.py
call run_ExtractSubcatchmentResults.bat

pause
REM Collect link time series data from the SWMM report files and save to csv-file
REM echo.
REM echo Running ExtractLinkData.py
REM call run_ExtractLinkData.bat


REM Wait for user input before closing terminal
pause