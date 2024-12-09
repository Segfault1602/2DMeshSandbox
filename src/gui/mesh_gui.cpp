#include "mesh_gui.h"

#include "Stk.h"
#include "audio.h"
#include "audio_file_manager.h"
#include "circular_mesh_manager.h"
#include "fft_utils.h"
#include "glm/detail/qualifier.hpp"
#include "imgui.h"
#include "implot.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <iostream>
#include <memory>
#include <sndfile.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
std::atomic<bool> g_render_complete{false};

bool g_waveform_updated = false;

float g_render_time = 0;

// Constants
int kSR = 48000;
const char kOutputFile[] = "mesh.wav";

constexpr int kN_FFT = 2048; // FFT size
constexpr int kOverlap = 2016;

std::vector<float> g_fft_frq(kN_FFT, 0); // FFT output frequencies
float g_min_dB = -50;
float g_max_dB = 50;
std::vector<float> g_spectrogram(kN_FFT, -50);
std::vector<float> g_spectrogram_2(kN_FFT, -50);

std::vector<float> g_waveform;
std::vector<float> g_waveform_2;
float g_waveform_duration_second = 0.f;

bool g_update_spectrum = false;

SpectrogramInfo g_spectrogram_info{
    .fft_size = kN_FFT,
    .num_bins = 0,
    .overlap = kOverlap,
    .samplerate = kSR,
    .num_freqs = 0,
};

float render_time_sec = 1.f;

std::unique_ptr<CircularMeshManager> g_mesh_manager;

const std::vector<const char*> kCircularModes = {"(0,1)", "(1,1)", "(2,1)", "(0,2)", "(3,1)", "(1,2)",
                                                 "(4,1)", "(2,2)", "(0,3)", "(5,1)", "(3,2)", "(6,1)"};

const std::vector<float> kCircularRatios = {1.f,    1.594f, 2.136f, 2.296f, 2.653f, 2.918f,
                                            3.156f, 3.501f, 3.600f, 3.652f, 4.060f, 4.154f};

void update_waveform()
{
    SF_INFO sf_info{0};
    SNDFILE* file = sf_open(kOutputFile, SFM_READ, &sf_info);

    assert(sf_info.channels == 1);

    // Backup old waveform
    g_waveform_2 = std::move(g_waveform);

    sf_count_t n_frames = sf_info.frames;
    g_waveform.resize(n_frames);
    sf_readf_float(file, g_waveform.data(), n_frames);

    g_waveform_duration_second = static_cast<float>(n_frames) / sf_info.samplerate;

    sf_close(file);
    g_waveform_updated = true;
}

void update_spectrogram(float time)
{
    SF_INFO sf_info{0};
    SNDFILE* file = sf_open(kOutputFile, SFM_READ, &sf_info);

    g_spectrogram_info.fft_size = kN_FFT;
    g_spectrogram_info.samplerate = sf_info.samplerate;
    g_spectrogram_info.overlap = kOverlap;
    g_spectrogram_info.fft_hop_size = g_spectrogram_info.fft_size - g_spectrogram_info.overlap;
    g_spectrogram_info.num_freqs = g_spectrogram_info.fft_size / 2;
    g_spectrogram_info.num_bins = g_spectrogram_info.samplerate * 1.f / g_spectrogram_info.fft_hop_size - 1;

    assert(sf_info.channels == 1);

    sf_count_t n_frames = sf_info.frames;

    size_t audio_buffer_size = n_frames + g_spectrogram_info.fft_size;
    std::vector<float> m_samples(audio_buffer_size);
    sf_readf_float(file, m_samples.data(), n_frames);

    // backup old spectrogram
    g_spectrogram_2 = std::move(g_spectrogram);

    const size_t spectrogram_size = g_spectrogram_info.num_freqs * g_spectrogram_info.num_bins;
    g_spectrogram.resize(spectrogram_size);

    std::vector<float> window(g_spectrogram_info.fft_size, 0);
    GetWindow(FFTWindowType::Hann, window.data(), g_spectrogram_info.fft_size);

    int idx = 0;
    std::vector<float> fft_in(g_spectrogram_info.fft_size);
    std::vector<float> m_fft_out(g_spectrogram_info.fft_size);
    for (int b = 0; b < g_spectrogram_info.num_bins; ++b)
    {
        if (idx + g_spectrogram_info.fft_size > audio_buffer_size)
        {
            std::cout << "End of buffer, idx: " << idx << std::endl;
            break;
        }

        for (int i = 0; i < g_spectrogram_info.fft_size; ++i)
        {
            fft_in[i] = m_samples[idx + i] * window[i];
        }

        fft(fft_in.data(), m_fft_out.data(), g_spectrogram_info.fft_size);
        fft_abs(m_fft_out.data(), m_fft_out.data(), g_spectrogram_info.fft_size);
        for (int f = 0; f < g_spectrogram_info.num_freqs; ++f)
        {
            g_spectrogram[(f * g_spectrogram_info.num_bins) + b] =
                20 * log10f(std::abs(m_fft_out[g_spectrogram_info.num_freqs - 1 - f]));
        }
        idx += g_spectrogram_info.fft_hop_size;
    }

    g_fft_frq.resize(g_spectrogram_info.num_freqs);
    for (int f = 0; f < g_spectrogram_info.num_freqs; ++f)
    {
        g_fft_frq[f] = f * (float)g_spectrogram_info.samplerate / (float)g_spectrogram_info.fft_size;
    }

    sf_close(file);
}

