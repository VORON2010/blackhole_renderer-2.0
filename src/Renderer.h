#pragma once

#include <glad/glad.h>
#include "State.h"
#include <string>
#include <stdio.h>

struct FBO {
    GLuint fbo = 0;
    GLuint tex = 0;
    int w = 0, h = 0;
    void destroy();
    void init(int width, int height);
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    void Init();
    void Resize(int windowW, int windowH, float renderScale);
    void RenderFrame(SimulationState& state, float time, float dt, int frame, uint8_t* keyboardKeys);
    
    void CheckAndSaveScreenshot(SimulationState& state, float& screenshotTimer);
    void CheckAndRecordVideo(SimulationState& state, float dt);
    
    GLuint GetCurrentCameraFBO(int frame) { return fboB[(frame+1)%2].fbo; }
    
private:
    void bindUniforms(GLuint prog, int w, int h, float time, float dt, int frame, SimulationState& state);
    void bindTexture(GLuint prog, int channel, GLuint tex, int w, int h);
    void removeDefines(std::string& code);
    void loadShaders();
    
    GLuint progA, progB, progC, progD, progImage;
    FBO fboA[2], fboB[2], fboC, fboD;
    
    int currentRenderW = 0;
    int currentRenderH = 0;
    
    GLuint VAO, VBO;
    GLuint keyboardTex;
    
    float recordAccum = 0.0f;
    bool wasRecording = false;
    FILE* ffmpegPipe = nullptr;
    int recordWidth = 0;
    int recordHeight = 0;
};
