# CMakeLists.txt 
# XML Game Engine
# author: beefviper
# date: Sept 18, 2020

cmake_minimum_required(VERSION 3.28)

project("XMLGameEngine")

include(scripts/cmake/options.cmake)
include(scripts/cmake/output.cmake)

# The engine is a library (the xgelib target, lib/);
# the programs that use it are separate targets - xgecli (the command line,
# cli/) and xgegui (the Qt application, gui/, built when Qt 6 is found); every
# project has its own source/ and include/ - see
# scripts/cmake/executables.cmake. Static by default; -DXGE_BUILD_SHARED=ON
# builds it as a shared library (a DLL on Windows) instead.
if (XGE_BUILD_SHARED)
	add_library(xgelib SHARED)
else()
	add_library(xgelib STATIC)
endif()

include(scripts/cmake/dependencies.cmake)
include(scripts/cmake/targets.cmake)
include(scripts/cmake/platform.cmake)
include(scripts/cmake/assets.cmake)

# The whole engine: game loading and evaluation, collisions, and the XML, Window
# and Audio backends that are built (below). Nothing here reads the command line or has a main();
# that is cli/ (xgecli). The test suite links this same library
# (scripts/cmake/tests.cmake), so there is one list of files to keep.
set(ENGINE_SOURCES
	"lib/source/game.cpp"
	"lib/source/data_folder.cpp"
	"lib/source/engine.cpp"
	"lib/source/object.cpp"
	"lib/source/states.cpp"
	"lib/source/color.cpp"
	"lib/source/bitmap.cpp"
	"lib/source/svg.cpp"
	"lib/source/builtin_font.cpp"
	"lib/source/keycode.cpp"
	"lib/source/game_xml.cpp"
	"lib/source/xml_document.cpp"
	"lib/source/xsd_lite.cpp"
	"lib/source/game_expr.cpp"
	"lib/source/command.cpp"
	"lib/source/collision_detector.cpp"
	"lib/source/command_executor.cpp"
	"lib/source/window.cpp"
	"lib/source/sound.cpp"
	"lib/source/audio.cpp"
)

set(ENGINE_HEADERS
	"lib/include/game.h"
	"lib/include/data_folder.h"
	"lib/include/engine.h"
	"lib/include/object.h"
	"lib/include/states.h"
	"lib/include/types.h"
	"lib/include/color.h"
	"lib/include/bitmap.h"
	"lib/include/svg.h"
	"lib/include/builtin_font.h"
	"lib/include/keycode.h"
	"lib/include/game_xml.h"
	"lib/include/xml_document.h"
	"lib/include/xsd_lite.h"
	"lib/include/game_expr.h"
	"lib/include/command.h"
	"lib/include/collision_detector.h"
	"lib/include/command_executor.h"
	"lib/include/window.h"
	"lib/include/sound.h"
	"lib/include/audio.h"
)

# The backends that are built (options.cmake), each a pair of files named after
# it. SFML 3, raylib and SDL2 give a window and a sound backend; OpenGL a
# window only.
foreach(backend IN ITEMS xerces tinyxml2 pugixml rapidxml)
	string(TOUPPER ${backend} upper)
	if (XGE_WITH_${upper})
		list(APPEND ENGINE_SOURCES "lib/source/xml_${backend}.cpp")
		list(APPEND ENGINE_HEADERS "lib/include/xml_${backend}.h")
	endif()
endforeach()

foreach(backend IN ITEMS sfml raylib sdl2 opengl)
	string(TOUPPER ${backend} upper)
	if (backend STREQUAL "sfml")
		set(upper SFML3)
	endif()

	if (XGE_WITH_${upper})
		list(APPEND ENGINE_SOURCES "lib/source/window_${backend}.cpp")
		list(APPEND ENGINE_HEADERS "lib/include/window_${backend}.h")

		if (NOT backend STREQUAL "opengl")
			list(APPEND ENGINE_SOURCES "lib/source/audio_${backend}.cpp")
			list(APPEND ENGINE_HEADERS "lib/include/audio_${backend}.h")
		endif()
	endif()
endforeach()

target_sources(xgelib PRIVATE
	${ENGINE_SOURCES}
	${ENGINE_HEADERS}
)

include(scripts/cmake/executables.cmake)

if (BUILD_TESTING)
	include(scripts/cmake/tests.cmake)
endif()
