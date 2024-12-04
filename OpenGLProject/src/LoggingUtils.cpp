#include "LoggingUtils.h"
#include "Constants.h"
#include <GL/glew.h>

#include <iostream>
#include <map>

std::map<unsigned int, std::string> debugSources = {
    {GL_DEBUG_SOURCE_API, "API"},
    {GL_DEBUG_SOURCE_WINDOW_SYSTEM, "Window System"},
    {GL_DEBUG_SOURCE_SHADER_COMPILER, "Shader Compiler"},
    {GL_DEBUG_SOURCE_THIRD_PARTY, "Third Party"},
    {GL_DEBUG_SOURCE_APPLICATION, "Application"},
    {GL_DEBUG_SOURCE_OTHER, "Other"}
};

std::map<unsigned int, std::string> debugTypes = {
    {GL_DEBUG_TYPE_ERROR, "Error"},
    {GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "Deprecated Behavior"},
    {GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "Undefined Behavior"},
    {GL_DEBUG_TYPE_PORTABILITY, "Portability"},
    {GL_DEBUG_TYPE_PERFORMANCE, "Performance"},
    {GL_DEBUG_TYPE_OTHER, "Other"}
};

std::map<unsigned int, std::string> debugSeverityLevels = {
    {GL_DEBUG_SEVERITY_HIGH, "HIGH"},
    {GL_DEBUG_SEVERITY_MEDIUM, "MEDIUM"},
    {GL_DEBUG_SEVERITY_LOW, "LOW"},
    {GL_DEBUG_SEVERITY_NOTIFICATION, "NOTIFICATION"}

};

void GLAPIENTRY openGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar * message, const void* userParam) {
    std::cerr << "[OPENGL DEBUG MESSAGE]: " << message << '\n';
    
    std::cerr << "\t-> Source: " << debugSources[source] << '\n';
    std::cerr << "\t-> Type: " << debugTypes[type] << '\n';
    std::cerr << "\t-> Severity: " << debugSeverityLevels[severity] << '\n';
}

void printAppInfo() {
    std::cout << "Application: " << Window::WINDOW_NAME << '\n';
    std::cout << "OpenGL: "
        << "\n\t" << "Version: " << glGetString(GL_VERSION)
        << "\n\t" << "Renderer: " << glGetString(GL_RENDERER)
        << '\n';
    std::cout << "\n\n";
}

void logError(int errorCode, const std::string& errorMsg, const bool isWarning) {
    std::cerr << ((isWarning) ? "[WARNING]" : "[ERROR]") << ":\t(Exit code : " << errorCode << ") " << errorMsg << '\n';
}