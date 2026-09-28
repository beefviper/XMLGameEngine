# options.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

option(FORCE_LOCAL_XERCESC "Force using a locally fetched XercesC instance" OFF)
option(FORCE_LOCAL_EXPRTK "Force using a locally fetched exprtk instance" OFF)
option(FORCE_LOCAL_SFML "Force using a locally fetched SFML instance" OFF)
option(FORCE_LOCAL_RAYLIB "Force using a locally fetched raylib instance" OFF)
option(FORCE_LOCAL_SDL2 "Force using a locally fetched SDL2 instance" OFF)
option(FORCE_LOCAL_SDL2_IMAGE "Force using a locally fetched SDL2_image instance" OFF)
option(FORCE_LOCAL_SDL2_TTF "Force using a locally fetched SDL2_ttf instance" OFF)
option(FORCE_LOCAL_CATCH2 "Force using a locally fetched Catch2 instance" OFF)

# Declared here, before scripts/cmake/tests.cmake's own include(CTest), so
# CTest's module (which declares this same cache variable, defaulting to ON)
# sees it already set and leaves our default alone: the test suite pulls in
# Catch2 (found via vcpkg/find_package, or fetched and built from source if
# not installed), so it's opt-in rather than part of an ordinary build.
option(BUILD_TESTING "Build the Catch2-based test suite in tests/" OFF)
