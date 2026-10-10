@echo off
rem games-test-all.bat
rem XML Game Engine
rem author: beefviper
rem
rem Tests the engine, then every game built by games-build-all.bat against the
rem engine: each is played with no window from the same keys and the same random
rem seed, by the generated program and by xgecli, and the pictures at the same
rem frames must match (see lib/include/trace.h). Run it from the repo root:
rem
rem     scripts\shell\windows\games-test-all.bat [config]
rem
rem config is Debug (the default), Release, RelWithDebInfo or MinSizeRel. The
rem games must have been built with BUILD_TESTING on (games-build-all.bat does),
rem and xgecli and xgetest from a build with -DBUILD_TESTING=ON. Nothing opens a
rem window. The frames of a game that fails are kept in %TEMP%\xge-trace\<game>
rem as frameNNNN.bmp (the generated program's) and frameNNNN.engine.bmp.

setlocal enabledelayedexpansion

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

rem what every game is played with: the games share their keys (Space starts,
rem then W A S D, the arrows and Space to fire), so one script does for them all
set "FRAMES=10,30,60,100,150,200,250,300,350,400"
set "KEYS=3+space,5-space,20+d,60-d,60+a,100-a,100+w,140-w,140+s,180-s,180+left,220-left,220+right,260-right,260+up,300-up,300+down,340-down,340+space,342-space,360+space,362-space,380+space,382-space"

rem how far apart a colour may be (the edge of a text is not drawn the same by a
rem picture kept and a text drawn), and how many pixels may be further apart than
rem that (the edge of a shape a little differently placed)
set "TOLERANCE=64"
set "ALLOW=100"

rem how many frames apart the same sound may be heard (a touch found a frame
rem earlier or later is not heard)
set "SLACK=1"

rem this file is <root>\scripts\shell\windows, so the root is three up
pushd "%~dp0..\..\.."
set "ROOT=%CD%"
set "GENERATED=%ROOT%\output\%CONFIG%\generated"
set "XGETEST=%ROOT%\output\%CONFIG%\xgetest.exe"
set "XGECLI=%ROOT%\output\%CONFIG%\xgecli.exe"
set "TRACE=%TEMP%\xge-trace"

if not exist "%GENERATED%" (
    echo error: %GENERATED% not found. Run games-generate-all.bat and games-build-all.bat %CONFIG% first.
    popd
    exit /b 1
)

if exist "%TRACE%" rmdir /s /q "%TRACE%"
mkdir "%TRACE%"

set /a FAILED=0
set "FAILS="

if exist "%XGETEST%" (
    echo engine tests
    pushd "%ROOT%\output\%CONFIG%"
    "%XGETEST%" >nul
    if errorlevel 1 (
        echo   FAILED, run %XGETEST% to see
        set /a FAILED+=1
        set "FAILS=!FAILS! xgetest"
    ) else (
        echo   ok
    )
    popd
) else (
    echo note: %XGETEST% not found, engine tests skipped
)

set /a PASSED=0
for /d %%D in ("%GENERATED%\*") do (
    set "GAME=%%~nxD"
    set "EXE=%%D\output\%CONFIG%\%%~nxD.exe"
    set "FRAMEDIR=%TRACE%\%%~nxD"
    if not exist "!EXE!" (
        echo !GAME!: FAILED ^(not built^)
        set /a FAILED+=1
        set "FAILS=!FAILS! !GAME!"
    ) else (
        pushd "%%D\output\%CONFIG%"
        "!EXE!" --trace "!FRAMEDIR!" --frames "%FRAMES%" --keys "%KEYS%" > "%TRACE%\!GAME!.generated.log" 2>&1
        set "RAN=!errorlevel!"
        popd
        if not "!RAN!"=="0" (
            echo !GAME!: FAILED ^(the generated program stopped with !RAN!, see %TRACE%\!GAME!.generated.log^)
            set /a FAILED+=1
            set "FAILS=!FAILS! !GAME!"
        ) else (
            "%XGECLI%" "!GAME!" --compare "!FRAMEDIR!" --frames "%FRAMES%" --keys "%KEYS%" --tolerance %TOLERANCE% --allow %ALLOW% --slack %SLACK% > "%TRACE%\!GAME!.compare.log" 2>&1
            if errorlevel 1 (
                echo !GAME!: FAILED
                for /f "delims=" %%L in ('findstr /c:"DIFFERENT" /c:"Error" "%TRACE%\!GAME!.compare.log"') do echo   %%L
                set /a FAILED+=1
                set "FAILS=!FAILS! !GAME!"
            ) else (
                echo !GAME!: ok
                set /a PASSED+=1
                rmdir /s /q "!FRAMEDIR!"
            )
        )
    )
)

echo.
echo matched !PASSED! games, failed !FAILED!  ^(%CONFIG%^)
if !FAILED! gtr 0 (
    echo failed:!FAILS!
    echo the frames of the games that differ are in %TRACE%
    popd
    exit /b 1
)

popd
exit /b 0
