#include "Application.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <imgui.h>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

Application::Application() {}
Application::~Application() {
    ui.Shutdown();
    if (window) glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::keyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Application* app = (Application*)glfwGetWindowUserPointer(window);
    if (app) app->keyCallback(key, action);
}

void Application::keyCallback(int key, int action) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_B) state.showSettings = !state.showSettings;
        if (key == GLFW_KEY_M) state.showMap = !state.showMap;
        if (key == GLFW_KEY_N) state.showMonitor = !state.showMonitor;
        if (key == GLFW_KEY_F12) state.takeScreenshot = true;
        if (key == GLFW_KEY_F11) {
            static bool fullscreen = false;
            static int windowedX, windowedY, windowedWidth, windowedHeight;
            fullscreen = !fullscreen;
            if (fullscreen) {
                glfwGetWindowPos(window, &windowedX, &windowedY);
                glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            } else {
                glfwSetWindowMonitor(window, nullptr, windowedX, windowedY, windowedWidth, windowedHeight, 0);
            }
        }
        if (key == GLFW_KEY_F5) {
            state.recording = !state.recording;
        }
        if (key == GLFW_KEY_F6) {
            // Add keyframe via hotkey
            Keyframe kf;
            unsigned int fbo = renderer.GetCurrentCameraFBO(frame);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
            float camData[4];
            int w = state.windowWidth * state.renderScale;
            if (state.recording && state.offlineRender) w = state.windowWidth; // account for override
            glReadPixels(w - 3, 0, 1, 1, GL_RGBA, GL_FLOAT, camData);
            kf.pos[0] = camData[0]; kf.pos[1] = camData[1]; kf.pos[2] = camData[2];
            glReadPixels(w - 4, 0, 1, 1, GL_RGBA, GL_FLOAT, camData);
            kf.fwd[0] = camData[0]; kf.fwd[1] = camData[1]; kf.fwd[2] = camData[2];
            glReadPixels(w - 2, 0, 1, 1, GL_RGBA, GL_FLOAT, camData);
            kf.right[0] = camData[0]; kf.right[1] = camData[1]; kf.right[2] = camData[2];
            glReadPixels(w - 1, 0, 1, 1, GL_RGBA, GL_FLOAT, camData);
            kf.up[0] = camData[0]; kf.up[1] = camData[1]; kf.up[2] = camData[2];
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

            kf.spin = state.spin; kf.q = state.q; kf.acc = state.accretionRate;
            kf.bright = state.brightmut; kf.dark = state.darkmut; kf.redden = state.reddening;
            kf.sat = state.saturation; kf.jetBright = state.jetBrightmut; kf.jetLength = state.jetLength; kf.jetWidth = state.jetWidth;
            kf.color[0] = state.diskColor[0]; kf.color[1] = state.diskColor[1]; kf.color[2] = state.diskColor[2];
            kf.radius = state.outerRadiusRs; kf.bhSize = state.bhSize; kf.rot = state.diskRotSpeed;
            kf.time = state.keyframes.empty() ? 0.0f : state.keyframes.back().time + 3.0f; 
            state.keyframes.push_back(kf);
        }
        if (key == GLFW_KEY_F7) {
            if (state.keyframes.size() >= 2) {
                state.playCinematic = !state.playCinematic;
                state.cinematicTime = 0.0f;
                state.forceCamPos = state.playCinematic;
            }
        }
    }
}

bool Application::Init() {
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(state.windowWidth, state.windowHeight, "blackhole_render 2.0", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyCallbackStatic);
    glfwSwapInterval(0); // VSync off

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return false;
    }

    const char* renderer_str = (const char*)glGetString(GL_RENDERER);
    std::string gpuName = renderer_str ? renderer_str : "Unknown GPU";
    
    if (gpuName.find("RTX") != std::string::npos || gpuName.find("RX 6") != std::string::npos || gpuName.find("RX 7") != std::string::npos || gpuName.find("GTX 1080") != std::string::npos) {
        state.renderScale = 1.0f;
        state.quality = 1.0f;
#ifdef _WIN32
        std::string msg = "System Check Passed: Your PC is powerful enough!\n\nDetected GPU: " + gpuName + "\n\nThe 'Mega Realistic Volumetric Gas' preset and maximum integration steps (4000) have been automatically enabled for cinematic quality.";
        MessageBoxA(nullptr, msg.c_str(), "Hardware Performance Check", MB_OK | MB_ICONINFORMATION);
#endif
    } else {
        state.renderScale = 0.5f;
        state.quality = 0.5f;
#ifdef _WIN32
        std::string msg = "System Check: Mid/Low-end GPU detected.\n\nDetected GPU: " + gpuName + "\n\nRender scale and quality have been automatically lowered for better performance. You can adjust this in settings.";
        MessageBoxA(nullptr, msg.c_str(), "Hardware Performance Check", MB_OK | MB_ICONWARNING);
#endif
    }

    renderer.Init();
    ui.Init(window);
    return true;
}

void Application::UpdateInput() {
    double mx, my; 
    glfwGetCursorPos(window, &mx, &my);
    my = state.windowHeight - my;
    
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse) {
        if (state.mouse.z <= 0.0f) { state.mouse.z = mx; state.mouse.w = my; }
        state.mouse.x = mx; state.mouse.y = my;
    } else {
        state.mouse.z = -1.0f; state.mouse.w = -1.0f;
    }
}

void Application::Run() {
    double lastTime = glfwGetTime();
    
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float dt = (float)(currentTime - lastTime);
        lastTime = currentTime;
        
        float actualRenderScale = state.renderScale;
        if (state.recording && state.offlineRender) {
            dt = 1.0f / 30.0f; // Fixed framerate for perfect offline rendering without lag
            actualRenderScale = 1.0f; // Force 100% scale
        }
        
        time += dt;

        glfwGetFramebufferSize(window, &state.windowWidth, &state.windowHeight);
        renderer.Resize(state.windowWidth, state.windowHeight, actualRenderScale);

        glfwPollEvents();
        UpdateInput();
        
        cinematic.Update(state, dt);

        uint8_t keys[350] = {0};
        for (int i=0; i<350; i++) {
            if (glfwGetKey(window, i) == GLFW_PRESS) keys[i] = 255;
        }
        
        ui.NewFrame();
        
        float ramMB = 0.0f;
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            ramMB = pmc.WorkingSetSize / 1024.0f / 1024.0f;
        }
#endif
        
        // Pass 0 as fboB_id for now. It needs a getter if we really want to bind it.
        // Or we can hack around it. Let's provide a way to get fboB from renderer?
        // Let's pass 0, wait, keyframing needs fboB ID! 
        // We'll add a getter in Renderer.h: GLuint GetCurrentCameraFBO(int frame);
        
        // UI rendering requires fboB id, we'll fix that next step.
        // For now, we will pass 0.
        
        ui.Render(state, ImGui::GetIO().Framerate, ramMB, screenshotTimer, frame, renderer.GetCurrentCameraFBO(frame), state.windowWidth * state.renderScale);
        
        renderer.RenderFrame(state, time, dt, frame, keys);
        
        renderer.CheckAndSaveScreenshot(state, screenshotTimer);
        if (screenshotTimer > 0.0f) screenshotTimer -= dt;
        if (state.videoSavedTimer > 0.0f) state.videoSavedTimer -= dt;
        
        if (!state.captureImGui) renderer.CheckAndRecordVideo(state, dt);

        ui.EndFrame();
        
        if (state.captureImGui) renderer.CheckAndRecordVideo(state, dt);
        
        glfwSwapBuffers(window);
        frame++;
    }
}
