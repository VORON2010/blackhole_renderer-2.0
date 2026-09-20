#include "Renderer.h"
#include "Shader.h"
#include <iostream>
#include <filesystem>
#include <regex>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../extern/glfw/deps/stb_image_write.h"

#ifdef _WIN32
#include <windows.h>
#endif

void FBO::destroy() {
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (tex) glDeleteTextures(1, &tex);
    fbo = 0; tex = 0;
}

void FBO::init(int width, int height) {
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

Renderer::Renderer() {
}

Renderer::~Renderer() {
}

void Renderer::removeDefines(std::string& code) {
    std::vector<std::string> defs = {
        "iBlackHoleMassSol", "iSpin", "iQ", "iAccretionRate", 
        "iBrightmut", "iDarkmut", "iReddening", "iSaturation", 
        "iJetBrightmut", "iQuality", "iOuterRadiusRs"
    };
    for (const auto& def : defs) {
        std::regex e(R"(#define\s+)" + def + R"(\b[^\n]*\n)");
        code = std::regex_replace(code, e, "");
    }
}

void Renderer::loadShaders() {
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

    removeDefines(bufferACode);
    removeDefines(bufferBCode);
    removeDefines(bufferCCode);
    removeDefines(bufferDCode);
    removeDefines(imageCode);

    progA = createProgram(bufferACode);
    progB = createProgram(bufferBCode);
    progC = createProgram(bufferCCode);
    progD = createProgram(bufferDCode);
    progImage = createProgram(imageCode);
}

void Renderer::Init() {
    float vertices[] = { -1.f,-1.f, 1.f,-1.f, -1.f,1.f, 1.f,-1.f, 1.f,1.f, -1.f,1.f };
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glGenTextures(1, &keyboardTex);
    glBindTexture(GL_TEXTURE_2D, keyboardTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 350, 1, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    loadShaders();
    
    currentRenderW = 1280;
    currentRenderH = 720;
    
    fboA[0].init(currentRenderW, currentRenderH); fboA[1].init(currentRenderW, currentRenderH);
    fboB[0].init(currentRenderW, currentRenderH); fboB[1].init(currentRenderW, currentRenderH);
    fboC.init(currentRenderW, currentRenderH); fboD.init(currentRenderW, currentRenderH);
    
    auto clearFBO = [](GLuint fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    };
    clearFBO(fboA[0].fbo); clearFBO(fboA[1].fbo);
    clearFBO(fboB[0].fbo); clearFBO(fboB[1].fbo);
    clearFBO(fboC.fbo); clearFBO(fboD.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::Resize(int windowW, int windowH, float renderScale) {
    int targetW = (int)(windowW * renderScale);
    int targetH = (int)(windowH * renderScale);
    if (targetW > 0 && targetH > 0 && (targetW != currentRenderW || targetH != currentRenderH)) {
        
        float camState0[6 * 4] = {0};
        float camState1[6 * 4] = {0};
        bool hasState = (currentRenderW >= 6 && fboB[0].fbo != 0);
        if (hasState) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, fboB[0].fbo);
            glReadPixels(currentRenderW - 6, 0, 6, 1, GL_RGBA, GL_FLOAT, camState0);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, fboB[1].fbo);
            glReadPixels(currentRenderW - 6, 0, 6, 1, GL_RGBA, GL_FLOAT, camState1);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        }

        currentRenderW = targetW;
        currentRenderH = targetH;
        fboA[0].init(currentRenderW, currentRenderH); fboA[1].init(currentRenderW, currentRenderH);
        fboB[0].init(currentRenderW, currentRenderH); fboB[1].init(currentRenderW, currentRenderH);
        fboC.init(currentRenderW, currentRenderH); fboD.init(currentRenderW, currentRenderH);
        
        if (hasState && currentRenderW >= 6) {
            glBindTexture(GL_TEXTURE_2D, fboB[0].tex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, currentRenderW - 6, 0, 6, 1, GL_RGBA, GL_FLOAT, camState0);
            glBindTexture(GL_TEXTURE_2D, fboB[1].tex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, currentRenderW - 6, 0, 6, 1, GL_RGBA, GL_FLOAT, camState1);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}

void Renderer::bindUniforms(GLuint prog, int w, int h, float time, float dt, int frame, SimulationState& state) {
    glUseProgram(prog);
    glUniform1f(glGetUniformLocation(prog, "g_camPitch"), 0.0f);
    glUniform1f(glGetUniformLocation(prog, "g_camYaw"), 0.0f);
    glUniform1f(glGetUniformLocation(prog, "g_camRoll"), 0.0f);
    glUniform1f(glGetUniformLocation(prog, "g_forceCamAngles"), 0.0f);
    glUniform3f(glGetUniformLocation(prog, "iResolution"), w, h, 1.0f);
    glUniform1f(glGetUniformLocation(prog, "iTime"), time);
    glUniform1f(glGetUniformLocation(prog, "iTimeDelta"), dt);
    glUniform1i(glGetUniformLocation(prog, "iFrame"), frame);
    glUniform4f(glGetUniformLocation(prog, "iMouse"), state.mouse.x, state.mouse.y, state.mouse.z, state.mouse.w);
    
    glUniform1f(glGetUniformLocation(prog, "g_iBlackHoleMassSol"), 1e7f); // fixed mass?
    glUniform1f(glGetUniformLocation(prog, "g_iSpin"), state.spin);
    glUniform1f(glGetUniformLocation(prog, "g_iQ"), state.q);
    glUniform1f(glGetUniformLocation(prog, "g_iAccretionRate"), state.accretionRate);
    glUniform1f(glGetUniformLocation(prog, "g_iBrightmut"), state.brightmut);
    glUniform1f(glGetUniformLocation(prog, "g_iDarkmut"), state.darkmut);
    glUniform1f(glGetUniformLocation(prog, "g_iReddening"), state.reddening);
    glUniform1f(glGetUniformLocation(prog, "g_iSaturation"), state.saturation);
    glUniform1f(glGetUniformLocation(prog, "g_iJetBrightmut"), state.jetBrightmut);
    glUniform1f(glGetUniformLocation(prog, "g_jetLength"), state.jetLength);
    glUniform1f(glGetUniformLocation(prog, "g_jetWidth"), state.jetWidth);
    glUniform1f(glGetUniformLocation(prog, "g_iQuality"), state.quality);
    glUniform1f(glGetUniformLocation(prog, "g_iOuterRadiusRs"), state.outerRadiusRs);
    glUniform1f(glGetUniformLocation(prog, "g_iBhSize"), state.bhSize);
    glUniform1f(glGetUniformLocation(prog, "g_diskRotSpeed"), state.diskRotSpeed);

    glUniform1i(glGetUniformLocation(prog, "g_showMap"), state.showMap ? 1 : 0);
    glUniform1i(glGetUniformLocation(prog, "g_showBackground"), state.showBackground ? 1 : 0);
    glUniform1f(glGetUniformLocation(prog, "g_stopBackground"), state.stopBackground);
    glUniform3f(glGetUniformLocation(prog, "g_iDiskColor"), state.diskColor[0], state.diskColor[1], state.diskColor[2]);

    glUniform1f(glGetUniformLocation(prog, "g_moveSpeed"), state.moveSpeed);
    glUniform1f(glGetUniformLocation(prog, "g_chromAb"), state.chromAb);
    glUniform1i(glGetUniformLocation(prog, "g_thermalMode"), state.thermalMode ? 1 : 0);
    glUniform1i(glGetUniformLocation(prog, "g_telescopeMode"), state.telescopeMode ? 1 : 0);
    glUniform1i(glGetUniformLocation(prog, "g_pixelateMode"), state.pixelateMode ? 1 : 0);

    glUniform1i(glGetUniformLocation(prog, "g_forceCamPos"), state.forceCamPos ? 1 : 0);
    glUniform3f(glGetUniformLocation(prog, "g_overridePos"), state.overridePos[0], state.overridePos[1], state.overridePos[2]);
    glUniform3f(glGetUniformLocation(prog, "g_overrideFwd"), state.overrideFwd[0], state.overrideFwd[1], state.overrideFwd[2]);
    glUniform3f(glGetUniformLocation(prog, "g_overrideRight"), state.overrideRight[0], state.overrideRight[1], state.overrideRight[2]);
    glUniform3f(glGetUniformLocation(prog, "g_overrideUp"), state.overrideUp[0], state.overrideUp[1], state.overrideUp[2]);
    
    glUniform1i(glGetUniformLocation(prog, "iChannel0"), 0);
    glUniform1i(glGetUniformLocation(prog, "iChannel1"), 1);
    glUniform1i(glGetUniformLocation(prog, "iChannel2"), 2);
    glUniform1i(glGetUniformLocation(prog, "iChannel3"), 3);
}

void Renderer::bindTexture(GLuint prog, int channel, GLuint tex, int w, int h) {
    glActiveTexture(GL_TEXTURE0 + channel);
    glBindTexture(GL_TEXTURE_2D, tex);
    std::string resName = "iChannelResolution[" + std::to_string(channel) + "]";
    glUniform3f(glGetUniformLocation(prog, resName.c_str()), w, h, 1.0f);
}

void Renderer::RenderFrame(SimulationState& state, float time, float dt, int frame, uint8_t* keyboardKeys) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_2D, keyboardTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 350, 1, GL_RED, GL_UNSIGNED_BYTE, keyboardKeys);

    glViewport(0, 0, currentRenderW, currentRenderH);
    glBindVertexArray(VAO);

    // Buffer B (Camera updates)
    glBindFramebuffer(GL_FRAMEBUFFER, fboB[frame%2].fbo);
    bindUniforms(progB, currentRenderW, currentRenderH, time, dt, frame, state);
    bindTexture(progB, 0, fboA[(frame+1)%2].tex, currentRenderW, currentRenderH);
    bindTexture(progB, 1, fboB[(frame+1)%2].tex, currentRenderW, currentRenderH);
    bindTexture(progB, 3, keyboardTex, 350, 1);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Buffer A (Main Raytracer)
    glBindFramebuffer(GL_FRAMEBUFFER, fboA[frame%2].fbo);
    bindUniforms(progA, currentRenderW, currentRenderH, time, dt, frame, state);
    bindTexture(progA, 2, fboB[frame%2].tex, currentRenderW, currentRenderH);
    bindTexture(progA, 3, fboA[(frame+1)%2].tex, currentRenderW, currentRenderH);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Buffer C (Blur H)
    glBindFramebuffer(GL_FRAMEBUFFER, fboC.fbo);
    bindUniforms(progC, currentRenderW, currentRenderH, time, dt, frame, state);
    bindTexture(progC, 0, fboB[frame%2].tex, currentRenderW, currentRenderH); 
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Buffer D (Blur V)
    glBindFramebuffer(GL_FRAMEBUFFER, fboD.fbo);
    bindUniforms(progD, currentRenderW, currentRenderH, time, dt, frame, state);
    bindTexture(progD, 0, fboC.tex, currentRenderW, currentRenderH);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Image (Composite to Screen)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, state.windowWidth, state.windowHeight);
    bindUniforms(progImage, state.windowWidth, state.windowHeight, time, dt, frame, state);
    bindTexture(progImage, 0, fboA[frame%2].tex, currentRenderW, currentRenderH);
    bindTexture(progImage, 3, fboD.tex, currentRenderW, currentRenderH);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::CheckAndSaveScreenshot(SimulationState& state, float& screenshotTimer) {
    if (state.takeScreenshot) {
        std::vector<unsigned char> pixels(state.windowWidth * state.windowHeight * 4);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadBuffer(GL_BACK);
        glReadPixels(0, 0, state.windowWidth, state.windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        stbi_flip_vertically_on_write(1);
        
        static int shotCounter = 1;
        char filename[256];
        std::filesystem::create_directories("screenshoots");
        sprintf(filename, "screenshoots/screenshot_%d.png", shotCounter++);
        stbi_write_png(filename, state.windowWidth, state.windowHeight, 4, pixels.data(), state.windowWidth * 4);
        
        state.takeScreenshot = false;
        screenshotTimer = 3.0f;
    }
}

void Renderer::CheckAndRecordVideo(SimulationState& state, float dt) {
    if (state.recording) {
        wasRecording = true;
        recordAccum += dt;
        if (recordAccum > 1.0f / 30.0f) {
            recordAccum -= 1.0f / 30.0f;
                if (!ffmpegPipe) {
                    recordWidth = state.windowWidth & ~1;
                    recordHeight = state.windowHeight & ~1;
                    std::filesystem::create_directories("video");
                int vidIdx = 1;
                char outFilename[256];
                while (true) {
                    sprintf(outFilename, "video/video%d.mp4", vidIdx);
                    if (!std::filesystem::exists(outFilename)) break;
                    vidIdx++;
                }
                strcpy(state.lastSavedVideo, outFilename);
                char cmd[512];
                sprintf(cmd, "ffmpeg -y -f rawvideo -vcodec rawvideo -s %dx%d -pix_fmt rgba -r 30 -i - -c:v libx264 -preset ultrafast -pix_fmt yuv420p \"%s\"", recordWidth, recordHeight, outFilename);
#ifdef _WIN32
                ffmpegPipe = _popen(cmd, "wb");
#else
                ffmpegPipe = popen(cmd, "w");
#endif
                std::cout << "Started recording to " << outFilename << "...\n";
            }
            
            std::vector<unsigned char> pixels(recordWidth * recordHeight * 4);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glPixelStorei(GL_PACK_ROW_LENGTH, 0); // PREVENT ImGui stride bugs
            glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
            glPixelStorei(GL_PACK_SKIP_ROWS, 0);
            glReadBuffer(GL_BACK);
            glReadPixels(0, 0, recordWidth, recordHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            
            if (ffmpegPipe) {
                int rowSize = recordWidth * 4;
                std::vector<unsigned char> flipped(recordWidth * recordHeight * 4);
                for (int i = 0; i < recordHeight; ++i) {
                    memcpy(&flipped[i * rowSize], &pixels[(recordHeight - 1 - i) * rowSize], rowSize);
                }
                fwrite(flipped.data(), 1, flipped.size(), ffmpegPipe);
            }
        }
    } else {
        if (wasRecording) {
            wasRecording = false;
            if (ffmpegPipe) {
                std::cout << "Saving MP4 video (flushing pipe)...\n";
#ifdef _WIN32
                _pclose(ffmpegPipe);
#else
                pclose(ffmpegPipe);
#endif
                ffmpegPipe = nullptr;
                state.videoSavedTimer = 3.0f;
                std::cout << "Video saved successfully to " << state.lastSavedVideo << "!\n";
            }
        }
    }
}
