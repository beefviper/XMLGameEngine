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
	"tests/test_command_parsing.cpp"
	"tests/test_object_variables.cpp"
	"tests/test_conditions.cpp"
	"tests/test_win_condition.cpp"
	"tests/test_collision_geometry.cpp"
	"tests/test_swept_collision.cpp"
	"tests/test_input_resolution.cpp"
	"tests/test_stick.cpp"
	"tests/test_collision_rules.cpp"
	"tests/test_lockstep_bounce.cpp"
	"tests/test_size_expressions.cpp"
	"tests/test_engine_input.cpp"
	"tests/test_new_verbs.cpp"
	"tests/test_deflect.cpp"
	"tests/test_xml_format.cpp"
	"tests/test_equations.cpp"
	"tests/test_backends.cpp"
	"tests/test_group.cpp"
	"tests/test_breakout.cpp"
	"tests/test_frogger.cpp"
	"tests/test_spacerace.cpp"
	"tests/test_kaboom.cpp"
	"tests/test_freeway.cpp"
	"tests/test_depthcharge.cpp"
	"tests/test_astrosmash.cpp"
	"tests/test_lines_and_pixels.cpp"
	"tests/test_bitmap_sprites.cpp"
	"tests/test_svg_sprites.cpp"
	"tests/test_lunarlander.cpp"
	"tests/test_asteroids.cpp"
	"tests/test_ai_games.cpp"
	"tests/test_builtin_font.cpp"
	"tests/test_sound.cpp"
	"tests/test_gameplay_verbs.cpp"
	"tests/test_frostbite.cpp"
	"tests/test_donkeykong.cpp"
	"tests/test_berserk.cpp"
	"tests/test_pictures.cpp"
	"tests/test_cli.cpp"
	"tests/test_data_folder.cpp"
	"tests/test_generate.cpp"
	"tests/test_generated_games.cpp"
	"tests/test_prototype_pong.cpp"
	"prototypes/pong_min/source/pong_min.cpp"
	"tests/test_galaxian.cpp"
	"tests/test_pitfall.cpp"
	"tests/test_missilecommand.cpp"
	"tests/test_combat.cpp"
	"tests/test_airseabattle.cpp"
	"tests/test_megamania.cpp"
	"cli/source/cli.cpp"
	"cli/source/generate.cpp"
)

# cli.cpp and generate.cpp are xgecli's own code, compiled in again here so
# test_cli.cpp and test_generate.cpp can call it; the library does not contain
# it (see executables.cmake).
target_include_directories(xgetest PRIVATE cli/include prototypes/pong_min/source)

target_compile_features(xgetest PRIVATE cxx_std_20)

# The same warnings as the library (platform.cmake), /bigobj included:
# several test files include game_expr.h, whose exprtk use generates enough
# object sections to hit MSVC's C1128 without it.
xge_warnings(xgetest)

target_link_libraries(xgetest PRIVATE xgelib Catch2::Catch2WithMain)

# test_generate.cpp runs --generate, which needs libxslt.
xge_link_xslt(xgetest)

# Every game, generated (xgecli --generate windows-cpp) into the build folder
# as the build runs, and its data (source/<game>.cpp) compiled into the tests:
# test_generated_games.cpp plays each one against its file, frame by frame.
# generated_games.h names them; the function for a game is its file's name in
# camel case and "Game", as gameFunctionName in cli/source/generate.cpp.
set(XGE_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
set(generated_declarations "")
set(generated_entries "")
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
		DEPENDS xgecli xgedata "${CMAKE_CURRENT_SOURCE_DIR}/${game_file}"
		COMMENT "Generating ${game_file} for the tests")
	target_sources(xgetest PRIVATE "${data}")

	string(APPEND generated_declarations "xge::GameData ${function}();\n")
	string(APPEND generated_entries "\t{ \"${game_file}\", ${function} },\n")
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
target_include_directories(xgetest PRIVATE "${XGE_GENERATED_DIR}")

# games/ and assets/ end up next to the test binary, in output/<config> (same
# as they do for the programs - see output.cmake and assets.cmake), because
# several tests load the shipped games; the tests run from that folder.
add_dependencies(xgetest xgedata)

# In output/<config>, with its DLLs in libraries/ (output.cmake). Before
# catch_discover_tests(), which runs the program after it is built.
xge_place_program(xgetest)

include(CTest)
include(Catch)
catch_discover_tests(xgetest
	WORKING_DIRECTORY "$<TARGET_FILE_DIR:xgetest>")
