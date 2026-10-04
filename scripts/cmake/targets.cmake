# targets.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

# xgelib is the engine library. The third-party libraries below are
# PUBLIC because the engine's own headers include theirs (game_expr.h
# includes exprtk, xml_xerces.h includes Xerces, window_sfml.h includes SFML,
# and so on), so anything that includes the engine's headers needs them too.
target_compile_features(xgelib PUBLIC cxx_std_20)

target_include_directories(xgelib PUBLIC lib/include)

target_link_libraries(xgelib PUBLIC XercesC::XercesC)

# SFML::Audio is the SFML 3 Audio backend (audio_sfml.h); raylib's and SDL2's
# sound (audio_raylib.h, audio_sdl2.h) come with the libraries linked below,
# with nothing more to link. Like the Window backends, every game plays its
# sounds with one Audio backend (chosen by whichever AudioBackend Engine is
# constructed with - see audio.h), and all of them are built in.
target_link_libraries(xgelib PUBLIC SFML::System
	SFML::Window SFML::Graphics SFML::Network SFML::Audio)

# Raylib and SDL2 backends - see window_raylib.h/window_sdl2.h. Every game
# still only uses one Window backend at runtime (chosen by whichever
# WindowBackend Engine is constructed with - see window.h), but all four
# (SFML3 and OpenGL included) are built in so that choice can be made at
# runtime instead of at build time.
target_link_libraries(xgelib PUBLIC raylib)

# The OpenGL backend (window_opengl.h) is built on GLFW, which raylib is built
# on too: one `glfw` target either way (found, fetched, or raylib's own).
target_link_libraries(xgelib PUBLIC glfw)

# The OpenGL backend calls OpenGL itself (window_opengl.cpp), so it links the
# system's OpenGL library: opengl32 on Windows, libGL on Linux.
target_link_libraries(xgelib PUBLIC OpenGL::GL)

target_link_libraries(xgelib PUBLIC SDL2::SDL2)
if (TARGET SDL2::SDL2main)
	target_link_libraries(xgelib PUBLIC SDL2::SDL2main)
endif()

target_link_libraries(xgelib PUBLIC SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf)

# lunasvg is PRIVATE: only svg.cpp uses it, and svg.h speaks in the engine's
# own Bitmap, so nothing that includes the engine's headers needs it.
target_link_libraries(xgelib PRIVATE lunasvg::lunasvg)

# TinyXML2, PugiXML, and RapidXML XML backends - see xml_tinyxml2.h/
# xml_pugixml.h/xml_rapidxml.h. Same reasoning as the four Window backends
# above: every game only ever parses with one (chosen by whichever
# XmlBackend Game is constructed with - see xml_document.h), but all four
# (Xerces included) are built in so that choice can be made at runtime
# instead of at build time.
target_link_libraries(xgelib PUBLIC tinyxml2::tinyxml2 pugixml::pugixml rapidxml::rapidxml)

if (EXPRTK_PACKAGE_FOUND)
	target_include_directories(xgelib PUBLIC ${EXPRTK_INCLUDE_DIRS})
else()
	target_link_libraries(xgelib PUBLIC exprtk)
endif()
