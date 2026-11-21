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
# frame by frame), long/ (hidden: [.long]) and generated/ (the generated
# games, a program of their own).
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
	"xge/tests/engine/test_schema_checker.cpp"
	"xge/tests/engine/test_tree.cpp"
	"xge/tests/engine/test_command_values.cpp"
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
	"xge/tests/engine/test_step_lengths.cpp"
	"xge/tests/engine/test_formulas.cpp"
	"xge/tests/engine/test_calc.cpp"
	"xge/tests/engine/test_backends.cpp"
	"xge/tests/engine/test_group.cpp"
	"xge/tests/games/test_pong.cpp"
	"xge/tests/games/test_breakout.cpp"
	"xge/tests/games/test_frogger.cpp"
	"xge/tests/games/test_spacerace.cpp"
	"xge/tests/games/test_sparring.cpp"
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
	"xge/tests/engine/test_drawn_text.cpp"
	"xge/tests/engine/test_picture_scale.cpp"
	"xge/tests/engine/test_gameplay_verbs.cpp"
	"xge/tests/engine/test_pieces.cpp"
	"xge/tests/engine/test_scenes.cpp"
	"xge/tests/engine/test_text.cpp"
	"xge/tests/games/test_frostbite.cpp"
	"xge/tests/engine/test_pictures.cpp"
	"xge/tests/engine/test_resources.cpp"
	"xge/tests/engine/test_store.cpp"
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
	"xge/tests/engine/test_prototype_pong.cpp"
	"games/prototypes/pong_min/source/pong_min.cpp"
	"xge/tests/engine/test_prototype_pong_full.cpp"
	"games/prototypes/pong/source/pong.cpp"
	"xge/cli/source/cli.cpp"
	"xge/cli/source/generate.cpp"
	"xge/tests/long/test_random_keys.cpp"
	"xge/tests/long/test_damaged.cpp"
	"xge/tests/long/test_speed.cpp"
	"xge/tests/long/test_size.cpp"
	"xge/tests/long/test_pace.cpp"
	"xge/tests/long/test_whole_games.cpp"
)

# cli.cpp and generate.cpp are xgecli's own code, compiled in again here so
# test_cli.cpp and test_generate.cpp can call it; the library does not contain
# it (see executables.cmake). pong_min.cpp and pong.cpp are the hand-written
# Pongs (games/prototypes/), stand-alone on SFML: test_prototype_pong.cpp and
# test_prototype_pong_full.cpp check what they do.
target_include_directories(xgetest PRIVATE xge/cli/include xge/tests xge/tests/helpers games/prototypes/pong_min/source games/prototypes/pong/source)

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

# A test program kept out of the ordinary ctest, given to ctest in a folder
# of its own, build/<folder>, so its cases are counted and timed as
# xgetest's are and run side by side:
#
#     ctest --test-dir build/<folder> -C Debug -j 16 --output-on-failure
#
# Every case is a test (catch_discover_tests, after the program is built;
# with SPEC, only the cases it picks, as "[long]" picks the hidden long ones),
# but those tagged in EACH_GAME, which take a section per game: each game is
# a test of its own, "<tag>: <game>", over the games in GAMES. Those tagged
# in SERIAL are a test per tag, run with nothing beside them (they time the
# engine against the clock).
function(xge_ctest_folder target folder)
	cmake_parse_arguments(arg "" "SPEC" "EACH_GAME;GAMES;SERIAL" ${ARGN})
	set(folder_dir "${CMAKE_BINARY_DIR}/${folder}")
	set(program "$<TARGET_FILE:${target}>")
	set(games_dir "${XGE_OUTPUT_DIR}/games")

	set(lines "if(EXISTS \"${program}\")\n")
	foreach(tag IN LISTS arg_EACH_GAME)
		foreach(game IN LISTS arg_GAMES)
			string(APPEND lines
				"  add_test(\"${tag}: ${game}\" \"${program}\" \"[${tag}]\" -c \"${game}\")\n"
				"  set_tests_properties(\"${tag}: ${game}\" PROPERTIES WORKING_DIRECTORY \"${games_dir}\")\n")
		endforeach()
	endforeach()
	foreach(tag IN LISTS arg_SERIAL)
		string(APPEND lines
			"  add_test(\"${tag}\" \"${program}\" \"[${tag}]\")\n"
			"  set_tests_properties(\"${tag}\" PROPERTIES WORKING_DIRECTORY \"${games_dir}\" RUN_SERIAL ON)\n")
	endforeach()
	string(APPEND lines "endif()\n")

	get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
	if (multi_config)
		file(GENERATE OUTPUT "${folder_dir}/each-$<CONFIG>.cmake" CONTENT "${lines}")
		set(each_file "${folder_dir}/each-\${CTEST_CONFIGURATION_TYPE}.cmake")
	else()
		file(GENERATE OUTPUT "${folder_dir}/each.cmake" CONTENT "${lines}")
		set(each_file "${folder_dir}/each.cmake")
	endif()

	# The cases left, found by Catch; its file is taken back out of this
	# folder's tests, so the ordinary ctest does not run them.
	set(spec "${arg_SPEC}")
	foreach(tag IN LISTS arg_EACH_GAME arg_SERIAL)
		string(APPEND spec "~[${tag}]")
	endforeach()
	get_property(before DIRECTORY PROPERTY TEST_INCLUDE_FILES)
	catch_discover_tests(${target}
		TEST_SPEC "${spec}"
		WORKING_DIRECTORY "${games_dir}")
	get_property(after DIRECTORY PROPERTY TEST_INCLUDE_FILES)
	set(discovered "${after}")
	if (before)
		list(REMOVE_ITEM discovered ${before})
	endif()
	set_property(DIRECTORY PROPERTY TEST_INCLUDE_FILES "${before}")

	set(testfile "include(\"${each_file}\" OPTIONAL)\n")
	foreach(file IN LISTS discovered)
		string(APPEND testfile "include(\"${file}\" OPTIONAL)\n")
	endforeach()
	file(WRITE "${folder_dir}/CTestTestfile.cmake" "${testfile}")
