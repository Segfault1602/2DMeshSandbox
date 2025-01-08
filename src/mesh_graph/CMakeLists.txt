set(MESH_SOURCE
    rectilinear_mesh.cpp
    junction.cpp
    trimesh.cpp
    wave_math.cpp
    rimguide.cpp
    rimguide_utils.cpp
    mesh_2d.cpp
    listener.cpp
    allpass.cpp
    )

add_library(mesh_graph STATIC ${MESH_SOURCE})
target_link_libraries(mesh_graph utils stk)
target_include_directories(mesh_graph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${STK_INCLUDE_DIR})
target_compile_options(mesh_graph PRIVATE -Wall -Wpedantic)
# target_compile_definitions(mesh_graph PRIVATE -DSLOW_JUNCTION)

add_executable(mesh_graph_test mesh_graph_test.cpp)
target_link_libraries(mesh_graph_test mesh_graph utils sndfile)
target_compile_options(mesh_graph_test PRIVATE -Wall -Wpedantic -fsanitize=address)
target_link_options(mesh_graph_test PRIVATE -fsanitize=address)

add_executable(trimesh_test trimesh_test.cpp)
target_link_libraries(trimesh_test mesh_graph utils sndfile stk)
target_compile_options(trimesh_test PRIVATE -Wall -Wpedantic -fsanitize=address)
target_link_options(trimesh_test PRIVATE -fsanitize=address)

add_executable(mesh_perf_test perf_tests.cpp)
target_include_directories(mesh_perf_test PRIVATE ${doctest_SOURCE_DIR}/doctest)
target_link_libraries(mesh_perf_test PRIVATE mesh_graph utils nanobench doctest)