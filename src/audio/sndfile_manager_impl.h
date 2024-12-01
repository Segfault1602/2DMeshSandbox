#pragma once

#include "audio_file_manager.h"

#include <atomic>
#include <cstddef>
#include <samplerate.h>
#include <sndfile.h>
#include <string>
#include <string_view>
#include <vector>

class SndFileManagerImpl : public AudioFileManager
{
  public:
    SndFileManagerImpl() = default;
    ~SndFileManagerImpl() override = default;
    void SetSampleRate(int sample_rate) override;

    bool OpenAudioFile(std::string_view file_name) override;

    std::string GetOpenFileName() const override;
    bool IsFileOpen() const override;

    void ProcessBlock(float* out_buffer, size_t frame_size, size_t num_channels, float gain = 1.f) override;

    AudioPlayerState GetState() const override;
    void Play() override;
    void Pause() override;
    void Resume() override;
    void Stop() override;

  private:
    SNDFILE* file_ = nullptr;
    SF_INFO file_info_{};
    size_t current_frame_ = 0;
    std::atomic<bool> is_playing_ = false;
    bool is_paused_ = false;
    std::string file_name_;
    int sample_rate_ = 48000;
    bool need_resample_ = false;

    SRC_STATE* src_state_ = nullptr;

    std::vector<float> buffer_;

    AudioPlayerState state_ = AudioPlayerState::kStopped;
};