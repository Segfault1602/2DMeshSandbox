#include "audio.h"

#include "rtaudio_impl.h"

std::unique_ptr<AudioManager> AudioManager::CreateAudioManager()
{
    return std::make_unique<RtAudioManagerImpl>();
}