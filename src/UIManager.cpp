#include "UIManager.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <math.h>

void UIManager::Init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 16.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.27f, 0.27f, 0.29f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.33f, 0.33f, 0.35f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.19f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.27f, 0.27f, 0.29f, 1.00f);
}

void UIManager::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::NewFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::Render(SimulationState& state, float fps, float ramMB, float screenshotTimer, int currentFrame, unsigned int fboB_id, int renderW) {
    if (state.showSettings) {
        ImGui::Begin(u8"Панель Управления", &state.showSettings, ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::CollapsingHeader(u8"Физика Черной Дыры", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat(u8"Масса ЧД (размер)", &state.bhSize, 0.1f, 5.0f);
            ImGui::SliderFloat(u8"Размер диска", &state.outerRadiusRs, 5.0f, 50.0f);
            ImGui::SliderFloat(u8"Спин (Вращение a)", &state.spin, 0.0f, 0.999f);
            ImGui::SliderFloat(u8"Заряд (Q)", &state.q, 0.0f, 1.0f);
            ImGui::SliderFloat(u8"Темп аккреции", &state.accretionRate, 0.0f, 0.005f, "%.5f");
            ImGui::SliderFloat(u8"Яркость диска", &state.brightmut, 0.0f, 5.0f);
            ImGui::SliderFloat(u8"Яркость джетов", &state.jetBrightmut, 0.0f, 5.0f);
            ImGui::SliderFloat(u8"Длина джетов", &state.jetLength, 0.1f, 5.0f);
            ImGui::SliderFloat(u8"Ширина джетов", &state.jetWidth, 0.1f, 5.0f);
            ImGui::SliderFloat(u8"Плотность (Тьма)", &state.darkmut, 0.0f, 2.0f);
            ImGui::SliderFloat(u8"Доплеровское покраснение", &state.reddening, 0.0f, 1.0f);
            ImGui::SliderFloat(u8"Насыщенность цвета", &state.saturation, 0.0f, 2.0f);
            ImGui::ColorEdit3(u8"Цвет плазмы", state.diskColor);
            ImGui::SliderFloat(u8"Скорость вращения диска", &state.diskRotSpeed, 0.0f, 100.0f);
        }

        if (ImGui::CollapsingHeader(u8"Камера", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat(u8"Скорость камеры", &state.moveSpeed, 0.1f, 50.0f);
            
            ImGui::Separator();
            ImGui::Text(u8"Кинематографическая камера");
            if (ImGui::Button(u8"Сохранить кадр (Keyframe)")) {
                Keyframe kf;
                glBindFramebuffer(GL_READ_FRAMEBUFFER, fboB_id);
                float camData[4];
                glReadPixels(renderW - 3, 0, 1, 1, GL_RGBA, GL_FLOAT, camData);
                kf.pos[0] = camData[0]; kf.pos[1] = camData[1]; kf.pos[2] = camData[2];
                glReadPixels(renderW - 4, 0, 1, 1, GL_RGBA, GL_FLOAT, camData);
                kf.fwd[0] = camData[0]; kf.fwd[1] = camData[1]; kf.fwd[2] = camData[2];
                glReadPixels(renderW - 2, 0, 1, 1, GL_RGBA, GL_FLOAT, camData);
                kf.right[0] = camData[0]; kf.right[1] = camData[1]; kf.right[2] = camData[2];
                glReadPixels(renderW - 1, 0, 1, 1, GL_RGBA, GL_FLOAT, camData);
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
            ImGui::SameLine();
            if (ImGui::Button(u8"Очистить")) { state.keyframes.clear(); state.playCinematic = false; state.forceCamPos = false; }
            
            ImGui::SliderFloat(u8"Скорость", &state.cinematicSpeed, 0.1f, 5.0f);
            
            if (state.keyframes.size() >= 2) {
                if (ImGui::Button(state.playCinematic ? u8"Стоп" : u8"PLAY (Синематик)")) {
                    state.playCinematic = !state.playCinematic;
                    state.cinematicTime = 0.0f;
                    state.forceCamPos = state.playCinematic;
                }
            }
            ImGui::Text(u8"Ключевых кадров: %d", (int)state.keyframes.size());
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), u8"Хоткеи: F6 (добавить), F7 (старт/стоп)");
        }

        if (ImGui::CollapsingHeader(u8"Эффекты и Графика", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat(u8"Масштаб рендера", &state.renderScale, 0.1f, 1.0f);
            ImGui::Checkbox(u8"Фон (Звезды)", &state.showBackground);
            ImGui::Checkbox(u8"Оффлайн рендер видео (100% ГПУ)", &state.offlineRender);
            ImGui::Checkbox(u8"Захватывать интерфейс (ImGui)", &state.captureImGui);
            ImGui::SliderFloat(u8"Качество (Лучи)", &state.quality, 0.1f, 5.0f);
            ImGui::SliderFloat(u8"Хроматическая аберрация", &state.chromAb, 0.0f, 0.05f);
            ImGui::Checkbox(u8"Тепловизор", &state.thermalMode);
            ImGui::Checkbox(u8"Радиотелескоп", &state.telescopeMode);
            ImGui::Checkbox(u8"Пикселизация (8-bit)", &state.pixelateMode);
        }
        ImGui::End();
    }

    if (state.showMonitor) {
        ImGui::Begin(u8"Монитор ресурсов");
        ImGui::Text("FPS: %.1f", fps);
        ImGui::Text("RAM: %.1f MB", ramMB);
        ImGui::End();
    }
    
    if (state.recording) {
        ImGui::SetNextWindowPos(ImVec2(20, 20));
        ImGui::Begin("REC", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
        if ((currentFrame / 15) % 2 == 0) ImGui::TextColored(ImVec4(1,0,0,1), "* RECORDING F5");
        ImGui::End();
    }

    if (state.videoSavedTimer > 0.0f) {
        ImGui::SetNextWindowPos(ImVec2(20, 20));
        ImGui::Begin("VideoSaved", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
        ImGui::TextColored(ImVec4(0,1,0,1), u8"Видео сохранено: %s", state.lastSavedVideo);
        ImGui::End();
    }

    if (screenshotTimer > 0.0f) {
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::Begin("Notif", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
        ImGui::Text(u8"Скриншот сохранен (cmake-build-debug)");
        ImGui::End();
    }
}

void UIManager::EndFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
