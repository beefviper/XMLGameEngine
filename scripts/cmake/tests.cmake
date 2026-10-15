# tests.cmake
# XML Game Engine
# author: beefviper
# date: Sept 28, 2026
#
# Only included (from the top-level CMakeLists.txt) when BUILD_TESTING is ON
# - an ordinary build of the game doesn't need Catch2 or a second executable.

if (NOT FORCE_LOCAL_CATCH2)
	find_package(Catch2 3 QUIET)
endif()

if (Catch2_FOUND)
	message(STATUS "Catch2 found: ${Catch2_DIR}")
else()
	message(STATUS "Catch2 not found, using FetchContent to download and build it locally.")

	FetchContent_Declare(Catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2.git
		GIT_TAG v3.7.1
		EXCLUDE_FROM_ALL
		SYSTEM)

	FetchContent_MakeAvailable(Catch2)

	# Only shipped alongside an installed/vcpkg Catch2's own CMake package;
	# fetching it from source doesn't add this to CMAKE_MODULE_PATH for us,
	# and catch_discover_tests() below lives in it (Catch.cmake).
	list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()

# The tests play the same games through every backend and compare them, so they
# need every backend built: options.cmake turns them all on for BUILD_TESTING.
if (NOT XGE_ALL_BACKENDS)
	message(FATAL_ERROR "The test suite needs every backend: leave XGE_ALL_BACKENDS on (BUILD_TESTING turns it on).")
endif()

# A second executable: the test files plus the engine library itself (the
# xgelib target), so tests call into the real command.cpp/game.cpp
# etc., not a hand-copied reimplementation of them. The library brings the
# include directory and every third-party library it needs along with it.
add_executable(xgetest
	"xge/tests/test_main.cpp"
	"xge/tests/test_command_parsing.cpp"
	"xge/tests/test_object_variables.cpp"
	"xge/tests/test_conditions.cpp"
	"xge/tests/test_win_condition.cpp"
	"xge/tests/test_collision_geometry.cpp"
	"xge/tests/test_swept_collision.cpp"
	"xge/tests/test_input_resolution.cpp"
	"xge/tests/test_stick.cpp"
	"xge/tests/test_collision_rules.cpp"
	"xge/tests/test_lockstep_bounce.cpp"
	"xge/tests/test_size_expressions.cpp"
	"xge/tests/test_engine_input.cpp"
	"xge/tests/test_new_verbs.cpp"
	"xge/tests/test_deflect.cpp"
	"xge/tests/test_xml_format.cpp"
	"xge/tests/test_equations.cpp"
	"xge/tests/test_backends.cpp"
	"xge/tests/test_group.cpp"
	"xge/tests/test_pong.cpp"
	"xge/tests/test_breakout.cpp"
	"xge/tests/test_frogger.cpp"
	"xge/tests/test_spacerace.cpp"
	"xge/tests/test_kaboom.cpp"
	"xge/tests/test_freeway.cpp"
	"xge/tests/test_depthcharge.cpp"
	"xge/tests/test_astrosmash.cpp"
	"xge/tests/test_atlantis.cpp"
	"xge/tests/test_pacman.cpp"
	"xge/tests/test_lines_and_pixels.cpp"
	"xge/tests/test_bitmap_sprites.cpp"
	"xge/tests/test_svg_sprites.cpp"
	"xge/tests/test_lunarlander.cpp"
	"xge/tests/test_asteroids.cpp"
	"xge/tests/test_demonattack.cpp"
	"xge/tests/test_builtin_font.cpp"
	"xge/tests/test_sound.cpp"
	"xge/tests/test_gameplay_verbs.cpp"
	"xge/tests/test_scenes.cpp"
	"xge/tests/test_frostbite.cpp"
	"xge/tests/test_donkeykong.cpp"
	"xge/tests/test_berserk.cpp"
	"xge/tests/test_pictures.cpp"
	"xge/tests/test_cli.cpp"
	"xge/tests/test_data_folder.cpp"
	"xge/tests/test_generate.cpp"
	"xge/tests/test_galaxian.cpp"
	"xge/tests/test_pitfall.cpp"
	"xge/tests/test_missilecommand.cpp"
	"xge/tests/test_combat.cpp"
	"xge/tests/test_airseabattle.cpp"
	"xge/tests/test_megamania.cpp"
	"xge/tests/test_adventure.cpp"
	"xge/cli/source/cli.cpp"
	"xge/cli/source/generate.cpp"
)

# cli.cpp and generate.cpp are xgecli's own code, compiled in again here so
# test_cli.cpp and test_generate.cpp can call it; the library does not contain
# it (see executables.cmake).
target_include_directories(xgetest PRIVATE xge/cli/include)

target_compile_features(xgetest PRIVATE cxx_std_20)

