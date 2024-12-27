# CMakeLists.txt 
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
	
	"include/game.h"
	"include/engine.h"
	"include/object.h"
	"include/states.h"
	"include/utils.h"
	"include/game_xml.h"
	"include/game_expr.h"
	"include/game_sfml.h"
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	PROPERTY VS_STARTUP_PROJECT XMLGameEngine)

include(FetchContent)

option(FORCE_LOCAL_XERCESC "Force using a locally fetched XercesC instance" OFF)
option(FORCE_LOCAL_EXPRTK "Force using a locally fetched exprtk instance" OFF)
option(FORCE_LOCAL_SFML "Force using a locally fetched SFML instance" OFF)

if (NOT FORCE_LOCAL_XERCESC)
    find_package(XercesC QUIET)
endif()

if (NOT FORCE_LOCAL_EXPRTK)
    find_package(exprtk QUIET)
endif()

if (NOT FORCE_LOCAL_SFML)
    find_package(SFML 2.5...<3.0 COMPONENTS system window graphics network audio QUIET)
endif()

if (XercesC_FOUND)
    message(STATUS "XERCESC found: ${XercesC_LIBRARIES}")
else()
    message(STATUS "XercesC not found, using FetchContent to download and build it locally.")

	FetchContent_Declare(XercesC
		GIT_REPOSITORY https://github.com/apache/xerces-c.git
		GIT_TAG v3.3.0
		EXCLUDE_FROM_ALL)

	list(APPEND FETCHED_LIBRARIES XercesC)
endif()

if (exprtk_FOUND)
    message(STATUS "exprtk found: ${exprtk_LIBRARIES}")
else()
    message(STATUS "exprtk not found, using FetchContent to download and build it locally.")

	FetchContent_Declare(exprtk
		GIT_REPOSITORY https://github.com/ArashPartow/exprtk.git
		GIT_TAG 0.0.3-cmake
		EXCLUDE_FROM_ALL)

	list(APPEND FETCHED_LIBRARIES exprtk)
endif()

if (SFML_FOUND)
    message(STATUS "SFML found: ${SFML_LIBRARIES}")
else()
    message(STATUS "SFML not found, using FetchContent to download and build it locally.")

    FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.2
		EXCLUDE_FROM_ALL
    )

	list(APPEND FETCHED_LIBRARIES SFML)

    set(SFML_BUILD_FROM_SOURCE ON CACHE BOOL "Force SFML to build from source" FORCE)
    set(SFML_USE_SYSTEM_DEPS OFF CACHE BOOL "Use SFML's bundled dependencies" FORCE)
endif()

if (FETCHED_LIBRARIES)
	FetchContent_MakeAvailable(${FETCHED_LIBRARIES})
endif()

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(${PROJECT_NAME} PRIVATE xerces-c)
target_link_libraries(${PROJECT_NAME} PRIVATE exprtk)
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-system
	sfml-window sfml-graphics sfml-network sfml-audio)

target_compile_options(${PROJECT_NAME} PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/external:anglebrackets /external:W0 /analyze:external- /bigobj>)

if (NOT XercesC_FOUND)
	set_target_properties(xerces-c PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
	target_compile_options(xerces-c PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>:/W0> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>)
	set_target_properties(xerces-c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif()

if (NOT exprtk_FOUND)
target_compile_options(exprtk INTERFACE
	$<$<CXX_COMPILER_ID:MSVC>:/W0> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>)
endif()

if (NOT SFML_FOUND)
	set_target_properties(sfml-system PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
	set_target_properties(sfml-window PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
	set_target_properties(sfml-graphics PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
	set_target_properties(sfml-network PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
	set_target_properties(sfml-audio PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>)

add_dependencies(${PROJECT_NAME} data-target)

if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    set(data_xsd
		"assets/xmlgameengine.xsd")

    set(data_xml
		"games/pong.xml"
		"games/pong_full.xml"
		"games/breakout.xml"
		"games/spaceinvaders.xml")

    set(data_assets
		"assets/tuffy.ttf"
		"assets/paddle.jpg")

    foreach(item IN LISTS data_xsd data_xml data_assets)
        message(STATUS ${item})
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${item}" "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${item}")
    endforeach()
endif()

add_custom_target(data-target ALL
	DEPENDS ${data_xsd} ${data_xml} ${data_assets}
	SOURCES	${data_xsd} ${data_xml} ${data_assets})
	
source_group("XSD Files" FILES ${data_xsd})
source_group("XML Files" FILES ${data_xml})
source_group("Asset Files" FILES ${data_assets})
