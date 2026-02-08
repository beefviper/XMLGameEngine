# engine.cmake
# XML Game Engine
# author: beefviper
# date: Feb 6, 2026

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(${PROJECT_NAME} PRIVATE XercesC::XercesC)

target_link_libraries(${PROJECT_NAME} PRIVATE sfml-system
	sfml-window sfml-graphics sfml-network sfml-audio)

if (EXPRTK_PACKAGE_FOUND)
	target_include_directories(${PROJECT_NAME} PRIVATE ${EXPRTK_INCLUDE_DIRS})
else()
	target_link_libraries(${PROJECT_NAME} PRIVATE exprtk)
endif()
