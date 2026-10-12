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

# The stylesheets xgecli --generate runs, a folder per target (generate.cpp),
# and the target's modules, copied into a program unchanged.
set(data_generators
	"generators/windows-cpp/generate.xsl"
	"generators/windows-cpp/text.xsl"
	"generators/windows-cpp/data.xsl"
	"generators/windows-cpp/program.xsl"
	"generators/windows-cpp/engine.xml"
	"generators/windows-cpp/modules/game_parts.h")

# The engine's own files a generated program compiles unchanged
# (generators/windows-cpp/engine.xml, which this list must match):
# the whole engine but for reading XML and choosing among libraries. Copied
# under lib/ as they are here; they are already the engine's sources, so they
# are not listed again beside the data.
set(data_engine
	"lib/include/types.h"
	"lib/include/geometry.h"
	"lib/source/geometry.cpp"
	"lib/include/color.h"
	"lib/source/color.cpp"
	"lib/include/bitmap.h"
	"lib/source/bitmap.cpp"
	"lib/include/svg.h"
	"lib/source/svg.cpp"
	"lib/include/spelling.h"
	"lib/source/spelling.cpp"
	"lib/include/keycode.h"
	"lib/source/keycode.cpp"
	"lib/include/command.h"
	"lib/source/command.cpp"
	"lib/include/object.h"
	"lib/source/object.cpp"
	"lib/include/states.h"
	"lib/source/states.cpp"
	"lib/include/sound.h"
	"lib/source/sound.cpp"
	"lib/include/game_data.h"
	"lib/include/game_expr.h"
	"lib/source/game_expr.cpp"
	"lib/include/collision_detector.h"
	"lib/source/collision_detector.cpp"
	"lib/include/command_executor.h"
	"lib/source/command_executor.cpp"
	"lib/include/game.h"
	"lib/source/game.cpp"
	"lib/include/audio.h"
	"lib/include/audio_sfml.h"
	"lib/source/audio_sfml.cpp"
	"lib/include/builtin_font.h"
	"lib/source/builtin_font.cpp"
	"lib/include/window.h"
	"lib/include/window_sfml.h"
	"lib/source/window_sfml.cpp"
	"lib/include/engine.h"
	"lib/source/engine.cpp")

set(data_outputs "")
foreach(item IN LISTS data_xsd data_xml data_assets data_generators data_engine)
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
