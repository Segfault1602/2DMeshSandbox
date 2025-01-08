#include "audio.h"
#include "audio_gui.h"
#include "graphics/camera.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "mesh_gui.h"
#include "rtaudio_impl.h"

#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <cstddef>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <implot.h>
#include <iostream>
#include <memory>
#include <vector>

namespace
{
bool mousePressed = false;
double lastX = 0, lastY = 0;
bool rotation_key_pressed = false;
Camera camera;

void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            mousePressed = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        }
        else if (action == GLFW_RELEASE)
        {
            mousePressed = false;
        }
    }
}

void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    camera.add_zoom(yoffset);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    float deltaX = float(xpos - lastX) * 0.01f;
    float deltaY = float(ypos - lastY) * 0.01f;
    if (mousePressed && !rotation_key_pressed)
    {

        // Update camera position in XY plane
        camera.move(deltaX, deltaY);
    }
    if (mousePressed && rotation_key_pressed)
    {
        camera.rotate(deltaY, -deltaX);
    }
    lastX = xpos;
    lastY = ypos;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        rotation_key_pressed = true;
    }
    else if (key == GLFW_KEY_R && action == GLFW_RELEASE)
    {
        rotation_key_pressed = false;
    }
}
} // namespace

int main()
{
    std::cout << "Audio Testbench" << std::endl;

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        return 1;
    }

    // const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1440, 1280, "2D Mesh Sandbox", nullptr, nullptr);
    if (window == nullptr)
    {
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    camera.set_position(glm::vec3(0.0f, 1.f, 1.f));

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, mouse_scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    if (gladLoadGL() == 0)
    {
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(
        window,
        true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    ImVec4 clear_color = ImVec4(0.f, 0.f, 0.f, 1.00f);

    std::unique_ptr<AudioManager> audio_manager = AudioManager::CreateAudioManager();

    audio_manager->StartAudioStream();
    MeshShape mesh_shape = MeshShape::Circle;
    init_mesh_gui();

    bool show_audio_config_window = false;
    static bool opt_fullscreen = false;
    static bool opt_padding = false;

    // glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)1440 / (float)1024, 0.1f, 100.0f);
    std::vector<glm::vec3> start = {glm::vec3(0, 0, 0), glm::vec3(0, 0, 0)};
    std::vector<glm::vec3> end = {glm::vec3(1, 0, 0), glm::vec3(0, 1, 0)};

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImVec2 top_left = ImGui::GetMainViewport()->Pos;
        // ImGui::SetNextWindowPos(top_left, ImGuiCond_Appearing);
        // ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size, ImGuiCond_Always);

        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(viewport->WorkSize, ImGuiCond_Always);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        else
        {
            dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
        }

        // // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
        // // and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        {
            window_flags |= ImGuiWindowFlags_NoBackground;
        }

        if (!opt_padding)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        }

        if (!opt_padding)
        {
            ImGui::PopStyleVar();
        }
        if (opt_fullscreen)
        {
            ImGui::PopStyleVar(2);
        }

        // ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size, ImGuiCond_Once);
        // ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos, ImGuiCond_Once);
        ImGui::Begin("Mesh2D", nullptr, window_flags);

        // Submit the DockSpace
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Options"))
            {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                ImGui::MenuItem("Fullscreen", nullptr, &opt_fullscreen);
                ImGui::MenuItem("Padding", nullptr, &opt_padding);
                ImGui::Separator();

                ImGui::MenuItem("Audio Menu", nullptr, &show_audio_config_window);
                ImGui::Separator();

                if (ImGui::BeginMenu("Mesh Shape"))
                {
                    static int shape_type = 0;
                    bool pressed = false;
                    pressed |= ImGui::RadioButton("Circular", &shape_type, 0);
                    pressed |= ImGui::RadioButton("Rectangular", &shape_type, 1);

                    mesh_shape = static_cast<MeshShape>(shape_type);

                    if (pressed)
                    {
                        change_mesh_shape(mesh_shape);
                    }

                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();

        static auto first_time = true;
        if (first_time)
        {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id);
            ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2(100, 100));

            auto dock_id_mesh = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.35f, nullptr, &dockspace_id);
            ImGuiID dock_id_simulation{0};
            auto dock_id_audio_player =
                ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.20f, nullptr, &dock_id_simulation);
            // ImGui::DockBuilderDockWindow("Audio", dock_id_audio);
            ImGui::DockBuilderDockWindow("Mesh Config", dock_id_mesh);
            ImGui::DockBuilderDockWindow("Experimental", dock_id_mesh);
            ImGui::DockBuilderDockWindow("Audio Player", dock_id_audio_player);
            ImGui::DockBuilderDockWindow("Spectrogram", dock_id_simulation);
            ImGui::DockBuilderDockWindow("Waveform", dock_id_simulation);
            ImGui::DockBuilderDockWindow("Spectrum", dock_id_simulation);
            ImGui::DockBuilderDockWindow("Mesh Shape", dock_id_simulation);

            ImGui::DockBuilderFinish(dockspace_id);
        }
        if (show_audio_config_window)
        {
            if (ImGui::Begin("Audio", &show_audio_config_window))
            {
                draw_audio_device_gui(audio_manager.get());
                ImGui::End();
            }
        }

        ImGui::Begin("Audio Player");
        draw_audio_player(audio_manager.get());
        ImGui::End();

        if (first_time)
        {
            ImGui::SetNextWindowFocus();
        }
        ImGui::Begin("Mesh Config");
        bool reset_camera = false;
        draw_mesh_config(reset_camera);
        ImGui::End();

        ImGui::Begin("Experimental");
        draw_experimental_config_menu();
        ImGui::End();

        if (reset_camera)
        {
            camera.set_position({0.f, 1.5f, 0.4f});
        }

        ImGui::Begin("Spectrogram");
        draw_spectrogram();
        ImGui::End();

        ImGui::Begin("Waveform");
        draw_waveform();
        ImGui::End();

        ImGui::Begin("Spectrum");
        draw_spectrum();
        ImGui::End();

        ImGui::Begin("Mesh Shape");
        draw_mesh_shape();
        ImGui::End();

        ImGui::End();

        // Rendering
        ImGui::Render();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
        int display_w = 0;
        int display_h = 0;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                     clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // Update projection matrix with current aspect ratio
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)display_w / (float)display_h, 0.1f, 100.0f);

        // camera.position = glm::vec3(3, 3, 3);
        glm::mat4 view = camera.look_at();
        // line1.set_mvp(projection * view);
        // line1.draw();
        render_gl_mesh(projection * view);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        first_time = false;
    }
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
