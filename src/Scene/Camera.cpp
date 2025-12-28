#include "Camera.hpp"
#include <Core/Engine/InputManager.hpp> // Breaks cyclic dependency
#include <Engine/Components/TelemetryComponents.hpp>


Camera::Camera(glm::dvec3 position, glm::quat orientation):
	m_attachedEntityID(m_camEntity.id),
	m_defaultPosition(position),
	m_defaultOrientation(orientation),
	m_inFreeFlyMode(true) {

	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);

	m_keyToCamMovementBindings = {
		{GLFW_KEY_W,					Input::CameraMovement::FORWARD},
		{GLFW_KEY_S,					Input::CameraMovement::BACKWARD},
		{GLFW_KEY_A,					Input::CameraMovement::LEFT},
		{GLFW_KEY_D,					Input::CameraMovement::RIGHT},
		{GLFW_KEY_E,					Input::CameraMovement::UP},
		{GLFW_KEY_Q,					Input::CameraMovement::DOWN}
	};

	reset();
	tick();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void Camera::reset() {
	m_camEntity = m_registry->createEntity("Camera");

	m_position = m_defaultPosition;
	m_orientation = m_defaultOrientation;

	m_inFreeFlyMode = true;
	m_revertPosition = false;
	m_attachedEntityID = m_camEntity.id;
}


void Camera::resetCameraQuatRoll(const glm::vec3 &forwardVector) {
	// TODO: Fix logic
	// Projects the camera forward vector onto the horizontal (X-Y) plane.
	glm::vec3 horizontalForward = glm::normalize(glm::vec3(forwardVector.x, forwardVector.y, 0.0f));

	// If the horizontal forward vector is near zero (e.g., looking straight up or down),
	// use the original forward vector to prevent a singularity.
	if (glm::length(horizontalForward) < 0.0001f) {
		m_orientation = glm::quatLookAt(forwardVector, m_worldUp);
	}
	else {
		// Reconstruct the quaternion without roll
		m_orientation = glm::quatLookAt(horizontalForward, m_worldUp);
	}

	/*
	glm::vec3 cameraForwardVector = m_orientation * forwardVector;

	// Projects the camera forward vector onto the horizontal (X-Y) plane (because our up axis is Z).
	// This zeros out the Z-component, which effectively removes roll.
	glm::vec3 horizontalForward = glm::normalize(glm::vec3(cameraForwardVector.x, cameraForwardVector.y, 0.0f));

	if (glm::length(horizontalForward) < 0.0001f)
		// If the horizontal forward vector is (0, 0, 0), use a default orientation
		m_orientation = glm::quatLookAt(forwardVector, m_worldUp);
	else
		m_orientation = glm::quatLookAt(horizontalForward, m_worldUp);
	*/
}


void Camera::setOrbitRadii(EntityID orbitEntityID) {
	CoreComponent::Transform &entityTransform = m_registry->getComponent<CoreComponent::Transform>(orbitEntityID);

	double entityRenderScale = SpaceUtils::ToRenderSpace_Scale(entityTransform.scale);
	static const float fixedMinRadius = 0.2f;
	static const float fixedMaxRadius = 5000.0f;
	static const float initialDistanceMult = 3.0f;

	// NOTE: Orbit radius determination formulas below are based on fine-tuned adjustments specific to how Astrocelerate handles object scaling. Thus, they are arbitrary and have no real mathematical basis.

	m_minOrbitRadius = std::fmax(fixedMinRadius, SpaceUtils::GetRenderableScale(entityRenderScale) * (1 + fixedMinRadius));
	m_maxOrbitRadius = std::fmax(fixedMaxRadius, entityRenderScale * fixedMaxRadius);

	m_orbitRadius = std::fmax(m_minOrbitRadius, entityRenderScale * initialDistanceMult);
}


