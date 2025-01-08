#include "mesh_manager.h"

#include <memory>

#include <sndfile.h>

#include "gaussian.h"
#include "listener.h"
#include "mesh_2d.h"

float MeshManager::get_progress() const
{
    return progress_;
}

bool MeshManager::is_rendering() const
{
    return is_rendering_;
}

float MeshManager::get_render_runtime() const
{
    return render_runtime_;
}

void MeshManager::render_async_worker(std::unique_ptr<Mesh2D>&& mesh, RenderCompleteCallback cb)
{
    const size_t out_size = render_time_seconds_ * sample_rate_;
    std::vector<float> out_buffer(out_size);

    stk::PoleZero dc_blocker;
    dc_blocker.setBlockZero(dc_blocker_alpha_);

    std::vector<float> impulse;
    if (excitation_type_ == ExcitationType::DIRAC)
    {
        impulse.push_back(1);
    }
    else if (excitation_type_ == ExcitationType::RAISE_COSINE)
    {
        impulse = raised_cosine(excitation_frequency_, sample_rate_);
    }
    else if (excitation_type_ == ExcitationType::FILE)
    {
        SF_INFO sf_info{0};
        SNDFILE* file = sf_open(excitation_filename_.c_str(), SFM_READ, &sf_info);
        if (!file)
        {
            std::cerr << "Failed to open file" << std::endl;
        }
        else
        {
            impulse.resize(sf_info.frames);
            sf_readf_float(file, impulse.data(), sf_info.frames);
            sf_close(file);
        }
    }

    ListenerInfo listener_info{};
    listener_info.position = {-0.4f, 0.f, 0.8f};
    listener_info.samplerate = mesh->get_samplerate();
    listener_info.type = listener_type_;

    if (listener_type_ == ListenerType::POINT)
    {
        listener_info.position = {mesh->get_output_pos().x, mesh->get_output_pos().y, 0.0f};
    }

    Listener listener;
    listener.init(*mesh, listener_info);
    if (listener_type_ == ListenerType::ALL)
    {
        listener.set_gain(0.2f);
    }
    else if (listener_type_ == ListenerType::BOUNDARY)
    {
        // TODO: figure out how to scale this properly
        listener.set_gain(5.f);
    }
    else if (listener_type_ == ListenerType::POINT)
    {
        listener.set_gain(10.f);
    }

    progress_ = 0.f;

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < out_size - 1; i++)
    {
        float input = 0.f;
        if (i < impulse.size())
        {
            input = -impulse[i] * excitation_amplitude_;
        }
        mesh->tick(input);
        out_buffer[i] = listener.tick();

        if (use_dc_blocker_)
        {
            out_buffer[i] = dc_blocker.tick(out_buffer[i]);
        }

        float new_progress = static_cast<float>(i) / static_cast<float>(out_size);
        if (new_progress - progress_ > 0.01f)
        {
            progress_ = new_progress;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    render_runtime_ = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    SF_INFO out_sf_info{0};
    out_sf_info.channels = 1;
    out_sf_info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    out_sf_info.samplerate = static_cast<int>(sample_rate_);
    out_sf_info.frames = static_cast<sf_count_t>(out_size);

    SNDFILE* out_file = sf_open("mesh.wav", SFM_WRITE, &out_sf_info);
    if (!out_file)
    {
        std::cerr << "Failed to open output file" << std::endl;
        return;
    }

    sf_writef_float(out_file, out_buffer.data(), out_size);
    sf_write_sync(out_file);
    sf_close(out_file);

    is_rendering_ = false;

    if (cb)
    {
        cb();
    }
}