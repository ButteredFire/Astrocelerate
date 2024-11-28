#include "Shaders.h"
#include "Constants.h"
#include "LoggingUtils.h"
#include <GL/glew.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <string.h>
#include <vector>
#include <map>


std::map<std::string, ShaderProperties> shaderRegistry = {
    {"Vertex", {0, GL_VERTEX_SHADER}},
    {"Fragment", {1, GL_FRAGMENT_SHADER}}
};

ShaderSources parseShaderFile(const std::string& filePath) {
    // Opens shader file as a stream
    std::ifstream stream(filePath);
    std::string line;
    
    if (!stream.is_open()) {
        logError(Error::CANNOT_PARSE_SHADER_FILE, "Cannot parse shader file " + quote(filePath) + '!');
    }

    int numOfShaders = shaderRegistry.size();
    std::vector<std::stringstream> ss(numOfShaders);
    unsigned int ssShaderIndex = 0;

    std::map<int, std::string> ssIndexToShaderName;
    std::map<std::string, std::string> shaderSources;

    // Parse shader fule 
    while (std::getline(stream, line)) {

        // If the line contains the keyword "#shader"
        // (a.k.a. if the line containing the keyword "#shader" is not *not* found (not std::string::npos)
        if (line.find("#shader") != std::string::npos) {
            for (auto map : shaderRegistry) {
                // map.second refers to the second element of each map entry, aka std::pair<...>
                if (line.find(map.first) != std::string::npos) {
                    ssShaderIndex = map.second.stringstreamIndex;
                    ssIndexToShaderName[ssShaderIndex] = map.first;
                }

            }
        }
        else {
            ss[ssShaderIndex] << line << '\n';
        }
    }

    for (int i = 0; i < numOfShaders; i++) {
        shaderSources[ssIndexToShaderName[i]] = ss[i].str();
    }

    return shaderSources;
}


static unsigned int compileShader(unsigned int shaderType, const std::string& srcCode, ShaderSources& sources) {
    unsigned int shader = glCreateShader(shaderType);
    const char* src = srcCode.c_str(); // fun fact: srcCode.c_str(); is equal to &srcCode[0];  (memory address of the first character in srcCode)
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // Handle potential syntax errors in srcCode and shader invalidity
    int compileStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
        int errMsgLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &errMsgLength);

        char* errMsg = (char*) alloca(errMsgLength * sizeof(char));
        glGetShaderInfoLog(shader, errMsgLength, &errMsgLength, errMsg);

        // Use a lookup to check whether shader name exists in the shader registry (more efficient than simply looping through `sources`)
        auto it = std::find_if(sources.begin(), sources.end(), [&](const auto& shd) {
            return shaderRegistry[shd.first].glConst == shaderType;
        });
        if (it != sources.end()) // if the condition above (return ...;) is true
            logError(Error::CANNOT_COMPILE_SHADER, "Failed to compile " + it->first + " shader!");

        // Ensure that errMsg is not null before logging
        if (errMsg) logError(Error::CANNOT_COMPILE_SHADER, errMsg);

        return 0;
    }

    return shader;
}


unsigned int createShader(ShaderSources sources) {
    unsigned int program = glCreateProgram();

    int numOfShaders = sources.size();
    std::vector<unsigned int> compiledShaders;

    
    for (auto shaderSource : sources) {
        std::string name = shaderSource.first;
        std::string src = shaderSource.second;

        // Check whether the provided shaders exist in the shader registry
        if (shaderRegistry.find(name) == shaderRegistry.end())
            logError(Error::UNKNOWN_SHADER, "Provided shader " +
                quote(name) + " is not found in shader registry, and so may not be properly compiled.", true);

        // Check whether source and name are empty
        if (src.empty()) {
            if (name.empty()) {
                logError(Error::CANNOT_COMPILE_SHADER, "Cannot create an unidentified shader!");
                return 0; // return x (x >= 0) to return an unusable program ID
            }
            logError(Error::CANNOT_COMPILE_SHADER, "Cannot identify source code for " + name + " shader!");
            return 1;
        }

        unsigned int shader = compileShader(shaderRegistry[name].glConst, src, sources);
        glAttachShader(program, shader);
        compiledShaders.push_back(shader);
    }

    glLinkProgram(program);
    glValidateProgram(program);

    // Delete shaders as by now they will have been linked into a program
    // and thus they are safe to delete
    for (auto shader : compiledShaders) {
        glDeleteShader(shader);
    }

    return program;
}