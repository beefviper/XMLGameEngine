﻿# CMakeLists.txt 
# XML Game Engine
# author: beefviper
# date: Sept 18, 2020

cmake_minimum_required(VERSION 3.19)

project("XMLGameEngine")

add_executable(${PROJECT_NAME}
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
	"source/include/game_sfml.h"
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	PROPERTY VS_STARTUP_PROJECT XMLGameEngine
)

set(_VCPKG_INSTALLED_DIR "") # delete me

include(FetchContent)

FetchContent_Declare(XercesC
	GIT_REPOSITORY https://github.com/apache/xerces-c.git
	GIT_TAG v3.3.0
	EXCLUDE_FROM_ALL
)

FetchContent_Declare(SFML
	GIT_REPOSITORY https://github.com/SFML/SFML.git
	GIT_TAG 2.6.2
	EXCLUDE_FROM_ALL
)

FetchContent_Declare(exprtk
	GIT_REPOSITORY https://github.com/ArashPartow/exprtk.git
	GIT_TAG 0.0.3-cmake
	EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(XercesC SFML exprtk)

find_path(EXPRTK_INCLUDE_DIRS "exprtk.hpp" HINTS ${PROJECT_BINARY_DIR}/_deps/exprtk-src/include)

target_include_directories(${PROJECT_NAME} PUBLIC source/include)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${EXPRTK_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE xerces-c)
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-system
	sfml-window sfml-graphics sfml-network sfml-audio
)

target_compile_options(${PROJECT_NAME} PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/external:anglebrackets /external:W0 /analyze:external- /bigobj>)

set_target_properties(xerces-c PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_compile_options(xerces-c PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W0> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>
)

set_target_properties(xerces-c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set_target_properties(sfml-system PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set_target_properties(sfml-window PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set_target_properties(sfml-graphics PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set_target_properties(sfml-network PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set_target_properties(sfml-audio PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

add_dependencies(${PROJECT_NAME} data-target)

if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    set(data_xsd
		"games/assets/xmlgameengine.xsd")

    set(data_xml
		"games/pong.xml"
		"games/pong_full.xml"
		"games/breakout.xml"
		"games/spaceinvaders.xml")

    set(data_assets
		"games/assets/tuffy.ttf"
		"games/assets/paddle.jpg")

    foreach(item IN LISTS data_xsd data_xml data_assets)
        message(STATUS ${item})
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${item}" "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${item}"
        )
    endforeach()
endif()

add_custom_target(data-target ALL
	DEPENDS ${data_xsd} ${data_xml} ${data_assets}
	SOURCES	${data_xsd} ${data_xml} ${data_assets})
	
source_group("XSD Files" FILES ${data_xsd})
source_group("XML Files" FILES ${data_xml})
source_group("Asset Files" FILES ${data_assets})
