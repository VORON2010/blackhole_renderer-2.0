#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

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
        "uniform float g_jetLength;\n"
        "uniform float g_jetWidth;\n"
        "uniform float g_iQuality;\n"
        "uniform float g_iOuterRadiusRs;\n"
        "uniform float g_iBhSize;\n"
        "uniform float g_diskRotSpeed;\n"
        "uniform int g_showMap;\n"
        "uniform int g_showBackground;\n"
        "uniform float g_stopBackground;\n"
        "uniform vec3 g_iDiskColor;\n"
        "uniform float g_moveSpeed;\n"
        "uniform float g_chromAb;\n"
        "uniform int g_thermalMode;\n"
        "uniform int g_telescopeMode;\n"
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
        "#define iJetLength g_jetLength\n"
        "#define iJetWidth g_jetWidth\n"
        "#define iQuality g_iQuality\n"
        "#define iOuterRadiusRs g_iOuterRadiusRs\n"
        "#define iBhSize g_iBhSize\n";
    
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
