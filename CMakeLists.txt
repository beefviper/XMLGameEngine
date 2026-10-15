# CMakeLists.txt 
# XML Game Engine
# author: beefviper
# date: Sept 18, 2020

cmake_minimum_required(VERSION 3.28)

project("XMLGameEngine")

include(scripts/cmake/options.cmake)
include(scripts/cmake/output.cmake)

# The engine is a library (the xgelib target, xge/lib/);
# the programs that use it are separate targets - xgecli (the command line,
# xge/cli/) and xgegui (the Qt application, xge/gui/, built when Qt 6 is found); every
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
# that is xge/cli/ (xgecli). The test suite links this same library
# (scripts/cmake/tests.cmake), so there is one list of files to keep.
set(ENGINE_SOURCES
	"xge/lib/source/game.cpp"
	"xge/lib/source/data_folder.cpp"
	"xge/lib/source/engine.cpp"
	"xge/lib/source/engine_backends.cpp"
	"xge/lib/source/object.cpp"
	"xge/lib/source/states.cpp"
	"xge/lib/source/color.cpp"
	"xge/lib/source/bitmap.cpp"
	"xge/lib/source/svg.cpp"
	"xge/lib/source/builtin_font.cpp"
	"xge/lib/source/keycode.cpp"
	"xge/lib/source/spelling.cpp"
	"xge/lib/source/game_xml.cpp"
	"xge/lib/source/xml_document.cpp"
	"xge/lib/source/xsd_lite.cpp"
	"xge/lib/source/game_expr.cpp"
	"xge/lib/source/scenes.cpp"
	"xge/lib/source/command.cpp"
	"xge/lib/source/geometry.cpp"
	"xge/lib/source/verbs.cpp"
	"xge/lib/source/frame.cpp"
	"xge/lib/source/random.cpp"
	"xge/lib/source/collision_detector.cpp"
	"xge/lib/source/command_executor.cpp"
	"xge/lib/source/window.cpp"
	"xge/lib/source/sound.cpp"
	"xge/lib/source/audio.cpp"
)

set(ENGINE_HEADERS
	"xge/lib/include/game.h"
	"xge/lib/include/data_folder.h"
	"xge/lib/include/engine.h"
	"xge/lib/include/object.h"
	"xge/lib/include/states.h"
	"xge/lib/include/scenes.h"
	"xge/lib/include/types.h"
	"xge/lib/include/color.h"
	"xge/lib/include/bitmap.h"
	"xge/lib/include/svg.h"
	"xge/lib/include/builtin_font.h"
	"xge/lib/include/keycode.h"
	"xge/lib/include/spelling.h"
	"xge/lib/include/game_xml.h"
	"xge/lib/include/xml_document.h"
	"xge/lib/include/xsd_lite.h"
	"xge/lib/include/game_data.h"
	"xge/lib/include/game_expr.h"
	"xge/lib/include/command.h"
	"xge/lib/include/geometry.h"
	"xge/lib/include/verbs.h"
	"xge/lib/include/frame.h"
	"xge/lib/include/random.h"
	"xge/lib/include/collision_detector.h"
	"xge/lib/include/command_executor.h"
	"xge/lib/include/window.h"
	"xge/lib/include/sound.h"
	"xge/lib/include/audio.h"
)

# The backends that are built (options.cmake), each a pair of files named after
# it. SFML 3, raylib and SDL2 give a window and a sound backend; OpenGL a
# window only.
foreach(backend IN ITEMS xerces tinyxml2 pugixml rapidxml)
	string(TOUPPER ${backend} upper)
	if (XGE_WITH_${upper})
		list(APPEND ENGINE_SOURCES "xge/lib/source/xml_${backend}.cpp")
		list(APPEND ENGINE_HEADERS "xge/lib/include/xml_${backend}.h")
	endif()
endforeach()

foreach(backend IN ITEMS sfml raylib sdl2 opengl)
	string(TOUPPER ${backend} upper)
	if (backend STREQUAL "sfml")
		set(upper SFML3)
	endif()

	if (XGE_WITH_${upper})
		list(APPEND ENGINE_SOURCES "xge/lib/source/window_${backend}.cpp")
		list(APPEND ENGINE_HEADERS "xge/lib/include/window_${backend}.h")

		if (NOT backend STREQUAL "opengl")
			list(APPEND ENGINE_SOURCES "xge/lib/source/audio_${backend}.cpp")
			list(APPEND ENGINE_HEADERS "xge/lib/include/audio_${backend}.h")
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
