#include "Camera.hpp"
#include <Core/Engine/InputManager.hpp> // Breaks cyclic dependency
#include <Engine/Components/TelemetryComponents.hpp>


Camera::Camera(Entity self, GLFWwindow* window, glm::dvec3 position, glm::quat orientation):
	m_camEntity(self),
	m_attachedEntityID(m_camEntity.id),
	m_window(window),
	m_defaultPosition(position),
	m_defaultOrientation(orientation),
	m_inFreeFlyMode(true) {

	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);

	reset();
	update();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void Camera::reset() {
	m_position = m_defaultPosition;
	m_orientation = m_defaultOrientation;

	m_inFreeFlyMode = false;
	m_attachedEntityID = m_camEntity.id;
	m_orbitRadius = 2.0f;
}


void Camera::update(double physicsUpdateTimeDiff) {
	/* In a +Z-up coordinate system:
		* +Z is Up
		* -Y is Front: The negative Y-axis points forward (the direction the camera looks by default).
	*/
	static const glm::vec3 forward = glm::vec3(0.0, -1.0, 0.0);


	// Update camera's position if currently attached to an entity
	if (m_attachedEntityID != m_camEntity.id) {
		PhysicsComponent::ReferenceFrame &entityRefFrame = m_registry->getComponent<PhysicsComponent::ReferenceFrame>(m_attachedEntityID);
		glm::dvec3 entityPosition = SpaceUtils::ToSimulationSpace(entityRefFrame._computedGlobalPosition);


		// Interpolate entity positions between now and the time of the last physics update.
		// Explanation: While physics updates happen at a fixed time step (e.g., 60 Hz), rendering is uncapped. This can result in jittery movements of entities that are especially noticeable in atttached/orbital mode. To fix this, we must interpolate the entity positions between the two time points for smoothness.
		double alpha = physicsUpdateTimeDiff / SimulationConsts::TIME_STEP;

		//glm::dvec3 interpolatedEntityPosition = glm::mix(
		//	SpaceUtils::ToSimulationSpace(entityRefFrame.previousPosition),
		//	entityPosition,
		//	alpha
		//);



		// Rotate the orbit radius (offset from entity) vector using the camera's orientation
		glm::vec3 scaledOrbitRadius = SpaceUtils::ToSimulationSpace(glm::dvec3(0.0, static_cast<double>(m_orbitRadius), 0.0));	// Offset from entity's origin
		glm::dvec3 rotatedOffset = m_orientation * scaledOrbitRadius;

		m_position = entityPosition + rotatedOffset;


		// Calculate camera orientation to look at the target point
		glm::dvec3 directionToEntity = entityPosition - m_position;
		glm::vec3 lookDirection = glm::normalize(directionToEntity);


		// Derive camera transforms
		m_front = lookDirection;
		m_localUp = m_orientation * m_worldUp;
		m_right = glm::normalize(glm::cross(m_front, m_localUp));
	}
	else {
		m_front = m_orientation * forward;
		m_localUp = m_orientation * m_worldUp;
		m_right = glm::normalize(glm::cross(m_front, m_localUp));
	}
}



glm::mat4 Camera::getRenderSpaceViewMatrix() const {
	glm::vec3 descaledPosition = SpaceUtils::ToRenderSpace_Position(m_position);
	return glm::lookAt(descaledPosition, descaledPosition + m_front, m_localUp);
}


CoreComponent::Transform Camera::getGlobalTransform() const {
	CoreComponent::Transform transform{};
	transform.position = m_position;
	transform.rotation = m_orientation;
	
	return transform;
}


void Camera::attachToEntity(EntityID entityID) {
	if (m_attachedEntityID == m_camEntity.id && entityID != m_camEntity.id) {
		m_freeFlyPosition = m_position;
		m_freeFlyOrientation = m_orientation;

		m_inFreeFlyMode = false;
	}

	m_attachedEntityID = entityID;
	update(); // Forces an immediate update after changing attachment
}


void Camera::detachFromEntity() {
	if (m_attachedEntityID != m_camEntity.id) {
		m_inFreeFlyMode = true;
		m_position = m_freeFlyPosition;
		m_orientation = m_freeFlyOrientation;
	}

	m_attachedEntityID = m_camEntity.id;
	update();
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
		float angleX = glm::radians(deltaX * mouseSensitivity);
		float angleY = glm::radians(deltaY * mouseSensitivity);
		
		glm::quat yawQuat = glm::angleAxis(-angleX, m_worldUp);
		glm::quat pitchQuat = glm::angleAxis(-angleY, m_right);


		//m_orbitYaw += glm::radians(deltaX * mouseSensitivity);
		//m_orbitPitch += glm::radians(deltaY * mouseSensitivity);
		m_orientation = glm::normalize(pitchQuat * yawQuat * m_orientation);
		//
		//// Clamp orbit pitch to prevent flipping or going too far
		//m_orbitPitch = glm::clamp(m_orbitPitch, glm::radians(-70.0f), glm::radians(70.0f));
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
		static const float minDistance = 0.5f;
		static const float maxDistance = 300.0f;

		const float expZoom = m_orbitRadius * 1.5f;	// Exponential zoom multiplier

		m_orbitRadius -= (deltaY * expZoom) * mouseSensitivity;
		m_orbitRadius = glm::clamp(m_orbitRadius, minDistance, maxDistance);
	}
}
