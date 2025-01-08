#include "circular_mesh_manager.h"

#include "gaussian.h"
#include "junction.h"
#include "line.h"
#include "listener.h"
#include "mat2d.h"
#include "rectilinear_mesh.h"
#include "rimguide.h"
#include "rimguide_utils.h"
#include "trimesh.h"
#include "wave_math.h"

#include <Generator.h>
#include <PoleZero.h>
#include <SineWave.h>

#include <glm/glm.hpp>
#include <imgui.h>
#include <implot.h>
#include <sndfile.h>

#include <imfilebrowser.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <numbers>
#include <thread>
#include <utility>
#include <vector>

namespace
{
float rand_float()
{
    float rn = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    rn = 2.f * rn - 1.f;
    return rn;
}

const std::vector<const char*> kCircularModes = {"(0,1)", "(1,1)", "(2,1)", "(0,2)", "(3,1)", "(1,2)",
                                                 "(4,1)", "(2,2)", "(0,3)", "(5,1)", "(3,2)", "(6,1)"};

const std::vector<float> kCircularRatios = {1.f,    1.594f, 2.136f, 2.296f, 2.653f, 2.918f,
                                            3.156f, 3.501f, 3.600f, 3.652f, 4.060f, 4.154f};

} // namespace

CircularMeshManager::CircularMeshManager()
{
    update_mesh_object();
}

CircularMeshManager::~CircularMeshManager() = default;

void CircularMeshManager::compute_parameters()
{
    wave_speed_ = get_wave_speed(tension_, density_);
    sample_distance_ = get_sample_distance(wave_speed_, sample_rate_);
    fundamental_frequency_ = get_fundamental_frequency(radius_, wave_speed_, sample_rate_);

    if (!use_custom_cutoff_)
    {
        cutoff_freq_ = fundamental_frequency_;
        cutoff_freq_hz_ = (fundamental_frequency_ * sample_rate_) / (2 * M_PI);
    }
    else
    {
        cutoff_freq_ = cutoff_freq_hz_ * 2 * M_PI / sample_rate_;
    }

    friction_coeff_ = get_friction_coeff(radius_, wave_speed_, decays_, cutoff_freq_);
    friction_delay_ = get_friction_delay(
        friction_coeff_, fundamental_frequency_); // Keep using the fundamental frequency for the delay calculation?
    max_radius_ = get_max_radius(radius_, friction_delay_, sample_distance_, minimum_rimguide_delay_);

    float vertical_scaler = 1;
    switch (mesh_type_)
    {
    case MeshType::TRIANGULAR_MESH:
        vertical_scaler = 2.f / std::numbers::sqrt3_v<float>;
        break;
    case MeshType::RECTILINEAR_MESH:
        vertical_scaler = 1.f;
        break;
    default:
        break;
    }

    auto grid_size = get_grid_size(max_radius_, sample_distance_, vertical_scaler);
    grid_size_ = {static_cast<uint32_t>(grid_size[0]), static_cast<uint32_t>(grid_size[1])};

    // Put fundamental frequency in Hz
    fundamental_frequency_ = fundamental_frequency_ * sample_rate_ / (2 * M_PI);
}

void CircularMeshManager::update_mesh_object()
{
    compute_parameters();
    is_simulation_running_ = false;

    switch (mesh_type_)
    {
    case MeshType::TRIANGULAR_MESH:
        mesh_ = std::make_unique<TriMesh>(grid_size_.x, grid_size_.y, sample_distance_);
        break;
    case MeshType::RECTILINEAR_MESH:
        mesh_ = std::make_unique<RectilinearMesh>(grid_size_.x, grid_size_.y, sample_distance_);
        break;
    default:
        break;
    }

    RimguideInfo info = get_rimguide_info();

    auto mask = mesh_->get_mask_for_radius(max_radius_);
    mesh_->init(mask);
    mesh_->init_boundary(info);

    mesh_->set_input(input_pos_.x, input_pos_.y);
    mesh_->set_output(output_pos_.x, output_pos_.y);

    if (clamp_center_)
    {
        mesh_->clamp_center_with_rimguide();
    }

    update_gl_mesh();
    line_->set_color({1.f, 1.f, 1.f});
    circle_line_->set_color({1.f, 0.2f, 0.2f});
}

