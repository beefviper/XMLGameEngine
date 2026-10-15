@echo off
rem games-test-all.bat
rem XML Game Engine
rem author: beefviper
rem
rem Builds and runs xgetest-generated, the generated games' tests
rem (xge/tests/test_generated_games.cpp): building it generates every game
rem into build\generated\ and compiles each one's data in, and each is played
rem beside the engine playing its file, every object compared every frame.
rem An ordinary build never builds it. Run it from the repo root:
rem
rem     scripts\shell\windows\games-test-all.bat [config]
rem
rem config is Debug (the default), Release, RelWithDebInfo or MinSizeRel. The
rem main build\ folder must be configured with tests:
rem     cmake -B build -DBUILD_TESTING=ON

setlocal

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

rem this file is <root>\scripts\shell\windows, so the root is three up
pushd "%~dp0..\..\.."
set "ROOT=%CD%"

if not exist "%ROOT%\build\CMakeCache.txt" (
    echo error: build\ is not configured. Run cmake -B build -DBUILD_TESTING=ON first.
    popd
    exit /b 1
)

echo building xgetest-generated ^(%CONFIG%^)
cmake --build "%ROOT%\build" --config %CONFIG% --target xgetest-generated --parallel
if errorlevel 1 (
    echo error: xgetest-generated did not build. Is build\ configured with -DBUILD_TESTING=ON?
    popd
    exit /b 1
)

echo.
"%ROOT%\output\%CONFIG%\xgetest-generated.exe"
set "RESULT=%errorlevel%"

popd
exit /b %RESULT%
