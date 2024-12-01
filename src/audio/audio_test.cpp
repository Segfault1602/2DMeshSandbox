#include <iostream>

#include "audio.h"

int main()
{
    auto audio_manager = AudioManager::CreateAudioManager();
    if (!audio_manager)
    {
        std::cerr << "Failed to create audio manager" << std::endl;
        return 1;
    }

    return 0;
}