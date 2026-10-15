# assets.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

add_dependencies(xgelib xgedata)

# The games and assets are copied into output/<config> (output.cmake), next to
# the programs, in the same folders as in the repository: games/descriptions/,
# games/assets/ and games/generators/, and xge/xgedef.xsd (the schema, which
# every game names as ../../xge/xgedef.xsd) and the engine files the generator
# copies in xge/lib/. Each file is its own copy step, so a build copies only the
# files that changed.
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

# The engine's own files a generated program compiles unchanged
# (games/generators/windows-cpp/engine.xml, which this list must match):
# the whole engine but for reading XML and choosing among libraries. Copied
# under xge/lib/ as they are here; they are already the engine's sources, so they
# are not listed again beside the data.
set(data_engine
	"xge/lib/include/types.h"
	"xge/lib/include/geometry.h"
	"xge/lib/source/geometry.cpp"
	"xge/lib/include/random.h"
	"xge/lib/source/random.cpp"
	"xge/lib/include/verbs.h"
	"xge/lib/source/verbs.cpp"
	"xge/lib/include/frame.h"
	"xge/lib/source/frame.cpp"
	"xge/lib/include/color.h"
	"xge/lib/source/color.cpp"
	"xge/lib/include/bitmap.h"
	"xge/lib/source/bitmap.cpp"
	"xge/lib/include/svg.h"
	"xge/lib/source/svg.cpp"
	"xge/lib/include/spelling.h"
	"xge/lib/source/spelling.cpp"
	"xge/lib/include/keycode.h"
	"xge/lib/source/keycode.cpp"
	"xge/lib/include/command.h"
	"xge/lib/source/command.cpp"
	"xge/lib/include/object.h"
	"xge/lib/source/object.cpp"
	"xge/lib/include/states.h"
	"xge/lib/source/states.cpp"
	"xge/lib/include/sound.h"
	"xge/lib/source/sound.cpp"
	"xge/lib/include/game_data.h"
	"xge/lib/include/scenes.h"
	"xge/lib/source/scenes.cpp"
	"xge/lib/include/game_expr.h"
	"xge/lib/source/game_expr.cpp"
	"xge/lib/include/collision_detector.h"
	"xge/lib/source/collision_detector.cpp"
	"xge/lib/include/command_executor.h"
	"xge/lib/source/command_executor.cpp"
	"xge/lib/include/game.h"
	"xge/lib/source/game.cpp"
	"xge/lib/include/audio.h"
	"xge/lib/include/audio_sfml.h"
	"xge/lib/source/audio_sfml.cpp"
	"xge/lib/include/builtin_font.h"
	"xge/lib/source/builtin_font.cpp"
	"xge/lib/include/window.h"
	"xge/lib/include/window_sfml.h"
	"xge/lib/source/window_sfml.cpp"
	"xge/lib/include/engine.h"
	"xge/lib/source/engine.cpp")

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
