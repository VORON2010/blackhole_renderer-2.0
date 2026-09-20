#pragma once

#include <vector>

struct Keyframe {
    float pos[3];
    float fwd[3];
    float right[3];
    float up[3];
    float pitch, yaw, roll;
    float spin, q, acc, bright, dark, redden, sat, color[3], radius, bhSize, rot, jetBright, jetLength, jetWidth;
    float time;
};

struct vec4 { float x,y,z,w; };

struct SimulationState {
    bool showSettings = true;
    bool showMap = false;
    bool showBackground = true;
    bool showMonitor = false; bool captureImGui = false;
    
    bool takeScreenshot = false;
    bool recording = false;
    
    float diskColor[3] = {1.0f, 1.0f, 1.0f};
    float moveSpeed = 10.0f;
    float chromAb = 0.0f;
    bool thermalMode = false;
    bool telescopeMode = false;
    bool pixelateMode = false;

    float renderScale = 1.0f;
    float quality = 1.0f;
    
    float spin = 0.997f;
    float q = 0.0f;
    float accretionRate = 5e-4f;
    float brightmut = 1.0f;
    float darkmut = 0.5f;
    float reddening = 0.3f;
    float saturation = 0.5f;
    float jetBrightmut = 1.0f;
    float jetLength = 1.0f;
    float jetWidth = 1.0f;
    float outerRadiusRs = 20.0f;
    char lastSavedVideo[256] = " \; float videoSavedTimer = 0.0f; float bhSize = 1.0f;
    float diskRotSpeed = 1.0f;
    float stopBackground = 0.0f;

    // Cinematic overrides
    bool forceCamPos = false;
    float overridePos[3] = {0,0,0};
    float overrideFwd[3] = {0,0,-1};
    float overrideRight[3] = {1,0,0};
    float overrideUp[3] = {0,1,0};

    // Viewport
    int windowWidth = 1280;
    int windowHeight = 720;
    vec4 mouse = {0.0f, 0.0f, -1.0f, -1.0f};
    
    // Keyframes
    std::vector<Keyframe> keyframes;
    bool playCinematic = false;
    float cinematicTime = 0.0f;
    float cinematicSpeed = 1.0f;
    
    // Recording features
    bool offlineRender = true; // Force 100% internal resolution & fixed 30fps while recording
};
