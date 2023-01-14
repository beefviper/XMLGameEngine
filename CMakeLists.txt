# CMakeLists.txt 
# XML Game Engine
# author: beefviper
# date: Sept 18, 2020

# CMakeList.txt : CMake project for XMLGameEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("XMLGameEngine")

if (MSVC)
    add_compile_options(/bigobj)
endif()

# Add source to this project's executable.
add_executable (${PROJECT_NAME}
	"source/main.cpp"
	"source/game.cpp"
	"source/engine.cpp"
	"source/object.cpp"
	"source/states.cpp"
	"source/utils.cpp"
	"source/game_xml.cpp"
	"source/game_expr.cpp"
	"source/game_sfml.cpp"
	
	"source/include/game.h"
	"source/include/engine.h"
	"source/include/object.h"
	"source/include/states.h"
	"source/include/utils.h"
	"source/include/game_xml.h"
	"source/include/game_expr.h"
	"source/include/game_sfml.h")

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	PROPERTY VS_STARTUP_PROJECT XMLGameEngine)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

find_path(EXPRTK_INCLUDE_DIRS "exprtk.hpp")

find_package(SFML COMPONENTS system window graphics network audio CONFIG REQUIRED)
find_package(XercesC CONFIG REQUIRED)

target_include_directories(${PROJECT_NAME} PUBLIC source/include)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE "${EXPRTK_INCLUDE_DIRS}")

target_link_libraries(${PROJECT_NAME} PRIVATE sfml-system sfml-window sfml-graphics sfml-network sfml-audio)
target_link_libraries(${PROJECT_NAME} PRIVATE xerces_xerces-c XercesC::XercesC)

add_dependencies(${PROJECT_NAME} data-target)

# TODO: Add tests and install targets if needed.

# we don't want to copy if we're building in the source dir
if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)

    # list of files for which we add a copy rule
    set(data_SHADOW
		"games/pong.xml"
		"games/pong_full.xml"
		"games/breakout.xml"
		"games/spaceinvaders.xml"
		"games/assets/tuffy.ttf"
		"games/assets/xmlgameengine.xsd"
		"games/assets/paddle.jpg")

    foreach(item IN LISTS data_SHADOW)
        message(STATUS ${item})
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${item}" "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${item}"
        )
    endforeach()
endif()

# files are only copied if a target depends on them
add_custom_target(data-target ALL DEPENDS ${data_SHADOW})