void CircularMeshManager::draw_config_menu(bool& reset_camera)
{
    ImGui::BeginDisabled(is_rendering_);
    bool config_changed = false;
    ImGui::SeparatorText("Mesh Config");

    constexpr float kColOffset = 130;

    ImGui::Text("Mesh Type:");
    ImGui::SameLine(kColOffset);
    config_changed |= ImGui::RadioButton("Triangular", reinterpret_cast<int*>(&mesh_type_),
                                         static_cast<int>(MeshType::TRIANGULAR_MESH));
    ImGui::SameLine();
    config_changed |= ImGui::RadioButton("Rectilinear", reinterpret_cast<int*>(&mesh_type_),
                                         static_cast<int>(MeshType::RECTILINEAR_MESH));

    ImGui::Text("Sample Rate:");
    ImGui::SameLine(kColOffset);
    config_changed |= ImGui::InputInt("##sample_rate", &sample_rate_);
    if (sample_rate_ > 48000)
    {
        sample_rate_ = 48000;
    }
    else if (sample_rate_ < 8000)
    {
        sample_rate_ = 8000;
    }

    ImGui::Text("Diameter (cm):");
    ImGui::SameLine(kColOffset);
    static int diameter_cm = radius_ * 2 * 100;
    config_changed |= ImGui::SliderInt("##radius", &diameter_cm, 1, 100);
    radius_ = (diameter_cm / 2.f) / 100.f;

    ImGui::Text("Decays:");
    ImGui::SameLine(kColOffset);
    config_changed |= ImGui::SliderFloat("##decays", &decays_, 0, 100);

    ImGui::Checkbox("##cutoff_checkbox", &use_custom_cutoff_);
    ImGui::BeginDisabled(!use_custom_cutoff_);
    ImGui::SameLine();
    ImGui::Text("Cutoff:");
    ImGui::SameLine(kColOffset);
    config_changed |= ImGui::SliderFloat("##cutoff", &cutoff_freq_hz_, 20, 2000);
    ImGui::EndDisabled();

    ImGui::Text("Density:");
    ImGui::SameLine(kColOffset);
    config_changed |= ImGui::SliderFloat("##density", &density_, 0.1f, 1.f);

    ImGui::Text("Tension:");
    ImGui::SameLine(kColOffset);
    config_changed |= ImGui::SliderInt("##tension", &tension_, 1000, 10000);

    ImGui::Text("Min. Rim Delay:");
    ImGui::SameLine(kColOffset);
    config_changed |= ImGui::SliderFloat("##min_rimguide_delay", &minimum_rimguide_delay_, 1.5f, 20.f);

    ImGui::Text("Input Pos:");
    ImGui::SameLine(kColOffset);
    config_changed |= ImGui::SliderFloat2("##input_pos", &input_pos_.x, 0.f, 1.f);

    ImGui::Text("Clamped bound.:");
    ImGui::SameLine(kColOffset);
    config_changed |= ImGui::Checkbox("##solid_boundary", &is_solid_boundary_);

    if (config_changed)
    {
        update_mesh_object();
    }

    assert(mesh_ != nullptr);

    ImGui::SeparatorText("Excitation");

    ImGui::Text("Excitation Type:");
    ImGui::SameLine(kColOffset);
    static int excitation_type = 0;
    std::vector<const char*> excitation_names = {"Raised Cosine", "Dirac", "File"};
    ImGui::Combo("##excitation_type", &excitation_type, excitation_names.data(), excitation_names.size());
    excitation_type_ = static_cast<ExcitationType>(excitation_type);

    if (excitation_type_ == ExcitationType::RAISE_COSINE)
    {
        ImGui::Text("Frequency (Hz):");
        ImGui::SameLine(kColOffset);
        ImGui::SliderFloat("##excitation_freq", &excitation_frequency_, 10.f, 1000.f);
    }
    else if (excitation_type_ == ExcitationType::FILE)
    {
        static ImGui::FileBrowser file_dialog;
        file_dialog.SetTypeFilters({".wav"});

        ImGui::Text("File:");
        ImGui::SameLine(kColOffset);
        if (ImGui::Button("Load"))
        {
            file_dialog.Open();
        }
        ImGui::SameLine();
        ImGui::Text("%s", excitation_filename_.c_str());

        file_dialog.Display();
        if (file_dialog.HasSelected())
        {
            excitation_filename_ = file_dialog.GetSelected().string();
            file_dialog.ClearSelected();
        }
    }

    ImGui::Text("Amplitude:");
    ImGui::SameLine(kColOffset);
    ImGui::SliderFloat("##excitation_amp", &excitation_amplitude_, 0.f, 20.f);

    ImGui::SeparatorText("Listener config");
    std::vector<const char*> listener_types = {"All", "Boundary", "Point"};
    ImGui::Text("Listener Type:");
    ImGui::SameLine(kColOffset);
    ImGui::Combo("##listener_type", reinterpret_cast<int*>(&listener_type_), listener_types.data(),
                 listener_types.size());

    if (listener_type_ == ListenerType::POINT)
    {
        ImGui::Text("Listener Pos:");
        ImGui::SameLine(kColOffset);
        config_changed |= ImGui::SliderFloat2("##output_pos", &output_pos_.x, 0.f, 1.f);
    }

    ImGui::Checkbox("Use DC Blocker", &use_dc_blocker_);
    if (use_dc_blocker_)
    {
        ImGui::SliderFloat("Alpha", &dc_blocker_alpha_, 0.85f, 0.999f);
    }

    ImGui::EndDisabled();

    ImGui::SeparatorText("Derived Parameters");
    constexpr float kColOffset2 = 200;
    ImGui::Text("Wave Speed:");
    ImGui::SameLine(kColOffset2);
    ImGui::Text("%f m/s", wave_speed_);

    ImGui::Text("Sample Distance:");
    ImGui::SameLine(kColOffset2);
    ImGui::Text("%f m", sample_distance_);

    ImGui::Text("Fundamental Frequency:");
    ImGui::SameLine(kColOffset2);
    ImGui::Text("%f Hz", fundamental_frequency_);

    ImGui::Text("Friction Coefficient:");
    ImGui::SameLine(kColOffset2);
    ImGui::Text("%f", friction_coeff_);

    ImGui::Text("Friction Delay:");
    ImGui::SameLine(kColOffset2);
    ImGui::Text("%f s", friction_delay_);

    ImGui::Text("Max Radius:");
    ImGui::SameLine(kColOffset2);
    ImGui::Text("%f m", max_radius_);

    ImGui::Text("Grid Size:");
    ImGui::SameLine(kColOffset2);
    ImGui::Text("%d x %d", grid_size_.x, grid_size_.y);

    ImGui::Text("Junctions count:");
    ImGui::SameLine(kColOffset2);
    ImGui::Text("%zu", mesh_->get_junction_count());

    ImGui::Text("Rimguides count:");
    ImGui::SameLine(kColOffset2);
    ImGui::Text("%zu", mesh_->get_rimguide_count());

    ImGui::SeparatorText("Theoretical Modes");
    if (ImGui::BeginTable("modes", 2))
    {
        ImGui::TableSetupColumn("Mode");
        ImGui::TableSetupColumn("Frequency (Hz)");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < kCircularModes.size(); ++i)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", kCircularModes[i]);
            ImGui::TableNextColumn();
            ImGui::Text("%f", kCircularRatios[i] * fundamental_frequency_);
        }

        ImGui::EndTable();
    }

    ImGui::SeparatorText("Simulation");
    draw_simulation_menu(reset_camera);
}

