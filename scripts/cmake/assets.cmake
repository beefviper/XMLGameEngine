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
		"games/breakout.xml"
		"games/spaceinvaders.xml"
		"games/frogger.xml"
		"games/spacerace.xml"
		"games/kaboom.xml"
		"games/freeway.xml"
		"games/depthcharge.xml"
		"games/astrosmash.xml")

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

function(add_data_source_group group_name)
	source_group("${group_name}" FILES ${ARGN})
endfunction()

add_data_source_group("XSD Files" ${data_xsd})
add_data_source_group("XML Files" ${data_xml})
add_data_source_group("Asset Files" ${data_assets})