void InitWindow()
{
    stk::Stk::setSampleRate(kSR);
    ImPlot::GetStyle().Colormap = ImPlotColormap_Plasma;
    ImVec4 colormap[] = {
        ImVec4(0.f, 143.f / 255.f, 1.f, 1.0f), ImVec4(0.f, 0.f, 158.f / 255.f, 1.f),  ImVec4(0.f, 0.f, 0.f, 1.0f),
        ImVec4(1.f, 70.f / 255.f, 0.f, 1.0f),  ImVec4(1.f, 156.f / 255.f, 0.f, 1.0f),
    };
    ImPlot::AddColormap("Simulation", colormap, 5, false);
}

} // namespace

void init_mesh_gui()
{
    InitWindow();
    g_mesh_manager = std::make_unique<CircularMeshManager>();

    // Init spectrogram data
    g_spectrogram_info.fft_size = kN_FFT;
    g_spectrogram_info.samplerate = kSR;
    g_spectrogram_info.overlap = kOverlap;
    g_spectrogram_info.fft_hop_size = g_spectrogram_info.fft_size - g_spectrogram_info.overlap;
    g_spectrogram_info.num_freqs = g_spectrogram_info.fft_size / 2;
    g_spectrogram_info.num_bins = g_spectrogram_info.samplerate * 1.f / g_spectrogram_info.fft_hop_size - 1;

    const size_t spectrogram_size = g_spectrogram_info.num_freqs * g_spectrogram_info.num_bins;
    g_spectrogram.resize(spectrogram_size);
    g_spectrogram_2.resize(spectrogram_size);

    g_fft_frq.resize(g_spectrogram_info.num_freqs);
    for (int f = 0; f < g_spectrogram_info.num_freqs; ++f)
    {
        g_fft_frq[f] = f * (float)g_spectrogram_info.samplerate / (float)g_spectrogram_info.fft_size;
    }
}

