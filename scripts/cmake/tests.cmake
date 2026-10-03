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
		EXCLUDE_FROM_ALL)

	FetchContent_MakeAvailable(Catch2)

	# Only shipped alongside an installed/vcpkg Catch2's own CMake package;
	# fetching it from source doesn't add this to CMAKE_MODULE_PATH for us,
	# and catch_discover_tests() below lives in it (Catch.cmake).
	list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()

# A second executable: the test files plus the engine library itself (the
# XGELIB target), so tests call into the real command.cpp/game.cpp
# etc., not a hand-copied reimplementation of them. The library brings the
# include directory and every third-party library it needs along with it.
add_executable(XGETEST
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
	"tests/test_xml_format.cpp"
	"tests/test_group.cpp"
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
	"tests/test_pictures.cpp"
	"tests/test_cli.cpp"
	"tests/test_data_folder.cpp"
	"cli/source/cli.cpp"
)

# cli.cpp is XGECLI's command line code, compiled in again here so test_cli.cpp
# can call it; the library does not contain it (see executables.cmake).
target_include_directories(XGETEST PRIVATE cli/include)

target_compile_features(XGETEST PRIVATE cxx_std_20)

# Same flags platform.cmake sets on XGELIB - in particular /bigobj:
# several test files include game_expr.h, whose exprtk use generates enough
# object sections to hit MSVC's C1128 without it.
target_compile_options(XGETEST PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>)

target_compile_options(XGETEST PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/external:anglebrackets /external:W0 /analyze:external- /bigobj>)

target_link_libraries(XGETEST PRIVATE XGELIB Catch2::Catch2WithMain)

# games/ and assets/ end up next to the test binary (same as they do for the
# programs - see assets.cmake), because several tests load the shipped games.
add_dependencies(XGETEST XGEDATA)

include(CTest)
include(Catch)
catch_discover_tests(XGETEST
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
