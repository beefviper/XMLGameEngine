# engine.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(${PROJECT_NAME} PRIVATE XercesC::XercesC)

target_link_libraries(${PROJECT_NAME} PRIVATE SFML::System
	SFML::Window SFML::Graphics SFML::Network SFML::Audio)

# Raylib and SDL2 backends - see window_raylib.h/window_sdl2.h. Every game
# still only links one Window backend at runtime (chosen by whichever
# WindowBackend Engine is constructed with - see window.h), but all three
# are built in so that choice can be made at runtime instead of at build time.
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2)
if (TARGET SDL2::SDL2main)
	target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf)

# TinyXML2, PugiXML, and RapidXML XML backends - see xml_tinyxml2.h/
# xml_pugixml.h/xml_rapidxml.h. Same reasoning as the three Window backends
# above: every game only ever parses with one (chosen by whichever
# XmlBackend Game is constructed with - see xml_document.h), but all four
# (Xerces included) are built in so that choice can be made at runtime
# instead of at build time.
target_link_libraries(${PROJECT_NAME} PRIVATE tinyxml2::tinyxml2 pugixml::pugixml rapidxml::rapidxml)

if (EXPRTK_PACKAGE_FOUND)
	target_include_directories(${PROJECT_NAME} PRIVATE ${EXPRTK_INCLUDE_DIRS})
else()
	target_link_libraries(${PROJECT_NAME} PRIVATE exprtk)
endif()
