set(AUDIOLIB_SOURCE
    audio.cpp
    rtaudio_impl.cpp
    test_tone.cpp
    sndfile_manager_impl.cpp
    fft_utils.cpp
    )

add_library(audiolib STATIC ${AUDIOLIB_SOURCE})
target_compile_options(audiolib PRIVATE -fsanitize=address -fno-omit-frame-pointer)
target_link_libraries(audiolib PRIVATE rtaudio utils sndfile pffft stk samplerate)
target_include_directories(audiolib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${STK_INCLUDE_DIR})
target_link_options(audiolib PUBLIC -fsanitize=address)

add_executable(audio_test audio_test.cpp)
target_link_libraries(audio_test PRIVATE audiolib)