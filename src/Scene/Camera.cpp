#include "Camera.hpp"
#include <Core/InputManager.hpp> // Breaks cyclic dependency


Camera::Camera(GLFWwindow* window, glm::vec3 position, glm::quat orientation):
	m_window(window),
	m_position(position),
	m_orientation(orientation) {

	update();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void Camera::update() {
	/* In a +Z-up coordinate system:
		* +Z is Up
		* -Y is Front: The negative Y-axis points forward (the direction the camera looks by default).
	*/
	m_front = m_orientation * glm::vec3(0.0, -1.0, 0.0);
	m_localUp = m_orientation * m_worldUp;
	m_right = glm::normalize(glm::cross(m_front, m_localUp));
}



glm::mat4 Camera::getViewMatrix() const {
	return glm::lookAt(m_position, m_position + m_front, m_localUp);
}


Component::Transform Camera::getGlobalTransform() const {
	Component::Transform transform{};
	transform.position = m_position;
	transform.rotation = m_orientation;
	
	return transform;
}


void Camera::processKeyboardInput(Input::CameraMovement direction, double dt) {
	using namespace Input;

	float velocity = movementSpeed * dt;

	switch (direction) {
	case CameraMovement::FORWARD:
		m_position += m_front * velocity;
		break;

	case CameraMovement::BACKWARD:
		m_position -= m_front * velocity;
		break;

	case CameraMovement::LEFT:
		m_position -= m_right * velocity;
		break;

	case CameraMovement::RIGHT:
		m_position += m_right * velocity;
		break;

	case CameraMovement::UP:
		m_position += m_worldUp * velocity;
		break;

	case CameraMovement::DOWN:
		m_position -= m_worldUp * velocity;
		break;
	}
}


void Camera::processMouseInput(float deltaX, float deltaY) {
	float angleX = glm::radians(deltaX * mouseSensitivity);
	float angleY = glm::radians(deltaY * mouseSensitivity);

	clampPitch(angleY, 89.0f);

	glm::quat yawQuat = glm::angleAxis(-angleX, m_worldUp);
	glm::quat pitchQuat = glm::angleAxis(-angleY, m_right);

	// NOTE: Quaternion multiplication is not commutative
	m_orientation = glm::normalize(pitchQuat * yawQuat * m_orientation);

	update();
}


void Camera::clampPitch(float& angleY, const float pitchLimit) {
	float newPitch = m_pitch + glm::degrees(-angleY);

	if (newPitch > pitchLimit) {
		angleY = -1 * glm::radians(pitchLimit - m_pitch);
		m_pitch = pitchLimit;
		return;
	}

	if (newPitch < -pitchLimit) {
		angleY = -1 * glm::radians(-pitchLimit - m_pitch);
		m_pitch = -pitchLimit;
		return;
	}

	m_pitch = newPitch;
}


void Camera::processMouseScroll(float deltaY) {
	static const float maxFOV = zoom;  // Set initial zoom value as upper bound to disallow zooming from exceeding original FOV

	zoom -= deltaY;
	if (zoom < 1.0f)
		zoom = 1.0f;
	if (zoom > maxFOV)
		zoom = maxFOV;
}
