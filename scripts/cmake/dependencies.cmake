# dependencies.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

include(FetchContent)

# Every third-party dependency below follows the same shape: try to find it
# already installed (via find_package/find_path just below), and if that
# fails, FetchContent_Declare it from its upstream git repo so
# FetchContent_MakeAvailable() (further down) downloads and builds it as
# part of this build.
#   FOUND_VAR      the variable find_package/find_path already set, truthy
#                  when found locally
#   DISPLAY_NAME   name to use in the found/not-found status messages
#   INFO_VAR       variable to print alongside "found" - usually <Name>_DIR
#                  (the install dir CMake sets for any config-mode find,
#                  a real location on disk); XercesC/exprtk/RapidXML use
#                  something else since they're found via find_path() or
#                  an old-style Module rather than a config package
#   NAME           name passed to FetchContent_Declare/MakeAvailable
#   REPO / TAG     upstream git repo and tag/commit to fetch
#   SET_FOUND_VAR  only needed for exprtk/RapidXML, which have no _FOUND
#                  variable of their own for the rest of the project to
#                  check - this defines one when found locally
#   FETCH_VERB     defaults to "download and build it locally"; RapidXML
#                  overrides it since it's header-only (nothing to build)
macro(declare_fetched_dependency)
	cmake_parse_arguments(DFD "" "FOUND_VAR;DISPLAY_NAME;INFO_VAR;NAME;REPO;TAG;SET_FOUND_VAR;FETCH_VERB" "" ${ARGN})

	if (NOT DFD_FETCH_VERB)
		set(DFD_FETCH_VERB "download and build it locally")
	endif()

	if (${DFD_FOUND_VAR})
		# Not every find_package()/find_path() result populates the same
		# kind of variable (a modern config-mode package may define only
		# imported targets, no classic _LIBRARIES list) - so only show the
		# ": <value>" detail when INFO_VAR actually resolved to something,
		# rather than printing a bare trailing colon for those.
		if (DFD_INFO_VAR AND ${DFD_INFO_VAR})
			message(STATUS "${DFD_DISPLAY_NAME} found: ${${DFD_INFO_VAR}}")
		else()
			message(STATUS "${DFD_DISPLAY_NAME} found")
		endif()

		if (DEFINED DFD_SET_FOUND_VAR)
			set(${DFD_SET_FOUND_VAR} TRUE)
		endif()
	else()
		message(STATUS "${DFD_DISPLAY_NAME} not found, using FetchContent to ${DFD_FETCH_VERB}.")

		FetchContent_Declare(${DFD_NAME}
			GIT_REPOSITORY ${DFD_REPO}
			GIT_TAG ${DFD_TAG}
			EXCLUDE_FROM_ALL)

		list(APPEND FETCHED_LIBRARIES ${DFD_NAME})
	endif()
endmacro()

if (NOT FORCE_LOCAL_XERCESC)
	find_package(XercesC QUIET)
endif()

if (NOT FORCE_LOCAL_EXPRTK)
	find_path(EXPRTK_INCLUDE_DIRS "exprtk.hpp")
endif()

if (NOT FORCE_LOCAL_SFML)
	find_package(SFML 3 COMPONENTS System Window Graphics Network Audio QUIET)
endif()

if (NOT FORCE_LOCAL_RAYLIB)
	find_package(raylib QUIET)
endif()

# The system's OpenGL library, for the OpenGL backend (window_opengl.cpp). It
# is part of every platform's SDK, so it is only ever found, never fetched.
find_package(OpenGL REQUIRED)

# GLFW is the OpenGL backend's window and keyboard (window_opengl.h), and raylib
# is built on it too: vcpkg installs it for raylib, and a raylib built from
# source brings its own `glfw` target.
if (NOT FORCE_LOCAL_GLFW)
	find_package(glfw3 QUIET)
endif()

if (NOT FORCE_LOCAL_SDL2)
	find_package(SDL2 QUIET)
endif()

if (NOT FORCE_LOCAL_SDL2_IMAGE)
	find_package(SDL2_image QUIET)
endif()

if (NOT FORCE_LOCAL_SDL2_TTF)
	find_package(SDL2_ttf QUIET)
endif()

if (NOT FORCE_LOCAL_TINYXML2)
	find_package(tinyxml2 QUIET)
endif()

if (NOT FORCE_LOCAL_PUGIXML)
	find_package(pugixml QUIET)
endif()

