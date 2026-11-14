# options.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

# All FORCE_LOCAL_* options below share the same shape - see
# declare_fetched_dependency() in dependencies.cmake for the matching
# find-or-fetch logic each one controls.
macro(force_local_option name display_name)
	option(FORCE_LOCAL_${name} "Force using a locally fetched ${display_name} instance" OFF)
endmacro()

force_local_option(XERCESC "XercesC")
force_local_option(SFML "SFML")
force_local_option(RAYLIB "raylib")
force_local_option(GLFW "GLFW")
force_local_option(SDL2 "SDL2")
force_local_option(TINYXML2 "TinyXML2")
force_local_option(PUGIXML "PugiXML")
force_local_option(RAPIDXML "RapidXML")
force_local_option(VULKANHEADERS "Vulkan-Headers")
force_local_option(VOLK "volk")
force_local_option(LUNASVG "lunasvg")
force_local_option(CATCH2 "Catch2")

# Declared here, before scripts/cmake/tests.cmake's own include(CTest), so
# CTest's module (which declares this same cache variable, defaulting to ON)
# sees it already set and leaves our default alone: the test suite pulls in
# Catch2 (found via vcpkg/find_package, or fetched and built from source if
# not installed), so it's opt-in rather than part of an ordinary build.
option(BUILD_TESTING "Build the Catch2-based test suite in xge/tests/" OFF)

# How the engine library (the xgelib target) is built, and so how
# xgecli, xgegui and the tests link to it. OFF (the default) is a static
# library: its code is copied into each program, which is one self-contained
# .exe. ON is a shared library (a DLL on Windows, a .so on Linux): one copy
# of the engine that the programs load at run time, which is what a plug-in
# or a second front end sharing one engine install would want. See
# docs/reference/engine.md, "Building".
option(XGE_BUILD_SHARED "Build the engine as a shared library instead of a static one" OFF)

# A sanitizer build, for GCC and Clang, off unless asked for: "address"
# checks memory and undefined behaviour (-fsanitize=address,undefined),
# "thread" checks data races (-fsanitize=thread). Only the project's own
# targets are instrumented (xge_warnings in platform.cmake), not the fetched
# libraries; their strict warnings are turned off, because with both GCC
# takes many minutes a file. Run with ASAN_OPTIONS=detect_container_overflow=0
# (the libraries' containers are not instrumented). See docs/reference/engine.md.
set(XGE_SANITIZE "" CACHE STRING "Sanitizer build for GCC and Clang: address (with undefined) or thread; empty for none")
set_property(CACHE XGE_SANITIZE PROPERTY STRINGS "" address thread)
if (XGE_SANITIZE AND NOT XGE_SANITIZE MATCHES "^(address|thread)$")
	message(FATAL_ERROR "XGE_SANITIZE is '${XGE_SANITIZE}'; expected address or thread")
endif()
if (XGE_SANITIZE AND MSVC)
	message(FATAL_ERROR "XGE_SANITIZE is for GCC and Clang; MSVC has only /fsanitize=address, which this option does not set")
endif()

# Which window, sound and XML backends are built into the engine. The default
# is one of each, SFML 3 (window and sound) and Xerces (XML), which is all a
# game needs and the least there is to find, fetch and compile. The others are
# there to compare against and to choose between: ask for each with
# -DXGE_WITH_<NAME>=ON, or for every one at once with -DXGE_ALL_BACKENDS=ON.
# The test suite plays the same games through all of them and so needs all of
# them: BUILD_TESTING turns XGE_ALL_BACKENDS on.
#
# Sound follows its library: SFML3, RAYLIB and SDL2 each bring their own, and
# OPENGL, VULKAN and the DIRECT3D ones are windows only. A program built without a backend still knows its
# name; asking for it says it is not built in (see WindowFactory::available).
option(XGE_ALL_BACKENDS "Build every window, sound and XML backend (BUILD_TESTING turns this on)" OFF)

if (BUILD_TESTING)
	set(XGE_ALL_BACKENDS ON)
endif()

macro(backend_option name display default)
	option(XGE_WITH_${name} "Build the ${display} backend" ${default})

	if (XGE_ALL_BACKENDS)
		set(XGE_WITH_${name} ON)
	endif()
endmacro()

backend_option(SFML3 "SFML 3 (window and sound)" ON)
backend_option(RAYLIB "raylib (window and sound)" OFF)
backend_option(SDL2 "SDL2 (window and sound)" OFF)
backend_option(OPENGL "OpenGL, through GLFW (window)" OFF)
backend_option(VULKAN "Vulkan, through GLFW (window)" OFF)
backend_option(DIRECT3D9 "Direct3D 9, Windows only (window)" OFF)
backend_option(DIRECT3D11 "Direct3D 11, Windows only (window)" OFF)
backend_option(DIRECT3D12 "Direct3D 12, Windows only (window)" OFF)
backend_option(XERCES "Xerces (XML, with full schema validation)" ON)
backend_option(TINYXML2 "TinyXML2 (XML)" OFF)
backend_option(PUGIXML "PugiXML (XML)" OFF)
backend_option(RAPIDXML "RapidXML (XML)" OFF)

# Direct3D comes with Windows (its SDK with Visual Studio's C++ tools), and is
# on nowhere else: every backend on Linux is the other four.
foreach(direct3d IN ITEMS DIRECT3D9 DIRECT3D11 DIRECT3D12)
	if (XGE_WITH_${direct3d} AND NOT WIN32)
		if (NOT XGE_ALL_BACKENDS)
			message(WARNING "Direct3D is a Windows library: XGE_WITH_${direct3d} is turned off here.")
		endif()
		set(XGE_WITH_${direct3d} OFF)
	endif()
endforeach()

if (NOT (XGE_WITH_SFML3 OR XGE_WITH_RAYLIB OR XGE_WITH_SDL2 OR XGE_WITH_OPENGL OR XGE_WITH_VULKAN OR XGE_WITH_DIRECT3D9 OR XGE_WITH_DIRECT3D11 OR XGE_WITH_DIRECT3D12))
	message(FATAL_ERROR "No window backend is built: turn on at least one of XGE_WITH_SFML3, XGE_WITH_RAYLIB, XGE_WITH_SDL2, XGE_WITH_OPENGL, XGE_WITH_VULKAN, XGE_WITH_DIRECT3D9, XGE_WITH_DIRECT3D11 and XGE_WITH_DIRECT3D12 (or XGE_ALL_BACKENDS).")
endif()

if (NOT (XGE_WITH_XERCES OR XGE_WITH_TINYXML2 OR XGE_WITH_PUGIXML OR XGE_WITH_RAPIDXML))
	message(FATAL_ERROR "No XML backend is built: turn on at least one of XGE_WITH_XERCES, XGE_WITH_TINYXML2, XGE_WITH_PUGIXML and XGE_WITH_RAPIDXML (or XGE_ALL_BACKENDS).")
endif()

set(XGE_BACKENDS_BUILT "")
foreach(backend IN ITEMS SFML3 RAYLIB SDL2 OPENGL VULKAN DIRECT3D9 DIRECT3D11 DIRECT3D12 XERCES TINYXML2 PUGIXML RAPIDXML)
	if (XGE_WITH_${backend})
		list(APPEND XGE_BACKENDS_BUILT ${backend})
	endif()
endforeach()
message(STATUS "Backends built: ${XGE_BACKENDS_BUILT}")
