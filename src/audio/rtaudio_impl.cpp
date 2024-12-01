#include "rtaudio_impl.h"

#include "audio_file_manager.h"
#include "ring_buffer.tpp"
#include "sndfile_manager_impl.h"

#include <RtAudio.h>
#include <_string.h>
#include <cassert>
#include <iostream>
#include <vector>

namespace
{
void RtAudioErrorCb(RtAudioErrorType type, const std::string& errorText)
{
    std::cerr << "RTAudio Error: " << errorText << std::endl;
}
} // namespace

RtAudioManagerImpl::RtAudioManagerImpl()
    : current_output_device_id_(-1)
    , current_input_device_id_(-1)
{
    rtaudio_ = std::make_unique<RtAudio>(RtAudio::Api::MACOSX_CORE, RtAudioErrorCb);

    std::vector<RtAudio::Api> apis;
    rtaudio_->getCompiledApi(apis);
    for (auto api : apis)
    {
        std::cout << "Compiled API: " << RtAudio::getApiDisplayName(api) << std::endl;
    }

    // Don't open input by default
    current_output_device_id_ = rtaudio_->getDefaultOutputDevice();
    current_input_device_id_ = -1;

    audio_file_manager_ = std::make_unique<SndFileManagerImpl>();
}

RtAudioManagerImpl::~RtAudioManagerImpl()
{
    if (rtaudio_->isStreamOpen())
    {
        rtaudio_->closeStream();
    }
}

bool RtAudioManagerImpl::StartAudioStream()
{
    auto out_device_info = rtaudio_->getDeviceInfo(current_output_device_id_);
    RtAudio::StreamParameters out_parameters;
    out_parameters.deviceId = out_device_info.ID;
    out_parameters.nChannels = out_device_info.outputChannels;
    out_parameters.firstChannel = 0;

    auto in_device_info = rtaudio_->getDeviceInfo(current_input_device_id_);
    RtAudio::StreamParameters in_parameters;
    in_parameters.deviceId = in_device_info.ID;
    in_parameters.nChannels = 1; // Only mono for now
    in_parameters.firstChannel = input_selected_channels_;

    // TODO: make these configurable
    uint32_t buffer_frames = 512;

    RtAudioErrorType error =
        rtaudio_->openStream(&out_parameters, (in_parameters.deviceId != 0) ? &in_parameters : nullptr, RTAUDIO_FLOAT32,
                             sample_rate_, &buffer_frames, &RtAudioCbStatic, this);

    if (error != RTAUDIO_NO_ERROR)
    {
        std::cerr << "Failed to open audio stream: " << rtaudio_->getErrorText() << std::endl;
        return false;
    }

    error = rtaudio_->startStream();
    if (error != RTAUDIO_NO_ERROR)
    {
        std::cerr << "Failed to start audio stream: " << rtaudio_->getErrorText() << std::endl;
        rtaudio_->closeStream();
        return false;
    }

    output_stream_parameters_ = out_parameters;
    input_stream_parameters_ = in_parameters;
    buffer_size_ = buffer_frames;

    test_tone_.SetSampleRate(sample_rate_);

    std::cout << "Audio stream started" << std::endl;

    return true;
}

void RtAudioManagerImpl::StopAudioStream()
{
    if (rtaudio_->isStreamRunning())
    {
        rtaudio_->stopStream();
    }

    if (rtaudio_->isStreamOpen())
    {
        rtaudio_->closeStream();
    }
}

bool RtAudioManagerImpl::IsAudioStreamRunning() const
{
    return rtaudio_->isStreamRunning();
}

AudioStreamInfo RtAudioManagerImpl::GetAudioStreamInfo() const
{
    AudioStreamInfo info = {0};
    if (current_input_device_id_ == -1)
    {
        return info;
    }

    auto input_device_info = rtaudio_->getDeviceInfo(current_input_device_id_);
    info.sample_rate = sample_rate_;
    info.buffer_size = buffer_size_;
    info.num_input_channels = input_device_info.inputChannels;
    info.num_output_channels = output_stream_parameters_.nChannels;
    return info;
}

void RtAudioManagerImpl::SetOutputDevice(std::string_view device_name)
{
    if (device_name == "None")
    {
        StopAudioStream();
        return;
    }

    auto devices = rtaudio_->getDeviceIds();
    for (auto device : devices)
    {
        auto info = rtaudio_->getDeviceInfo(device);
        if (info.name == device_name && device != current_output_device_id_ && info.outputChannels > 0)
        {
            assert(info.outputChannels > 0);
            current_output_device_id_ = device;
            StopAudioStream();
            StartAudioStream();
            return;
        }
    }
}

