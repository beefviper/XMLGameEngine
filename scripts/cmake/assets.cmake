# assets.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

add_dependencies(xgelib xgedata)

# The games and assets are copied into output/<config> (output.cmake), next to
# the programs, keeping their games/ and assets/ folders and xgedef.xsd (the
# schema, at the top of the repository, which every game names as ../xgedef.xsd). Each file is its own
# copy step, so a build copies only the files that changed.
set(data_xsd
	"xgedef.xsd")

set(data_xml
	"games/pong.xml"
	"games/breakout.xml"
	"games/spaceinvaders.xml"
	"games/spaceinvaders2.xml"
	"games/frogger.xml"
	"games/spacerace.xml"
	"games/kaboom.xml"
	"games/freeway.xml"
	"games/depthcharge.xml"
	"games/astrosmash.xml"
	"games/lunarlander.xml"
	"games/asteroids.xml"
	"games/berserk.xml"
	"games/demonattack.xml"
	"games/frostbite.xml"
	"games/donkeykong.xml"
	"games/galaxian.xml"
	"games/pong_min.xml"
	"games/pitfall.xml"
	"games/missilecommand.xml"
	"games/combat.xml"
	"games/airseabattle.xml"
	"games/megamania.xml")

set(data_assets
	"assets/tuffy.ttf"
	"assets/paddle.jpg"
	"assets/Space Invaders Color Sprites.svg")

# The stylesheets xgecli --generate runs, a folder per target (generate.cpp).
set(data_generators
	"generators/windows-cpp/generate.xsl"
	"generators/windows-cpp/check.xsl"
	"generators/windows-cpp/main.xsl"
	"generators/windows-cpp/groups.xsl"
	"generators/windows-cpp/looks.xsl"
	"generators/windows-cpp/guards.xsl"
	"generators/windows-cpp/values.xsl"
	"generators/windows-cpp/functions.xml"
	"generators/windows-cpp/tables.xml"
	"generators/windows-cpp/modules/physics.h"
	"generators/windows-cpp/modules/pictures.h"
	"generators/windows-cpp/modules/sound.h"
	"generators/windows-cpp/modules/sound.cpp")

set(data_outputs "")
foreach(item IN LISTS data_xsd data_xml data_assets data_generators)
	message(STATUS ${item})
	add_custom_command(
		OUTPUT "${XGE_OUTPUT_DIR}/${item}"
		COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${item}" "${XGE_OUTPUT_DIR}/${item}"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${item}")
	list(APPEND data_outputs "${XGE_OUTPUT_DIR}/${item}")
endforeach()

add_custom_target(xgedata ALL
	DEPENDS ${data_outputs}
	SOURCES	${data_xsd} ${data_xml} ${data_assets} ${data_generators})

function(add_data_source_group group_name)
	source_group("${group_name}" FILES ${ARGN})
endfunction()

add_data_source_group("XSD Files" ${data_xsd})
add_data_source_group("XML Files" ${data_xml})
add_data_source_group("Asset Files" ${data_assets})
add_data_source_group("Generator Files" ${data_generators})
