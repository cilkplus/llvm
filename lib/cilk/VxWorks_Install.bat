@echo off

set oldpath=%CD%
set BASEDIR=%WIND_BASE%

if "%BASEDIR%"=="" (echo Can not get %%WIND_BASE%% path, Pleae input %%WIND_BASE%% path first.) else (goto PathIsReady)

:Input_Path
set /p BASEDIR=Input %%WIND_BASE%% path: 
if "%BASEDIR%"=="" goto Input_Path

:PathIsReady
echo %%WIND_BASE%% is [%BASEDIR%]
set BASEDIREND=%BASEDIR:~-1%

if "%BASEDIREND%"=="\" set BASEDIR=%BASEDIR:~0,-1%
set installdir=%BASEDIR%\pkgs\os\lang-lib

cd /d %installdir%
set chcd=%cd%

if NOT "%chcd%" == "%installdir%" (
echo Install target path [%installdir%] not exist!
SET BASEDIR=
goto Input_Path
)

echo CilkPlus will be installed to %installdir%\cilk_kernel and %installdir%\cilk_usr

PAUSE

mkdir cilk_kernel
cd cilk_kernel
mkdir include
mkdir runtime
xcopy "%oldpath%\include" include/e/s/y
xcopy "%oldpath%\runtime" runtime/e/s/y
xcopy "%oldpath%\mk\vxworks_krnl" . /e/s/y

cd ..
mkdir cilk_usr
cd cilk_usr
mkdir include
mkdir runtime
xcopy "%oldpath%\include" include/e/s/y
xcopy "%oldpath%\runtime" runtime/e/s/y
xcopy "%oldpath%\mk\vxworks_usr" . /e/s/y
cd /d %oldpath%



