@echo off
rem generate-all.bat
rem XML Game Engine
rem author: beefviper
rem
rem Generates every game in games\ as a windows-cpp program, into
rem output\<config>\generated\<game>\ . Run it from the repo root:
rem
rem     scripts\shell\windows\generate-all.bat [config]
rem
rem config is the build configuration whose xgecli.exe is used: Debug (the
rem default), Release, RelWithDebInfo or MinSizeRel. Build it first with
rem     cmake --build build --config <config>

setlocal enabledelayedexpansion

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

rem this file is <root>\scripts\shell\windows, so the root is three up
pushd "%~dp0..\..\.."
set "ROOT=%CD%"
set "XGECLI=%ROOT%\output\%CONFIG%\xgecli.exe"

if not exist "%XGECLI%" (
    echo error: %XGECLI% not found. Build the %CONFIG% configuration first.
    popd
    exit /b 1
)

set /a DONE=0
set /a FAILED=0
set "FAILS="

for %%G in ("%ROOT%\games\*.xml") do (
    echo generating %%~nG
    "%XGECLI%" --generate windows-cpp --output "%ROOT%\output\%CONFIG%\generated\%%~nG" "%%~nxG" >nul
    if errorlevel 1 (
        echo   FAILED
        set /a FAILED+=1
        set "FAILS=!FAILS! %%~nG"
    ) else (
        set /a DONE+=1
    )
)

echo.
echo generated !DONE!, failed !FAILED! into output\%CONFIG%\generated\
if !FAILED! gtr 0 (
    echo failed:!FAILS!
    popd
    exit /b 1
)

popd
exit /b 0
