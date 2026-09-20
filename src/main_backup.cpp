#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <regex>

#include <windows.h>
#include <psapi.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../extern/glfw/deps/stb_image_write.h"
#include <iomanip>
#include <deque>
#include <time.h>



bool g_showSettings = true;
bool g_showMap = false;
bool g_showBackground = true;
bool g_showMonitor = false;
bool g_takeScreenshot = false;
bool g_recording = false;
float g_iDiskColor[3] = {1.0f, 1.0f, 1.0f};
float g_moveSpeed = 10.0f;
float g_chromAb = 0.0f;
bool g_thermalMode = false;
bool g_pixelateMode = false;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_B) g_showSettings = !g_showSettings;
        if (key == GLFW_KEY_M) g_showMap = !g_showMap;
        if (key == GLFW_KEY_N) g_showMonitor = !g_showMonitor;
        if (key == GLFW_KEY_F12) g_takeScreenshot = true;
        if (key == GLFW_KEY_F5) {
            g_recording = !g_recording;
        }
    }
}

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compileShader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed! " << (type == GL_VERTEX_SHADER ? "VERT" : "FRAG") << "\n" << infoLog << "\n";
    }
    return shader;
}

GLuint createProgram(const std::string& fragSource) {
    std::string vertexSource = "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "void main() { gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); }\n";
    
            std::string shadertoyPrefix = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 iResolution;\n"
        "uniform float iTime;\n"
        "uniform float iTimeDelta;\n"
        "uniform int iFrame;\n"
        "uniform vec4 iMouse;\n"
        "uniform sampler2D iChannel0;\n"
        "uniform sampler2D iChannel1;\n"
        "uniform sampler2D iChannel2;\n"
        "uniform sampler2D iChannel3;\n"
        "uniform vec3 iChannelResolution[4];\n"
        "uniform float g_iBlackHoleMassSol;\n"
        "uniform float g_iSpin;\n"
        "uniform float g_iQ;\n"
        "uniform float g_iAccretionRate;\n"
        "uniform float g_iBrightmut;\n"
        "uniform float g_iDarkmut;\n"
        "uniform float g_iReddening;\n"
        "uniform float g_iSaturation;\n"
        "uniform float g_iJetBrightmut;\n"
        "uniform float g_iQuality;\n"
        "uniform float g_iOuterRadiusRs;\n"
        "uniform float g_diskRotSpeed;\n"
        "uniform int g_showMap;\n"
        "uniform int g_showBackground;\n"
        "uniform float g_stopBackground;\n"
        
        "uniform vec3 g_iDiskColor;\n"
        "uniform float g_moveSpeed;\n"
        "uniform float g_chromAb;\n"
        "uniform int g_thermalMode;\n"
        "uniform int g_pixelateMode;\n"
        "#define iBlackHoleMassSol g_iBlackHoleMassSol\n"
        "#define iSpin g_iSpin\n"
        "#define iQ g_iQ\n"
        "#define iAccretionRate g_iAccretionRate\n"
        "#define iBrightmut g_iBrightmut\n"
        "#define iDarkmut g_iDarkmut\n"
        "#define iReddening g_iReddening\n"
        "#define iSaturation g_iSaturation\n"
        "#define iJetBrightmut g_iJetBrightmut\n"
        "#define iQuality g_iQuality\n"
        "#define iOuterRadiusRs g_iOuterRadiusRs\n";
    
    std::string fullFrag = shadertoyPrefix + fragSource + "\nvoid main() { mainImage(FragColor, gl_FragCoord.xy); }\n";

    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fullFrag, GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

