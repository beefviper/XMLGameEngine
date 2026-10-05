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

		# SYSTEM: what the library's targets say about their include
		# directories are system include directories to everything that links
		# them, so no warning is ever reported from its headers.
		FetchContent_Declare(${DFD_NAME}
			GIT_REPOSITORY ${DFD_REPO}
			GIT_TAG ${DFD_TAG}
			EXCLUDE_FROM_ALL
			SYSTEM)

		list(APPEND FETCHED_LIBRARIES ${DFD_NAME})
	endif()
endmacro()

# Only the libraries of the backends that are built (options.cmake) are found
# or fetched: the engine itself needs exprtk and lunasvg, and nothing else.
if (XGE_WITH_XERCES AND NOT FORCE_LOCAL_XERCESC)
	find_package(XercesC QUIET)
endif()

if (NOT FORCE_LOCAL_EXPRTK)
	find_path(EXPRTK_INCLUDE_DIRS "exprtk.hpp")
endif()

# SFML's network module is not asked for: nothing in the engine uses it, and it
# is the part that needs a TLS library and libssh2 built beside it.
if (XGE_WITH_SFML3 AND NOT FORCE_LOCAL_SFML)
	find_package(SFML 3 COMPONENTS System Window Graphics Audio QUIET)
endif()

if (XGE_WITH_RAYLIB AND NOT FORCE_LOCAL_RAYLIB)
	find_package(raylib QUIET)
endif()

if (XGE_WITH_OPENGL)
	# The system's OpenGL library, for the OpenGL backend (window_opengl.cpp). It
	# is part of every platform's SDK, so it is only ever found, never fetched.
	find_package(OpenGL REQUIRED)

	# GLFW is the OpenGL backend's window and keyboard (window_opengl.h), and raylib
	# is built on it too: vcpkg installs it for raylib, and a raylib built from
	# source brings its own `glfw` target.
	if (NOT FORCE_LOCAL_GLFW)
		find_package(glfw3 QUIET)
	endif()
endif()

if (XGE_WITH_SDL2)
	if (NOT FORCE_LOCAL_SDL2)
		find_package(SDL2 QUIET)
	endif()

	if (NOT FORCE_LOCAL_SDL2_IMAGE)
		find_package(SDL2_image QUIET)
	endif()

	if (NOT FORCE_LOCAL_SDL2_TTF)
		find_package(SDL2_ttf QUIET)
	endif()
endif()

if (XGE_WITH_TINYXML2 AND NOT FORCE_LOCAL_TINYXML2)
	find_package(tinyxml2 QUIET)
endif()

if (XGE_WITH_PUGIXML AND NOT FORCE_LOCAL_PUGIXML)
	find_package(pugixml QUIET)
endif()

# RapidXML has no official CMake package of its own to find_package() -
# same situation exprtk is in below, so this probes for its header the same
# way exprtk does (EXPRTK_INCLUDE_DIRS), rather than pretending a RapidXML
# config package might exist.
if (XGE_WITH_RAPIDXML AND NOT FORCE_LOCAL_RAPIDXML)
	find_path(RAPIDXML_INCLUDE_DIRS "rapidxml.hpp")
endif()

# lunasvg draws the SVG files an <svg> sprite names (lib/source/svg.cpp). It is
# the engine's own tool, not a Window backend: only svg.cpp includes it, and
# what it draws goes to every backend as an ordinary picture. The plutovg
# library it draws with comes along inside it. With vcpkg: vcpkg install lunasvg
if (NOT FORCE_LOCAL_LUNASVG)
	find_package(lunasvg QUIET)
endif()

# Qt 6 is only for xgegui (gui/); nothing in the engine library uses it. Unlike
# everything above it is found, never fetched: building Qt from source is not
# something a configure step should do. Without it xgegui is left out and the
# rest builds as before. With vcpkg: vcpkg install qtbase[widgets]
find_package(Qt6 COMPONENTS Widgets QUIET)

