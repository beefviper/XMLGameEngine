# build_generated.cmake
# XML Game Engine
# author: beefviper
# date: Nov 21, 2026
#
# One game written out and built as a program of its own, as a person would:
# run as a test by ctest --test-dir build/generated (tests.cmake), never
# included. It generates the game with xgecli --generate GENERATE (engine-cpp
# when not given) into OUTPUT, configures it with the main build's generator
# and toolchain (its own CMakeLists.txt sets the engine's warnings), or for
# web-asm through EMCMAKE (Emscripten's emcmake), and builds it. Any step that
# fails, or any warning, fails the test, with the compiler's lines printed.
#
#     cmake -DGAME=<name> -DXGECLI=<xgecli> -DOUTPUT=<folder> -DCONFIG=<config>
#           -DGENERATOR=<generator> [-DPLATFORM=<platform>] [-DTOOLCHAIN=<file>]
#           [-DDEPS=<build/_deps>] [-DGENERATE=web-asm -DEMCMAKE=<emcmake>]
#           -P build_generated.cmake

foreach(required GAME XGECLI OUTPUT CONFIG GENERATOR)
	if (NOT ${required})
		message(FATAL_ERROR "build_generated.cmake needs -D${required}=")
	endif()
endforeach()
if (NOT GENERATE)
	set(GENERATE engine-cpp)
endif()
if (GENERATE STREQUAL "web-asm" AND NOT EMCMAKE)
	message(FATAL_ERROR "build_generated.cmake needs -DEMCMAKE= for web-asm")
endif()

get_filename_component(xgecli_dir "${XGECLI}" DIRECTORY)
execute_process(
	COMMAND "${XGECLI}" "${GAME}" --generate ${GENERATE} -o "${OUTPUT}"
	WORKING_DIRECTORY "${xgecli_dir}"
	RESULT_VARIABLE result OUTPUT_VARIABLE said ERROR_VARIABLE said)
if (NOT result EQUAL 0)
	message(FATAL_ERROR "${GAME} did not generate:\n${said}")
endif()

if (GENERATE STREQUAL "web-asm")
	# Emscripten's own toolchain, beside emcmake, named outright: emcmake
	# leaves it out when the environment names another (vcpkg's). A web
	# page has no platform.
	get_filename_component(emscripten "${EMCMAKE}" REALPATH)
	get_filename_component(emscripten "${emscripten}" DIRECTORY)
	set(configure "${EMCMAKE}" "${CMAKE_COMMAND}" -S "${OUTPUT}" -B "${OUTPUT}/build" -G "${GENERATOR}"
		"-DCMAKE_TOOLCHAIN_FILE=${emscripten}/cmake/Modules/Platform/Emscripten.cmake")
else()
	set(configure "${CMAKE_COMMAND}" -S "${OUTPUT}" -B "${OUTPUT}/build" -G "${GENERATOR}")
	if (PLATFORM)
		list(APPEND configure -A "${PLATFORM}")
	endif()
	if (TOOLCHAIN)
		list(APPEND configure "-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN}")
	endif()
endif()
list(APPEND configure "-DCMAKE_BUILD_TYPE=${CONFIG}")
# A library the main build fetched is taken from its copy, not fetched again.
foreach(library SFML lunasvg)
	string(TOLOWER "${library}" folder)
	if (DEPS AND EXISTS "${DEPS}/${folder}-src")
		string(TOUPPER "${library}" upper)
		list(APPEND configure "-DFETCHCONTENT_SOURCE_DIR_${upper}=${DEPS}/${folder}-src")
	endif()
endforeach()

execute_process(COMMAND ${configure} RESULT_VARIABLE result OUTPUT_VARIABLE said ERROR_VARIABLE said)
if (NOT result EQUAL 0)
	message(FATAL_ERROR "${GAME} did not configure:\n${said}")
endif()

execute_process(
	COMMAND "${CMAKE_COMMAND}" --build "${OUTPUT}/build" --config "${CONFIG}" --parallel 4
	RESULT_VARIABLE result OUTPUT_VARIABLE said ERROR_VARIABLE said)
file(WRITE "${OUTPUT}/build.log" "${said}")
if (NOT result EQUAL 0)
	message(FATAL_ERROR "${GAME} did not build (${OUTPUT}/build.log):\n${said}")
endif()

string(REGEX MATCHALL "[^\n]*(warning C[0-9]+|warning:)[^\n]*" warnings "${said}")
if (warnings)
	list(REMOVE_DUPLICATES warnings)
	list(JOIN warnings "\n" lines)
	message(FATAL_ERROR "${GAME} built with warnings:\n${lines}")
endif()

# The program (or page) is in OUTPUT/output; a game's object files are near a gigabyte
# in Debug, so a clean build's folder goes. A failed one stays to look into.
file(REMOVE_RECURSE "${OUTPUT}/build")
message(STATUS "${GAME} generated and built without a warning")
