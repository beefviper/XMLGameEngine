# executables.cmake
# XML Game Engine
# author: beefviper
# date: Oct 1, 2026
#
# The programs built on the engine library (the xgelib target):
#   xgecli  the command line front end: picks a game file from the arguments,
#           loads it, and runs it in a window (xge/cli/)
#   xgegui  the Qt front end (xge/gui/): the game in a window, with play, pause and
#           step controls and a tree of the game's data that can be edited
#           while it runs. Built only when Qt 6 is found (dependencies.cmake)
# They are defined here, in the top-level directory's scope, so they land in
# the build directory itself, next to the games/ and assets/ copies that
# assets.cmake makes.

add_executable(xgecli
	"xge/cli/source/main.cpp"
	"xge/cli/source/cli.cpp"
	"xge/cli/source/generate.cpp"
	"xge/cli/include/cli.h"
	"xge/cli/include/generate.h"
)

set(XGE_PROGRAMS xgecli)

if (Qt6_FOUND)
	# The headers are listed so that AUTOMOC finds the Q_OBJECT classes in them.
	add_executable(xgegui
		"xge/gui/source/main.cpp"
		"xge/gui/source/main_window.cpp"
		"xge/gui/source/game_session.cpp"
		"xge/gui/source/game_view.cpp"
		"xge/gui/source/qt_window.cpp"
		"xge/gui/source/inspector.cpp"
		"xge/gui/source/key_queue.cpp"
		"xge/gui/source/game_stage.cpp"
		"xge/gui/source/app_settings.cpp"
		"xge/gui/source/session_options.cpp"
		"xge/gui/source/options_dialog.cpp"
		"xge/gui/source/theme.cpp"
		"xge/gui/include/main_window.h"
		"xge/gui/include/game_session.h"
		"xge/gui/include/game_view.h"
		"xge/gui/include/qt_window.h"
		"xge/gui/include/inspector.h"
		"xge/gui/include/key_queue.h"
		"xge/gui/include/game_stage.h"
		"xge/gui/include/app_settings.h"
		"xge/gui/include/session_options.h"
		"xge/gui/include/options_dialog.h"
		"xge/gui/include/theme.h"
	)

	set_target_properties(xgegui PROPERTIES AUTOMOC ON)
	target_include_directories(xgegui PRIVATE xge/gui/include)
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

# xge/cli/include is not part of the library's include directory: only xgecli uses it.
target_include_directories(xgecli PRIVATE xge/cli/include)

# --generate needs libxslt (dependencies.cmake); without it generate.cpp says so.
xge_link_xslt(xgecli)

# The hand-written Pong (games/prototypes/pong_min): the example of what the
# windows-cpp generator is to write, pong_min.xml as plain code on the
# engine's own small functions (the frame, the verbs, the numbers) and its
# SFML window. Linked against the library for now; the files a generated
# game needs are listed when the generator copies them.
if (XGE_WITH_SFML3)
	add_executable(pong_min
		"games/prototypes/pong_min/source/main.cpp"
		"games/prototypes/pong_min/source/pong_min.cpp"
		"games/prototypes/pong_min/source/pong_min.h"
	)
	target_compile_features(pong_min PRIVATE cxx_std_20)
	target_include_directories(pong_min PRIVATE games/prototypes/pong_min/source)
	target_link_libraries(pong_min PRIVATE xgelib)
	xge_warnings(pong_min)
	xge_place_program(pong_min)

	# And pong.xml by hand as a stand-alone game (games/prototypes/pong):
	# plain SFML, none of the engine, its font copied beside it.
	add_executable(pong
		"games/prototypes/pong/source/main.cpp"
		"games/prototypes/pong/source/pong.cpp"
		"games/prototypes/pong/source/pong.h"
	)
	target_compile_features(pong PRIVATE cxx_std_20)
	target_link_libraries(pong PRIVATE SFML::Graphics SFML::Audio)
	xge_warnings(pong)
	xge_place_program(pong)
	add_custom_command(TARGET pong POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/games/assets/tuffy.ttf" "$<TARGET_FILE_DIR:pong>/tuffy.ttf"
		VERBATIM)
endif()
