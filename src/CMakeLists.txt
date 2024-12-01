add_subdirectory(audio)
add_subdirectory(utils)
add_subdirectory(mesh_graph)
add_subdirectory(graphics)
add_subdirectory(gui)

set(EXE_SOURCE
    main.cpp
    )

add_executable(${PROJECT_NAME} ${EXE_SOURCE})

target_link_libraries(${PROJECT_NAME} PRIVATE imgui implot ${OPENGL_LIBRARIES} sndfile audiolib rtaudio graphics mesh2dgui)
target_include_directories(${PROJECT_NAME} PUBLIC ${IMGUI_FILEBROWSER_INCLUDE_DIR} ${STK_INCLUDE_DIR})
target_compile_definitions(${PROJECT_NAME} PUBLIC GL_SILENCE_DEPRECATION)
target_compile_options(${PROJECT_NAME} PUBLIC -Wall -fsanitize=address -fno-omit-frame-pointer)
target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=address)