void RtAudioManagerImpl::SetInputDevice(std::string_view device_name)
{
    if (device_name == "None")
    {
        StopAudioStream();
        return;
    }

    auto devices = rtaudio_->getDeviceIds();
    for (auto device : devices)
    {
        auto info = rtaudio_->getDeviceInfo(device);
        if (info.name == device_name && device != current_input_device_id_ && info.inputChannels > 0)
        {
            assert(info.inputChannels > 0);
            current_input_device_id_ = device;
            StopAudioStream();
            StartAudioStream();
            return;
        }
    }
}

void RtAudioManagerImpl::SetAudioDriver(std::string_view driver_name)
{
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);
    for (auto api : apis)
    {
        if (RtAudio::getApiDisplayName(api) == driver_name && api != current_audio_api_)
        {
            if (IsAudioStreamRunning())
            {
                StopAudioStream();
            }

            rtaudio_ = std::make_unique<RtAudio>(api, RtAudioErrorCb);
            current_output_device_id_ = rtaudio_->getDefaultOutputDevice();
            current_input_device_id_ = rtaudio_->getDefaultInputDevice();
            current_audio_api_ = api;
            StartAudioStream();
            return;
        }
    }
}

void RtAudioManagerImpl::SelectInputChannels(uint8_t channels)
{
    if (input_selected_channels_ == channels)
    {
        return;
    }

    input_selected_channels_ = channels;
    StopAudioStream();
    StartAudioStream();
}

std::vector<std::string> RtAudioManagerImpl::GetOutputDevicesName() const
{
    std::vector<unsigned int> devices = rtaudio_->getDeviceIds();
    std::vector<std::string> device_names;

    // First device is 'none'
    device_names.emplace_back("None");

    for (unsigned int i = 0; i < devices.size(); ++i)
    {
        auto info = rtaudio_->getDeviceInfo(devices[i]);
        if (info.outputChannels > 0)
        {
            device_names.push_back(info.name);
        }
    }

    return device_names;
}

std::vector<std::string> RtAudioManagerImpl::GetInputDevicesName() const
{
    std::vector<unsigned int> devices = rtaudio_->getDeviceIds();
    std::vector<std::string> device_names;

    // First device is 'none'
    device_names.emplace_back("None");

    for (unsigned int i = 0; i < devices.size(); ++i)
    {
        auto info = rtaudio_->getDeviceInfo(devices[i]);
        if (info.inputChannels > 0)
        {
            device_names.push_back(info.name);
        }
    }

    return device_names;
}

std::vector<std::string> RtAudioManagerImpl::GetSupportedAudioDrivers() const
{
    std::vector<std::string> drivers;
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);
    for (auto api : apis)
    {
        drivers.push_back(RtAudio::getApiDisplayName(api));
    }
    return drivers;
}

std::string RtAudioManagerImpl::GetCurrentAudioDriver() const
{
    return RtAudio::getApiDisplayName(rtaudio_->getCurrentApi());
}

void RtAudioManagerImpl::PlayTestTone(bool play)
{
    play_test_tone_ = play;
}

AudioFileManager* RtAudioManagerImpl::GetAudioFileManager()
{
    return audio_file_manager_.get();
}

void RtAudioManagerImpl::Hit()
{
    queue_hit_ = true;
}

int RtAudioManagerImpl::RtAudioCbStatic(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                                        double streamTime, RtAudioStreamStatus status, void* userData)
{
    return static_cast<RtAudioManagerImpl*>(userData)->RtAudioCbImpl(outputBuffer, inputBuffer, nBufferFrames,
                                                                     streamTime, status);
}

int RtAudioManagerImpl::RtAudioCbImpl(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                                      double streamTime, RtAudioStreamStatus status)
{
    if (status & RTAUDIO_INPUT_OVERFLOW)
    {
        std::cerr << "Stream overflow detected!" << std::endl;
    }
    if (status & RTAUDIO_OUTPUT_UNDERFLOW)
    {
        std::cerr << "Stream underflow detected!" << std::endl;
    }

    float* output = static_cast<float*>(outputBuffer);
    float test_tone = 0.f;
    float* input = static_cast<float*>(inputBuffer);

    if (output)
    {
        memset(output, 0, nBufferFrames * output_stream_parameters_.nChannels * sizeof(float));
        audio_file_manager_->ProcessBlock(output, nBufferFrames, output_stream_parameters_.nChannels);
        // Just write silence for now
        for (auto i = 0; i < nBufferFrames; i++)
        {
            float tick = 0.f;
            if (play_test_tone_)
            {
                test_tone = test_tone_.Tick();
            }

            for (auto j = 0; j < output_stream_parameters_.nChannels; j++)
            {
                output[(i * output_stream_parameters_.nChannels) + j] += test_tone + tick;
            }
        }
    }

    return 0;
}