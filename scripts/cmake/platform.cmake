# platform.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	PROPERTY VS_STARTUP_PROJECT XGECLI)

# Warning levels for the engine library; XGECLI, XGEGUI and the tests get the
# same ones from scripts/cmake/executables.cmake and tests.cmake.
if (XGE_BUILD_SHARED)
	# A DLL has to say what it exports; rather than marking every class in the
	# headers, export all of them, and put the DLL next to the programs.
	set_target_properties(XGELIB PROPERTIES
		WINDOWS_EXPORT_ALL_SYMBOLS ON
		RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif()

target_compile_options(XGELIB PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>)

target_compile_options(XGELIB PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/external:anglebrackets /external:W0 /analyze:external- /bigobj>)

if (WIN32 AND TARGET Freetype)
	message(STATUS "*** sanitizing Freetype INTERFACE_LINK_LIBRARIES on Windows")

	get_target_property(ft_libs Freetype INTERFACE_LINK_LIBRARIES)
	set(clean_libs "")

	foreach(item IN LISTS ft_libs)
		# Skip the legacy MSVC keywords inside generator expressions
		if (NOT item MATCHES "\\$<BUILD_INTERFACE:(optimized|debug)>")
			list(APPEND clean_libs "${item}")
		endif()
	endforeach()

	set_target_properties(Freetype PROPERTIES
		INTERFACE_LINK_LIBRARIES "${clean_libs}"
	)
endif()

# Vendored dependencies come in with their own warning levels, which have
# nothing to do with this project's own code quality - MSVC's /W4 and
# -Wall above are for XGELIB only, so silence warnings on
# third-party targets here instead of fixing warnings in code we don't own.
function(silence_third_party_warnings target scope)
	target_compile_options(${target} ${scope}
		$<$<CXX_COMPILER_ID:MSVC>:/W0> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>)
endfunction()

if (NOT XercesC_FOUND)
	set_target_properties(xerces-c PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
	silence_third_party_warnings(xerces-c PRIVATE)
	set_target_properties(xerces-c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif()

if (NOT EXPRTK_PACKAGE_FOUND)
	silence_third_party_warnings(exprtk INTERFACE)
endif()

if (NOT SFML_FOUND)
	foreach(sfml_target IN ITEMS sfml-system sfml-window sfml-graphics sfml-network sfml-audio)
		set_target_properties(${sfml_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
	endforeach()
endif()

# Same DLL-next-to-the-exe fix as SFML above, for whichever of raylib/SDL2/
# SDL2_image/SDL2_ttf ended up fetched and built from source rather than
# found already installed (vcpkg's own copy-on-build step already handles
# that case). Guarded per target since a fetched library may build as a
# static lib (no runtime DLL to place) depending on its own defaults.
foreach(fetched_target IN ITEMS raylib SDL2 SDL2main SDL2_image SDL2_ttf tinyxml2 pugixml rapidxml)
	if (TARGET ${fetched_target})
		get_target_property(fetched_target_type ${fetched_target} TYPE)
		if (fetched_target_type STREQUAL "SHARED_LIBRARY")
			set_target_properties(${fetched_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
		endif()
	endif()
endforeach()
