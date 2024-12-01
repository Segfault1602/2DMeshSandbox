#pragma once

#include <RtAudio.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <sndfile.h>
#include <string>
#include <string_view>

#include "audio.h"
#include "audio_file_manager.h"
#include "test_tone.h"

class AudioFileManager;

#ifdef TWO_PI
#undef TWO_PI
#endif
#include <Stk.h>

class RtAudioManagerImpl : public AudioManager
{
  public:
    RtAudioManagerImpl();
    ~RtAudioManagerImpl() override;

    bool StartAudioStream() override;
    void StopAudioStream() override;
    bool IsAudioStreamRunning() const override;
    AudioStreamInfo GetAudioStreamInfo() const override;

    void SetOutputDevice(std::string_view device_name) override;
    void SetInputDevice(std::string_view device_name) override;
    void SetAudioDriver(std::string_view driver_name) override;
    void SelectInputChannels(uint8_t channels) override;

    std::vector<std::string> GetOutputDevicesName() const override;
    std::vector<std::string> GetInputDevicesName() const override;
    std::vector<std::string> GetSupportedAudioDrivers() const override;
    std::string GetCurrentAudioDriver() const override;

    void PlayTestTone(bool play) override;

    AudioFileManager* GetAudioFileManager() override;

    void Hit() override;

  private:
    static int RtAudioCbStatic(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime,
                               RtAudioStreamStatus status, void* userData);
    int RtAudioCbImpl(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime,
                      RtAudioStreamStatus status);

    std::unique_ptr<RtAudio> rtaudio_;
    RtAudio::StreamParameters output_stream_parameters_;
    RtAudio::StreamParameters input_stream_parameters_;

    int current_output_device_id_ = -1;
    int current_input_device_id_ = -1;

    uint8_t input_selected_channels_ = 0;

    uint32_t buffer_size_ = 512;
    uint32_t sample_rate_ = 48000;
    RtAudio::Api current_audio_api_ = RtAudio::Api::UNSPECIFIED;

    bool play_test_tone_ = false;

    TestToneGenerator test_tone_;

    std::unique_ptr<AudioFileManager> audio_file_manager_;

    // STK
    std::atomic_bool queue_hit_ = false;
};