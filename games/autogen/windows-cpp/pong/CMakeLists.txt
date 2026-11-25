# CMakeLists.txt
# Pong, from pong.xml.

cmake_minimum_required(VERSION 3.28)

project(pong LANGUAGES CXX)

# The built game, and any DLLs it needs, go to output/ (with a folder for the
# configuration, Debug or Release, under Visual Studio).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/output")

# SFML 3 for the window: as installed (vcpkg install sfml), or else fetched
# and built here.
find_package(SFML 3 COMPONENTS Graphics Audio QUIET)
if (NOT SFML_FOUND)
	message(STATUS "SFML 3 not found, using FetchContent to download and build it.")
	include(FetchContent)
	set(SFML_BUILD_AUDIO ON CACHE BOOL "" FORCE)
	set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
	FetchContent_Declare(SFML GIT_REPOSITORY https://github.com/SFML/SFML.git GIT_TAG 3.1.0 EXCLUDE_FROM_ALL SYSTEM)
	FetchContent_MakeAvailable(SFML)
endif()

add_executable(pong
	source/main.cpp
	source/pong.h
	source/pong.cpp
)
target_compile_features(pong PRIVATE cxx_std_20)
target_link_libraries(pong PRIVATE SFML::Graphics SFML::Audio)

# Every warning worth having, kept at zero.
target_compile_options(pong PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4
		/w14242 /w14254 /w14263 /w14265 /w14287 /w14296 /w14311 /w14545 /w14546 /w14547
		/w14549 /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928
		/external:anglebrackets /external:W0 /analyze:external- /utf-8>
	$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion
		-Wsign-conversion -Wnon-virtual-dtor -Woverloaded-virtual -Wcast-align -Wunused
		-Wnull-dereference -Wformat=2 -Wimplicit-fallthrough
		-Wold-style-cast -Wmisleading-indentation>
	$<$<CXX_COMPILER_ID:GNU>:-Wdouble-promotion -Wduplicated-cond -Wduplicated-branches
		-Wlogical-op -Wuseless-cast>)

set_property(TARGET pong PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:pong>")

# Visual Studio starts the game when you press F5, rather than ALL_BUILD.
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT pong)


# The font goes beside the program.
add_custom_command(TARGET pong POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:pong>/assets")

# On Windows the program needs SFML's DLLs beside it when they are shared.
if (WIN32)
	add_custom_command(TARGET pong POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_RUNTIME_DLLS:pong>" "$<TARGET_FILE_DIR:pong>"
		COMMAND_EXPAND_LISTS)
endif()
