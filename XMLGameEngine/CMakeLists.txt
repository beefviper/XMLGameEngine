# CMakeLists.txt 
# XML Game Engine
# author: beefviper
# date: Sept 18, 2020

# CMakeList.txt : CMake project for XMLGameEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

if (MSVC)
    add_compile_options(/bigobj)
endif()

find_package(SFML COMPONENTS system window graphics network audio CONFIG REQUIRED)
find_package(XercesC CONFIG REQUIRED)

find_path(EXPRTK_INCLUDE_DIRS "exprtk.hpp")

# Add source to this project's executable.
add_executable (${PROJECT_NAME} "main.cpp"
	"include/game.h" "game.cpp" "include/engine.h" "engine.cpp"
	"include/object.h" "object.cpp" "include/states.h" "states.cpp"
    "include/utils.h" "utils.cpp" "include/game_xml.h" "game_xml.cpp"
    "include/game_expr.h" "game_expr.cpp" "include/game_sfml.h" "game_sfml.cpp")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(${PROJECT_NAME} PRIVATE sfml-system sfml-window sfml-graphics sfml-network sfml-audio)
target_link_libraries(${PROJECT_NAME} PRIVATE xerces_xerces-c XercesC::XercesC)

add_dependencies(${PROJECT_NAME} data-target)

# TODO: Add tests and install targets if needed.

# we don't want to copy if we're building in the source dir
if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)

    # list of files for which we add a copy rule
    set(data_SHADOW "games/pong.xml" "games/breakout.xml" "assets/tuffy.ttf"
        "assets/xmlgameengine.xsd" "assets/paddle.jpg")

    foreach(item IN LISTS data_SHADOW)
        message(STATUS ${item})
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${item}" "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${item}"
        )
    endforeach()
endif()

# files are only copied if a target depends on them
add_custom_target(data-target ALL DEPENDS ${data_SHADOW})