if (Qt6_FOUND)
	message(STATUS "Qt6 found: ${Qt6_DIR}")
else()
	message(STATUS "Qt6 not found, xgegui will not be built (install qtbase with the widgets feature).")
endif()

if (XGE_WITH_XERCES)
	declare_fetched_dependency(
		FOUND_VAR XercesC_FOUND
		DISPLAY_NAME "XercesC"
		INFO_VAR XercesC_LIBRARIES
		NAME XercesC
		REPO https://github.com/apache/xerces-c.git
		TAG v3.3.0)
endif()

declare_fetched_dependency(
	FOUND_VAR EXPRTK_INCLUDE_DIRS
	DISPLAY_NAME "exprtk"
	INFO_VAR EXPRTK_INCLUDE_DIRS
	NAME exprtk
	REPO https://github.com/ArashPartow/exprtk.git
	TAG 0.0.3-cmake
	SET_FOUND_VAR EXPRTK_PACKAGE_FOUND)

if (XGE_WITH_SFML3)
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
		set(SFML_BUILD_NETWORK OFF CACHE BOOL "Nothing in the engine uses SFML's network module" FORCE)
	endif()
endif()

if (XGE_WITH_RAYLIB)
	declare_fetched_dependency(
		FOUND_VAR raylib_FOUND
		DISPLAY_NAME "raylib"
		INFO_VAR raylib_DIR
		NAME raylib
		REPO https://github.com/raysan5/raylib.git
		TAG 5.5)
endif()

# GLFW is only for the OpenGL backend. With raylib built too and neither it nor
# GLFW found, the fetched raylib builds GLFW itself and defines the same `glfw`
# target, so GLFW is not fetched a second time; without raylib it is.
if (XGE_WITH_OPENGL)
	if (glfw3_FOUND OR raylib_FOUND OR NOT XGE_WITH_RAYLIB)
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
endif()

if (XGE_WITH_SDL2)
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
endif()

if (XGE_WITH_TINYXML2)
	declare_fetched_dependency(
		FOUND_VAR tinyxml2_FOUND
		DISPLAY_NAME "TinyXML2"
		INFO_VAR tinyxml2_DIR
		NAME tinyxml2
		REPO https://github.com/leethomason/tinyxml2.git
		TAG 11.0.0)
endif()

if (XGE_WITH_PUGIXML)
	declare_fetched_dependency(
		FOUND_VAR pugixml_FOUND
		DISPLAY_NAME "PugiXML"
		INFO_VAR pugixml_DIR
		NAME pugixml
		REPO https://github.com/zeux/pugixml.git
		TAG v1.16)
endif()

# Pinned by commit rather than a tag - upstream (the discord/rapidxml
# mirror of the last released 1.13, the version this project's own
# xml_rapidxml.cpp is written against) has never cut a tagged release.
# RapidXML is header-only with no CMakeLists.txt of its own (checked -
# it has none), so unlike this project's other fetched libraries it's
# not add_subdirectory()'d by FetchContent_MakeAvailable below - see the
# rapidxml::rapidxml target synthesized further down instead.
if (XGE_WITH_RAPIDXML)
	declare_fetched_dependency(
		FOUND_VAR RAPIDXML_INCLUDE_DIRS
		DISPLAY_NAME "RapidXML"
		INFO_VAR RAPIDXML_INCLUDE_DIRS
		NAME rapidxml
		REPO https://github.com/discord/rapidxml.git
		TAG 2ae4b2888165a393dfb6382168825fddf00c27b9
		SET_FOUND_VAR RAPIDXML_PACKAGE_FOUND
		FETCH_VERB "download it locally")
endif()

declare_fetched_dependency(
	FOUND_VAR lunasvg_FOUND
	DISPLAY_NAME "lunasvg"
	INFO_VAR lunasvg_DIR
	NAME lunasvg
	REPO https://github.com/sammycage/lunasvg.git
	TAG v3.5.0)

