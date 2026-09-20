#pragma once

#include "State.h"
#include "Renderer.h"
#include "UIManager.h"
#include "Cinematic.h"

struct GLFWwindow;

class Application {
public:
    Application();
    ~Application();

    bool Init();
    void Run();
    
private:
    static void keyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods);
    void keyCallback(int key, int action);
    void UpdateInput();

    GLFWwindow* window = nullptr;
    SimulationState state;
    Renderer renderer;
    UIManager ui;
    CinematicManager cinematic;
    
    float time = 0.0f;
    int frame = 0;
    float screenshotTimer = 0.0f;
};
