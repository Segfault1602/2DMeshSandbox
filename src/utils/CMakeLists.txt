set(UTILS_SOURCE
    file_writer.cpp
    threadpool.cpp)

add_library(utils STATIC ${UTILS_SOURCE})
target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
