# targets.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

# xgelib is the engine library. The third-party libraries below are
# PUBLIC because the engine's own headers include theirs (xml/xerces.h
# includes Xerces, window/sfml.h includes SFML, and so on), so anything that includes the engine's headers needs them too.
target_compile_features(xgelib PUBLIC cxx_std_20)

target_include_directories(xgelib PUBLIC xge/lib)

# Only the backends that are built (options.cmake) are linked, and each one
# defines XGE_WITH_<NAME> for the engine and its users: the factories (window/window.cpp,
# audio/audio.cpp, xml/document.cpp) use it to know what they can make, and so does
# anything that asks WindowFactory::available() and its two siblings.
function(xge_backend name)
	target_compile_definitions(xgelib PUBLIC XGE_WITH_${name})
endfunction()

# Xerces: the XML backend with full schema validation (xml/xerces.h).
if (XGE_WITH_XERCES)
	xge_backend(XERCES)
	target_link_libraries(xgelib PUBLIC XercesC::XercesC)
endif()

# SFML 3's window and sound (window/sfml.h, audio/sfml.h). Nothing in the engine
# uses SFML's network module, so it is neither found nor linked. Every game
# draws with one Window backend and plays its sounds with one Audio backend at
# run time (chosen by whichever WindowBackend and AudioBackend Engine is
# constructed with - see window/window.h and audio/audio.h), out of the ones built.
if (XGE_WITH_SFML3)
	xge_backend(SFML3)
	target_link_libraries(xgelib PUBLIC SFML::System
		SFML::Window SFML::Graphics SFML::Audio)
endif()

# Raylib's window and sound - see window/raylib.h and audio/raylib.h.
if (XGE_WITH_RAYLIB)
	xge_backend(RAYLIB)
	target_link_libraries(xgelib PUBLIC raylib)
endif()

# The OpenGL backend (window/opengl.h) is built on GLFW, which raylib is built
# on too: one `glfw` target either way (found, fetched, or raylib's own). It
# calls OpenGL itself (window/opengl.cpp), so it links the system's OpenGL
# library: opengl32 on Windows, libGL on Linux.
if (XGE_WITH_OPENGL)
	xge_backend(OPENGL)
	target_link_libraries(xgelib PUBLIC glfw OpenGL::GL)
endif()

# SDL2's window and sound - see window/sdl2.h and audio/sdl2.h. Its libraries are
# also the OpenGL backend's, for its pictures and text (options.cmake).
if (XGE_WITH_SDL2)
	xge_backend(SDL2)
endif()

if (XGE_NEEDS_SDL2)
	target_link_libraries(xgelib PUBLIC SDL2::SDL2)
	if (TARGET SDL2::SDL2main)
		target_link_libraries(xgelib PUBLIC SDL2::SDL2main)
	endif()

	target_link_libraries(xgelib PUBLIC SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf)
endif()

# lunasvg is PRIVATE: only picture/svg.cpp uses it, and picture/svg.h speaks in the engine's
# own Bitmap, so nothing that includes the engine's headers needs it.
target_link_libraries(xgelib PRIVATE lunasvg::lunasvg)

# TinyXML2, PugiXML and RapidXML XML backends - see xml/tinyxml2.h/
# xml/pugixml.h/xml_rapidxml.h. Every game parses with one (chosen by whichever
# XmlBackend Game is constructed with - see xml/document.h), out of the ones
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

# libxslt for xgecli's --generate (xge/cli/source/generate.cpp), and for the tests
# that compile that file in again. XGE_WITH_XSLT is 1 when it was found, so the
# file can say, when it was not, that this program cannot generate.
# XGE_ENGINE_FOLDER is xge/ in the repository: a generated program's engine
# files are copied from xge/lib/ there, not from a copy beside the programs.
function(xge_link_xslt target)
	target_compile_definitions(${target} PRIVATE XGE_ENGINE_FOLDER="${PROJECT_SOURCE_DIR}/xge")
	if (LibXslt_FOUND AND LibXml2_FOUND)
		target_compile_definitions(${target} PRIVATE XGE_WITH_XSLT=1)
		target_link_libraries(${target} PRIVATE LibXslt::LibXslt LibXslt::LibExslt LibXml2::LibXml2)
	else()
		target_compile_definitions(${target} PRIVATE XGE_WITH_XSLT=0)
	endif()
endfunction()
