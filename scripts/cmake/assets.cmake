# assets.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

add_dependencies(${PROJECT_NAME} data-target)

if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
	set(data_xsd
		"assets/xmlgameengine.xsd")

	set(data_xml
		"games/pong.xml"
		"games/pong_full.xml"
		"games/breakout.xml"
		"games/spaceinvaders.xml")

	set(data_assets
		"assets/tuffy.ttf"
		"assets/paddle.jpg")

	foreach(item IN LISTS data_xsd data_xml data_assets)
		message(STATUS ${item})
		add_custom_command(
			OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${item}"
			COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${item}" "${CMAKE_CURRENT_BINARY_DIR}/${item}"
			DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${item}")
	endforeach()
endif()

add_custom_target(data-target ALL
	DEPENDS ${data_xsd} ${data_xml} ${data_assets}
	SOURCES	${data_xsd} ${data_xml} ${data_assets})

source_group("XSD Files" FILES ${data_xsd})
source_group("XML Files" FILES ${data_xml})
source_group("Asset Files" FILES ${data_assets})
