# sources.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

set(core_sources
    "source/core/engine.cpp"
    "source/core/game.cpp"
)

set(engine_sources
    "source/core/engine/system_clock.cpp"
    "source/core/engine/system_console.cpp"
    "source/core/engine/system_file.cpp"
    "source/core/engine/system_memory.cpp"
    "source/core/engine/system_thread.cpp"

    "source/core/engine/service_timer.cpp"
    "source/core/engine/service_logger.cpp"
    "source/core/engine/service_resource.cpp"

    "source/core/engine/loop_input.cpp"
    "source/core/engine/loop_render.cpp"
    "source/core/engine/loop_update.cpp"
)

set(game_sources
    "source/core/game/compile_parse.cpp"
    "source/core/game/compile_validate.cpp"
    "source/core/game/compile_evaluate.cpp"
    "source/core/game/compile_generate.cpp"

    "source/core/game/runtime_object.cpp"
    "source/core/game/runtime_states.cpp"
)

set(window_sources
    "source/window/window_factory.cpp"
    "source/window/window_sfml3.cpp"
    "source/window/window_sdl2.cpp"
)

set(xml_sources
    "source/xml/xml_factory.cpp"
    "source/xml/xml_xerces.cpp"
    "source/xml/xml_tinyxml2.cpp"
)

set(main_sources
    "source/main.cpp"
)

set(core_headers
    "include/core/types.h"
    "include/core/engine.h"
    "include/core/game.h"
)

set(engine_headers
    "include/core/engine/system_clock.h"
    "include/core/engine/system_console.h"
    "include/core/engine/system_file.h"
    "include/core/engine/system_memory.h"
    "include/core/engine/system_thread.h"

    "include/core/engine/service_timer.h"
    "include/core/engine/service_logger.h"
    "include/core/engine/service_resource.h"

    "include/core/engine/loop_input.h"
    "include/core/engine/loop_render.h"
    "include/core/engine/loop_update.h"
)

set(game_headers
    "include/core/game/compile_parse.h"
    "include/core/game/compile_validate.h"
    "include/core/game/compile_evaluate.h"
    "include/core/game/compile_generate.h"

    "include/core/game/runtime_object.h"
    "include/core/game/runtime_states.h"
)

set(window_headers
    "include/window/window_interface.h"
    "include/window/window_factory.h"
    "include/window/window_sfml3.h"
    "include/window/window_sdl2.h"
)

set(xml_headers
    "include/xml/xml_interface.h"
    "include/xml/xml_factory.h"
    "include/xml/xml_xerces.h"
    "include/xml/xml_tinyxml2.h"
)

set(all_sources
    ${main_sources}
    ${core_sources}
    ${engine_sources}
    ${game_sources}
    ${window_sources}
    ${xml_sources}
)

set(all_headers
    ${core_headers}
    ${engine_headers}
    ${game_headers}
    ${window_headers}
    ${xml_headers}
)

target_sources(${PROJECT_NAME} PRIVATE
    ${all_sources}
    ${all_headers}
)

source_group("Source Files\\Core" FILES ${core_sources})
source_group("Source Files\\Core\\Engine" FILES ${engine_sources})
source_group("Source Files\\Core\\Game" FILES ${game_sources})

source_group("Source Files\\Window" FILES ${window_sources})
source_group("Source Files\\XML" FILES ${xml_sources})
source_group("Source Files" FILES ${main_sources})

source_group("Header Files\\Core" FILES ${core_headers})
source_group("Header Files\\Core\\Engine" FILES ${engine_headers})
source_group("Header Files\\Core\\Game" FILES ${game_headers})

source_group("Header Files\\Window" FILES ${window_headers})
source_group("Header Files\\XML" FILES ${xml_headers})
