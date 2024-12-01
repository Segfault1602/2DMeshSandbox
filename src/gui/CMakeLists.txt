set(GUI_SOURCE
    audio_gui.cpp
    mesh_gui.cpp
    circular_mesh_manager.cpp
    )

add_library(mesh2dgui STATIC ${GUI_SOURCE})
target_link_libraries(mesh2dgui PRIVATE imgui implot ${OPENGL_LIBRARIES} audiolib sndfile mesh_graph graphics)
target_include_directories(mesh2dgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${IMGUI_FILEBROWSER_INCLUDE_DIR} ${STK_INCLUDE_DIR})
target_compile_definitions(mesh2dgui PUBLIC GL_SILENCE_DEPRECATION)
target_compile_options(mesh2dgui PUBLIC -Wall -fsanitize=address -fno-omit-frame-pointer)
target_link_options(mesh2dgui PUBLIC -fsanitize=address)
