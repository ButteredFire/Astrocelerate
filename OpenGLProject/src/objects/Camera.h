#pragma once

#ifndef CAMERA_H
#define CAMERA_H

#include "../rendering/Shader.h"
#include "../utils/KeyboardUtils.h"

#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

class Camera {
private:
	// Camera position and orientation
	glm::vec3 m_Position;
	glm::vec3 m_Orientation = glm::vec3(0.0f, 0.0f, 1.0f);
		// A vector that defines the camera's "up" direction (used to establish its orientation)
	glm::vec3 m_UpDirection = glm::vec3(0.0f, 1.0f, 0.0f);

	// Shader
	Shader m_Shader;
	const char* m_PerspectiveUniformLoc;
	const char* m_ProjectionUniformLoc;

	// Camera properties
	int m_Width;
	int m_Height;

	float m_zNear = 0.1f;
	float m_zFar = 100.0f;

	float m_DefaultSpeed = 0.01f;
	float m_SprintSpeed = 0.1f;
	float m_CurrentSpeed = m_DefaultSpeed;
	float m_Sensitivity = 100.0f;
	float m_FOVdeg = 45.0f;

public:
	Camera(int width, int height, const glm::vec3& position, Shader& shader);
	~Camera();

	void configurePerspective(float FOVdeg, float zNear, float zFar, const char* perspectiveUniformLoc);
	void configureProjection(const char* projectionUniformLoc);

	void updateMatrices();

	void updateInputs(GLFWwindow* window);
};

#endif