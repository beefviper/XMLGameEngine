# dependencies.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

include(FetchContent)

# Every third-party library follows the same shape: look for it already
# installed (vcpkg or the system) and, if it is not there, FetchContent_Declare
# it from its upstream git repo so FetchContent_MakeAvailable() (below)
# downloads and builds it as part of this build. One macro does it for all of
# them:
#
#   xge_dependency(<name>
#       [WHEN <variable>]               only when it is true (default: always); a
#                                       backend that is not built needs nothing
#       PACKAGE <find_package args>     look for it with find_package(<args> QUIET),
#                                       or
#       HEADER <file>                   with find_path(), for a header-only library
#                                       that has no CMake package
#       REPO <git url> TAG <tag or commit>
#       [SETTINGS <VARIABLE=VALUE>...]  ON/OFF cache settings for the fetched copy
#       [BROUGHT_BY <library>]          the fetched copy of that library builds this
#                                       one too, so it is not fetched on its own
#       [REQUIRED] [MISSING <text>])
#
# With no REPO it is a system library (OpenGL, Qt): found or not, never fetched.
# Not found, MISSING says what follows from that, and with REQUIRED the configure
# stops there.
#
# <name> is what the messages call it ("Found <name> <version>", the way CMake
# itself says it, with the version when the package reports one; where it was
# found is not said, as that is a different variable for each library) and what
# is passed to FetchContent. It is
# also how the FORCE_LOCAL_<NAME> option (options.cmake) is found: its upper
# case. A library found with PACKAGE leaves <package>_FOUND for the rest of the
# project to check; one found with HEADER leaves <NAME>_INCLUDE_DIRS and
# <NAME>_PACKAGE_FOUND. A macro, not a function, so that those stay set.
macro(xge_dependency name)
	cmake_parse_arguments(XD "REQUIRED" "WHEN;HEADER;REPO;TAG;BROUGHT_BY;MISSING" "PACKAGE;SETTINGS" ${ARGN})
	string(TOUPPER "${name}" XD_UPPER)
	unset(XD_PACKAGE_NAME) # a macro's variables outlive the call: not the last library's

	set(XD_ENABLED TRUE)
	if (DEFINED XD_WHEN)
		if (NOT ${XD_WHEN})
			set(XD_ENABLED FALSE)
		endif()
	endif()

	if (XD_ENABLED)
		# Look for it.
		set(XD_FOUND FALSE)

		if (XD_HEADER)
			set(XD_VARIABLE ${XD_UPPER}_INCLUDE_DIRS)

			if (FORCE_LOCAL_${XD_UPPER})
				unset(${XD_VARIABLE} CACHE)
			else()
				find_path(${XD_VARIABLE} "${XD_HEADER}")
			endif()

			if (${XD_VARIABLE})
				set(XD_FOUND TRUE)
				set(${XD_UPPER}_PACKAGE_FOUND TRUE)
			endif()

			set(XD_VERB "download it locally")
		else()
			list(GET XD_PACKAGE 0 XD_PACKAGE_NAME)

			if (NOT FORCE_LOCAL_${XD_UPPER})
				find_package(${XD_PACKAGE} QUIET)
			endif()

			if (${XD_PACKAGE_NAME}_FOUND)
				set(XD_FOUND TRUE)
			endif()

			set(XD_VERB "download and build it locally")
		endif()

		# Say what was found, or fetch it.
		if (XD_FOUND)
			# The version, as the sanity check it is: the copy found can be older
			# than the tag pinned below. Not every library says (a header search
			# cannot), and then there is just the name.
			if (XD_PACKAGE_NAME AND ${XD_PACKAGE_NAME}_VERSION)
				message(STATUS "Found ${name} ${${XD_PACKAGE_NAME}_VERSION}")
			else()
				message(STATUS "Found ${name}")
			endif()
		elseif (NOT XD_REPO)
			if (XD_REQUIRED)
				message(FATAL_ERROR "${name} not found: ${XD_MISSING}")
			else()
				message(STATUS "${name} not found, ${XD_MISSING}")
			endif()
		elseif (XD_BROUGHT_BY AND "${XD_BROUGHT_BY}" IN_LIST FETCHED_LIBRARIES)
			message(STATUS "${name} not found, and neither is ${XD_BROUGHT_BY}: the ${XD_BROUGHT_BY} built here brings its own.")
		else()
			message(STATUS "${name} not found, using FetchContent to ${XD_VERB}.")

			# SYSTEM: what the library's targets say about their include
			# directories are system include directories to everything that links
			# them, so no warning is ever reported from its headers.
			FetchContent_Declare(${name}
				GIT_REPOSITORY ${XD_REPO}
				GIT_TAG ${XD_TAG}
				EXCLUDE_FROM_ALL
				SYSTEM)

			list(APPEND FETCHED_LIBRARIES ${name})

			foreach(XD_SETTING IN LISTS XD_SETTINGS)
				string(REPLACE "=" ";" XD_PAIR "${XD_SETTING}")
				list(GET XD_PAIR 0 XD_SETTING_NAME)
				list(GET XD_PAIR 1 XD_SETTING_VALUE)
				set(${XD_SETTING_NAME} ${XD_SETTING_VALUE} CACHE BOOL "Set for the ${name} fetched by xge_dependency" FORCE)
			endforeach()
		endif()
	endif()
