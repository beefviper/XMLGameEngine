# assets.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

add_dependencies(xgelib xgedata)

# The games are copied into output/<config> (output.cmake), next to the
# programs, in the same folders as in the repository: games/descriptions/,
# games/assets/ and games/generators/. The schema goes at the top of
# output/<config> (xgedef.xsd): a game names it as ../../xge/xgedef.xsd, which
# is right in the repository, and the engine looks for it by its file name in
# the folders above a game when it is not there (findSchema, engine/folder.h).
# Each file is its own copy step, so a build copies only the files that
# changed. The engine's own files are not copied: --generate takes them from
# xge/lib/ in the repository (xge_link_xslt, targets.cmake).
set(data_xsd
	"xge/xgedef.xsd")

set(data_xml
	"games/descriptions/pong.xml"
	"games/descriptions/breakout.xml"
	"games/descriptions/spaceinvaders.xml"
	"games/descriptions/spaceinvaders2.xml"
	"games/descriptions/frogger.xml"
	"games/descriptions/spacerace.xml"
	"games/descriptions/kaboom.xml"
	"games/descriptions/freeway.xml"
	"games/descriptions/depthcharge.xml"
	"games/descriptions/astrosmash.xml"
	"games/descriptions/lunarlander.xml"
	"games/descriptions/asteroids.xml"
	"games/descriptions/berserk.xml"
	"games/descriptions/demonattack.xml"
	"games/descriptions/frostbite.xml"
	"games/descriptions/donkeykong.xml"
	"games/descriptions/galaxian.xml"
	"games/descriptions/pong_min.xml"
	"games/descriptions/pitfall.xml"
	"games/descriptions/missilecommand.xml"
	"games/descriptions/combat.xml"
	"games/descriptions/airseabattle.xml"
	"games/descriptions/atlantis.xml"
	"games/descriptions/pacman.xml"
	"games/descriptions/megamania.xml"
	"games/descriptions/adventure.xml")

set(data_assets
	"games/assets/tuffy.ttf"
	"games/assets/paddle.jpg"
	"games/assets/Space Invaders Color Sprites.svg")

# The stylesheets xgecli --generate runs, a folder per target (generate.cpp),
# and the target's modules, copied into a program unchanged.
set(data_generators
	"games/generators/windows-cpp/generate.xsl"
	"games/generators/windows-cpp/text.xsl"
	"games/generators/windows-cpp/data.xsl"
	"games/generators/windows-cpp/program.xsl"
	"games/generators/windows-cpp/engine.xml"
	"games/generators/windows-cpp/modules/game_parts.h")

set(data_outputs "")
foreach(item IN LISTS data_xsd)
	get_filename_component(name "${item}" NAME)
	add_custom_command(
		OUTPUT "${XGE_OUTPUT_DIR}/${name}"
		COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${item}" "${XGE_OUTPUT_DIR}/${name}"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${item}")
	list(APPEND data_outputs "${XGE_OUTPUT_DIR}/${name}")
endforeach()

foreach(item IN LISTS data_xml data_assets data_generators)
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
