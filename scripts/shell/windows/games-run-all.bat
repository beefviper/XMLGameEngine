@echo off
rem games-run-all.bat
rem XML Game Engine
rem author: beefviper
rem
rem Generates, builds and tests every game, stopping at the first step that
rem fails: games-generate-all.bat, games-build-all.bat, games-test-all.bat.
rem Run it from the repo root:
rem
rem     scripts\shell\windows\games-run-all.bat [config]
rem
rem config is Debug (the default), Release, RelWithDebInfo or MinSizeRel. The
rem engine itself must be built first, with tests: cmake -B build -DBUILD_TESTING=ON
rem and cmake --build build --config <config>

setlocal

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

set "HERE=%~dp0"

call "%HERE%games-generate-all.bat" %CONFIG%
if errorlevel 1 exit /b 1
echo.
call "%HERE%games-build-all.bat" %CONFIG%
if errorlevel 1 exit /b 1
echo.
call "%HERE%games-test-all.bat" %CONFIG%
exit /b %errorlevel%
