# executables.cmake
# XML Game Engine
# author: beefviper
# date: Oct 1, 2026
#
# The programs built on the engine library (the XGELIB target):
#   XGECLI  the command line front end: picks a game file from the arguments,
#           loads it, and runs it in a window (cli/)
#   XGEGUI  the Qt front end (gui/): the game in a window, with play, pause and
#           step controls and a tree of the game's data that can be edited
#           while it runs. Built only when Qt 6 is found (dependencies.cmake)
# They are defined here, in the top-level directory's scope, so they land in
# the build directory itself, next to the games/ and assets/ copies that
# assets.cmake makes.

add_executable(XGECLI
	"cli/source/main.cpp"
	"cli/source/cli.cpp"
	"cli/include/cli.h"
)

set(XGE_PROGRAMS XGECLI)

if (Qt6_FOUND)
	# The headers are listed so that AUTOMOC finds the Q_OBJECT classes in them.
	add_executable(XGEGUI
		"gui/source/main.cpp"
		"gui/source/main_window.cpp"
		"gui/source/game_session.cpp"
		"gui/source/game_view.cpp"
		"gui/source/qt_window.cpp"
		"gui/source/inspector.cpp"
		"gui/include/main_window.h"
		"gui/include/game_session.h"
		"gui/include/game_view.h"
		"gui/include/qt_window.h"
		"gui/include/inspector.h"
	)

	set_target_properties(XGEGUI PROPERTIES AUTOMOC ON)
	target_include_directories(XGEGUI PRIVATE gui/include)
	target_link_libraries(XGEGUI PRIVATE Qt6::Widgets)

	# Qt loads its platform plug-in (platforms/qwindows.dll) from a folder next
	# to the program at run time, so it is never among the DLLs the program
	# links to, and the copying vcpkg does after each build does not see it:
	# without this the program stops at start with "Could not find the Qt
	# platform plugin windows". windeployqt puts it, and any other plug-in the
	# program needs, in place. Qt6::windeployqt is used rather than the tool's
	# file name because vcpkg points it at windeployqt.debug.bat for the Debug
	# configuration (the plain tool looks for release DLLs and fails on a debug
	# build), and a target is resolved for the configuration being built, which
	# also holds for Visual Studio's multi-configuration generator.
	if (WIN32 AND TARGET Qt6::windeployqt)
		add_custom_command(TARGET XGEGUI POST_BUILD
			COMMAND Qt6::windeployqt --no-translations --no-compiler-runtime "$<TARGET_FILE:XGEGUI>")
	endif()

	list(APPEND XGE_PROGRAMS XGEGUI)
endif()

foreach(program IN LISTS XGE_PROGRAMS)
	target_link_libraries(${program} PRIVATE XGELIB)
	target_compile_features(${program} PRIVATE cxx_std_20)

	# Same flags platform.cmake sets on the library.
	target_compile_options(${program} PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>:/W4> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>)
	target_compile_options(${program} PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>:/external:anglebrackets /external:W0 /analyze:external- /bigobj>)

	# The games and assets are copied next to the program, see assets.cmake.
	add_dependencies(${program} XGEDATA)
endforeach()

# cli/include is not part of the library's include directory: only XGECLI uses it.
target_include_directories(XGECLI PRIVATE cli/include)