# RapidXML has no official CMake package of its own to find_package() -
# same situation exprtk is in below, so this probes for its header the same
# way exprtk does (EXPRTK_INCLUDE_DIRS), rather than pretending a RapidXML
# config package might exist.
if (NOT FORCE_LOCAL_RAPIDXML)
	find_path(RAPIDXML_INCLUDE_DIRS "rapidxml.hpp")
endif()

# Qt 6 is only for XGEGUI (gui/); nothing in the engine library uses it. Unlike
# everything above it is found, never fetched: building Qt from source is not
# something a configure step should do. Without it XGEGUI is left out and the
# rest builds as before. With vcpkg: vcpkg install qtbase[widgets]
find_package(Qt6 COMPONENTS Widgets QUIET)

if (Qt6_FOUND)
	message(STATUS "Qt6 found: ${Qt6_DIR}")

	# Optional: with OpenGL the game view is a QOpenGLWidget, which presents
	# each frame on the display's vertical blank (no tearing). Without it the
	# view is an ordinary widget and the program works the same, but can tear.
	# With vcpkg: vcpkg install qtbase[widgets,opengl]
	find_package(Qt6OpenGLWidgets QUIET)

	if (Qt6OpenGLWidgets_FOUND)
		message(STATUS "Qt6 OpenGLWidgets found: the game view will wait for the display's refresh.")
	else()
		message(STATUS "Qt6 OpenGLWidgets not found: the game view will not wait for the display's refresh (frames can tear).")
	endif()
else()
	message(STATUS "Qt6 not found, XGEGUI will not be built (install qtbase with the widgets feature).")
endif()

declare_fetched_dependency(
	FOUND_VAR XercesC_FOUND
	DISPLAY_NAME "XercesC"
	INFO_VAR XercesC_LIBRARIES
	NAME XercesC
	REPO https://github.com/apache/xerces-c.git
	TAG v3.3.0)

declare_fetched_dependency(
	FOUND_VAR EXPRTK_INCLUDE_DIRS
	DISPLAY_NAME "exprtk"
	INFO_VAR EXPRTK_INCLUDE_DIRS
	NAME exprtk
	REPO https://github.com/ArashPartow/exprtk.git
	TAG 0.0.3-cmake
	SET_FOUND_VAR EXPRTK_PACKAGE_FOUND)

declare_fetched_dependency(
	FOUND_VAR SFML_FOUND
	DISPLAY_NAME "SFML"
	INFO_VAR SFML_DIR
	NAME SFML
	REPO https://github.com/SFML/SFML.git
	TAG 3.1.0)

if (NOT SFML_FOUND)
	set(SFML_BUILD_FROM_SOURCE ON CACHE BOOL "Force SFML to build from source" FORCE)
	set(SFML_USE_SYSTEM_DEPS OFF CACHE BOOL "Use SFML's bundled dependencies" FORCE)
endif()

declare_fetched_dependency(
	FOUND_VAR raylib_FOUND
	DISPLAY_NAME "raylib"
	INFO_VAR raylib_DIR
	NAME raylib
	REPO https://github.com/raysan5/raylib.git
	TAG 5.5)

# When neither GLFW nor raylib was found, the fetched raylib builds GLFW itself
# and defines the same `glfw` target, so GLFW is not fetched a second time.
if (glfw3_FOUND OR raylib_FOUND)
	declare_fetched_dependency(
		FOUND_VAR glfw3_FOUND
		DISPLAY_NAME "GLFW"
		INFO_VAR glfw3_DIR
		NAME glfw
		REPO https://github.com/glfw/glfw.git
		TAG 3.4)

	if (NOT glfw3_FOUND)
		set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Do not build GLFW's examples" FORCE)
		set(GLFW_BUILD_TESTS OFF CACHE BOOL "Do not build GLFW's tests" FORCE)
		set(GLFW_BUILD_DOCS OFF CACHE BOOL "Do not build GLFW's documentation" FORCE)
		set(GLFW_INSTALL OFF CACHE BOOL "Disable GLFW's own install rules" FORCE)
	endif()
else()
	message(STATUS "GLFW not found, and neither is raylib: the raylib built here brings its own.")
endif()

declare_fetched_dependency(
	FOUND_VAR SDL2_FOUND
	DISPLAY_NAME "SDL2"
	INFO_VAR SDL2_DIR
	NAME SDL2
	REPO https://github.com/libsdl-org/SDL.git
	TAG release-2.30.9)

