#include "Shader.h"
#include "../Constants.h"
#include "../utils/LoggingUtils.h"
#include <GL/glew.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <string.h>
#include <vector>
#include <map>


Shader::Shader(const std::string& filePath) 
    : m_FilePath(filePath) {
    m_ShaderSources = parseShaderFile();
    m_ShaderID = createShader(m_ShaderSources);
    bind();
}

Shader::~Shader() {
    glDeleteProgram(m_ShaderID);
}

void Shader::bind() const {
    if (m_ShaderID) glUseProgram(m_ShaderID);
}

void Shader::unbind() const {
    glUseProgram(0);
}


int Shader::getUniformLocation(const char* uniformName) const {
    int location =  glGetUniformLocation(m_ShaderID, uniformName);
    if (location == -1) {
        logError(Error::UNKNOWN_UNIFORM, "Uniform " + (std::string) quote(uniformName) + " does not exist.", true);
    }

    return location;
}


Shader::m_T_ShaderSources Shader::parseShaderFile() {
    const std::string& filePath = Shader::m_FilePath;
    // Opens shader file as a stream
    std::ifstream stream(filePath);
    std::string line;
    
    if (!stream.is_open()) {
        logError(Error::CANNOT_PARSE_SHADER_FILE, "Cannot parse shader file " + quote(filePath) + '!');
    }

    int numOfShaders = m_ShaderRegistry.size();
    std::vector<std::stringstream> ss(numOfShaders);
    unsigned int ssShaderIndex = 0;

    std::map<int, std::string> ssIndexToShaderName;
    std::map<std::string, std::string> shaderSources;

    // Parse shader fule 
    while (std::getline(stream, line)) {

        // If the line contains the keyword "#shader"
        // (a.k.a. if the line containing the keyword "#shader" is not *not* found (not std::string::npos)
        if (line.find("#shader") != std::string::npos) {
            for (auto& map : m_ShaderRegistry) {
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


unsigned int Shader::compileShader(unsigned int& shaderType, const std::string& srcCode, Shader::m_T_ShaderSources& sources) {
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
            return m_ShaderRegistry[shd.first].glConst == shaderType;
        });
        if (it != sources.end()) // if the condition above (return ...;) is true
            logError(Error::CANNOT_COMPILE_SHADER, "Failed to compile " + it->first + " shader!");

        // Ensure that errMsg is not null before logging
        if (errMsg) logError(Error::CANNOT_COMPILE_SHADER, errMsg);

        return 0;
    }

    return shader;
}


unsigned int Shader::createShader(Shader::m_T_ShaderSources& sources) {
    unsigned int program = glCreateProgram();

    int numOfShaders = sources.size();
    std::vector<unsigned int> compiledShaders;

    
    for (auto& shaderSource : sources) {
        std::string name = shaderSource.first;
        std::string src = shaderSource.second;

        // Check whether the provided shaders exist in the shader registry
        if (m_ShaderRegistry.find(name) == m_ShaderRegistry.end())
            logError(Error::UNKNOWN_SHADER, "Provided shader " +
                quote(name) + " is not found in shader registry, and so may not be properly compiled.", true);

        // Check whether source and name are empty
        if (src.empty()) {
            if (name.empty()) {
                logError(Error::CANNOT_COMPILE_SHADER, "Cannot create an unidentified shader!");
                return 0;
            }
            logError(Error::CANNOT_COMPILE_SHADER, "Cannot identify source code for " + name + " shader!");
            return 1;
        }

        unsigned int shader = compileShader(m_ShaderRegistry[name].glConst, src, sources);
        glAttachShader(program, shader);
        compiledShaders.push_back(shader);
    }

    glLinkProgram(program);
    glValidateProgram(program);

    // Delete shaders as by now they will have been linked into a program
    // and thus they are safe to delete
    for (auto& shader : compiledShaders)
        glDeleteShader(shader);

    return program;
}