void draw_audio_player(AudioManager* audio_manager)
{
    static bool autoplay = false;
    ImGui::Checkbox("Autoplay", &autoplay);

    if (g_render_complete)
    {
        // update spectrogram data
        update_waveform();
        update_spectrogram(0.f);
        g_update_spectrum = true;
        if (autoplay)
        {
            if (audio_manager->GetAudioFileManager()->GetState() != AudioFileManager::AudioPlayerState::kPlaying)
            {
                if (audio_manager->GetAudioFileManager()->OpenAudioFile(kOutputFile))
                {
                    audio_manager->GetAudioFileManager()->Play();
                }
            }
        }
        g_render_time = g_mesh_manager->get_render_runtime();
        g_render_complete = false;
    }

    bool is_playing = audio_manager->GetAudioFileManager()->GetState() == AudioFileManager::AudioPlayerState::kPlaying;
    if (ImGui::Button(is_playing ? "Pause" : "Play"))
    {
        if (!is_playing)
        {
            if (audio_manager->GetAudioFileManager()->OpenAudioFile(kOutputFile))
            {
                audio_manager->GetAudioFileManager()->Play();
            }
        }
        else
        {
            audio_manager->GetAudioFileManager()->Pause();
        }
    }

    std::string file_name = audio_manager->GetAudioFileManager()->GetOpenFileName();
    if (!file_name.empty())
    {
        ImGui::SameLine();
        ImGui::Text("Playing: %s", file_name.c_str());
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        audio_manager->GetAudioFileManager()->Stop();
    }

    ImGui::SeparatorText("Render");
    ImGui::BeginDisabled(g_mesh_manager->is_rendering());

    if (ImGui::Button("Render"))
    {
        g_mesh_manager->render_async(render_time_sec, []() { g_render_complete = true; });
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::ProgressBar(g_mesh_manager->get_progress());

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Render Time (s):");
    ImGui::SameLine();
    ImGui::PushItemWidth(100);
    ImGui::SliderFloat("##rendertime", &render_time_sec, 1.f, 10.f);
    ImGui::PopItemWidth();

    ImGui::Text("Render Time: %0.2f ms", g_render_time);

    // get simulation time for one second
    float normalized_time = g_render_time / render_time_sec;
    ImGui::Text("Took %0.2f ms to render 1 seconds", normalized_time);
}

void draw_mesh_config(bool& reset_camera)
{
    g_mesh_manager->draw_config_menu(reset_camera);
}

void draw_experimental_config_menu()
{
    g_mesh_manager->draw_experimental_config_menu();
}

void render_gl_mesh(glm::mat4 mvp)
{
    g_mesh_manager->render_gl_mesh(mvp);
}

void draw_spectrogram()
{
    static bool show_previous_spectrogram = false;
    const double tmin = 0.f;
    const double tmax = 1;
    const float h = ImGui::GetWindowSize()[1];
    const float w = ImGui::GetWindowSize()[0];
    if (ImPlot::BeginPlot("##Spectrogram", ImVec2(0.9 * w, 0.9 * h), ImPlotFlags_NoMouseText))
    {
        ImPlot::SetupAxisLimits(ImAxis_X1, tmin, tmax, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, g_spectrogram_info.samplerate / 2000, ImGuiCond_Always);
        ImPlot::SetupAxisFormat(ImAxis_Y1, "%g kHz");

        if (!show_previous_spectrogram)
        {
            ImPlot::PlotHeatmap("##Heat", g_spectrogram.data(), g_spectrogram_info.num_freqs,
                                g_spectrogram_info.num_bins, g_min_dB, g_max_dB, nullptr, {tmin, 0},
                                {tmax, g_fft_frq[g_spectrogram_info.num_freqs - 1] / 1000});
        }
        else
        {
            ImPlot::PlotHeatmap("##Heat", g_spectrogram_2.data(), g_spectrogram_info.num_freqs,
                                g_spectrogram_info.num_bins, g_min_dB, g_max_dB, nullptr, {tmin, 0},
                                {tmax, g_fft_frq[g_spectrogram_info.num_freqs - 1] / 1000});
        }
        ImPlot::EndPlot();
    }

    ImGui::SameLine();
    ImPlot::ColormapScale("##Scale", g_min_dB, g_max_dB, {-1, 0.9f * h}, "%g dB");
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        ImGui::OpenPopup("Range");
    }
    if (ImGui::BeginPopup("Range"))
    {
        ImGui::SliderFloat("Max", &g_max_dB, g_min_dB, 100);
        ImGui::SliderFloat("Min", &g_min_dB, -100, g_max_dB);
        ImGui::EndPopup();
    }
    ImGui::Checkbox("Show Previous", &show_previous_spectrogram);
}

void draw_waveform()
{
    const float h = ImGui::GetWindowSize()[1];
    static bool show_previous_waveform = false;
    if (ImPlot::BeginPlot("##Waveform", ImVec2(-1, 0.9f * h), ImPlotFlags_NoMouseText))
    {
        static std::vector<float> x(g_waveform.size());

        ImPlot::SetupAxes("Time", "Amplitude");
        if (g_waveform_updated)
        {
            x.resize(g_waveform.size());
            for (size_t i = 0; i < g_waveform.size(); i++)
            {
                x[i] = static_cast<float>(i) / g_spectrogram_info.samplerate;
            }
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, g_waveform_duration_second, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 1, ImGuiCond_Always);
            g_waveform_updated = false;
        }
        ImPlot::PushStyleColor(ImPlotCol_PlotBg, IM_COL32(75, 75, 75, 255));
        ImPlot::PlotLine("##Wave", x.data(), g_waveform.data(), g_waveform.size());
        ImPlot::PopStyleColor();

        if (show_previous_waveform)
        {
            ImPlot::PlotLine("##Wave2", x.data(), g_waveform_2.data(), g_waveform_2.size());
        }

        ImPlot::EndPlot();
    }

    ImGui::Checkbox("Show Previous", &show_previous_waveform);
}

void draw_mesh_shape()
{
    g_mesh_manager->plot_mesh();
}