# The same warnings as the library (platform.cmake), /bigobj included:
# several test files include game_expr.h, whose exprtk use generates enough
# object sections to hit MSVC's C1128 without it.
xge_warnings(xgetest)

target_link_libraries(xgetest PRIVATE xgelib Catch2::Catch2WithMain)

# test_generate.cpp runs --generate, which needs libxslt.
xge_link_xslt(xgetest)

# games/ (the games, their assets and the generators) and the schema end up
# next to the test binary, in output/<config> (same as they do for the
# programs - see output.cmake and assets.cmake), because several tests load
# the shipped games; the tests run
# in games/ there, as xgecli and xgegui do (test_main.cpp makes sure of it when
# xgetest is started from anywhere else).
add_dependencies(xgetest xgedata)

# In output/<config>, with its DLLs in libraries/ (output.cmake). Before
# catch_discover_tests(), which runs the program after it is built.
xge_place_program(xgetest)

include(CTest)
include(Catch)
catch_discover_tests(xgetest
	WORKING_DIRECTORY "$<TARGET_FILE_DIR:xgetest>/games")

# The generated games' tests are a second test program, xgetest-generated,
# built only when asked for (cmake --build build --target xgetest-generated,
# or scripts/shell/windows/games-test-all.bat), never by an ordinary build:
# building it generates every game (xgecli --generate windows-cpp) into
# build/generated/ and compiles each one's data (source/<game>.cpp) in, and
# test_generated_games.cpp plays each against its file, frame by frame.
# generated_games.h names them; the function for a game is its file's name in
# camel case and "Game", as gameFunctionName in xge/cli/source/generate.cpp.
add_executable(xgetest-generated EXCLUDE_FROM_ALL
	"xge/tests/test_main.cpp"
	"xge/tests/test_generated_games.cpp"
)
# Visual Studio's Build Solution leaves it out too.
set_target_properties(xgetest-generated PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD ON)
target_compile_features(xgetest-generated PRIVATE cxx_std_20)
xge_warnings(xgetest-generated)
target_link_libraries(xgetest-generated PRIVATE xgelib Catch2::Catch2WithMain)
add_dependencies(xgetest-generated xgedata)
xge_place_program(xgetest-generated)

set(XGE_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
set(generated_declarations "")
set(generated_entries "")
# The stylesheets and modules, file by file, so a change to the generator
# generates every game again (depending on xgedata alone only orders the steps).
list(TRANSFORM data_generators PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/" OUTPUT_VARIABLE generator_files)
foreach(game_file IN LISTS data_xml)
	get_filename_component(game "${game_file}" NAME_WE)

	string(REGEX MATCHALL "[A-Za-z0-9]+" words "${game}")
	set(function "")
	foreach(word IN LISTS words)
		if (function STREQUAL "")
			set(function "${word}")
		else()
			string(SUBSTRING "${word}" 0 1 first)
			string(SUBSTRING "${word}" 1 -1 rest)
			string(TOUPPER "${first}" first)
			string(APPEND function "${first}${rest}")
		endif()
	endforeach()
	if (function MATCHES "^[0-9]")
		set(function "n${function}")
	endif()
	string(APPEND function "Game")

	set(data "${XGE_GENERATED_DIR}/${game}/source/${game}.cpp")
	add_custom_command(
		OUTPUT "${data}"
		COMMAND xgecli "${game}" --generate windows-cpp -o "${XGE_GENERATED_DIR}/${game}"
		WORKING_DIRECTORY "${XGE_OUTPUT_DIR}"
		DEPENDS xgecli xgedata "${CMAKE_CURRENT_SOURCE_DIR}/${game_file}" ${generator_files}
		COMMENT "Generating ${game_file} for the tests")
	target_sources(xgetest-generated PRIVATE "${data}")

	string(APPEND generated_declarations "xge::GameData ${function}();\n")
	# The tests run in the games folder, so a game is descriptions/<game>.xml.
	string(APPEND generated_entries "\t{ \"descriptions/${game}.xml\", ${function} },\n")
endforeach()

file(WRITE "${XGE_GENERATED_DIR}/generated_games.h.in"
	"// generated_games.h\n"
	"// XML Game Engine\n"
	"// author: beefviper\n"
	"// date: Oct 11, 2026\n"
	"//\n"
	"// Written by scripts/cmake/tests.cmake: every game's generated data.\n\n"
	"#pragma once\n\n"
	"#include \"game_data.h\"\n\n"
	"${generated_declarations}\n"
	"struct GeneratedGame\n{\n\tconst char* file;\n\txge::GameData (*describe)();\n};\n\n"
	"inline const GeneratedGame generatedGames[] = {\n${generated_entries}};\n")
configure_file("${XGE_GENERATED_DIR}/generated_games.h.in" "${XGE_GENERATED_DIR}/generated_games.h" COPYONLY)
target_include_directories(xgetest-generated PRIVATE "${XGE_GENERATED_DIR}")
