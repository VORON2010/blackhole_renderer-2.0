#pragma once
#include "State.h"

struct GLFWwindow;

class UIManager {
public:
    void Init(GLFWwindow* window);
    void Shutdown();
    void NewFrame();
    void Render(SimulationState& state, float fps, float ramMB, float screenshotTimer, int currentFrame, unsigned int fboB_id, int renderW);
    void EndFrame();
};
