# assets.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

add_dependencies(xgelib xgedata)

# The games are copied into output/<config> (output.cmake), next to the
# programs, in the same folders as in the repository: games/descriptions/,
# games/assets/ and games/generators/, and the schema, games/xgedef.xsd, which
# a game names as ../xgedef.xsd (the engine also looks for it by its file name
# in the folders above a game when it is not there: findSchema, engine/folder.h).
# Each file is its own copy step, so a build copies only the files that
# changed. The engine's own files are not copied: --generate takes them from
# xge/lib/ in the repository (xge_link_xslt, targets.cmake).
set(data_xsd
	"games/xgedef.xsd"
	"games/xgedump.xsd")

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
	"games/descriptions/adventure.xml"
	"games/descriptions/trickshot.xml"
	"games/descriptions/mariobros.xml"
	"games/descriptions/fishingderby.xml"
	"games/descriptions/qbert.xml"
	"games/descriptions/yarsrevenge.xml"
	"games/descriptions/tennis.xml"
	"games/descriptions/hauntedhouse.xml"
	"games/descriptions/tetris.xml"
	"games/descriptions/columns.xml"
	"games/descriptions/capsules.xml"
	"games/descriptions/puttpark.xml"
	"games/descriptions/pinball.xml"
	"games/descriptions/cannonhill.xml"
	"games/descriptions/speedway.xml"
	"games/descriptions/checkers.xml"
	"games/descriptions/reversi.xml"
	"games/descriptions/fourinarow.xml"
	"games/descriptions/tictactoe.xml"
	"games/descriptions/dotsandboxes.xml"
	"games/descriptions/klondike.xml"
	"games/descriptions/crazyeights.xml"
	"games/descriptions/blackjack.xml"
	"games/descriptions/holdem.xml"
	"games/descriptions/hearts.xml"
	"games/descriptions/spades.xml"
	"games/descriptions/gin.xml"
	"games/descriptions/pyramid.xml"
	"games/descriptions/snake.xml"
	"games/descriptions/centipede.xml"
	"games/descriptions/crates.xml"
	"games/descriptions/doubles.xml"
	"games/descriptions/minefield.xml")

set(data_assets
	"games/assets/tuffy.ttf"
	"games/assets/pressstart2p.ttf"
	"games/assets/pressstart2p-ofl.txt"
	"games/assets/paddle.jpg"
	"games/assets/cards.png"
	"games/assets/Space Invaders Color Sprites.svg")

# The stylesheets xgecli --generate runs, a folder per target (generate.cpp),
# and the target's modules, copied into a program unchanged.
set(data_generators
	"games/generators/engine-cpp/generate.xsl"
	"games/generators/engine-cpp/text.xsl"
	"games/generators/engine-cpp/merge.xsl"
	"games/generators/engine-cpp/data.xsl"
	"games/generators/engine-cpp/program.xsl"
	"games/generators/engine-cpp/engine.xml"
	"games/generators/engine-cpp/modules/game_parts.h"
	"games/generators/windows-cpp/generate.xsl"
	"games/generators/windows-cpp/names.xsl"
	"games/generators/windows-cpp/values.xsl"
	"games/generators/windows-cpp/covers.xsl"
	"games/generators/windows-cpp/game.xsl"
	"games/generators/windows-cpp/program.xsl"
	"games/generators/windows-cpp/verbs/bounce.xsl"
	"games/generators/windows-cpp/verbs/deflect.xsl"
	"games/generators/windows-cpp/verbs/move.xsl"
	"games/generators/windows-cpp/verbs/reset.xsl"
	"games/generators/windows-cpp/verbs/stick.xsl"
	"games/generators/windows-cpp/verbs/trigger.xsl")

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
