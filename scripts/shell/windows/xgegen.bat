@echo off
rem xgegen.bat
rem XML Game Engine
rem author: beefviper
rem date: Nov 24, 2026
rem
rem Writes games out with one generator: every game, or the one named.
rem   xgegen <generator> [game]
rem Each goes to output\<config>\generated\<generator>\<game>, with the
rem xgecli of the newest build (Debug, else Release).

setlocal EnableDelayedExpansion

for %%r in ("%~dp0..\..\..") do set "root=%%~fr"
set "generators=%root%\games\generators"
set "descriptions=%root%\games\descriptions"

if "%~1" == "" goto usage

rem the generator: a folder of games\generators with a generate.xsl
set "generator=%~1"
if not exist "%generators%\%generator%\generate.xsl" (
	echo xgegen: no generator "%generator%"
	call :list_generators
	exit /b 1
)

rem the xgecli to run
set "config="
for %%c in (Debug Release) do (
	if not defined config if exist "%root%\output\%%c\xgecli.exe" set "config=%%c"
)
if not defined config (
	echo xgecli: not built yet ^(looked in output\Debug and output\Release^)
	exit /b 1
)
set "xgecli=%root%\output\%config%\xgecli.exe"
set "out=%root%\output\%config%\generated\%generator%"

rem one game, named with or without .xml
if not "%~2" == "" (
	set "game=%~n2"
	if not exist "%descriptions%\!game!.xml" (
		echo xgegen: no game "!game!" in games\descriptions
		exit /b 1
	)
	call :generate !game! whole
	exit /b !errorlevel!
)

rem every game
set /a written=0
set /a failed=0
set "failures="
for %%g in ("%descriptions%\*.xml") do (
	set "game=%%~ng"
	if "!game:_scratch=!" == "!game!" (
		call :generate !game! brief
		if errorlevel 1 (
			set /a failed+=1
			set "failures=!failures! !game!"
		) else (
			set /a written+=1
		)
	)
)
echo.
echo %generator%: %written% written, %failed% not
if %failed% gtr 0 (
	echo   not written:%failures%
	exit /b 1
)
exit /b 0

:generate
rem one game; when it fails, what xgecli said: all of it (whole) or its
rem error and the line after (brief), so a run of every game stays readable
echo %generator%: %1
"%xgecli%" %1 --generate %generator% -o "%out%\%1" > "%TEMP%\xgegen.log" 2>&1
if not errorlevel 1 exit /b 0
if "%2" == "whole" (
	type "%TEMP%\xgegen.log"
	exit /b 1
)
set /a shown=0
for /f "usebackq delims=" %%l in ("%TEMP%\xgegen.log") do (
	set "line=%%l"
	if !shown! == 1 (
		echo   %%l
		set /a shown=2
	)
	if !shown! == 0 if "!line:~0,6!" == "Error:" (
		echo   %%l
		set /a shown=1
	)
)
exit /b 1

:list_generators
echo generators:
for /d %%d in ("%generators%\*") do (
	if exist "%%d\generate.xsl" echo   %%~nxd
)
exit /b 0

:usage
echo usage: xgegen ^<generator^> [game]
echo.
echo   Writes games out with one generator, each to
echo   output\^<config^>\generated\^<generator^>\^<game^>.
echo   With only a generator it writes every game in games\descriptions;
echo   with a game, just that one.
echo.
call :list_generators
echo.
echo   e.g.  xgegen web-js pong_min
echo         xgegen engine-cpp
exit /b 1