void draw_spectrum()
{
    static uint32_t window_size[] = {512, 1024, 2048, 4096};
    static std::vector<const char*> window_name = {"Rectangular", "Hamming", "Hann", "Blackman"};
    static int selected_win_size = -1;
    static int selected_win_type = 2;
    static std::vector<float> window;
    static std::vector<float> spectrum;
    static std::vector<float> spectrum_2;
    static std::vector<float> freq;
    static bool show_theoretical_modes = false;

    bool config_changed = false;

    // Init window size on first run
    if (selected_win_size == -1)
    {
        selected_win_size = 2;
        window.resize(window_size[selected_win_size], 0);
        spectrum.resize(window_size[selected_win_size], 0);
        spectrum_2.resize(window_size[selected_win_size], 0);
        freq.resize(window_size[selected_win_size], 0);
        GetWindow(static_cast<FFTWindowType>(selected_win_type), window.data(), window_size[selected_win_size]);
    }

    ImGui::Text("Window Size:");
    ImGui::SameLine();
    if (ImGui::BeginCombo("##spectrum_win", std::format("{}", window_size[selected_win_size]).c_str(),
                          ImGuiComboFlags_WidthFitPreview))
    {
        for (int i = 0; i < 4; i++)
        {
            bool is_selected = (selected_win_size == i);
            if (ImGui::Selectable(std::format("{}", window_size[i]).c_str(), is_selected))
            {
                selected_win_size = i;
                config_changed |= true;
            }

            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::Text("Window Type:");
    ImGui::SameLine();
    if (ImGui::BeginCombo("##spectrum_win_type", window_name[selected_win_type], ImGuiComboFlags_WidthFitPreview))
    {
        for (int i = 0; i < 4; i++)
        {
            bool is_selected = (selected_win_type == i);
            if (ImGui::Selectable(window_name[i], is_selected))
            {
                selected_win_type = i;
                config_changed |= true;
            }

            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (config_changed)
    {
        window.resize(window_size[selected_win_size]);
        spectrum.resize(window_size[selected_win_size]);
        spectrum_2.resize(window_size[selected_win_size]);
        freq.resize(window_size[selected_win_size]);
        GetWindow(static_cast<FFTWindowType>(selected_win_type), window.data(), window_size[selected_win_size]);
    }

    static float offset = 0.f;
    ImGui::Text("Offset:");
    ImGui::SameLine();
    config_changed |= ImGui::SliderFloat("##Offset", &offset, 0.f, 1.f);

    if (g_waveform.empty())
    {
        return;
    }

    if (config_changed || g_update_spectrum)
    {
        g_update_spectrum = false;

        size_t start_index = static_cast<size_t>(offset * g_waveform.size());
        start_index = std::min(start_index, g_waveform.size() - window_size[selected_win_size]);
        for (size_t i = 0; i < spectrum.size(); i++)
        {
            spectrum[i] = g_waveform[start_index + i] * window[i];

            if (!g_waveform_2.empty())
            {
                spectrum_2[i] = g_waveform_2[start_index + i] * window[i];
            }
        }

        fft(spectrum.data(), spectrum.data(), spectrum.size());
        fft_abs(spectrum.data(), spectrum.data(), spectrum.size());

        fft(spectrum_2.data(), spectrum_2.data(), spectrum_2.size());
        fft_abs(spectrum_2.data(), spectrum_2.data(), spectrum_2.size());

        for (size_t i = 0; i < spectrum.size(); i++)
        {
            spectrum[i] = 20 * log10(std::abs(spectrum[i]));
            spectrum_2[i] = 20 * log10(std::abs(spectrum_2[i]));
        }

        for (size_t i = 0; i < freq.size(); i++)
        {
            freq[i] = i * (float)g_spectrogram_info.samplerate / (float)window_size[selected_win_size];
        }
    }
    static bool show_previous_spectrum = false;
    ImGui::Checkbox("Show Previous", &show_previous_spectrum);
    ImGui::SameLine();
    ImGui::Checkbox("Show Theoretical Modes", &show_theoretical_modes);

    const float h = ImGui::GetWindowSize()[1];
    ImPlot::PushStyleColor(ImPlotCol_PlotBg, IM_COL32(175, 175, 175, 255));
    ImPlot::PushStyleColor(ImPlotCol_AxisGrid, IM_COL32(5, 5, 5, 255));
    if (ImPlot::BeginPlot("##Spectrum", ImVec2(-1, 0.8f * h), ImPlotFlags_NoLegend))
    {
        ImPlot::SetupAxes("Freq", "Db");
        // ImPlot::SetupAxis(ImAxis_Y1, NULL, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("Spectrum", freq.data(), spectrum.data(), spectrum.size() / 2);

        if (show_previous_spectrum)
        {
            ImPlot::PushStyleColor(ImPlotCol_Line, IM_COL32(120, 35, 0, 255));

            ImPlot::PlotLine("Spectrum2", freq.data(), spectrum_2.data(), spectrum_2.size() / 2);
            ImPlot::PopStyleColor();
        }

        if (show_theoretical_modes)
        {
            for (size_t i = 0; i < kCircularModes.size(); ++i)
            {
                ImPlot::PushStyleColor(ImPlotCol_Line, IM_COL32(75, 35, 0, 255));
                float freq = kCircularRatios[i] * g_mesh_manager->current_fundamental_frequency();
                ImPlot::PlotInfLines(std::format("Mode {}", i).c_str(), &freq, 1);
                ImPlot::PopStyleColor();
            }
        }

        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor(2);
}