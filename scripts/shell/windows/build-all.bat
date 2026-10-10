@echo off
rem build-all.bat
rem XML Game Engine
rem author: beefviper
rem
rem Builds every program that generate-all.bat wrote to
rem output\<config>\generated\<game>\ , each in its own build\ folder, with
rem Visual Studio at /W4. Run it from the repo root:
rem
rem     scripts\shell\windows\build-all.bat [config]
rem
rem config is Debug (the default), Release, RelWithDebInfo or MinSizeRel, the
rem same as for generate-all.bat. The vcpkg toolchain is the one the main
rem build\ folder was configured with, else %VCPKG_ROOT%. Each game's compiler
rem output is kept in <game>\build\build.log.

setlocal enabledelayedexpansion

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

rem this file is <root>\scripts\shell\windows, so the root is three up
pushd "%~dp0..\..\.."
set "ROOT=%CD%"
set "GENERATED=%ROOT%\output\%CONFIG%\generated"

if not exist "%GENERATED%" (
    echo error: %GENERATED% not found. Run generate-all.bat %CONFIG% first.
    popd
    exit /b 1
)

rem vcpkg's toolchain file, so SFML is found: the main build's, else VCPKG_ROOT
set "TOOLCHAIN="
if exist "%ROOT%\build\CMakeCache.txt" (
    for /f "tokens=2 delims==" %%T in ('findstr /b "CMAKE_TOOLCHAIN_FILE:FILEPATH=" "%ROOT%\build\CMakeCache.txt"') do set "TOOLCHAIN=%%T"
)
if "%TOOLCHAIN%"=="" if defined VCPKG_ROOT set "TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
if "%TOOLCHAIN%"=="" (
    echo error: no vcpkg toolchain: configure the main build first, or set VCPKG_ROOT.
    popd
    exit /b 1
)

set /a BUILT=0
set /a FAILED=0
set /a WARNED=0
set "FAILS="

for /d %%D in ("%GENERATED%\*") do (
    set "GAME=%%~nxD"
    set "LOG=%%D\build\build.log"
    if not exist "%%D\build" mkdir "%%D\build"
    set "STAGE=configure"
    cmake -S "%%D" -B "%%D\build" -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%" -DCMAKE_CXX_FLAGS="/W4 /EHsc" > "!LOG!" 2>&1
    if not errorlevel 1 (
        set "STAGE=build"
        cmake --build "%%D\build" --config %CONFIG% --parallel >> "!LOG!" 2>&1
    )
    if errorlevel 1 (
        echo !GAME!: FAILED ^(!STAGE!, see !LOG!^)
        set /a FAILED+=1
        set "FAILS=!FAILS! !GAME!"
    ) else (
        set /a W=0
        for /f "delims=" %%L in ('findstr /c:"warning C" "!LOG!"') do set /a W+=1
        if !W!==0 (
            echo !GAME!: ok
        ) else (
            echo !GAME!: ok, !W! warning^(s^), see !LOG!
            set /a WARNED+=1
        )
        set /a BUILT+=1
    )
)

echo.
echo built !BUILT!, failed !FAILED!, with warnings !WARNED!  ^(%CONFIG%^)
if !FAILED! gtr 0 (
    echo failed:!FAILS!
    popd
    exit /b 1
)

popd
exit /b 0
