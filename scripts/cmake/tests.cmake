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

# A second executable built from the same engine sources as ${PROJECT_NAME}
# (see ENGINE_SOURCES/ENGINE_HEADERS in the top-level CMakeLists.txt) plus
# the actual test files - so tests call into the real command.cpp/game.cpp
# etc., not a hand-copied reimplementation of them.
add_executable(XMLGameEngineTests
	${ENGINE_SOURCES}
	${ENGINE_HEADERS}

	"tests/test_command_parsing.cpp"
	"tests/test_object_variables.cpp"
	"tests/test_conditions.cpp"
	"tests/test_collision_geometry.cpp"
)

target_compile_features(XMLGameEngineTests PRIVATE cxx_std_20)

target_include_directories(XMLGameEngineTests PRIVATE include)

target_link_libraries(XMLGameEngineTests PRIVATE Catch2::Catch2WithMain)

target_link_libraries(XMLGameEngineTests PRIVATE XercesC::XercesC)

target_link_libraries(XMLGameEngineTests PRIVATE SFML::System
	SFML::Window SFML::Graphics SFML::Network SFML::Audio)

if (EXPRTK_PACKAGE_FOUND)
	target_include_directories(XMLGameEngineTests PRIVATE ${EXPRTK_INCLUDE_DIRS})
else()
	target_link_libraries(XMLGameEngineTests PRIVATE exprtk)
endif()

# None of the current tests load a game XML file, but games/ and assets/
# ending up next to the test binary (same as they already do for
# ${PROJECT_NAME} - see assets.cmake) costs nothing and means a future test
# that does construct a real xge::Game just works.
add_dependencies(XMLGameEngineTests data-target)

include(CTest)
include(Catch)
catch_discover_tests(XMLGameEngineTests
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
