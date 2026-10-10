@echo off
rem games-test-all.bat
rem XML Game Engine
rem author: beefviper
rem
rem Runs xgetest twice: the engine's own tests, then the generated games against
rem the engine (the tests tagged [generated], tests/test_generated.cpp). Each game
rem that games-build-all.bat built is played with no window from the same keys and
rem the same random seed, by the generated program and by the engine, and the
rem pictures at the same frames and the sounds played must match (see
rem lib/include/trace.h). Run it from the repo root:
rem
rem     scripts\shell\windows\games-test-all.bat [config]
rem
rem config is Debug (the default), Release, RelWithDebInfo or MinSizeRel. The games
rem must have been generated and built (games-generate-all.bat, games-build-all.bat),
rem and xgetest built with -DBUILD_TESTING=ON. Nothing opens a window and nothing
rem makes a sound. The frames of a game that differs are kept in %TEMP%\xge-trace\<game>
rem as frameNNNN.bmp (the generated program's) and frameNNNN.engine.bmp.

setlocal

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

rem this file is <root>\scripts\shell\windows, so the root is three up
pushd "%~dp0..\..\.."
set "FOLDER=%CD%\output\%CONFIG%"
set "XGETEST=%FOLDER%\xgetest.exe"

if not exist "%XGETEST%" (
    echo error: %XGETEST% not found. Build the %CONFIG% configuration with -DBUILD_TESTING=ON first.
    popd
    exit /b 1
)

rem the tests run from the folder the games and assets are copied to
cd /d "%FOLDER%"

echo engine tests
"%XGETEST%" --reporter compact >"%TEMP%\xge-xgetest.log" 2>&1
set "ENGINE=%errorlevel%"
if not "%ENGINE%"=="0" (
    echo   FAILED, see %TEMP%\xge-xgetest.log or run %XGETEST%
) else (
    echo   ok
)

echo.
echo generated games against the engine
"%XGETEST%" "[generated]"
set "GENERATED=%errorlevel%"

popd

if not "%ENGINE%"=="0" exit /b 1
if not "%GENERATED%"=="0" exit /b 1
exit /b 0
