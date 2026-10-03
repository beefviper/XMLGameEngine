# output.cmake
# XML Game Engine
# author: beefviper
# date: Oct 3, 2026
#
# Everything needed to run a game goes in one folder per configuration at the
# top of the repository, output/Debug or output/Release (output/RelWithDebInfo,
# output/MinSizeRel): the programs (XGECLI, XGEGUI, XGETEST), the DLLs (.so on
# Linux) they load, and the games/ and assets/ folders copied next to them by
# assets.cmake. The build directory keeps only what the compiler and linker
# make on the way. output/ is ignored by git.
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

# Programs and DLLs are RUNTIME files; a .so on Linux is a LIBRARY file. Static
# libraries and a DLL's import .lib (ARCHIVE files) stay in the build directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${XGE_OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${XGE_OUTPUT_DIR}")
