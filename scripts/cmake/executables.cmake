# executables.cmake
# XML Game Engine
# author: beefviper
# date: Oct 1, 2026
#
# The programs built on the engine library (the xgelib target):
#   xgecli  the command line front end: picks a game file from the arguments,
#           loads it, and runs it in a window (cli/)
#   xgegui  the Qt front end (gui/): the game in a window, with play, pause and
#           step controls and a tree of the game's data that can be edited
#           while it runs. Built only when Qt 6 is found (dependencies.cmake)
# They are defined here, in the top-level directory's scope, so they land in
# the build directory itself, next to the games/ and assets/ copies that
# assets.cmake makes.

add_executable(xgecli
	"cli/source/main.cpp"
	"cli/source/cli.cpp"
	"cli/include/cli.h"
)

set(XGE_PROGRAMS xgecli)

if (Qt6_FOUND)
	# The headers are listed so that AUTOMOC finds the Q_OBJECT classes in them.
	add_executable(xgegui
		"gui/source/main.cpp"
		"gui/source/main_window.cpp"
		"gui/source/game_session.cpp"
		"gui/source/game_view.cpp"
		"gui/source/qt_window.cpp"
		"gui/source/inspector.cpp"
		"gui/source/key_queue.cpp"
		"gui/source/game_stage.cpp"
		"gui/source/app_settings.cpp"
		"gui/source/session_options.cpp"
		"gui/source/options_dialog.cpp"
		"gui/source/theme.cpp"
		"gui/include/main_window.h"
		"gui/include/game_session.h"
		"gui/include/game_view.h"
		"gui/include/qt_window.h"
		"gui/include/inspector.h"
		"gui/include/key_queue.h"
		"gui/include/game_stage.h"
		"gui/include/app_settings.h"
		"gui/include/session_options.h"
		"gui/include/options_dialog.h"
		"gui/include/theme.h"
	)

	set_target_properties(xgegui PROPERTIES AUTOMOC ON)
	target_include_directories(xgegui PRIVATE gui/include)
	target_link_libraries(xgegui PRIVATE Qt6::Widgets)

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
	#
	# The Qt DLLs go in libraries/ with the others and the plug-ins in
	# libraries/plugins (output.cmake); qt.conf next to the program tells Qt
	# where the plug-ins are, as a path relative to the program's folder.
	if (WIN32 AND TARGET Qt6::windeployqt)
		add_custom_command(TARGET xgegui POST_BUILD
			COMMAND Qt6::windeployqt --no-translations --no-compiler-runtime
				--libdir "$<TARGET_FILE_DIR:xgegui>/${XGE_LIBRARY_FOLDER}"
				--plugindir "$<TARGET_FILE_DIR:xgegui>/${XGE_LIBRARY_FOLDER}/plugins"
				"$<TARGET_FILE:xgegui>"
			VERBATIM)

		file(GENERATE OUTPUT "$<TARGET_FILE_DIR:xgegui>/qt.conf"
			CONTENT "[Paths]\nPlugins = ${XGE_LIBRARY_FOLDER}/plugins\n")
	endif()

	list(APPEND XGE_PROGRAMS xgegui)
endif()

foreach(program IN LISTS XGE_PROGRAMS)
	target_link_libraries(${program} PRIVATE xgelib)
	target_compile_features(${program} PRIVATE cxx_std_20)

	# The same warnings as the library (platform.cmake).
	xge_warnings(${program})

	# The games and assets are copied next to the program, see assets.cmake.
	add_dependencies(${program} xgedata)

	# In output/<config>, with its DLLs in libraries/ (output.cmake). After
	# windeployqt above, so the Qt DLLs are in the list it writes.
	xge_place_program(${program})
endforeach()

# cli/include is not part of the library's include directory: only xgecli uses it.
target_include_directories(xgecli PRIVATE cli/include)
