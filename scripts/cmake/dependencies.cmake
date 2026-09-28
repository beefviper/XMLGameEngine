# dependencies.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

include(FetchContent)

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

if (NOT FORCE_LOCAL_SDL2)
	find_package(SDL2 QUIET)
endif()

if (NOT FORCE_LOCAL_SDL2_IMAGE)
	find_package(SDL2_image QUIET)
endif()

if (NOT FORCE_LOCAL_SDL2_TTF)
	find_package(SDL2_ttf QUIET)
endif()

if (XercesC_FOUND)
	message(STATUS "XERCESC found: ${XercesC_LIBRARIES}")
else()
	message(STATUS "XercesC not found, using FetchContent to download and build it locally.")

	FetchContent_Declare(XercesC
		GIT_REPOSITORY https://github.com/apache/xerces-c.git
		GIT_TAG v3.3.0
		EXCLUDE_FROM_ALL)

	list(APPEND FETCHED_LIBRARIES XercesC)
endif()

if (EXPRTK_INCLUDE_DIRS)
	message(STATUS "exprtk found: ${EXPRTK_INCLUDE_DIRS}")
	set(EXPRTK_PACKAGE_FOUND TRUE)
else()
	message(STATUS "exprtk not found, using FetchContent to download and build it locally.")

	FetchContent_Declare(exprtk
		GIT_REPOSITORY https://github.com/ArashPartow/exprtk.git
		GIT_TAG 0.0.3-cmake
		EXCLUDE_FROM_ALL)

	list(APPEND FETCHED_LIBRARIES exprtk)
endif()

if (SFML_FOUND)
	message(STATUS "SFML found: ${SFML_LIBRARIES}")
else()
	message(STATUS "SFML not found, using FetchContent to download and build it locally.")

	FetchContent_Declare(SFML
		GIT_REPOSITORY https://github.com/SFML/SFML.git
		GIT_TAG 3.1.0
		EXCLUDE_FROM_ALL)

	list(APPEND FETCHED_LIBRARIES SFML)

	set(SFML_BUILD_FROM_SOURCE ON CACHE BOOL "Force SFML to build from source" FORCE)
	set(SFML_USE_SYSTEM_DEPS OFF CACHE BOOL "Use SFML's bundled dependencies" FORCE)
endif()

if (raylib_FOUND)
	message(STATUS "raylib found: ${raylib_LIBRARIES}")
else()
	message(STATUS "raylib not found, using FetchContent to download and build it locally.")

	FetchContent_Declare(raylib
		GIT_REPOSITORY https://github.com/raysan5/raylib.git
		GIT_TAG 5.5
		EXCLUDE_FROM_ALL)

	list(APPEND FETCHED_LIBRARIES raylib)
endif()

if (SDL2_FOUND)
	message(STATUS "SDL2 found: ${SDL2_LIBRARIES}")
else()
	message(STATUS "SDL2 not found, using FetchContent to download and build it locally.")

	FetchContent_Declare(SDL2
		GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
		GIT_TAG release-2.30.9
		EXCLUDE_FROM_ALL)

	list(APPEND FETCHED_LIBRARIES SDL2)
endif()

if (SDL2_image_FOUND)
	message(STATUS "SDL2_image found: ${SDL2_image_LIBRARIES}")
else()
	message(STATUS "SDL2_image not found, using FetchContent to download and build it locally.")

	# Built against whichever SDL2 target ends up available above (found or
	# fetched) - SDL2_image's own CMakeLists picks it up the same way this
	# project's other fetched libraries do.
	FetchContent_Declare(SDL2_image
		GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
		GIT_TAG release-2.8.2
		EXCLUDE_FROM_ALL)

	set(SDL2IMAGE_INSTALL OFF CACHE BOOL "Disable SDL2_image's own install rules" FORCE)
	set(SDL2IMAGE_VENDORED ON CACHE BOOL "Build SDL2_image's bundled image libraries from source" FORCE)

	list(APPEND FETCHED_LIBRARIES SDL2_image)
endif()

if (SDL2_ttf_FOUND)
	message(STATUS "SDL2_ttf found: ${SDL2_ttf_LIBRARIES}")
else()
	message(STATUS "SDL2_ttf not found, using FetchContent to download and build it locally.")

	FetchContent_Declare(SDL2_ttf
		GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
		GIT_TAG release-2.22.0
		EXCLUDE_FROM_ALL)

	set(SDL2TTF_INSTALL OFF CACHE BOOL "Disable SDL2_ttf's own install rules" FORCE)
	set(SDL2TTF_VENDORED ON CACHE BOOL "Build SDL2_ttf's bundled FreeType from source" FORCE)

	list(APPEND FETCHED_LIBRARIES SDL2_ttf)
endif()

if (FETCHED_LIBRARIES)
	FetchContent_MakeAvailable(${FETCHED_LIBRARIES})
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
