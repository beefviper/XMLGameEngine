# CMakeLists.txt 
# XML Game Engine
# author: beefviper
# date: Sept 18, 2020

cmake_minimum_required(VERSION 3.28)

project("XMLGameEngine")

include(scripts/cmake/options.cmake)

# The engine is a library (the XMLGameEngine target, named for the project);
# the programs that use it are separate targets - XGECLI (the command line,
# cli/) and XGEGUI (a stub for a future windowed front end, gui/) - see
# scripts/cmake/executables.cmake. Static by default; -DXGE_BUILD_SHARED=ON
# builds it as a shared library (a DLL on Windows) instead.
if (XGE_BUILD_SHARED)
	add_library(${PROJECT_NAME} SHARED)
else()
	add_library(${PROJECT_NAME} STATIC)
endif()

include(scripts/cmake/dependencies.cmake)
include(scripts/cmake/targets.cmake)
include(scripts/cmake/platform.cmake)
include(scripts/cmake/assets.cmake)

# The whole engine: game loading and evaluation, collisions, and every XML
# and Window backend. Nothing here reads the command line or has a main();
# that is cli/ (XGECLI). The test suite links this same library
# (scripts/cmake/tests.cmake), so there is one list of files to keep.
set(ENGINE_SOURCES
	"source/game.cpp"
	"source/engine.cpp"
	"source/object.cpp"
	"source/states.cpp"
	"source/color.cpp"
	"source/bitmap.cpp"
	"source/keycode.cpp"
	"source/game_xml.cpp"
	"source/xml_document.cpp"
	"source/xml_xerces.cpp"
	"source/xml_tinyxml2.cpp"
	"source/xml_pugixml.cpp"
	"source/xml_rapidxml.cpp"
	"source/xsd_lite.cpp"
	"source/game_expr.cpp"
	"source/command.cpp"
	"source/collision_detector.cpp"
	"source/command_executor.cpp"
	"source/window.cpp"
	"source/window_sfml.cpp"
	"source/window_raylib.cpp"
	"source/window_sdl2.cpp"
)

set(ENGINE_HEADERS
	"include/game.h"
	"include/engine.h"
	"include/object.h"
	"include/states.h"
	"include/types.h"
	"include/color.h"
	"include/bitmap.h"
	"include/keycode.h"
	"include/game_xml.h"
	"include/xml_document.h"
	"include/xml_xerces.h"
	"include/xml_tinyxml2.h"
	"include/xml_pugixml.h"
	"include/xml_rapidxml.h"
	"include/xsd_lite.h"
	"include/game_expr.h"
	"include/command.h"
	"include/collision_detector.h"
	"include/command_executor.h"
	"include/window.h"
	"include/window_sfml.h"
	"include/window_raylib.h"
	"include/window_sdl2.h"
)

target_sources(${PROJECT_NAME} PRIVATE
	${ENGINE_SOURCES}
	${ENGINE_HEADERS}
)

include(scripts/cmake/executables.cmake)

if (BUILD_TESTING)
	include(scripts/cmake/tests.cmake)
endif()
