# executables.cmake
# XML Game Engine
# author: beefviper
# date: Oct 1, 2026
#
# The programs built on the engine library (the XGELIB target):
#   XGECLI  the command line front end: picks a game file from the arguments,
#           loads it, and runs it in a window (cli/)
#   XGEGUI  a stub for a future graphical front end (gui/); it only returns
#           for now
# They are defined here, in the top-level directory's scope, so they land in
# the build directory itself, next to the games/ and assets/ copies that
# assets.cmake makes.

add_executable(XGECLI
	"cli/source/main.cpp"
	"cli/source/cli.cpp"
	"cli/include/cli.h"
)

add_executable(XGEGUI
	"gui/source/main.cpp"
)

foreach(program IN ITEMS XGECLI XGEGUI)
	target_link_libraries(${program} PRIVATE XGELIB)
	target_compile_features(${program} PRIVATE cxx_std_20)

	# Same flags platform.cmake sets on the library.
	target_compile_options(${program} PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>:/W4> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>)
	target_compile_options(${program} PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>:/external:anglebrackets /external:W0 /analyze:external- /bigobj>)

	# The games and assets are copied next to the program, see assets.cmake.
	add_dependencies(${program} XGEDATA)
endforeach()

# cli/include is not part of the library's include directory: only XGECLI uses it.
target_include_directories(XGECLI PRIVATE cli/include)
