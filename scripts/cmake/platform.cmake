# platform.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	PROPERTY VS_STARTUP_PROJECT xgecli)

# Warning levels for the engine library (xge_warnings below); xgecli, xgegui and the
# tests get the same ones from scripts/cmake/executables.cmake and tests.cmake.
if (XGE_BUILD_SHARED)
	# A DLL has to say what it exports; rather than marking every class in the
	# headers, export all of them. The DLL goes in output/<config>/libraries
	# with the others (output.cmake).
	set_target_properties(xgelib PROPERTIES
		WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# The warnings this project's own code is compiled with: as many as can be made
# to come out clean, on MSVC and on GCC and Clang. They are for xgelib, xgecli,
# xgegui and xgetest only; a third-party library is never compiled with them, and
# its headers are system headers (dependencies.cmake), so what is wrong in code
# that is not ours is never reported.
#
# MSVC: level 4, then the ones /W4 leaves off that are worth having (hidden
# virtuals, missing virtual destructors, narrowing, uninitialised members and so
# on: the usual set from the C++ Best Practices project); headers named with <>
# are external, with no warnings from them (and no code analysis), and /bigobj
# because the files that include game_expr.h, whose exprtk use generates enough
# object sections to hit MSVC's C1128 without it.
# GCC and Clang: the "all" and "extra" sets, pedantic, shadowing (a parameter or
# local named like a member), every implicit conversion that can lose a value or
# a sign, C-style casts, missing virtual destructors, fall-through and a few
# more; GCC also looks for float to double promotion in arithmetic (Clang's
# version of that warning fires on every float handed to a double parameter,
# which is every Catch::Approx(float) in the tests), repeated conditions and
# branches, pointless casts and && / || on the same operand.
function(xge_warnings target)
	target_compile_options(${target} PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>:/W4
			/w14242 /w14254 /w14263 /w14265 /w14287 /w14296 /w14311 /w14545 /w14546 /w14547
			/w14549 /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928
			/external:anglebrackets /external:W0 /analyze:external- /bigobj>
		$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion
			-Wsign-conversion -Wnon-virtual-dtor -Woverloaded-virtual -Wcast-align -Wunused
			-Wnull-dereference -Wformat=2 -Wimplicit-fallthrough
			-Wold-style-cast -Wmisleading-indentation>
		$<$<CXX_COMPILER_ID:GNU>:-Wdouble-promotion -Wduplicated-cond -Wduplicated-branches
			-Wlogical-op -Wuseless-cast>)
endfunction()

xge_warnings(xgelib)

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

# Third-party code is not ours to fix, so it is kept quiet two ways: a library
# that is found is an imported target, whose headers are system headers, and
# one that is fetched is declared SYSTEM and has its own build silenced
# (dependencies.cmake), so neither its headers nor its sources report anything
# in this project's build. Only what is specific to one of them is here.
if (XGE_WITH_XERCES AND NOT XercesC_FOUND)
	set_target_properties(xerces-c PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
	set_target_properties(xerces-c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${XGE_LIBRARY_DIR} LIBRARY_OUTPUT_DIRECTORY ${XGE_LIBRARY_DIR})
endif()

# A fetched library that sets its own output folder (SFML puts its DLLs in
# its bin/) is pointed back at output/<config>/libraries (output.cmake), where
# the programs find it when they start.
if (XGE_WITH_SFML3 AND NOT SFML_FOUND)
	foreach(sfml_target IN ITEMS sfml-system sfml-window sfml-graphics sfml-audio)
		set_target_properties(${sfml_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${XGE_LIBRARY_DIR} LIBRARY_OUTPUT_DIRECTORY ${XGE_LIBRARY_DIR})
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
			set_target_properties(${fetched_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${XGE_LIBRARY_DIR} LIBRARY_OUTPUT_DIRECTORY ${XGE_LIBRARY_DIR})
		endif()
	endif()
endforeach()
