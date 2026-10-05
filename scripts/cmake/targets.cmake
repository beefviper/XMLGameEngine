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

# Only the backends that are built (options.cmake) are linked, and each one
# defines XGE_WITH_<NAME> for the engine and its users: the factories (window.cpp,
# audio.cpp, xml_document.cpp) use it to know what they can make, and so does
# anything that asks WindowFactory::available() and its two siblings.
function(xge_backend name)
	target_compile_definitions(xgelib PUBLIC XGE_WITH_${name})
endfunction()

# Xerces: the XML backend with full schema validation (xml_xerces.h).
if (XGE_WITH_XERCES)
	xge_backend(XERCES)
	target_link_libraries(xgelib PUBLIC XercesC::XercesC)
endif()

# SFML 3's window and sound (window_sfml.h, audio_sfml.h). Nothing in the engine
# uses SFML's network module, so it is neither found nor linked. Every game
# draws with one Window backend and plays its sounds with one Audio backend at
# run time (chosen by whichever WindowBackend and AudioBackend Engine is
# constructed with - see window.h and audio.h), out of the ones built.
if (XGE_WITH_SFML3)
	xge_backend(SFML3)
	target_link_libraries(xgelib PUBLIC SFML::System
		SFML::Window SFML::Graphics SFML::Audio)
endif()

# Raylib's window and sound - see window_raylib.h and audio_raylib.h.
if (XGE_WITH_RAYLIB)
	xge_backend(RAYLIB)
	target_link_libraries(xgelib PUBLIC raylib)
endif()

# The OpenGL backend (window_opengl.h) is built on GLFW, which raylib is built
# on too: one `glfw` target either way (found, fetched, or raylib's own). It
# calls OpenGL itself (window_opengl.cpp), so it links the system's OpenGL
# library: opengl32 on Windows, libGL on Linux.
if (XGE_WITH_OPENGL)
	xge_backend(OPENGL)
	target_link_libraries(xgelib PUBLIC glfw OpenGL::GL)
endif()

# SDL2's window and sound - see window_sdl2.h and audio_sdl2.h.
if (XGE_WITH_SDL2)
	xge_backend(SDL2)
	target_link_libraries(xgelib PUBLIC SDL2::SDL2)
	if (TARGET SDL2::SDL2main)
		target_link_libraries(xgelib PUBLIC SDL2::SDL2main)
	endif()

	target_link_libraries(xgelib PUBLIC SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf)
endif()

# lunasvg is PRIVATE: only svg.cpp uses it, and svg.h speaks in the engine's
# own Bitmap, so nothing that includes the engine's headers needs it.
target_link_libraries(xgelib PRIVATE lunasvg::lunasvg)

# TinyXML2, PugiXML and RapidXML XML backends - see xml_tinyxml2.h/
# xml_pugixml.h/xml_rapidxml.h. Every game parses with one (chosen by whichever
# XmlBackend Game is constructed with - see xml_document.h), out of the ones
# built.
if (XGE_WITH_TINYXML2)
	xge_backend(TINYXML2)
	target_link_libraries(xgelib PUBLIC tinyxml2::tinyxml2)
endif()

if (XGE_WITH_PUGIXML)
	xge_backend(PUGIXML)
	target_link_libraries(xgelib PUBLIC pugixml::pugixml)
endif()

if (XGE_WITH_RAPIDXML)
	xge_backend(RAPIDXML)
	target_link_libraries(xgelib PUBLIC rapidxml::rapidxml)
endif()

if (EXPRTK_PACKAGE_FOUND)
	target_include_directories(xgelib SYSTEM PUBLIC ${EXPRTK_INCLUDE_DIRS})
else()
	target_link_libraries(xgelib PUBLIC exprtk)
endif()