void CircularMeshManager::draw_experimental_config_menu()
{
    ImGui::BeginDisabled(is_rendering_);
    bool config_changed = false;
    ImGui::SeparatorText("Experimental Mesh Config");

    constexpr float kIndent = 15;
    constexpr float kColOffset = 150;

    ImGui::Checkbox("Time Varying Allpass", &use_time_varying_allpass_);

    ImGui::BeginDisabled(!use_time_varying_allpass_);
    ImGui::Indent(kIndent);
    ImGui::Text("Mod Frequency:");
    ImGui::SameLine(kColOffset);
    config_changed |= ImGui::SliderFloat("##mod_freq", &allpass_mod_freq_, 0.f, 50.f);

    ImGui::Text("Amplitude:");
    ImGui::SameLine(kColOffset);
    config_changed |= ImGui::SliderFloat("##excitation_amp", &excitation_amplitude_, 0.f, 10.f);

    std::vector<const char*> types = {"Sync", "Phase offset", "Random", "Random Freq and Amp"};
    ImGui::Text("Modulation Type:");
    ImGui::SameLine(kColOffset);
    config_changed |= ImGui::Combo("##mod_type", reinterpret_cast<int*>(&allpass_type_), types.data(), types.size());

    if (allpass_type_ == TimeVaryingAllpassType::RANDOM || allpass_type_ == TimeVaryingAllpassType::RANDOM_FREQ_AND_AMP)
    {
        ImGui::Text("Random Freq:");
        ImGui::SameLine(kColOffset);
        config_changed |= ImGui::SliderFloat("##rand_freq", &allpass_random_freq_, 0.f, 1.f);
    }
    if (allpass_type_ == TimeVaryingAllpassType::RANDOM_FREQ_AND_AMP)
    {
        ImGui::Text("Random Amp:");
        ImGui::SameLine(kColOffset);
        config_changed |= ImGui::SliderFloat("##rand_amp", &allpass_random_mod_amp_, 0.f, 1.f);
    }
    if (allpass_type_ == TimeVaryingAllpassType::PHASE_OFFSET)
    {
        ImGui::Text("Phase Offset:");
        ImGui::SameLine(kColOffset);
        config_changed |= ImGui::SliderFloat("##phase_offset", &allpass_phase_offset_, 0.f, 1.f);
    }

    ImGui::Unindent(kIndent);
    ImGui::EndDisabled();

    config_changed |= ImGui::Checkbox("Clamp center", &clamp_center_);

    config_changed |= ImGui::Checkbox("Automatic pitch bend", &use_automatic_pitch_bend_);

    if (use_automatic_pitch_bend_)
    {
        config_changed |= ImGui::SliderFloat("Pitch bend amount", &pitch_bend_amount_, -1.f, 1.f);
    }

    config_changed |= ImGui::Checkbox("Square law nonlinearity", &use_square_law_nonlinearity_);
    if (use_square_law_nonlinearity_)
    {
        config_changed |= ImGui::SliderFloat("Non linear factor", &nonlinear_factor_, 0.f, 1.f);
    }

    config_changed |= ImGui::Checkbox("Nonlinear allpass", &use_nonlinear_allpass_);
    if (use_nonlinear_allpass_)
    {
        config_changed |= ImGui::SliderFloat2("Coeff 1", nonlinear_allpass_coeffs_, -1.f, 1.f);
    }

    config_changed |= ImGui::Checkbox("Extra diffusion filters", &use_extra_diffusion_filters_);

    if (use_extra_diffusion_filters_)
    {
        ImGui::Indent(kIndent);
        if (ImGui::Button("-"))
        {
            if (diffusion_filter_count_ > 0)
            {
                diffusion_filter_count_--;
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("+"))
        {
            diffusion_filter_count_++;
        }

        diffusion_filter_coeffs_.resize(diffusion_filter_count_);

        for (size_t i = 0; i < diffusion_filter_count_; i++)
        {
            ImGui::PushID(i);
            ImGui::Text("Filter %zu", i);
            ImGui::SameLine(kColOffset);
            ImGui::SliderFloat("##filter", &diffusion_filter_coeffs_[i], -1.f, 1.f);
            ImGui::PopID();
        }

        ImGui::Unindent(kIndent);
    }

    ImGui::EndDisabled();

    if (config_changed)
    {
        update_mesh_object();
    }
}

void CircularMeshManager::draw_simulation_menu(bool& reset_camera)
{
    if (!mesh_)
    {
        return;
    }

    static std::vector<float> impulse = raised_cosine(100, sample_rate_);
    static size_t impulse_idx = 0;

    if (ImGui::Button("Reset"))
    {
        is_simulation_running_ = false;
        impulse_idx = 0;
        if (excitation_type_ == ExcitationType::DIRAC)
        {
            impulse.clear();
            impulse.push_back(excitation_amplitude_);
        }
        else if (excitation_type_ == ExcitationType::RAISE_COSINE)
        {
            impulse = raised_cosine(excitation_frequency_, sample_rate_);

            for (auto& i : impulse)
            {
                i *= excitation_amplitude_;
            }
        }

        mesh_->clear();
    }

    if (ImGui::Button("Tick"))
    {
        if (impulse_idx < impulse.size())
        {
            mesh_->tick(impulse[impulse_idx]);
            impulse_idx++;
        }
        else
        {
            mesh_->tick(0);
        }
    }

    static float elapsed_time = 0.f;
    static int simul_speed = 1;
    if (is_simulation_running_)
    {
        elapsed_time += ImGui::GetIO().DeltaTime;
        if (elapsed_time > 1.f / simul_speed)
        {
            if (impulse_idx < impulse.size())
            {
                mesh_->tick(impulse[impulse_idx]);
                impulse_idx++;
            }
            else
            {
                mesh_->tick(0);
            }
            elapsed_time = 0.f;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button(is_simulation_running_ ? "Pause" : "Play"))
    {
        is_simulation_running_ = !is_simulation_running_;
    }

    ImGui::PushItemWidth(100);
    ImGui::SliderInt("Speed", (&simul_speed), 1, 60);
    ImGui::PopItemWidth();

    if (ImGui::Button("Reset Camera"))
    {
        reset_camera = true;
    }

    ImGui::SliderFloat("Vertical Scaler", &vertical_scaler_, 0.1f, 10.f);

    update_gl_mesh();
}

void CircularMeshManager::render_async(float render_time_seconds, RenderCompleteCallback cb)
{
    if (is_rendering_)
    {
        std::cerr << "Already rendering" << std::endl;
        return;
    }
    render_time_seconds_ = render_time_seconds;
    is_rendering_ = true;
    std::unique_ptr<Mesh2D> mesh;

    switch (mesh_type_)
    {
    case MeshType::TRIANGULAR_MESH:
        mesh = std::make_unique<TriMesh>(grid_size_.x, grid_size_.y, sample_distance_);
        break;
    case MeshType::RECTILINEAR_MESH:
        mesh = std::make_unique<RectilinearMesh>(grid_size_.x, grid_size_.y, sample_distance_);
        break;
    default:
        break;
    }

    RimguideInfo info = get_rimguide_info();
    auto mask = mesh->get_mask_for_radius(max_radius_);
    mesh->init(mask);
    mesh->init_boundary(info);

    mesh->set_input(input_pos_.x, input_pos_.y);
    mesh->set_output(output_pos_.x, output_pos_.y);

    if (clamp_center_)
    {
        mesh->clamp_center_with_rimguide();
    }

    if (use_time_varying_allpass_)
    {
        auto phase_offset = allpass_phase_offset_;
        auto rimguide_count = mesh->get_rimguide_count();
        for (size_t i = 0; i < rimguide_count; i++)
        {
            auto* rimguide = mesh->get_rimguide(i);
            auto modulator = std::make_unique<stk::SineWave>();
            float exc_amp = excitation_amplitude_;
            switch (allpass_type_)
            {
            case TimeVaryingAllpassType::SYNC:
            {
                modulator->setFrequency(allpass_mod_freq_);
                break;
            }
            case TimeVaryingAllpassType::PHASE_OFFSET:
            {
                modulator->setFrequency(allpass_mod_freq_);
                modulator->addPhase(phase_offset);
                phase_offset += allpass_phase_offset_;
                break;
            }
            case TimeVaryingAllpassType::RANDOM:
            {
                float random_freq = allpass_mod_freq_ * (1.f + rand_float() * allpass_random_freq_);
                random_freq = std::max(0.f, random_freq);
                modulator->setFrequency(random_freq);
                break;
            }
            case TimeVaryingAllpassType::RANDOM_FREQ_AND_AMP:
            {
                float random_freq = allpass_mod_freq_ * (1.f + rand_float() * allpass_random_freq_);
                random_freq = std::max(0.f, random_freq);

                exc_amp = excitation_amplitude_ * (1.f + rand_float() * allpass_random_mod_amp_);
                exc_amp = std::max(0.f, exc_amp);

                modulator->setFrequency(random_freq);
            }
            }
            rimguide->set_modulator(std::move(modulator), exc_amp);
        }
    }

    std::thread(&CircularMeshManager::render_async_worker, this, std::move(mesh), cb).detach();
}

void CircularMeshManager::plot_mesh() const
{
    static bool plot_connections = false;
    static bool plot_radius = false;

    ImGui::Text("Plot Connections:");
    ImGui::SameLine();
    ImGui::Checkbox("##Plot Connections", &plot_connections);
    ImGui::SameLine();
    ImGui::Text("Plot Radius:");
    ImGui::SameLine();
    ImGui::Checkbox("##Plot Radius", &plot_radius);

    if (!mesh_)
    {
        return;
    }

    if (ImPlot::BeginPlot("Mesh", ImVec2(-1, -1), ImPlotFlags_NoLegend | ImPlotFlags_Equal))
    {
        if (plot_connections)
        {
            ImPlot::PushStyleColor(ImPlotCol_Line, IM_COL32(15, 205, 247, 255));
            for (const auto& j : mesh_->junctions_.container())
            {
                if (j.get_type() == 0)
                {
                    continue;
                }

                std::vector<NEIGHBORS> dirs;

                switch (mesh_type_)
                {
                case MeshType::TRIANGULAR_MESH:
                    dirs = {NEIGHBORS::NORTH_EAST, NEIGHBORS::EAST, NEIGHBORS::SOUTH_EAST};
                    break;
                case MeshType::RECTILINEAR_MESH:
                    dirs = {NEIGHBORS::NORTH, NEIGHBORS::EAST, NEIGHBORS::SOUTH, NEIGHBORS::WEST};
                    break;
                default:
                    break;
                }

                for (auto dir : dirs)
                {
                    if (j.get_neighbor(dir) != nullptr)
                    {
                        auto pos = j.get_pos();
                        auto neighbor_pos = j.get_neighbor(dir)->get_pos();

                        ImVec2 delta = {neighbor_pos.x - pos.x, neighbor_pos.y - pos.y};
                        float xs[] = {pos.x, pos.x + (1.f * delta.x)};
                        float ys[] = {pos.y, pos.y + (1.f * delta.y)};

                        ImPlot::PlotLine("##connections", xs, ys, 2);
                    }
                }
            }
            ImPlot::PopStyleColor();
        }

        constexpr float kDefaultMarkerSize = 10;
        float marker_size = kDefaultMarkerSize;
        float x_extent = ImPlot::GetPlotLimits().Max().x - ImPlot::GetPlotLimits().Min().x;
        if (x_extent > 0.2)
        {
            marker_size = kDefaultMarkerSize * 0.2 / x_extent;
        }

        for (const auto& j : mesh_->junctions_.container())
        {
            if (j.get_type() == 0)
            {
                continue;
            }
            auto pos = j.get_pos();

            ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, marker_size);
            ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Circle);

            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, IM_COL32(255, 255, 255, 255));
            ImPlot::PlotScatter("##junctions", &pos.x, &pos.y, 1);
            ImPlot::PopStyleColor();
            ImPlot::PopStyleVar(2);

            if (j.has_rimguide())
            {
                ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, IM_COL32(15, 205, 247, 255));
                ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, marker_size);
                ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Circle);

                auto rimguide_pos = j.get_rimguide()->get_pos();
                ImPlot::PlotScatter("##junctions", &rimguide_pos.x, &rimguide_pos.y, 1);
                ImPlot::PopStyleVar(2);
                ImPlot::PopStyleColor();

                if (plot_connections)
                {
                    float xs[] = {pos.x, rimguide_pos.x};
                    float ys[] = {pos.y, rimguide_pos.y};
                    ImPlot::PlotLine("##connections", xs, ys, 2);
                }
            }
        }

        // Plot input
        auto input_pos = mesh_->get_input_pos();
        auto output_pos = mesh_->get_output_pos();
        ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, marker_size);
        ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Circle);
        ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, IM_COL32(255, 50, 50, 255));
        ImPlot::PlotScatter("##input", &input_pos.x, &input_pos.y, 1);
        ImPlot::PopStyleColor();

        if (listener_type_ == ListenerType::POINT)
        {
            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, IM_COL32(50, 50, 255, 255));
            ImPlot::PlotScatter("##output", &output_pos.x, &output_pos.y, 1);
            ImPlot::PopStyleColor();
        }
        ImPlot::PopStyleVar(2);

        if (plot_radius)
        {
            ImVec2 cntr = ImPlot::PlotToPixels(ImPlotPoint(0.f, 0.f));
            float radius = ImPlot::PlotToPixels(ImPlotPoint(max_radius_, 0.f)).x - cntr.x;
            float real_radius = ImPlot::PlotToPixels(ImPlotPoint(radius_, 0.f)).x - cntr.x;
            ImPlot::PushPlotClipRect();
            ImPlot::GetPlotDrawList()->AddCircle(cntr, radius, IM_COL32(255, 0, 0, 255), 100);
            ImPlot::GetPlotDrawList()->AddCircle(cntr, real_radius, IM_COL32(255, 255, 255, 255), 100);
            ImPlot::PopPlotClipRect();
        }
        ImPlot::EndPlot();
    }
}

