set(GRAPHICS_SOURCE
    line.cpp
    camera.cpp)

add_library(graphics STATIC ${GRAPHICS_SOURCE})
target_include_directories(graphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(graphics ${OPENGL_LIBRARIES} glfw glad glm)
target_compile_definitions(graphics PUBLIC GL_SILENCE_DEPRECATION)