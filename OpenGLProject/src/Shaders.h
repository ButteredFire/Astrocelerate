#pragma once

#ifndef SHADERS_H
#define SHADERS_H

#include <GL/glew.h>

#include <iostream>
#include <string.h>
#include <vector>
#include <map>

struct ShaderProperties {
	unsigned int stringstreamIndex;
	unsigned int glConst;
};

class Shader {
private:
	// Represent a map of shader names to their source code
	typedef std::map<std::string, std::string> m_T_ShaderSources;

	// List of shaders and their corresponding properties
	std::map<std::string, ShaderProperties> m_ShaderRegistry = {
		{"Vertex", {0, GL_VERTEX_SHADER}},
		{"Fragment", {1, GL_FRAGMENT_SHADER}}
	};

	std::string m_FilePath;

	m_T_ShaderSources m_ShaderSources;
	unsigned int m_ShaderID;

	m_T_ShaderSources parseShaderFile();
	unsigned int compileShader(unsigned int& shaderType, const std::string& srcCode, m_T_ShaderSources& sources);
	unsigned int createShader(m_T_ShaderSources& sources);

public:
	Shader(const std::string& filePath);
	~Shader();

	void bind() const;
	void unbind() const;

	inline unsigned int getShaderID() const { return m_ShaderID; };
	inline m_T_ShaderSources getShaderSources() const { return m_ShaderSources; };
};


#endif
