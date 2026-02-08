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
	find_package(SFML 2.5...<3.0 COMPONENTS system window graphics network audio QUIET)
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
		GIT_TAG 2.6.2
		EXCLUDE_FROM_ALL)

	list(APPEND FETCHED_LIBRARIES SFML)

	set(SFML_BUILD_FROM_SOURCE ON CACHE BOOL "Force SFML to build from source" FORCE)
	set(SFML_USE_SYSTEM_DEPS OFF CACHE BOOL "Use SFML's bundled dependencies" FORCE)
endif()

if (FETCHED_LIBRARIES)
	FetchContent_MakeAvailable(${FETCHED_LIBRARIES})
endif()

if(NOT TARGET XercesC::XercesC)
	add_library(XercesC::XercesC ALIAS xerces-c)
endif()
