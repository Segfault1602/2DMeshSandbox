find_package(OpenGL REQUIRED)

include(FetchContent)

FetchContent_Declare(
    GLFW
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

FetchContent_MakeAvailable(glfw)
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL "GLFW_INSTALL" CACHE INTERNAL "Generate installation target")

FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
)

FetchContent_MakeAvailable(glad)
set(GLAD_PROFILE "core" CACHE STRING "OpenGL profile")
set(GLAD_API "gl=" CACHE STRING "API type/version pairs, like \"gl=3.2,gles=\", no version means latest")
set(GLAD_GENERATOR "c" CACHE STRING "Language to generate the binding for")
# add_subdirectory(${glad_SOURCE_DIR} ${glad_BINARY_DIR})

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
)

FetchContent_MakeAvailable(imgui)

set(IMGUI_INCLUDE_DIR ${imgui_SOURCE_DIR}/ ${imgui_SOURCE_DIR}/backends/)
file(GLOB IMGUI_SOURCES ${imgui_SOURCE_DIR}/*.cpp)
file(GLOB IMGUI_HEADERS ${imgui_SOURCE_DIR}/*.h)
add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_SOURCES} ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
# add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)
target_include_directories(imgui PUBLIC ${IMGUI_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${GLFW_INCLUDE_DIR} ${GLAD_INCLUDE_DIR})
target_link_libraries(imgui ${OPENGL_LIBRARIES} glfw glad)

FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG master
)
FetchContent_MakeAvailable(implot)

set(IMPLOT_INCLUDE_DIR ${implot_SOURCE_DIR})
file(GLOB IMPLOT_SOURCES ${implot_SOURCE_DIR}/*.cpp)
add_library(implot STATIC ${IMPLOT_SOURCES})
target_include_directories(implot PUBLIC ${IMPLOT_INCLUDE_DIR} ${IMGUI_INCLUDE_DIR})

FetchContent_Declare(
    imgui-filebrowser
    GIT_REPOSITORY https://github.com/AirGuanZ/imgui-filebrowser.git
    GIT_TAG master
    )

FetchContent_MakeAvailable(imgui-filebrowser)
set(IMGUI_FILEBROWSER_INCLUDE_DIR ${imgui-filebrowser_SOURCE_DIR})

FetchContent_Declare(
    rtaudio
    GIT_REPOSITORY https://github.com/thestk/rtaudio.git
    GIT_TAG 6.0.1
    )

set(RTAUDIO_API_ASIO OFF CACHE BOOL "" FORCE)
set(RTAUDIO_API_JACK OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(RTAUDIO_TARGETNAME_UNINSTALL "RTAUDIO_UNINSTALL" CACHE BOOL "RTAUDIO_UNINSTALL" FORCE)
FetchContent_MakeAvailable(rtaudio)

FetchContent_Declare(
    libsndfile
    GIT_REPOSITORY https://github.com/libsndfile/libsndfile.git
    GIT_TAG 1.2.2
    )

set(BUILD_PROGRAMS OFF CACHE BOOL "Don't build libsndfile programs!")
set(BUILD_EXAMPLES OFF CACHE BOOL "Don't build libsndfile examples!")
set(BUILD_REGTEST OFF CACHE BOOL "Don't build libsndfile regtest!")
set(BUILD_PROGRAMS OFF CACHE BOOL "Don't build libsndfile programs!" FORCE)
set(ENABLE_EXTERNAL_LIBS OFF CACHE BOOL "Disable external libs support!" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Disable libsndfile tests!" FORCE)
set(ENABLE_MPEG OFF CACHE BOOL "Disable MPEG support!" FORCE)
set(ENABLE_CPACK OFF CACHE BOOL "Disable CPACK!" FORCE)
set(ENABLE_PACKAGE_CONFIG OFF CACHE BOOL "Disable package config!" FORCE)
set(INSTALL_PKGCONFIG_MODULE OFF CACHE BOOL "Disable pkgconfig module!" FORCE)

FetchContent_MakeAvailable(libsndfile)

FetchContent_Declare(
    pffft
    GIT_REPOSITORY https://bitbucket.org/jpommier/pffft.git
)

FetchContent_MakeAvailable(pffft)
add_library(pffft STATIC ${pffft_SOURCE_DIR}/pffft.c)
target_compile_definitions(pffft PRIVATE -D_USE_MATH_DEFINES)
target_include_directories(pffft PUBLIC ${pffft_SOURCE_DIR})

FetchContent_Declare(
    libsamplerate
    GIT_REPOSITORY https://github.com/libsndfile/libsamplerate.git
)
FetchContent_MakeAvailable(libsamplerate)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    nanobench
    GIT_REPOSITORY https://github.com/martinus/nanobench.git
    GIT_TAG v4.1.0
    GIT_SHALLOW TRUE)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(nanobench doctest)