#include "audio_gui.h"

#include "audio.h"
#include "imgui.h"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

void draw_audio_device_gui(AudioManager* audio_manager)
{
    assert(audio_manager != nullptr);

    static std::vector<std::string> supported_audio_drivers = audio_manager->GetSupportedAudioDrivers();
    static std::vector<std::string> output_devices = audio_manager->GetOutputDevicesName();

    // Audio Drivers Combo
    ImGui::Text("Audio Drivers ");
    ImGui::SameLine();
    static int selected_audio_driver = 0;
    if (ImGui::BeginCombo("##Audio Drivers", audio_manager->GetCurrentAudioDriver().c_str()))
    {
        for (int i = 0; i < supported_audio_drivers.size(); i++)
        {
            bool is_selected = (selected_audio_driver == i);
            if (ImGui::Selectable(supported_audio_drivers[i].c_str(), is_selected))
            {
                selected_audio_driver = i;
                std::cout << "Selected Audio Driver: " << supported_audio_drivers[i] << std::endl;
                audio_manager->SetAudioDriver(supported_audio_drivers[i]);
                // Refresh audio devices
                output_devices = audio_manager->GetOutputDevicesName();
            }

            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Output Devices Combo
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Output Devices");
    ImGui::SameLine();
    static int selected_output_device = 0;
    if (ImGui::BeginCombo("##Output Devices", output_devices[selected_output_device].c_str()))
    {
        for (int i = 0; i < output_devices.size(); i++)
        {
            bool is_selected = (selected_output_device == i);
            if (ImGui::Selectable(output_devices[i].c_str(), is_selected))
            {
                selected_output_device = i;
                std::cout << "Selected Output Device: " << output_devices[i] << std::endl;
                audio_manager->SetOutputDevice(output_devices[i]);
            }

            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Text("Stream Status: ");
    ImGui::SameLine();
    if (audio_manager->IsAudioStreamRunning())
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Running");
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Stopped");
    }

    auto audio_stream_info = audio_manager->GetAudioStreamInfo();
    ImGui::Text("Sample Rate: %d", audio_stream_info.sample_rate);
    ImGui::Text("Buffer Size: %d", audio_stream_info.buffer_size);
    ImGui::Text("Num Output Channels: %d", audio_stream_info.num_output_channels);

    static bool play_test_tone = false;
    if (ImGui::Checkbox("Play Test Tone", &play_test_tone))
    {
        audio_manager->PlayTestTone(play_test_tone);
    }
}