void CircularMeshManager::render_gl_mesh(glm::mat4 mvp) const
{
    if (!mesh_)
    {
        return;
    }

    if (!line_)
    {
        return;
    }

    line_->set_mvp(mvp);
    line_->draw();

    circle_line_->set_mvp(mvp);
    circle_line_->draw();
}

float CircularMeshManager::current_fundamental_frequency() const
{
    return fundamental_frequency_;
}

void CircularMeshManager::update_gl_mesh()
{
    std::vector<glm::vec3> start_points;
    std::vector<glm::vec3> end_points;
    for (const auto& j : mesh_->junctions_.container())
    {
        if (j.get_type() == 0)
        {
            continue;
        }

        auto pos = j.get_pos();

        if (mesh_type_ == MeshType::TRIANGULAR_MESH)
        {
            if (j.get_neighbor(EAST) != nullptr)
            {
                auto neighbor_pos = j.get_neighbor(EAST)->get_pos();
                start_points.emplace_back(pos.x, pos.y, j.get_output() * vertical_scaler_);
                end_points.emplace_back(neighbor_pos.x, neighbor_pos.y,
                                        j.get_neighbor(EAST)->get_output() * vertical_scaler_);
            }
            if (j.get_neighbor(SOUTH_EAST) != nullptr)
            {
                auto neighbor_pos = j.get_neighbor(SOUTH_EAST)->get_pos();
                start_points.emplace_back(pos.x, pos.y, j.get_output() * vertical_scaler_);
                end_points.emplace_back(neighbor_pos.x, neighbor_pos.y,
                                        j.get_neighbor(SOUTH_EAST)->get_output() * vertical_scaler_);
            }
            if (j.get_neighbor(NORTH_EAST) != nullptr)
            {
                auto neighbor_pos = j.get_neighbor(NORTH_EAST)->get_pos();
                start_points.emplace_back(pos.x, pos.y, j.get_output() * vertical_scaler_);
                end_points.emplace_back(neighbor_pos.x, neighbor_pos.y,
                                        j.get_neighbor(NORTH_EAST)->get_output() * vertical_scaler_);
            }
        }
        else if (mesh_type_ == MeshType::RECTILINEAR_MESH)
        {
            if (j.get_neighbor(EAST) != nullptr)
            {
                auto neighbor_pos = j.get_neighbor(EAST)->get_pos();
                start_points.emplace_back(pos.x, pos.y, j.get_output() * vertical_scaler_);
                end_points.emplace_back(neighbor_pos.x, neighbor_pos.y,
                                        j.get_neighbor(EAST)->get_output() * vertical_scaler_);
            }
            if (j.get_neighbor(SOUTH) != nullptr)
            {
                auto neighbor_pos = j.get_neighbor(SOUTH)->get_pos();
                start_points.emplace_back(pos.x, pos.y, j.get_output() * vertical_scaler_);
                end_points.emplace_back(neighbor_pos.x, neighbor_pos.y,
                                        j.get_neighbor(SOUTH)->get_output() * vertical_scaler_);
            }
        }

        if (j.has_rimguide())
        {
            auto rimguide_pos = j.get_rimguide()->get_pos();
            start_points.emplace_back(pos.x, pos.y, j.get_output() * vertical_scaler_);
            end_points.emplace_back(rimguide_pos.x, rimguide_pos.y, 0.f);
        }
    }

    if (!line_)
    {
        line_ = std::make_unique<Line>(start_points, end_points);
    }
    else
    {
        line_->update(start_points, end_points);
    }

    start_points.clear();
    end_points.clear();
    constexpr float kCircleRes = 50.f;
    for (size_t i = 0; i < kCircleRes; i++)
    {
        float angle = 2.f * M_PI * i / kCircleRes;
        float x = radius_ * cos(angle);
        float y = radius_ * sin(angle);
        start_points.emplace_back(x, y, 0.f);

        float next_x = radius_ * cos(2.f * M_PI * (i + 1) / kCircleRes);
        float next_y = radius_ * sin(2.f * M_PI * (i + 1) / kCircleRes);
        end_points.emplace_back(next_x, next_y, 0.f);
    }

    if (!circle_line_)
    {
        circle_line_ = std::make_unique<Line>(start_points, end_points);
    }
    circle_line_->update(start_points, end_points);
}

RimguideInfo CircularMeshManager::get_rimguide_info() const
{
    RimguideInfo info{};
    info.friction_coeff = -friction_coeff_;
    info.friction_delay = friction_delay_;
    info.wave_speed = wave_speed_;
    info.sample_rate = sample_rate_;
    info.is_solid_boundary = is_solid_boundary_;
    info.fundamental_frequency = fundamental_frequency_;
    info.use_automatic_pitch_bend = use_automatic_pitch_bend_;
    info.pitch_bend_amount = pitch_bend_amount_;
    info.use_square_law_nonlinearity = use_square_law_nonlinearity_;
    info.nonlinear_factor = nonlinear_factor_;
    info.use_nonlinear_allpass = use_nonlinear_allpass_;
    info.nonlinear_allpass_coeffs[0] = nonlinear_allpass_coeffs_[0];
    info.nonlinear_allpass_coeffs[1] = nonlinear_allpass_coeffs_[1];
    info.use_extra_diffusion_filters = use_extra_diffusion_filters_;
    info.diffusion_coeffs = diffusion_filter_coeffs_;

    info.get_rimguide_pos = std::bind(get_boundary_position, radius_, std::placeholders::_1);

    return info;
}