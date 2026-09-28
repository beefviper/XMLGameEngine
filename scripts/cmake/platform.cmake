# platform.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	PROPERTY VS_STARTUP_PROJECT XMLGameEngine)

target_compile_options(${PROJECT_NAME} PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>)

target_compile_options(${PROJECT_NAME} PRIVATE
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

if (NOT XercesC_FOUND)
	set_target_properties(xerces-c PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
	target_compile_options(xerces-c PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>:/W0> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>)
	set_target_properties(xerces-c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif()

if (NOT EXPRTK_PACKAGE_FOUND)
	target_compile_options(exprtk INTERFACE
		$<$<CXX_COMPILER_ID:MSVC>:/W0> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>)
endif()

if (NOT SFML_FOUND)
	set_target_properties(sfml-system PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
	set_target_properties(sfml-window PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
	set_target_properties(sfml-graphics PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
	set_target_properties(sfml-network PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
	set_target_properties(sfml-audio PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif()

# Same DLL-next-to-the-exe fix as SFML above, for whichever of raylib/SDL2/
# SDL2_image/SDL2_ttf ended up fetched and built from source rather than
# found already installed (vcpkg's own copy-on-build step already handles
# that case). Guarded per target since a fetched library may build as a
# static lib (no runtime DLL to place) depending on its own defaults.
foreach(fetched_target IN ITEMS raylib SDL2 SDL2main SDL2_image SDL2_ttf)
	if (TARGET ${fetched_target})
		get_target_property(fetched_target_type ${fetched_target} TYPE)
		if (fetched_target_type STREQUAL "SHARED_LIBRARY")
			set_target_properties(${fetched_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
		endif()
	endif()
endforeach()