# Built against whichever SDL2 target ends up available above (found or
# fetched) - SDL2_image's own CMakeLists picks it up the same way this
# project's other fetched libraries do.
declare_fetched_dependency(
	FOUND_VAR SDL2_image_FOUND
	DISPLAY_NAME "SDL2_image"
	INFO_VAR SDL2_image_DIR
	NAME SDL2_image
	REPO https://github.com/libsdl-org/SDL_image.git
	TAG release-2.8.2)

if (NOT SDL2_image_FOUND)
	set(SDL2IMAGE_INSTALL OFF CACHE BOOL "Disable SDL2_image's own install rules" FORCE)
	set(SDL2IMAGE_VENDORED ON CACHE BOOL "Build SDL2_image's bundled image libraries from source" FORCE)
endif()

declare_fetched_dependency(
	FOUND_VAR SDL2_ttf_FOUND
	DISPLAY_NAME "SDL2_ttf"
	INFO_VAR SDL2_ttf_DIR
	NAME SDL2_ttf
	REPO https://github.com/libsdl-org/SDL_ttf.git
	TAG release-2.22.0)

if (NOT SDL2_ttf_FOUND)
	set(SDL2TTF_INSTALL OFF CACHE BOOL "Disable SDL2_ttf's own install rules" FORCE)
	set(SDL2TTF_VENDORED ON CACHE BOOL "Build SDL2_ttf's bundled FreeType from source" FORCE)
endif()

declare_fetched_dependency(
	FOUND_VAR tinyxml2_FOUND
	DISPLAY_NAME "TinyXML2"
	INFO_VAR tinyxml2_DIR
	NAME tinyxml2
	REPO https://github.com/leethomason/tinyxml2.git
	TAG 11.0.0)

declare_fetched_dependency(
	FOUND_VAR pugixml_FOUND
	DISPLAY_NAME "PugiXML"
	INFO_VAR pugixml_DIR
	NAME pugixml
	REPO https://github.com/zeux/pugixml.git
	TAG v1.16)

# Pinned by commit rather than a tag - upstream (the discord/rapidxml
# mirror of the last released 1.13, the version this project's own
# xml_rapidxml.cpp is written against) has never cut a tagged release.
# RapidXML is header-only with no CMakeLists.txt of its own (checked -
# it has none), so unlike this project's other fetched libraries it's
# not add_subdirectory()'d by FetchContent_MakeAvailable below - see the
# rapidxml::rapidxml target synthesized further down instead.
declare_fetched_dependency(
	FOUND_VAR RAPIDXML_INCLUDE_DIRS
	DISPLAY_NAME "RapidXML"
	INFO_VAR RAPIDXML_INCLUDE_DIRS
	NAME rapidxml
	REPO https://github.com/discord/rapidxml.git
	TAG 2ae4b2888165a393dfb6382168825fddf00c27b9
	SET_FOUND_VAR RAPIDXML_PACKAGE_FOUND
	FETCH_VERB "download it locally")

if (FETCHED_LIBRARIES)
	FetchContent_MakeAvailable(${FETCHED_LIBRARIES})
endif()

# RapidXML (see above) has no upstream CMakeLists.txt, so nothing above
# already defined a rapidxml::rapidxml target the way FetchContent_MakeAvailable
# did for every other fetched library - synthesize the same kind of plain
# INTERFACE target exprtk's own upstream CMakeLists.txt defines for itself
# (add_library(exprtk INTERFACE ...) + an ALIAS), pointed at whichever
# include dir was found above - a system install (RAPIDXML_PACKAGE_FOUND)
# or the FetchContent source dir (rapidxml_SOURCE_DIR).
if (NOT TARGET rapidxml::rapidxml)
	add_library(rapidxml INTERFACE)
	add_library(rapidxml::rapidxml ALIAS rapidxml)

	if (RAPIDXML_PACKAGE_FOUND)
		target_include_directories(rapidxml INTERFACE ${RAPIDXML_INCLUDE_DIRS})
	else()
		target_include_directories(rapidxml INTERFACE ${rapidxml_SOURCE_DIR})
	endif()
endif()

# raylib's own CMake exports a plain `raylib` target (not namespaced) either
# way (found or fetched) - nothing to alias. SDL2/SDL2_image/SDL2_ttf modern
# enough to be pinned above all define their own SDL2::SDL2-style ALIAS
# targets from either path too; the defensive alias below only covers an
# older/nonstandard find_package result that doesn't.
if (SDL2_FOUND AND NOT TARGET SDL2::SDL2 AND TARGET SDL2)
	add_library(SDL2::SDL2 ALIAS SDL2)
endif()

if(NOT TARGET XercesC::XercesC)
	add_library(XercesC::XercesC ALIAS xerces-c)
endif()