struct FBO {
    GLuint fbo;
    GLuint tex;
    int w, h;
        void destroy() {
        if (fbo) glDeleteFramebuffers(1, &fbo);
        if (tex) glDeleteTextures(1, &tex);
        fbo = 0; tex = 0;
    }
    void init(int width, int height) {
        destroy();
        w = width; h = height;
        glGenFramebuffers(1, &fbo);
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};


float g_iBlackHoleMassSol = 1e7;
float g_iSpin = 0.997f;
float g_iQ = 0.0f;
float g_iAccretionRate = 5e-4f;
float g_iBrightmut = 1.0f;
float g_iDarkmut = 0.5f;
float g_iReddening = 0.3f;
float g_iSaturation = 0.5f;
float g_iJetBrightmut = 0.0f;
float g_iQuality = 1.0f;
float g_iOuterRadiusRs = 20.0f;
float g_diskRotSpeed = 1.0f;
float g_stopBackground = 0.0f;
float g_forceCamAngles = 0.0f;
float g_screenshotNotifTimer = 0.0f;
float g_bhPitch = 0.0f;
float g_bhYaw = 0.0f;
float g_bhRoll = 0.0f;

struct vec4 { float x,y,z,w; };

void bindUniforms(GLuint prog, int w, int h, float time, float dt, int frame, vec4 mouse) {
    glUseProgram(prog);
    glUniform1f(glGetUniformLocation(prog, "g_camPitch"), g_bhPitch);
    glUniform1f(glGetUniformLocation(prog, "g_camYaw"), g_bhYaw);
    glUniform1f(glGetUniformLocation(prog, "g_camRoll"), g_bhRoll);
    glUniform1f(glGetUniformLocation(prog, "g_forceCamAngles"), g_forceCamAngles);
    glUniform3f(glGetUniformLocation(prog, "iResolution"), w, h, 1.0f);
    glUniform1f(glGetUniformLocation(prog, "iTime"), time);
    glUniform1f(glGetUniformLocation(prog, "iTimeDelta"), dt);
    glUniform1i(glGetUniformLocation(prog, "iFrame"), frame);
    glUniform4f(glGetUniformLocation(prog, "iMouse"), mouse.x, mouse.y, mouse.z, mouse.w);
    
    glUniform1f(glGetUniformLocation(prog, "g_iBlackHoleMassSol"), g_iBlackHoleMassSol);
    glUniform1f(glGetUniformLocation(prog, "g_iSpin"), g_iSpin);
    glUniform1f(glGetUniformLocation(prog, "g_iQ"), g_iQ);
    glUniform1f(glGetUniformLocation(prog, "g_iAccretionRate"), g_iAccretionRate);
    glUniform1f(glGetUniformLocation(prog, "g_iBrightmut"), g_iBrightmut);
    glUniform1f(glGetUniformLocation(prog, "g_iDarkmut"), g_iDarkmut);
    glUniform1f(glGetUniformLocation(prog, "g_iReddening"), g_iReddening);
    glUniform1f(glGetUniformLocation(prog, "g_iSaturation"), g_iSaturation);
    glUniform1f(glGetUniformLocation(prog, "g_iJetBrightmut"), g_iJetBrightmut);
    glUniform1f(glGetUniformLocation(prog, "g_iQuality"), g_iQuality);
    glUniform1f(glGetUniformLocation(prog, "g_iOuterRadiusRs"), g_iOuterRadiusRs);
    glUniform1f(glGetUniformLocation(prog, "g_diskRotSpeed"), g_diskRotSpeed);

    glUniform1i(glGetUniformLocation(prog, "g_showMap"), g_showMap ? 1 : 0);
    glUniform1i(glGetUniformLocation(prog, "g_showBackground"), g_showBackground ? 1 : 0);
    glUniform1f(glGetUniformLocation(prog, "g_stopBackground"), g_stopBackground);
    glUniform3f(glGetUniformLocation(prog, "g_iDiskColor"), g_iDiskColor[0], g_iDiskColor[1], g_iDiskColor[2]);

    glUniform1f(glGetUniformLocation(prog, "g_moveSpeed"), g_moveSpeed);
    glUniform1f(glGetUniformLocation(prog, "g_chromAb"), g_chromAb);
    glUniform1i(glGetUniformLocation(prog, "g_thermalMode"), g_thermalMode ? 1 : 0);
    glUniform1i(glGetUniformLocation(prog, "g_pixelateMode"), g_pixelateMode ? 1 : 0);

    glUniform1i(glGetUniformLocation(prog, "iChannel0"), 0);
    glUniform1i(glGetUniformLocation(prog, "iChannel1"), 1);
    glUniform1i(glGetUniformLocation(prog, "iChannel2"), 2);
    glUniform1i(glGetUniformLocation(prog, "iChannel3"), 3);
}

void bindTexture(GLuint prog, int channel, GLuint tex, int w, int h) {
    glActiveTexture(GL_TEXTURE0 + channel);
    glBindTexture(GL_TEXTURE_2D, tex);
    std::string resName = "iChannelResolution[" + std::to_string(channel) + "]";
    glUniform3f(glGetUniformLocation(prog, resName.c_str()), w, h, 1.0f);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int windowWidth = 1280;
    int windowHeight = 720;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "blackhole_render 2.0", nullptr, nullptr);
    glfwMakeContextCurrent(window);
glfwSetKeyCallback(window, keyCallback);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 16.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    float vertices[] = { -1.f,-1.f, 1.f,-1.f, -1.f,1.f, 1.f,-1.f, 1.f,1.f, -1.f,1.f };
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    GLuint keyboardTex;
    glGenTextures(1, &keyboardTex);
    glBindTexture(GL_TEXTURE_2D, keyboardTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 350, 1, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Read shaders
    std::string bufferACode = readFile("assets/BufferA.glsl");
    if (bufferACode.empty()) bufferACode = readFile("../assets/BufferA.glsl");
    
    std::string bufferBCode = readFile("assets/BufferB.glsl");
    if (bufferBCode.empty()) bufferBCode = readFile("../assets/BufferB.glsl");

    std::string bufferCCode = readFile("assets/BufferC.glsl");
    if (bufferCCode.empty()) bufferCCode = readFile("../assets/BufferC.glsl");

    std::string bufferDCode = readFile("assets/BufferD.glsl");
    if (bufferDCode.empty()) bufferDCode = readFile("../assets/BufferD.glsl");

    std::string imageCode = readFile("assets/Image.glsl");
    if (imageCode.empty()) imageCode = readFile("../assets/Image.glsl");

    // Remove old #defines from shaders so our uniform overrides work
    auto removeDefines = [&](std::string& code) {
        std::vector<std::string> defs = {
            "iBlackHoleMassSol", "iSpin", "iQ", "iAccretionRate", 
            "iBrightmut", "iDarkmut", "iReddening", "iSaturation", 
            "iJetBrightmut", "iQuality", "iOuterRadiusRs"
        };
        for (const auto& def : defs) {
            std::regex e(R"(#define\s+)" + def + R"(\b[^\n]*\n)");
            code = std::regex_replace(code, e, "");
        }
    };
    removeDefines(bufferACode);
    removeDefines(bufferBCode);
    removeDefines(bufferCCode);
    removeDefines(bufferDCode);
    removeDefines(imageCode);

    GLuint progA = createProgram(bufferACode);
    GLuint progB = createProgram(bufferBCode);
    GLuint progC = createProgram(bufferCCode);
    GLuint progD = createProgram(bufferDCode);
    GLuint progImage = createProgram(imageCode);

    int w = 1280, h = 720;
    float g_renderScale = 1.0f;
    FBO fboA[2], fboB[2], fboC, fboD;
    fboA[0].init(w,h); fboA[1].init(w,h);
    fboB[0].init(w,h); fboB[1].init(w,h);
    fboC.init(w,h); fboD.init(w,h);

    // Clear all FBOs to prevent NaN/garbage data from propagating
    auto clearFBO = [](GLuint fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    };
    clearFBO(fboA[0].fbo); clearFBO(fboA[1].fbo);
    clearFBO(fboB[0].fbo); clearFBO(fboB[1].fbo);
    clearFBO(fboC.fbo); clearFBO(fboD.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int frame = 0;
    float lastTime = glfwGetTime();
    vec4 mouse = {0,0,0,0};

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
        int targetW = (int)(windowWidth * g_renderScale);
        int targetH = (int)(windowHeight * g_renderScale);
        if (targetW > 0 && targetH > 0 && (targetW != w || targetH != h)) {
            w = targetW;
            h = targetH;
            fboA[0].init(w, h); fboA[1].init(w, h);
            fboB[0].init(w, h); fboB[1].init(w, h);
            fboC.init(w, h); fboD.init(w, h);
        }

        
        // --- Fullscreen F11 ---
        static bool f11_pressed = false;
        static bool is_fullscreen = false;
        static int win_x = 0, win_y = 0, win_w = 800, win_h = 450;
        if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS) {
            if (!f11_pressed) {
                f11_pressed = true;
                if (!is_fullscreen) {
                    glfwGetWindowPos(window, &win_x, &win_y);
                    glfwGetWindowSize(window, &win_w, &win_h);
                    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                    is_fullscreen = true;
                } else {
                    glfwSetWindowMonitor(window, nullptr, win_x, win_y, win_w, win_h, 0);
                    is_fullscreen = false;
                }
            }
        } else {
            f11_pressed = false;
        }
        
        

        // --- FPS Window Title ---
        char titleBuf[256];
        snprintf(titleBuf, sizeof(titleBuf), "blackhole_render 2.0 | FPS: %.1f", ImGui::GetIO().Framerate);
        glfwSetWindowTitle(window, titleBuf);

        float time = glfwGetTime(); 
        float dt = time - lastTime; 
        // // if (dt > 0.1f) dt = 0.1f; // removed clamp to fix slow-mo // removed clamp to fix slow-mo
        lastTime = time;

        double mx, my; glfwGetCursorPos(window, &mx, &my);
        my = h - my;
        
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse) {
            if (mouse.z <= 0.0f) { mouse.z = mx; mouse.w = my; }
            mouse.x = mx; mouse.y = my;
        } else {
            mouse.z = -1.0f; mouse.w = -1.0f;
        }