endfunction()

# The generated games' tests are a second test program, xgetest-generated,
# built only when asked for (cmake --build build --target xgetest-generated,
# or scripts/shell/windows/games-test-all.bat), never by an ordinary build:
# building it generates every game (xgecli --generate engine-cpp) into
# build/generated/ and compiles each one's data (source/<game>.cpp) in, and
# test_generated_games.cpp plays each against its file, frame by frame.
# generated_games.h names them; the function for a game is its file's name in
# camel case and "Game", as gameFunctionName in xge/cli/source/generate.cpp.
add_executable(xgetest-generated EXCLUDE_FROM_ALL
	"xge/tests/helpers/test_main.cpp"
	"xge/tests/generated/test_generated_games.cpp"
	"xge/tests/generated/test_written_games.cpp"
	"xge/tests/written/pong_min.h"
)
# Visual Studio's Build Solution leaves it out too.
# In an IDE, out of the way in a folder of its own (with the hand-written
# Pongs, executables.cmake).
set_target_properties(xgetest-generated PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD ON FOLDER XGETestGenerated)
target_compile_features(xgetest-generated PRIVATE cxx_std_20)
xge_warnings(xgetest-generated)
target_link_libraries(xgetest-generated PRIVATE xgelib Catch2::Catch2WithMain)
add_dependencies(xgetest-generated xgedata)
xge_place_program(xgetest-generated FOLDER generated)

set(XGE_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
set(generated_declarations "")
set(generated_entries "")
# The stylesheets, file by file, so a change to the generator
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
		COMMAND xgecli "${game}" --generate engine-cpp -o "${XGE_GENERATED_DIR}/${game}"
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
	"struct GeneratedGame\n{\n\tconst char* file;\n\txge::GameData (*read)();\n};\n\n"
	"inline const GeneratedGame generatedGames[] = {\n${generated_entries}};\n")
configure_file("${XGE_GENERATED_DIR}/generated_games.h.in" "${XGE_GENERATED_DIR}/generated_games.h" COPYONLY)
target_include_directories(xgetest-generated PRIVATE "${XGE_GENERATED_DIR}" xge/tests xge/tests/helpers xge/tests/generated)

# The games windows-cpp writes (plan A5), each with the class it is written
# as, a line for each it learns: generated into build/generated/windows-cpp/
# and compiled in, so test_written_games.cpp plays each with its checks
# (xge/tests/written/<game>.h). test_generate.cpp holds the same list.
set(windows_cpp_games
	"pong_min=pong_min::Pong")
set(written_includes "")
set(written_entries "")
foreach(entry IN LISTS windows_cpp_games)
	string(REPLACE "=" ";" parts "${entry}")
	list(GET parts 0 game)
	list(GET parts 1 class)
	set(folder "${XGE_GENERATED_DIR}/windows-cpp/${game}")
	add_custom_command(
		OUTPUT "${folder}/source/${game}.cpp" "${folder}/source/${game}.h"
		COMMAND xgecli "${game}" --generate windows-cpp -o "${folder}"
		WORKING_DIRECTORY "${XGE_OUTPUT_DIR}"
		DEPENDS xgecli xgedata "${CMAKE_CURRENT_SOURCE_DIR}/games/descriptions/${game}.xml" ${generator_files}
		COMMENT "Generating ${game}.xml as windows-cpp for the tests")
	target_sources(xgetest-generated PRIVATE "${folder}/source/${game}.cpp")
	string(APPEND written_includes "#include \"windows-cpp/${game}/source/${game}.h\"\n#include \"written/${game}.h\"\n")
	string(APPEND written_entries "\t{ \"${game}\", [] { ${game}_plays<${class}>(); } },\n")
endforeach()
file(WRITE "${XGE_GENERATED_DIR}/written_games.h.in"
	"// written_games.h\n"
	"// XML Game Engine\n"
	"// author: beefviper\n"
	"// date: Nov 13, 2026\n"
	"//\n"
	"// Written by scripts/cmake/tests.cmake: the games windows-cpp writes.\n\n"
	"#pragma once\n\n"
	"${written_includes}\n"
	"struct WrittenGame\n{\n\tconst char* game;\n\tvoid (*plays)();\n};\n\n"
	"inline const WrittenGame writtenGames[] = {\n${written_entries}};\n")
configure_file("${XGE_GENERATED_DIR}/written_games.h.in" "${XGE_GENERATED_DIR}/written_games.h" COPYONLY)

# The long tests (xge/tests/long/) are in xgetest, hidden ([.long]), so an
# ordinary run skips them: every game with random keys for a long while,
# every game damaged many ways, load and frame times, pace by the clock.
# ctest --test-dir build/long -C <config> runs them, a test per game for the
# random keys and the damaged files.
set(shipped_games "")
foreach(game_file IN LISTS data_xml)
	get_filename_component(game "${game_file}" NAME_WE)
	list(APPEND shipped_games "${game}")
endforeach()
xge_ctest_folder(xgetest long
	SPEC "[long]"
	EACH_GAME keys damaged
	GAMES ${shipped_games}
	SERIAL speed pace)

# ctest --test-dir build/generated -C <config>: a test per generated game,
# and the games windows-cpp writes as one.
xge_ctest_folder(xgetest-generated generated
	EACH_GAME generated_games
	GAMES ${shipped_games})
