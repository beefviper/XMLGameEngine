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

# The tests are in folders: helpers/ (what the others share), engine/ (the
# language and the engine, a file per area), games/ (a file per game, played
# frame by frame) and generated/ (the generated games, a program of their own).
#
# The slowest tests' files come first (the XML libraries loading every game,
# then Pac-Man, sound, Donkey Kong and Berserk), so a ctest run with no timings
# from before starts them first.
#
# A second executable: the test files plus the engine library itself (the
# xgelib target), so tests call into the real game/command.cpp/game.cpp
# etc., not a hand-copied reimplementation of them. The library brings the
# include directory and every third-party library it needs along with it.
add_executable(xgetest
	"xge/tests/helpers/test_main.cpp"
	"xge/tests/engine/test_xml_libraries.cpp"
	"xge/tests/engine/test_names.cpp"
	"xge/tests/engine/test_vocabulary.cpp"
	"xge/tests/games/test_pacman.cpp"
	"xge/tests/engine/test_sound.cpp"
	"xge/tests/games/test_donkeykong.cpp"
	"xge/tests/games/test_berserk.cpp"
	"xge/tests/engine/test_command_parsing.cpp"
	"xge/tests/engine/test_object_variables.cpp"
	"xge/tests/engine/test_conditions.cpp"
	"xge/tests/engine/test_win_condition.cpp"
	"xge/tests/engine/test_collision_geometry.cpp"
	"xge/tests/engine/test_swept_collision.cpp"
	"xge/tests/engine/test_input_resolution.cpp"
	"xge/tests/engine/test_stick.cpp"
	"xge/tests/engine/test_collision_rules.cpp"
	"xge/tests/engine/test_lockstep_bounce.cpp"
	"xge/tests/engine/test_size_expressions.cpp"
	"xge/tests/engine/test_engine_input.cpp"
	"xge/tests/engine/test_controls.cpp"
	"xge/tests/engine/test_new_verbs.cpp"
	"xge/tests/engine/test_deflect.cpp"
	"xge/tests/engine/test_xml_format.cpp"
	"xge/tests/engine/test_formulas.cpp"
	"xge/tests/engine/test_calc.cpp"
	"xge/tests/engine/test_backends.cpp"
	"xge/tests/engine/test_group.cpp"
	"xge/tests/games/test_pong.cpp"
	"xge/tests/games/test_breakout.cpp"
	"xge/tests/games/test_frogger.cpp"
	"xge/tests/games/test_spacerace.cpp"
	"xge/tests/games/test_kaboom.cpp"
	"xge/tests/engine/test_cells.cpp"
	"xge/tests/engine/test_puzzles.cpp"
	"xge/tests/games/test_freeway.cpp"
	"xge/tests/games/test_depthcharge.cpp"
	"xge/tests/games/test_astrosmash.cpp"
	"xge/tests/games/test_atlantis.cpp"
	"xge/tests/engine/test_lines_and_pixels.cpp"
	"xge/tests/engine/test_bitmap_sprites.cpp"
	"xge/tests/engine/test_svg_sprites.cpp"
	"xge/tests/games/test_lunarlander.cpp"
	"xge/tests/games/test_asteroids.cpp"
	"xge/tests/games/test_demonattack.cpp"
	"xge/tests/engine/test_builtin_font.cpp"
	"xge/tests/engine/test_gameplay_verbs.cpp"
	"xge/tests/engine/test_pieces.cpp"
	"xge/tests/engine/test_scenes.cpp"
	"xge/tests/engine/test_text.cpp"
	"xge/tests/games/test_frostbite.cpp"
	"xge/tests/engine/test_pictures.cpp"
	"xge/tests/engine/test_cli.cpp"
	"xge/tests/engine/test_data_folder.cpp"
	"xge/tests/engine/test_generate.cpp"
	"xge/tests/games/test_galaxian.cpp"
	"xge/tests/games/test_pitfall.cpp"
	"xge/tests/engine/test_places.cpp"
	"xge/tests/engine/test_verbs.cpp"
	"xge/tests/games/test_missilecommand.cpp"
	"xge/tests/games/test_combat.cpp"
	"xge/tests/games/test_airseabattle.cpp"
	"xge/tests/games/test_megamania.cpp"
	"xge/tests/games/test_adventure.cpp"
	"xge/tests/engine/test_knock.cpp"
	"xge/tests/engine/test_slopes_and_keys.cpp"
	"xge/tests/games/test_puttpark.cpp"
	"xge/tests/engine/test_flippers.cpp"
	"xge/tests/games/test_pinball.cpp"
	"xge/tests/games/test_cannonhill.cpp"
	"xge/tests/games/test_speedway.cpp"
	"xge/tests/games/test_checkers.cpp"
	"xge/tests/games/test_reversi.cpp"
	"xge/tests/games/test_inarow.cpp"
	"xge/tests/games/test_dotsandboxes.cpp"
	"xge/tests/engine/test_cards.cpp"
	"xge/tests/engine/test_live.cpp"
	"xge/tests/engine/test_steering.cpp"
	"xge/tests/engine/test_hitbox.cpp"
	"xge/tests/engine/test_refused.cpp"
	"xge/tests/engine/test_holes.cpp"
	"xge/tests/engine/test_altitude.cpp"
	"xge/tests/engine/test_dark.cpp"
	"xge/tests/engine/test_computer.cpp"
	"xge/tests/engine/test_spawn.cpp"
	"xge/tests/engine/test_poses.cpp"
	"xge/tests/games/test_qbert.cpp"
	"xge/tests/games/test_mariobros.cpp"
	"xge/cli/source/cli.cpp"
	"xge/cli/source/generate.cpp"
)

