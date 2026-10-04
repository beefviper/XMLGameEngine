# output.cmake
# XML Game Engine
# author: beefviper
# date: Oct 3, 2026
#
# Everything needed to run a game goes in one folder per configuration at the
# top of the repository, output/Debug or output/Release (output/RelWithDebInfo,
# output/MinSizeRel):
#   xgecli, xgegui, xgetest   the programs
#   games/, assets/           copied next to them by assets.cmake
#   libraries/                every DLL (.so on Linux) the programs load, and
#                             Qt's plug-ins in libraries/plugins
# The build directory keeps only what the compiler and linker make on the way.
# output/ is ignored by git.
#
# Included before any target is made, so the CMAKE_*_OUTPUT_DIRECTORY values
# below are picked up by every target, the fetched dependencies' too.

set(XGE_OUTPUT_ROOT "${PROJECT_SOURCE_DIR}/output" CACHE PATH
	"Folder the per-configuration folders (Debug, Release) of programs, DLLs, games and assets go in")

# A single-configuration generator (Ninja, Makefiles) with no build type
# would give an output folder with no name; build Debug, as Visual Studio does.
get_property(xge_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (NOT xge_multi_config AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
endif()

# The generator expression is what puts each configuration in its own folder.
# With a plain path, Visual Studio and Ninja Multi-Config would add a Debug or
# Release folder only to the programs, not to the games and assets copied for
# them; with $<CONFIG> in it they add nothing, and every generator gives the
# same layout.
set(XGE_OUTPUT_DIR "${XGE_OUTPUT_ROOT}/$<CONFIG>")

# Windows only looks for a program's DLLs next to it, in the system folders and
# on PATH, and it does so before main() runs. A DLL in libraries/ is found
# because each program carries a manifest that names a private assembly called
# "libraries", and libraries/libraries.manifest lists the DLLs that belong to
# it; see xge_place_program() below. That needs the Microsoft linker to embed
# the manifest, so with another Windows toolchain the DLLs stay next to the
# programs. On Linux and macOS the programs find libraries/ through their run
# path, kept relative to the program ($ORIGIN) so output/<config> can be moved.
set(XGE_LIBRARY_FOLDER "libraries")
if (WIN32 AND NOT MSVC)
	set(XGE_LIBRARY_DIR "${XGE_OUTPUT_DIR}")
else()
	set(XGE_LIBRARY_DIR "${XGE_OUTPUT_DIR}/${XGE_LIBRARY_FOLDER}")
endif()

# DLLs are RUNTIME files and a .so is a LIBRARY file, so both default to
# libraries/; xge_place_program() moves each program back up a folder. Static
# libraries and a DLL's import .lib (ARCHIVE files) stay in the build directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${XGE_LIBRARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${XGE_LIBRARY_DIR}")

# A library in libraries/ finds the ones it needs beside itself; each program
# gets libraries/ itself in xge_place_program(). Relative, so output/<config>
# still works when it is copied somewhere else.
if (APPLE)
	set(CMAKE_BUILD_RPATH "@loader_path")
elseif (UNIX)
	set(CMAKE_BUILD_RPATH "$ORIGIN")
endif()

if (MSVC)
	# The manifest each program carries: "I need the assembly called libraries".
	# Windows looks for it in libraries/libraries.manifest next to the program.
	if (CMAKE_CXX_COMPILER_ARCHITECTURE_ID MATCHES "^[Xx]86$")
		set(XGE_MANIFEST_ARCH "x86")
	elseif (CMAKE_CXX_COMPILER_ARCHITECTURE_ID MATCHES "^ARM64")
		set(XGE_MANIFEST_ARCH "arm64")
	else()
		set(XGE_MANIFEST_ARCH "amd64")
	endif()

	set(XGE_PROGRAM_MANIFEST "${PROJECT_BINARY_DIR}/xge_libraries.manifest")
	file(WRITE "${XGE_PROGRAM_MANIFEST}"
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>
<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">
  <dependency>
    <dependentAssembly>
      <assemblyIdentity type=\"win32\" name=\"${XGE_LIBRARY_FOLDER}\" version=\"1.0.0.0\" processorArchitecture=\"${XGE_MANIFEST_ARCH}\"/>
    </dependentAssembly>
  </dependency>
</assembly>
")
endif()

# Puts a program in output/<config> rather than in libraries/ with the DLLs.
# With MSVC it also embeds the manifest above in the program and, after each
# build, runs deploy_libraries.cmake, which moves the DLLs vcpkg copied next to
# the program into libraries/ and writes libraries/libraries.manifest listing
# every DLL there. Call it after any other POST_BUILD step that puts DLLs in
# place (windeployqt) and before any that runs the program (catch_discover_tests).
function(xge_place_program target)
	set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${XGE_OUTPUT_DIR}")

	if (APPLE)
		set_target_properties(${target} PROPERTIES BUILD_RPATH "@loader_path/${XGE_LIBRARY_FOLDER}")
	elseif (UNIX)
		set_target_properties(${target} PROPERTIES BUILD_RPATH "$ORIGIN/${XGE_LIBRARY_FOLDER}")
	endif()

	if (MSVC)
		target_sources(${target} PRIVATE "${XGE_PROGRAM_MANIFEST}")
		add_custom_command(TARGET ${target} POST_BUILD
			COMMAND ${CMAKE_COMMAND}
				"-DPROGRAM_DIR=$<TARGET_FILE_DIR:${target}>"
				"-DLIBRARY_DIR=$<TARGET_FILE_DIR:${target}>/${XGE_LIBRARY_FOLDER}"
				"-DASSEMBLY_NAME=${XGE_LIBRARY_FOLDER}"
				"-DASSEMBLY_ARCH=${XGE_MANIFEST_ARCH}"
				-P "${PROJECT_SOURCE_DIR}/scripts/cmake/deploy_libraries.cmake"
			VERBATIM)
	endif()
endfunction()
