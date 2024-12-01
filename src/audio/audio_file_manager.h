#pragma once

#include <string>

class AudioFileManager
{
  public:
    enum class AudioPlayerState
    {
        kPlaying,
        kPaused,
        kStopRequested,
        kStopped
    };

    AudioFileManager() = default;
    virtual ~AudioFileManager() = default;

    virtual void SetSampleRate(int sample_rate) = 0;

    virtual bool OpenAudioFile(std::string_view file_name) = 0;
    virtual std::string GetOpenFileName() const = 0;
    virtual bool IsFileOpen() const = 0;

    virtual void ProcessBlock(float* out_buffer, size_t frame_size, size_t num_channels, float gain = 1.f) = 0;

    virtual AudioPlayerState GetState() const = 0;
    virtual void Play() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void Stop() = 0;
};