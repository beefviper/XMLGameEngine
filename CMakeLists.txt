# CMakeLists.txt 
# XML Game Engine
# author: beefviper
# date: Sept 18, 2020

cmake_minimum_required(VERSION 3.28)

project("XMLGameEngine")

# Visual Studio compiles a project's files one at a time unless told /MP. Set
# before the dependencies are fetched, so they build that way too.
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/MP> $<$<COMPILE_LANG_AND_ID:C,MSVC>:/MP>)

include(scripts/cmake/options.cmake)
include(scripts/cmake/output.cmake)

# The engine is a library (the xgelib target, xge/lib/);
# the programs that use it are separate targets - xgecli (the command line,
# xge/cli/) and xgegui (the Qt application, xge/gui/, built when Qt 6 is found), each
# with its own source/ and include/ - see scripts/cmake/executables.cmake. The
# engine's files are in a folder for each part of it (game/, play/, load/, xml/,
# window/, audio/, picture/, engine/), a header beside its source, and are
# included by that path ("game/game.h"). Static by default; -DXGE_BUILD_SHARED=ON
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
	"xge/lib/game/game.cpp"
	"xge/lib/game/cells.cpp"
	"xge/lib/game/think.cpp"
	"xge/lib/game/cards.cpp"
	"xge/lib/game/players.cpp"
	"xge/lib/engine/folder.cpp"
	"xge/lib/engine/engine.cpp"
	"xge/lib/engine/backends.cpp"
	"xge/lib/game/object.cpp"
	"xge/lib/game/states.cpp"
	"xge/lib/picture/color.cpp"
	"xge/lib/picture/bitmap.cpp"
	"xge/lib/picture/svg.cpp"
	"xge/lib/picture/image.cpp"
	"xge/lib/picture/font.cpp"
	"xge/lib/picture/shapes.cpp"
	"xge/lib/engine/keys.cpp"
	"xge/lib/load/spelling.cpp"
	"xge/lib/load/merge.cpp"
	"xge/lib/load/xml.cpp"
	"xge/lib/load/reading.cpp"
	"xge/lib/xml/document.cpp"
	"xge/lib/load/xsd.cpp"
	"xge/lib/load/expr.cpp"
	"xge/lib/load/check.cpp"
	"xge/lib/load/calc.cpp"
	"xge/lib/game/scenes.cpp"
	"xge/lib/game/command.cpp"
	"xge/lib/play/geometry.cpp"
	"xge/lib/play/verbs.cpp"
	"xge/lib/play/frame.cpp"
	"xge/lib/play/maze.cpp"
	"xge/lib/game/random.cpp"
	"xge/lib/play/collision.cpp"
	"xge/lib/play/executor.cpp"
	"xge/lib/window/window.cpp"
	"xge/lib/window/null.cpp"
	"xge/lib/audio/sound.cpp"
	"xge/lib/audio/wave.cpp"
	"xge/lib/audio/audio.cpp"
)

set(ENGINE_HEADERS
	"xge/lib/game/game.h"
	"xge/lib/engine/folder.h"
	"xge/lib/engine/engine.h"
	"xge/lib/game/object.h"
	"xge/lib/game/states.h"
	"xge/lib/game/condition.h"
	"xge/lib/game/scenes.h"
	"xge/lib/game/types.h"
	"xge/lib/picture/color.h"
	"xge/lib/picture/bitmap.h"
	"xge/lib/picture/svg.h"
	"xge/lib/picture/font.h"
	"xge/lib/picture/shapes.h"
	"xge/lib/engine/keys.h"
	"xge/lib/load/spelling.h"
	"xge/lib/load/merge.h"
	"xge/lib/load/xml.h"
	"xge/lib/load/reading.h"
	"xge/lib/xml/document.h"
	"xge/lib/load/xsd.h"
	"xge/lib/game/data.h"
	"xge/lib/load/expr.h"
	"xge/lib/load/check.h"
	"xge/lib/load/calc.h"
	"xge/lib/game/command.h"
	"xge/lib/play/geometry.h"
	"xge/lib/play/numbers.h"
	"xge/lib/play/verbs.h"
	"xge/lib/play/frame.h"
	"xge/lib/play/maze.h"
	"xge/lib/game/random.h"
	"xge/lib/play/collision.h"
	"xge/lib/play/executor.h"
	"xge/lib/window/window.h"
	"xge/lib/window/null.h"
	"xge/lib/audio/sound.h"
	"xge/lib/audio/wave.h"
	"xge/lib/audio/audio.h"
)

# The backends that are built (options.cmake), each a pair of files named after
# it in the folder of its kind (window/sfml.cpp). SFML 3, raylib and SDL2 give a window and a sound backend; OpenGL and
# Direct3D a window only. The Direct3D windows share one Win32 window (window/win32.cpp).
foreach(backend IN ITEMS xerces tinyxml2 pugixml rapidxml)
	string(TOUPPER ${backend} upper)
	if (XGE_WITH_${upper})
		list(APPEND ENGINE_SOURCES "xge/lib/xml/${backend}.cpp")
		list(APPEND ENGINE_HEADERS "xge/lib/xml/${backend}.h")
	endif()
endforeach()

foreach(backend IN ITEMS sfml raylib sdl2 opengl direct3d9 direct3d11)
	string(TOUPPER ${backend} upper)
	if (backend STREQUAL "sfml")
		set(upper SFML3)
	endif()

	if (XGE_WITH_${upper})
		list(APPEND ENGINE_SOURCES "xge/lib/window/${backend}.cpp")
		list(APPEND ENGINE_HEADERS "xge/lib/window/${backend}.h")

		if (NOT backend MATCHES "^(opengl|direct3d)")
			list(APPEND ENGINE_SOURCES "xge/lib/audio/${backend}.cpp")
			list(APPEND ENGINE_HEADERS "xge/lib/audio/${backend}.h")
		endif()
	endif()
endforeach()

if (XGE_WITH_DIRECT3D9 OR XGE_WITH_DIRECT3D11)
	list(APPEND ENGINE_SOURCES "xge/lib/window/win32.cpp")
	list(APPEND ENGINE_HEADERS "xge/lib/window/win32.h")
endif()

# The verbs: a file each in xge/lib/verbs/, listed in its all.cpp. They are
# small, so they are compiled as one.
file(GLOB ENGINE_VERBS CONFIGURE_DEPENDS "xge/lib/verbs/*.cpp")
list(APPEND ENGINE_SOURCES ${ENGINE_VERBS})
list(APPEND ENGINE_HEADERS "xge/lib/verbs/verb.h")
set_source_files_properties(${ENGINE_VERBS} PROPERTIES UNITY_GROUP verbs)

target_sources(xgelib PRIVATE
	${ENGINE_SOURCES}
	${ENGINE_HEADERS}
)
set_target_properties(xgelib PROPERTIES UNITY_BUILD ON UNITY_BUILD_MODE GROUP)

include(scripts/cmake/executables.cmake)

if (BUILD_TESTING)
	include(scripts/cmake/tests.cmake)
endif()