# cli.cpp and generate.cpp are xgecli's own code, compiled in again here so
# test_cli.cpp and test_generate.cpp can call it; the library does not contain
# it (see executables.cmake).
target_include_directories(xgetest PRIVATE xge/cli/include xge/tests/helpers)

target_compile_features(xgetest PRIVATE cxx_std_20)

# The same warnings as the library (platform.cmake), /bigobj included.
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
	"xge/tests/helpers/test_main.cpp"
	"xge/tests/generated/test_generated_games.cpp"
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
	"#include \"game/data.h\"\n\n"
	"${generated_declarations}\n"
	"struct GeneratedGame\n{\n\tconst char* file;\n\txge::GameData (*describe)();\n};\n\n"
	"inline const GeneratedGame generatedGames[] = {\n${generated_entries}};\n")
configure_file("${XGE_GENERATED_DIR}/generated_games.h.in" "${XGE_GENERATED_DIR}/generated_games.h" COPYONLY)
target_include_directories(xgetest-generated PRIVATE "${XGE_GENERATED_DIR}" xge/tests/helpers)

# The long tests are a third test program, xgetest-long, built and run only
# when asked for (cmake --build build --target xgetest-long, then
# output/<config>/xgetest-long): every game with random keys for a long while,
# every game damaged many ways, and every game's load and frame times. They
# take minutes, so they stay out of xgetest and ctest.
add_executable(xgetest-long EXCLUDE_FROM_ALL
	"xge/tests/helpers/test_main.cpp"
	"xge/tests/long/test_random_keys.cpp"
	"xge/tests/long/test_damaged.cpp"
	"xge/tests/long/test_speed.cpp"
	"xge/tests/long/test_pace.cpp"
	"xge/cli/source/cli.cpp"
	"xge/cli/source/generate.cpp"
)
set_target_properties(xgetest-long PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD ON)
target_include_directories(xgetest-long PRIVATE xge/cli/include xge/tests/helpers)
target_compile_features(xgetest-long PRIVATE cxx_std_20)
xge_warnings(xgetest-long)
target_link_libraries(xgetest-long PRIVATE xgelib Catch2::Catch2WithMain)
xge_link_xslt(xgetest-long)
add_dependencies(xgetest-long xgedata)
xge_place_program(xgetest-long)
