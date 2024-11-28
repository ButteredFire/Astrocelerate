#pragma once

#ifndef LOGGING_UTILS_H
#define LOGGING_UTILS_H

#include <GL/glew.h>

#include <iostream>
#include <map>

extern std::map<unsigned int, std::string> debugSources;
extern std::map<unsigned int, std::string> debugTypes;
extern std::map<unsigned int, std::string> debugSeverityLevels;

void printAppInfo();
void GLAPIENTRY openGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
void logError(int errorCode, const std::string& errorMsg, const bool isWarning = false);
std::string quote(const std::string& str);

#endif