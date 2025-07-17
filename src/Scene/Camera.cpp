#include "Camera.hpp"
#include <Core/Engine/InputManager.hpp> // Breaks cyclic dependency


Camera::Camera(Entity self, GLFWwindow* window, glm::dvec3 position, glm::quat orientation):
	m_camEntity(self),
	m_window(window),
	m_position(position),
	m_orientation(orientation) {

	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);

	update();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void Camera::update() {
	/* In a +Z-up coordinate system:
		* +Z is Up
		* -Y is Front: The negative Y-axis points forward (the direction the camera looks by default).
	*/


	// Update camera's position if currently attached to an entity
	static bool inFreeFlyMode = true;

	if (m_attachedEntityID != m_camEntity.id) {
		if (inFreeFlyMode) {
			m_freeFlyPosition = m_position;
			inFreeFlyMode = false;
		}

		PhysicsComponent::ReferenceFrame &entityRefFrame = m_registry->getComponent<PhysicsComponent::ReferenceFrame>(m_attachedEntityID);

		glm::dvec3 &entityPosition = entityRefFrame.globalTransform.position;
		glm::dvec3 camOffsetPos = entityPosition + SpaceUtils::ToSimulationSpace(glm::dvec3(m_attachmentOffset));
		
		// Compute camera position based on orbit angles and radius
		glm::dvec3 orbitalPos{};
		orbitalPos.x = camOffsetPos.x + m_orbitRadius * std::cos(m_orbitPitch) * std::sin(m_orbitYaw);
		orbitalPos.y = camOffsetPos.y + m_orbitRadius * std::sin(m_orbitPitch);
		orbitalPos.z = camOffsetPos.z + m_orbitRadius * std::cos(m_orbitPitch) * std::cos(m_orbitYaw);

		m_position = orbitalPos;


		// Calculate camera orientation to look at the target point
		glm::vec3 lookDirection = glm::normalize(camOffsetPos - m_position);
		m_orientation = glm::quatLookAt(lookDirection, m_worldUp); // NOTE: Always use m_worldUp for the "up" vector when looking at a target

		m_front = lookDirection;
		m_localUp = m_orientation * m_worldUp;
		m_right = glm::normalize(glm::cross(m_front, m_localUp));
	}
	else {
		if (!inFreeFlyMode) {
			m_position = m_freeFlyPosition;
			inFreeFlyMode = true;
		}

		m_front = m_orientation * glm::vec3(0.0, -1.0, 0.0);
		m_localUp = m_orientation * m_worldUp;
		m_right = glm::normalize(glm::cross(m_front, m_localUp));
	}
}



glm::mat4 Camera::getRenderSpaceViewMatrix() const {
	glm::vec3 scaledPosition = SpaceUtils::ToRenderSpace_Position(m_position);
	return glm::lookAt(scaledPosition, scaledPosition + m_front, m_localUp);
}


CommonComponent::Transform Camera::getGlobalTransform() const {
	CommonComponent::Transform transform{};
	transform.position = m_position;
	transform.rotation = m_orientation;
	
	return transform;
}


void Camera::attachToEntity(EntityID entityID) {
	m_attachedEntityID = entityID;
}


void Camera::detachFromEntity() {
	m_attachedEntityID = m_camEntity.id;
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
	if (m_attachedEntityID == m_camEntity.id) {
		// Free-fly mode
		float angleX = glm::radians(deltaX * mouseSensitivity);
		float angleY = glm::radians(deltaY * mouseSensitivity);

		clampPitch(angleY, 89.0f);

		glm::quat yawQuat = glm::angleAxis(-angleX, m_worldUp);
		glm::quat pitchQuat = glm::angleAxis(-angleY, m_right);

		// NOTE: Quaternion multiplication is not commutative
		m_orientation = glm::normalize(pitchQuat * yawQuat * m_orientation);
	}
	else {
		// Attached/Orbital mode
		m_orbitYaw += glm::radians(deltaX * mouseSensitivity);
		m_orbitPitch += glm::radians(deltaY * mouseSensitivity);

		// Clamp orbit pitch to prevent flipping or going too far
		m_orbitPitch = glm::clamp(m_orbitPitch, glm::radians(-70.0f), glm::radians(70.0f));
	}

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
	if (m_attachedEntityID == m_camEntity.id) {
		// Free-fly mode
		static const float maxFOV = zoom;  // Set initial zoom value as upper bound to disallow zooming from exceeding original FOV
		
		zoom -= deltaY;
		if (zoom < 1.0f)
			zoom = 1.0f;
		if (zoom > maxFOV)
			zoom = maxFOV;
	}
	else {
		// Attached/Orbital mode
		static const float minDistance = 1.0f;
		static const float maxDistance = 50.0f;

		m_orbitRadius -= deltaY * mouseSensitivity;
		m_orbitRadius = glm::clamp(m_orbitRadius, minDistance, maxDistance);
	}
}