endmacro()

# Only the libraries of the backends that are built (options.cmake) are found
# or fetched: the engine itself needs exprtk and lunasvg, and nothing else.
xge_dependency(XercesC WHEN XGE_WITH_XERCES
	PACKAGE XercesC
	REPO https://github.com/apache/xerces-c.git TAG v3.3.0)

xge_dependency(exprtk
	HEADER exprtk.hpp
	REPO https://github.com/ArashPartow/exprtk.git TAG 0.0.3-cmake)

# SFML's network module is not asked for: nothing in the engine uses it, and it
# is the part that needs a TLS library and libssh2 built beside it.
xge_dependency(SFML WHEN XGE_WITH_SFML3
	PACKAGE SFML 3 COMPONENTS System Window Graphics Audio
	REPO https://github.com/SFML/SFML.git TAG 3.1.0
	SETTINGS SFML_BUILD_FROM_SOURCE=ON SFML_USE_SYSTEM_DEPS=OFF SFML_BUILD_NETWORK=OFF)

xge_dependency(raylib WHEN XGE_WITH_RAYLIB
	PACKAGE raylib
	REPO https://github.com/raysan5/raylib.git TAG 5.5)

# The OpenGL backend (window_opengl.cpp) calls OpenGL itself: the system's
# library, part of every platform's SDK, so it is only ever found, never fetched.
# It also uses GLFW for its window and keyboard (window_opengl.h), which raylib is
# built on too: vcpkg installs it for raylib, and a raylib built from source brings
# its own `glfw` target.
xge_dependency(OpenGL WHEN XGE_WITH_OPENGL
	PACKAGE OpenGL
	# (No semicolon in a MISSING text: it would split the argument.)
	REQUIRED MISSING "the OpenGL backend needs the system's OpenGL (opengl32 comes with Windows, and on Linux install libgl-dev), or leave it out with -DXGE_WITH_OPENGL=OFF.")

xge_dependency(GLFW WHEN XGE_WITH_OPENGL
	PACKAGE glfw3
	REPO https://github.com/glfw/glfw.git TAG 3.4
	BROUGHT_BY raylib
	SETTINGS GLFW_BUILD_EXAMPLES=OFF GLFW_BUILD_TESTS=OFF GLFW_BUILD_DOCS=OFF GLFW_INSTALL=OFF)

# SDL2_image and SDL2_ttf are built against whichever SDL2 target ends up
# available (found or fetched); their own CMakeLists picks it up the same way
# this project's other fetched libraries do. The OpenGL backend uses them too.
xge_dependency(SDL2 WHEN XGE_NEEDS_SDL2
	PACKAGE SDL2
	REPO https://github.com/libsdl-org/SDL.git TAG release-2.30.9)

xge_dependency(SDL2_image WHEN XGE_NEEDS_SDL2
	PACKAGE SDL2_image
	REPO https://github.com/libsdl-org/SDL_image.git TAG release-2.8.2
	SETTINGS SDL2IMAGE_INSTALL=OFF SDL2IMAGE_VENDORED=ON)

xge_dependency(SDL2_ttf WHEN XGE_NEEDS_SDL2
	PACKAGE SDL2_ttf
	REPO https://github.com/libsdl-org/SDL_ttf.git TAG release-2.22.0
	SETTINGS SDL2TTF_INSTALL=OFF SDL2TTF_VENDORED=ON)

# Any 10 or 11 works: the wrapper uses a handful of calls that did not change, and
# the suite passes against both (11 only changed internal containers). No version
# is asked of find_package on purpose: TinyXML2's package is SameMajorVersion, so
# `10` refuses 11, `11` refuses 10, and a range across the two (10...<12) is
# refused by both, finding nothing at all. The pin is for the copy that is fetched.
xge_dependency(TinyXML2 WHEN XGE_WITH_TINYXML2
	PACKAGE tinyxml2
	REPO https://github.com/leethomason/tinyxml2.git TAG 11.0.0)

xge_dependency(PugiXML WHEN XGE_WITH_PUGIXML
	PACKAGE pugixml
	REPO https://github.com/zeux/pugixml.git TAG v1.16)

# RapidXML has no official CMake package to find, so its header is looked for the
# way exprtk's is, and it is pinned by commit rather than a tag: upstream (the
# discord/rapidxml mirror of the last released 1.13, the version this project's
# own xml_rapidxml.cpp is written against) has never cut a tagged release. It is
# header-only with no CMakeLists.txt of its own, so unlike this project's other
# fetched libraries it is not add_subdirectory()'d by FetchContent_MakeAvailable
# below: see the rapidxml::rapidxml target synthesized after it instead.
xge_dependency(RapidXML WHEN XGE_WITH_RAPIDXML
	HEADER rapidxml.hpp
	REPO https://github.com/discord/rapidxml.git TAG 2ae4b2888165a393dfb6382168825fddf00c27b9)

# lunasvg draws the SVG files an <svg> sprite names (lib/source/svg.cpp). It is
# the engine's own tool, not a Window backend: only svg.cpp includes it, and
# what it draws goes to every backend as an ordinary picture. The plutovg
# library it draws with comes along inside it. With vcpkg: vcpkg install lunasvg
xge_dependency(lunasvg
	PACKAGE lunasvg
	REPO https://github.com/sammycage/lunasvg.git TAG v3.5.0
	SETTINGS LUNASVG_BUILD_EXAMPLES=OFF PLUTOVG_BUILD_EXAMPLES=OFF)

# Qt 6 is only for xgegui (gui/); nothing in the engine library uses it. Unlike
# everything above it is found, never fetched: building Qt from source is not
# something a configure step should do. Without it xgegui is left out and the
# rest builds as before. With vcpkg: vcpkg install qtbase[widgets]
xge_dependency(Qt6
	PACKAGE Qt6 COMPONENTS Widgets
	MISSING "xgegui will not be built (install qtbase with the widgets feature).")

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
if (XGE_NEEDS_SDL2 AND SDL2_FOUND AND NOT TARGET SDL2::SDL2 AND TARGET SDL2)
	add_library(SDL2::SDL2 ALIAS SDL2)
endif()

if(XGE_WITH_XERCES AND NOT TARGET XercesC::XercesC)
	add_library(XercesC::XercesC ALIAS xerces-c)
endif()