void Camera::tick(double deltaUpdate) {
	/* In a +Z-up coordinate system:
		* +Z is Up
		* -Y is Front: The negative Y-axis points forward (the direction the camera looks by default).
	*/
	static const glm::vec3 forward = glm::vec3(0.0, -1.0, 0.0);


	// Update camera's position if currently attached to an entity
	if (!m_inFreeFlyMode) {
		CoreComponent::Transform &entityTransform = m_registry->getComponent<CoreComponent::Transform>(m_attachedEntityID);
		m_orbitedEntityPosition = entityTransform.position;
		

		// Interpolate entity positions between now and the time of the last physics update.
		// Explanation: While physics updates happen at a fixed time step (e.g., 60 Hz), rendering is uncapped. This can result in jittery movements of entities that are especially noticeable in atttached/orbital mode. To fix this, we must interpolate the entity positions between the two time points for smoothness.
		if (!m_orbitedEntityLastPosition.count(m_attachedEntityID))
			m_orbitedEntityLastPosition[m_attachedEntityID] = m_orbitedEntityPosition;

		glm::dvec3 interpolatedEntityPosition;

		if (Time::GetTimeScale() > 0.0) {
			// Prevents division by zero
			// IMPORTANT: Consider checking time step to check if it's zero in the future, if dynamic time steps are to be implemented
			double alpha = deltaUpdate / (SimulationConsts::TIME_STEP * Time::GetTimeScale());
			alpha = glm::clamp(alpha, 0.0, 1.0);

			interpolatedEntityPosition = glm::mix(
				m_orbitedEntityLastPosition[m_attachedEntityID].value(),
				m_orbitedEntityPosition,
				alpha
			);

			m_orbitedEntityLastPosition[m_attachedEntityID] = m_orbitedEntityPosition;
		}
		else
			interpolatedEntityPosition = m_orbitedEntityPosition;


		// Rotate the orbit radius (offset from entity) vector using the camera's orientation
		glm::vec3 scaledOrbitRadius = SpaceUtils::ToSimulationSpace(glm::dvec3(0.0, static_cast<double>(m_orbitRadius), 0.0));	// Offset from entity's origin
		glm::dvec3 rotatedOffset = m_orientation * scaledOrbitRadius;

		m_position = interpolatedEntityPosition + rotatedOffset;


		// Calculate camera orientation to look at the target point
		glm::dvec3 directionToEntity = m_orbitedEntityPosition - m_position;
		glm::vec3 lookDirection = glm::normalize(directionToEntity);


		// Derive camera transforms
		m_front = lookDirection;
		m_localUp = m_orientation * m_worldUp;
		m_right = glm::normalize(glm::cross(m_front, m_localUp));
	}
	else {
		m_orbitedEntityPosition = m_position;

		m_front = m_orientation * forward;
		m_localUp = m_orientation * m_worldUp;
		m_right = glm::normalize(glm::cross(m_front, m_localUp));
	}
}



glm::mat4 Camera::getRenderSpaceViewMatrix() const {
	glm::vec3 descaledPosition = SpaceUtils::ToRenderSpace_Position(m_position - m_orbitedEntityPosition);
	glm::vec3 descaledFront = glm::normalize(SpaceUtils::ToRenderSpace_Position(m_front));
	return glm::lookAt(descaledPosition, descaledPosition + descaledFront, m_localUp);
}


CoreComponent::Transform Camera::getRelativeTransform() const {
	CoreComponent::Transform transform{};
	transform.position = m_position - m_orbitedEntityPosition;
	transform.rotation = m_orientation;
	
	return transform;
}


CoreComponent::Transform Camera::getAbsoluteTransform() const {
	CoreComponent::Transform transform{};
	transform.position = m_position;
	transform.rotation = m_orientation;

	return transform;
}


void Camera::attachToEntity(EntityID entityID) {
	if (entityID != m_camEntity.id) {
		if (m_inFreeFlyMode) {
			m_inFreeFlyMode = false;

			m_freeFlyPosition = m_position;
			m_freeFlyOrientation = m_orientation;
		}

		setOrbitRadii(entityID);
	}
	else
		// If camera is attempting to attach to itself
		detachFromEntity();

	
	m_attachedEntityID = entityID;
	tick(); // Forces an immediate update after changing attachment
}


void Camera::detachFromEntity() {
	if (m_attachedEntityID != m_camEntity.id) {
		if (m_revertPosition) {
			// Revert the camera's back to its original state before entering orbital mode
			m_position = m_freeFlyPosition;
			m_orientation = m_freeFlyOrientation;
		}

		m_inFreeFlyMode = true;
	}

	m_attachedEntityID = m_camEntity.id;
	tick();
}


void Camera::processKeyboardInput(int key, double dt) {
	using namespace Input;

	if (m_keyToCamMovementBindings.find(key) == m_keyToCamMovementBindings.end())
		return;

	Input::CameraMovement direction = m_keyToCamMovementBindings[key];

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

	clampPitch(angleY, 90.0f);

	glm::quat yawQuat = glm::angleAxis(-angleX, m_worldUp);
	glm::quat pitchQuat = glm::angleAxis(-angleY, m_right);

	// NOTE: Quaternion multiplication is not commutative
	m_orientation = glm::normalize(pitchQuat * yawQuat * m_orientation);


	tick();
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
	if (m_inFreeFlyMode) {
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
		const float expZoom = m_orbitRadius * 1.5f;	// Exponential zoom multiplier

		m_orbitRadius -= (deltaY * expZoom) * mouseSensitivity;
		m_orbitRadius = glm::clamp(m_orbitRadius, m_minOrbitRadius, m_maxOrbitRadius);
	}
}
