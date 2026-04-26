#include "Camera.hpp"
#include <Engine/Input/InputManager.hpp> // Breaks cyclic dependency
#include <Engine/Registry/ECS/Components/TelemetryComponents.hpp>


Camera::Camera(glm::dvec3 position, glm::quat orientation):
	m_attachedEntityID(m_camEntity.id),
	m_defaultPosition(position),
	m_defaultOrientation(orientation),
	m_inFreeFlyMode(true) {

	m_ecsRegistry = ServiceLocator::GetService<ECSRegistry>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);


	m_config = {
		.movementSpeed = PhysicsConst::C,
		.mouseSensitivity = 0.1f,

		.zoom = 60.0f,
		.aspectRatio = 0.0f,
		.nearClipPlane = 0.01f,
		.farClipPlane = 1e8f
	};


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

	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void Camera::bindEvents() {
	static EventDispatcher::SubscriberIndex selfIndex = m_eventDispatcher->registerSubscriber<Camera>();

	m_eventDispatcher->subscribe<UpdateEvent::SessionStatus>(selfIndex,
		[this](const UpdateEvent::SessionStatus &event) {
			using enum UpdateEvent::SessionStatus::Status;

			switch (event.sessionStatus) {
			case PREPARE_FOR_INIT:
				reset();
				break;
			}
		}
	);


	m_eventDispatcher->subscribe<UpdateEvent::ViewportSize>(selfIndex,
		[this](const UpdateEvent::ViewportSize &event) {
			m_config.aspectRatio = static_cast<float>(event.sceneDimensions.x) / static_cast<float>(event.sceneDimensions.y);
		}
	);
}


void Camera::reset() {
	m_camEntity = m_ecsRegistry->createEntity("Camera");

	m_position = m_defaultPosition;
	m_orientation = m_defaultOrientation;

	m_inFreeFlyMode = true;
	m_revertPosition = false;
	m_attachedEntityID = m_camEntity.id;
}


void Camera::setOrbitRadii(EntityID orbitEntityID) {
	CoreComponent::Transform &entityTransform = m_ecsRegistry->getComponent<CoreComponent::Transform>(orbitEntityID);

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
	if (!m_inFreeFlyMode) {
		// Update camera's orbital position if currently attached to an entity
		
		CoreComponent::Transform &entityTransform = m_ecsRegistry->getComponent<CoreComponent::Transform>(m_attachedEntityID);
		m_orbitedEntityPosition = entityTransform.position;
		

		// Interpolate entity positions between now and the time of the last physics update.
		// Explanation: While physics updates happen at a fixed time step (e.g., 60 Hz), rendering is uncapped. This can result in jittery movements of entities that are especially noticeable in atttached/orbital mode. To fix this, we must interpolate the entity positions between the two time points for smoothness.
		if (!m_orbitedEntityLastPosition.count(m_attachedEntityID))
			m_orbitedEntityLastPosition[m_attachedEntityID] = m_orbitedEntityPosition;

		glm::dvec3 interpolatedEntityPosition;

		if (Time::GetTimeScale() > 0.0) {
			// Prevents division by zero
			// IMPORTANT: Consider checking time step to check if it's zero in the future, if dynamic time steps are to be implemented
			double alpha = deltaUpdate / (SimulationConst::TIME_STEP * Time::GetTimeScale());
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
		// Else, update the camera's free-fly position

		m_orbitedEntityPosition = m_position;

		m_front = m_orientation * m_camForward;
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

	float velocity = m_config.movementSpeed * dt;

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
	float angleX = glm::radians(deltaX * m_config.mouseSensitivity);
	float angleY = glm::radians(deltaY * m_config.mouseSensitivity);

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
		static const float maxFOV = m_config.zoom;  // Set initial zoom value as upper bound to disallow zooming from exceeding original FOV
		
		m_config.zoom -= deltaY;
		if (m_config.zoom < 1.0f)
			m_config.zoom = 1.0f;
		if (m_config.zoom > maxFOV)
			m_config.zoom = maxFOV;
	}
	else {
		// Attached/Orbital mode
		const float expZoom = m_orbitRadius * 1.5f;	// Exponential zoom multiplier

		m_orbitRadius -= (deltaY * expZoom) * m_config.mouseSensitivity;
		m_orbitRadius = glm::clamp(m_orbitRadius, m_minOrbitRadius, m_maxOrbitRadius);
	}
}