if (NOT lunasvg_FOUND)
	set(LUNASVG_BUILD_EXAMPLES OFF CACHE BOOL "Do not build lunasvg's examples" FORCE)
	set(PLUTOVG_BUILD_EXAMPLES OFF CACHE BOOL "Do not build plutovg's examples" FORCE)
endif()

# Compiles a fetched library's own sources with no warnings at all (the targets
# of its directory and of every directory below it): they are not this
# project's code to fix, and the warning levels of xge_warnings (platform.cmake)
# are not for them. GCC and Clang take -w after whatever else the library asks
# for; MSVC answers a second /W level with D9025 ("overriding /W4 with /W0"),
# so there only the targets known to set none are given /W0 (xerces-c and
# lunasvg's two), and a library that sets its own level keeps it.
set(XGE_MSVC_SILENCED_TARGETS xerces-c lunasvg plutovg)

function(xge_silence_directory directory)
	get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)

	foreach(target IN LISTS targets)
		get_target_property(type ${target} TYPE)
		if (NOT type MATCHES "^(STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY|EXECUTABLE)$")
			continue()
		endif()

		if (MSVC)
			if (target IN_LIST XGE_MSVC_SILENCED_TARGETS)
				target_compile_options(${target} PRIVATE /W0)
			endif()
		else()
			target_compile_options(${target} PRIVATE -w)
		endif()
	endforeach()

	get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
	foreach(subdirectory IN LISTS subdirectories)
		xge_silence_directory("${subdirectory}")
	endforeach()
endfunction()

if (FETCHED_LIBRARIES)
	FetchContent_MakeAvailable(${FETCHED_LIBRARIES})

	foreach(library IN LISTS FETCHED_LIBRARIES)
		FetchContent_GetProperties(${library} SOURCE_DIR library_source_dir)

		# RapidXML has no CMakeLists.txt and so no directory of its own to look at.
		if (EXISTS "${library_source_dir}/CMakeLists.txt")
			xge_silence_directory("${library_source_dir}")
		endif()
	endforeach()
endif()

# RapidXML (see above) has no upstream CMakeLists.txt, so nothing above
# already defined a rapidxml::rapidxml target the way FetchContent_MakeAvailable
# did for every other fetched library - synthesize the same kind of plain
# INTERFACE target exprtk's own upstream CMakeLists.txt defines for itself
# (add_library(exprtk INTERFACE ...) + an ALIAS), pointed at whichever
# include dir was found above - a system install (RAPIDXML_PACKAGE_FOUND)
# or the FetchContent source dir (rapidxml_SOURCE_DIR).
if (XGE_WITH_RAPIDXML AND NOT TARGET rapidxml::rapidxml)
	add_library(rapidxml INTERFACE)
	add_library(rapidxml::rapidxml ALIAS rapidxml)

	if (RAPIDXML_PACKAGE_FOUND)
		target_include_directories(rapidxml SYSTEM INTERFACE ${RAPIDXML_INCLUDE_DIRS})
	else()
		target_include_directories(rapidxml SYSTEM INTERFACE ${rapidxml_SOURCE_DIR})
	endif()
endif()

# raylib's own CMake exports a plain `raylib` target (not namespaced) either
# way (found or fetched) - nothing to alias. SDL2/SDL2_image/SDL2_ttf modern
# enough to be pinned above all define their own SDL2::SDL2-style ALIAS
# targets from either path too; the defensive alias below only covers an
# older/nonstandard find_package result that doesn't.
if (XGE_WITH_SDL2 AND SDL2_FOUND AND NOT TARGET SDL2::SDL2 AND TARGET SDL2)
	add_library(SDL2::SDL2 ALIAS SDL2)
endif()

if(XGE_WITH_XERCES AND NOT TARGET XercesC::XercesC)
	add_library(XercesC::XercesC ALIAS xerces-c)
endif()