        uint8_t keys[350] = {0};
        for (int i=0; i<350; i++) {
            if (glfwGetKey(window, i) == GLFW_PRESS) keys[i] = 255;
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glActiveTexture(GL_TEXTURE0 + 3);
        glBindTexture(GL_TEXTURE_2D, keyboardTex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 350, 1, GL_RED, GL_UNSIGNED_BYTE, keys);

        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        
        if (g_showSettings) {
        ImGui::Begin(u8"Панель Управления");
        ImGui::Text(u8"Нажмите B, чтобы скрыть UI");
        ImGui::Text(u8"F5 - Записать видео, F12 - Скриншот, M - Карта, N - Монитор");
        
        if (ImGui::CollapsingHeader(u8"Физика", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat(u8"Спин (a)", &g_iSpin, -0.999f, 0.999f);
            ImGui::SliderFloat(u8"Заряд (Q)", &g_iQ, -1.0f, 1.0f);
            ImGui::SliderFloat(u8"Внешний радиус (RS)", &g_iOuterRadiusRs, 5.0f, 50.0f);
            ImGui::SliderFloat(u8"Скорость вращения диска", &g_diskRotSpeed, 0.0f, 5.0f);
            ImGui::SliderFloat(u8"Темп аккреции", &g_iAccretionRate, 0.0f, 0.05f);
        }

        if (ImGui::CollapsingHeader(u8"Визуал", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat(u8"Яркость", &g_iBrightmut, 0.0f, 10.0f);
            ImGui::SliderFloat(u8"Затемнение", &g_iDarkmut, 0.0f, 2.0f);
            ImGui::SliderFloat(u8"Покраснение (Reddening)", &g_iReddening, 0.0f, 2.0f);
            ImGui::SliderFloat(u8"Насыщенность (Saturation)", &g_iSaturation, 0.0f, 3.0f);
            
            float color3[3] = { g_iDiskColor[0], g_iDiskColor[1], g_iDiskColor[2] };
            if (ImGui::ColorEdit3(u8"Цвет диска", color3)) {
                g_iDiskColor[0] = color3[0];
                g_iDiskColor[1] = color3[1];
                g_iDiskColor[2] = color3[2];
            }
            bool bg = g_showBackground == 1;
            if (ImGui::Checkbox(u8"Показывать фон", &bg)) g_showBackground = bg ? 1 : 0;
            ImGui::Checkbox(u8"Карта (M)", &g_showMap);
            
            bool stopBg = g_stopBackground == 1.0f;
            if (ImGui::Checkbox(u8"Остановить вращение фона", &stopBg)) g_stopBackground = stopBg ? 1.0f : 0.0f;
        }

        if (ImGui::CollapsingHeader(u8"Камера", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat(u8"Тангаж X (Pitch)", &g_bhPitch, -180.0f, 180.0f);
            ImGui::SliderFloat(u8"Рыскание Y (Yaw)", &g_bhYaw, -180.0f, 180.0f);
            ImGui::SliderFloat(u8"Крен Z (Roll)", &g_bhRoll, -180.0f, 180.0f);
            ImGui::SliderFloat(u8"Скорость камеры", &g_moveSpeed, 0.1f, 50.0f);
            if (ImGui::Button(u8"Сбросить поворот")) { g_bhPitch = 0; g_bhYaw = 0; g_bhRoll = 0; }
        }

        if (ImGui::CollapsingHeader(u8"Эффекты и Графика", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat(u8"Масштаб рендера", &g_renderScale, 0.1f, 1.0f);
            ImGui::SliderFloat(u8"Качество (Шаги)", &g_iQuality, 0.1f, 5.0f);
            ImGui::SliderFloat(u8"Хроматическая аберрация", &g_chromAb, 0.0f, 0.05f);
            ImGui::Checkbox(u8"Тепловизор", &g_thermalMode);
            ImGui::Checkbox(u8"Пикселизация (8-bit)", &g_pixelateMode);
        }
        ImGui::End();
    }

        static std::deque<float> fpsHistory;
        static std::deque<float> ramHistory;
        if (fpsHistory.size() > 100) fpsHistory.pop_front();
        if (ramHistory.size() > 100) ramHistory.pop_front();
        fpsHistory.push_back(io.Framerate);
        
        PROCESS_MEMORY_COUNTERS pmc;
        float ramMB = 0.0f;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            ramMB = pmc.WorkingSetSize / 1024.0f / 1024.0f;
        }
        ramHistory.push_back(ramMB);

        if (g_showMonitor) {
        ImGui::Begin(u8"Монитор ресурсов");
        ImGui::Text("FPS: %.1f", io.Framerate);
        
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
        DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
        float ramUsage = (float)physMemUsed / (float)totalPhysMem * 100.0f;
        
        ImGui::Text(u8"ОЗУ: %.1f%% (%.1f GB / %.1f GB)", ramUsage, physMemUsed / (1024.0*1024.0*1024.0), totalPhysMem / (1024.0*1024.0*1024.0));
        
        float targetFPS = 60.0f;
        float gpuLoad = (targetFPS / (io.Framerate + 0.1f)) * 50.0f * g_iQuality; 
        if (gpuLoad > 99.9f) gpuLoad = 99.9f;
        
        ImGui::Text(u8"Нагрузка GPU (оценка): %.1f%%", gpuLoad);
        ImGui::End();
    }
        
        if (g_recording) {
            ImGui::SetNextWindowPos(ImVec2(20, 20));
            ImGui::Begin("REC", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
            if (fmod(time, 1.0) < 0.5) ImGui::TextColored(ImVec4(1,0,0,1), "* RECORDING F5");
            ImGui::End();
        }


        glViewport(0, 0, w, h);
        glBindVertexArray(VAO);

        // Buffer B (Camera updates)
        glBindFramebuffer(GL_FRAMEBUFFER, fboB[frame%2].fbo);
        bindUniforms(progB, w, h, time, dt, frame, mouse);
        bindTexture(progB, 0, fboA[(frame+1)%2].tex, w, h);
        bindTexture(progB, 1, fboB[(frame+1)%2].tex, w, h);
        bindTexture(progB, 3, keyboardTex, 350, 1);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Buffer A (Main Raytracer)
        glBindFramebuffer(GL_FRAMEBUFFER, fboA[frame%2].fbo);
        bindUniforms(progA, w, h, time, dt, frame, mouse);
        bindTexture(progA, 2, fboB[frame%2].tex, w, h);
        bindTexture(progA, 3, fboA[(frame+1)%2].tex, w, h);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Buffer C (Blur H)
        glBindFramebuffer(GL_FRAMEBUFFER, fboC.fbo);
        bindUniforms(progC, w, h, time, dt, frame, mouse);
        bindTexture(progC, 0, fboB[frame%2].tex, w, h); 
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Buffer D (Blur V)
        glBindFramebuffer(GL_FRAMEBUFFER, fboD.fbo);
        bindUniforms(progD, w, h, time, dt, frame, mouse);
        bindTexture(progD, 0, fboC.tex, w, h);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Image (Composite to Screen)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        glViewport(0, 0, windowWidth, windowHeight);
        bindUniforms(progImage, windowWidth, windowHeight, time, dt, frame, mouse);
        bindTexture(progImage, 0, fboA[frame%2].tex, w, h);
        bindTexture(progImage, 3, fboD.tex, w, h);
        glDrawArrays(GL_TRIANGLES, 0, 6);

                  if (g_takeScreenshot) {
            std::vector<unsigned char> pixels(windowWidth * windowHeight * 4);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadBuffer(GL_BACK);
            glReadPixels(0, 0, windowWidth, windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            stbi_flip_vertically_on_write(1);
            
            static int shotCounter = 1;
            char filename[256];
            std::filesystem::create_directories("screenshoots");
            sprintf(filename, "screenshoots/screenshot_%d.png", shotCounter++);
            stbi_write_png(filename, windowWidth, windowHeight, 4, pixels.data(), windowWidth * 4);
            
            g_takeScreenshot = false;
            g_screenshotNotifTimer = 3.0f;
        }

        if (g_screenshotNotifTimer > 0.0f) {
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::Begin("Notif", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
        ImGui::Text(u8"Скриншот сохранен (cmake-build-debug)");
        ImGui::End();
        g_screenshotNotifTimer -= dt;
    }

        static float recordAccum = 0.0f;
        static bool wasRecording = false;
        static FILE* ffmpegPipe = nullptr;

        if (g_recording) {
            wasRecording = true;
            recordAccum += dt;
            if (recordAccum > 1.0f / 30.0f) {
                recordAccum -= 1.0f / 30.0f;
                if (!ffmpegPipe) {
                    std::filesystem::create_directories("video");
                    int vidIdx = 1;
                    char outFilename[256];
                    while (true) {
                        sprintf(outFilename, "video/video%d.mp4", vidIdx);
                        if (!std::filesystem::exists(outFilename)) break;
                        vidIdx++;
                    }
                    char cmd[512];
                    sprintf(cmd, "ffmpeg -y -f rawvideo -vcodec rawvideo -s %dx%d -pix_fmt rgba -r 30 -i - -c:v libx264 -preset ultrafast -pix_fmt yuv420p \"%s\"", windowWidth, windowHeight, outFilename);
                    ffmpegPipe = _popen(cmd, "wb");
                    std::cout << "Started recording to " << outFilename << "...\n";
                }
                
                std::vector<unsigned char> pixels(windowWidth * windowHeight * 4);
                glPixelStorei(GL_PACK_ALIGNMENT, 1);
                glReadBuffer(GL_BACK);
                glReadPixels(0, 0, windowWidth, windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                
                if (ffmpegPipe) {
                    int rowSize = windowWidth * 4;
                    std::vector<unsigned char> flipped(windowWidth * windowHeight * 4);
                    for (int i = 0; i < windowHeight; ++i) {
                        memcpy(&flipped[i * rowSize], &pixels[(windowHeight - 1 - i) * rowSize], rowSize);
                    }
                    fwrite(flipped.data(), 1, flipped.size(), ffmpegPipe);
                }
            }
        } else {
            if (wasRecording) {
                wasRecording = false;
                if (ffmpegPipe) {
                    std::cout << "Saving MP4 video (flushing pipe)...\n";
                    _pclose(ffmpegPipe);
                    ffmpegPipe = nullptr;
                    std::cout << "Video saved successfully!\n";
                }
            }
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);

        frame++;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
