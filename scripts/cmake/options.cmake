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
force_local_option(EXPRTK "exprtk")
force_local_option(SFML "SFML")
force_local_option(RAYLIB "raylib")
force_local_option(GLFW "GLFW")
force_local_option(SDL2 "SDL2")
force_local_option(SDL2_IMAGE "SDL2_image")
force_local_option(SDL2_TTF "SDL2_ttf")
force_local_option(TINYXML2 "TinyXML2")
force_local_option(PUGIXML "PugiXML")
force_local_option(RAPIDXML "RapidXML")
force_local_option(LUNASVG "lunasvg")
force_local_option(CATCH2 "Catch2")

# Declared here, before scripts/cmake/tests.cmake's own include(CTest), so
# CTest's module (which declares this same cache variable, defaulting to ON)
# sees it already set and leaves our default alone: the test suite pulls in
# Catch2 (found via vcpkg/find_package, or fetched and built from source if
# not installed), so it's opt-in rather than part of an ordinary build.
option(BUILD_TESTING "Build the Catch2-based test suite in tests/" OFF)

# How the engine library (the XGELIB target) is built, and so how
# XGECLI, XGEGUI and the tests link to it. OFF (the default) is a static
# library: its code is copied into each program, which is one self-contained
# .exe. ON is a shared library (a DLL on Windows, a .so on Linux): one copy
# of the engine that the programs load at run time, which is what a plug-in
# or a second front end sharing one engine install would want. See
# docs/readme.md, "Building".
option(XGE_BUILD_SHARED "Build the engine as a shared library instead of a static one" OFF)
