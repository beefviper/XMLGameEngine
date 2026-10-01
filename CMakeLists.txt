# CMakeLists.txt 
# XML Game Engine
# author: beefviper
# date: Sept 18, 2020

cmake_minimum_required(VERSION 3.28)

project("XMLGameEngine")

include(scripts/cmake/options.cmake)

# The engine is a library (the XGELIB target, lib/);
# the programs that use it are separate targets - XGECLI (the command line,
# cli/) and XGEGUI (a stub for a future windowed front end, gui/); every
# project has its own source/ and include/ - see
# scripts/cmake/executables.cmake. Static by default; -DXGE_BUILD_SHARED=ON
# builds it as a shared library (a DLL on Windows) instead.
if (XGE_BUILD_SHARED)
	add_library(XGELIB SHARED)
else()
	add_library(XGELIB STATIC)
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
	"lib/source/game.cpp"
	"lib/source/engine.cpp"
	"lib/source/object.cpp"
	"lib/source/states.cpp"
	"lib/source/color.cpp"
	"lib/source/bitmap.cpp"
	"lib/source/keycode.cpp"
	"lib/source/game_xml.cpp"
	"lib/source/xml_document.cpp"
	"lib/source/xml_xerces.cpp"
	"lib/source/xml_tinyxml2.cpp"
	"lib/source/xml_pugixml.cpp"
	"lib/source/xml_rapidxml.cpp"
	"lib/source/xsd_lite.cpp"
	"lib/source/game_expr.cpp"
	"lib/source/command.cpp"
	"lib/source/collision_detector.cpp"
	"lib/source/command_executor.cpp"
	"lib/source/window.cpp"
	"lib/source/window_sfml.cpp"
	"lib/source/window_raylib.cpp"
	"lib/source/window_sdl2.cpp"
)

set(ENGINE_HEADERS
	"lib/include/game.h"
	"lib/include/engine.h"
	"lib/include/object.h"
	"lib/include/states.h"
	"lib/include/types.h"
	"lib/include/color.h"
	"lib/include/bitmap.h"
	"lib/include/keycode.h"
	"lib/include/game_xml.h"
	"lib/include/xml_document.h"
	"lib/include/xml_xerces.h"
	"lib/include/xml_tinyxml2.h"
	"lib/include/xml_pugixml.h"
	"lib/include/xml_rapidxml.h"
	"lib/include/xsd_lite.h"
	"lib/include/game_expr.h"
	"lib/include/command.h"
	"lib/include/collision_detector.h"
	"lib/include/command_executor.h"
	"lib/include/window.h"
	"lib/include/window_sfml.h"
	"lib/include/window_raylib.h"
	"lib/include/window_sdl2.h"
)

target_sources(XGELIB PRIVATE
	${ENGINE_SOURCES}
	${ENGINE_HEADERS}
)

include(scripts/cmake/executables.cmake)

if (BUILD_TESTING)
	include(scripts/cmake/tests.cmake)
endif()
