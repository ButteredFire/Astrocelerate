#define GLM_ENABLE_EXPERIMENTAL
#include "Camera.h"

// Keyboard manager
Keyboard keyboard;

Camera::Camera(int width, int height, const glm::vec3& position, Shader& shader): 
	m_Width(width),
	m_Height(height),
	m_Position(position),
	m_Shader(shader) {}

Camera::~Camera() {}

void Camera::configurePerspective(float FOVdeg, float zNear, float zFar, const char* perspectiveUniformLoc) {
	m_FOVdeg = FOVdeg;
	m_zNear = zNear;
	m_zFar = zFar;
	m_PerspectiveUniformLoc = perspectiveUniformLoc;
}

void Camera::configureProjection(const char* projectionUniformLoc) {
	m_ProjectionUniformLoc = projectionUniformLoc;
}

void Camera::updateMatrices() {
	m_Shader.bind();
	unsigned int shaderID = m_Shader.getShaderID();

	glm::mat4 projection = glm::mat4(1.0f);
	glm::mat4 perspective = glm::mat4(1.0f);

	projection = glm::perspective(glm::radians(m_FOVdeg), (float)(m_Width / m_Height), m_zNear, m_zFar);
	perspective = glm::lookAt(m_Position, m_Position + m_Orientation, m_UpDirection);

	glUniformMatrix4fv(
		m_Shader.getUniformLocation(m_ProjectionUniformLoc), 1,
		GL_FALSE, glm::value_ptr(projection)
	);

	glUniformMatrix4fv(
		m_Shader.getUniformLocation(m_PerspectiveUniformLoc), 1,
		GL_FALSE, glm::value_ptr(perspective)
	);
}

void Camera::updateInputs(GLFWwindow* window) {
	keyboard.setTargetWindow(window);

	if (keyboard.isPressed(GLFW_KEY_W)) {
		m_Position += m_CurrentSpeed * m_Orientation;
	}
	if (keyboard.isPressed(GLFW_KEY_S)) {
		m_Position += m_CurrentSpeed * -m_Orientation;
	}


	if (keyboard.isPressed(GLFW_KEY_D)) {
		m_Position += m_CurrentSpeed * glm::normalize(glm::cross(m_Orientation, m_UpDirection));
	}
	if (keyboard.isPressed(GLFW_KEY_A)) {
		m_Position += m_CurrentSpeed * -glm::normalize(glm::cross(m_Orientation, m_UpDirection));
	}

	if (keyboard.isPressed(GLFW_KEY_SPACE)) {
		m_Position += m_CurrentSpeed * m_UpDirection;
	}
	if (keyboard.isPressed(GLFW_KEY_LEFT_CONTROL)) {
		m_Position += m_CurrentSpeed * -m_UpDirection;
	}

	if (keyboard.isPressed(GLFW_KEY_LEFT_SHIFT)) {
		m_CurrentSpeed = m_SprintSpeed;
	}
	else if (keyboard.isReleased(GLFW_KEY_LEFT_SHIFT)) {
		m_CurrentSpeed = m_DefaultSpeed;
	}
}
