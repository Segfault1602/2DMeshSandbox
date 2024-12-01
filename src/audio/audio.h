#pragma once

#include <memory>
#include <string>
#include <vector>

#include "audio_file_manager.h"

using AudioStreamInfo = struct _AudioStreamInfo
{
    unsigned int sample_rate;
    unsigned int buffer_size;
    unsigned int num_input_channels;
    unsigned int num_output_channels;
};

class AudioManager
{
  public:
    static std::unique_ptr<AudioManager> CreateAudioManager();

    AudioManager() = default;
    virtual ~AudioManager() = default;

    virtual bool StartAudioStream() = 0;
    virtual void StopAudioStream() = 0;
    virtual bool IsAudioStreamRunning() const = 0;
    virtual AudioStreamInfo GetAudioStreamInfo() const = 0;
    virtual void SelectInputChannels(uint8_t channels) = 0;

    virtual void SetOutputDevice(std::string_view device_name) = 0;
    virtual void SetInputDevice(std::string_view device_name) = 0;
    virtual void SetAudioDriver(std::string_view driver_name) = 0;

    virtual std::vector<std::string> GetOutputDevicesName() const = 0;
    virtual std::vector<std::string> GetInputDevicesName() const = 0;

    virtual std::vector<std::string> GetSupportedAudioDrivers() const = 0;
    virtual std::string GetCurrentAudioDriver() const = 0;

    virtual void PlayTestTone(bool play) = 0;

    virtual AudioFileManager* GetAudioFileManager() = 0;

    virtual void Hit() = 0;
};
