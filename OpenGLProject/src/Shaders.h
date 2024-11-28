#pragma once

#ifndef SHADERS_H
#define SHADERS_H

#include <GL/glew.h>

#include <iostream>
#include <string.h>
#include <vector>
#include <map>

// Represent a map of shader names to their source code
typedef std::map<std::string, std::string> ShaderSources;

struct ShaderProperties {
	unsigned int stringstreamIndex;
	unsigned int glConst;
};

extern std::map<std::string, ShaderProperties> shaderRegistry;

ShaderSources parseShaderFile(const std::string& filePath);
static unsigned int compileShader(unsigned int shaderType, const std::string& srcCode, ShaderSources& sources);
unsigned int createShader(ShaderSources sources);


#endif
