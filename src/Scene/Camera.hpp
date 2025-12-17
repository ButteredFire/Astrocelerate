/* Camera.hpp - Camera implementation.
*/

#pragma once

#include <External/GLFWVulkan.hpp>
#include <External/GLM.hpp>

#include <Core/Application/LoggingManager.hpp>
#include <Core/Application/EventDispatcher.hpp>
#include <Core/Data/Constants.h>
#include <Core/Engine/ECS.hpp>

#include <Core/Data/Input.hpp>

#include <Engine/Components/CoreComponents.hpp>
#include <Engine/Components/RenderComponents.hpp>
#include <Engine/Components/PhysicsComponents.hpp>

#include <Utils/SpaceUtils.hpp>

#include <Simulation/Systems/Time.hpp>


class InputManager;

class Camera {
public:
	Camera(glm::dvec3 position, glm::quat orientation);
	~Camera() = default;

	friend class InputManager;


	/* Updates the camera per frame.
		@param deltaUpdate (Default: 0): The time difference between now and the most recent physics update (for linear movement interpolation).
	*/
	void tick(double deltaUpdate = 0);


	/* Translates a GLFW key to a camera movement direction. */
	inline Input::CameraMovement glfwKeyToMovement(int key) { return m_keyToCamMovementBindings.at(key); }


	/* Gets the camera's view matrix in render space. */
	glm::mat4 getRenderSpaceViewMatrix() const;


	/* Gets the camera's relative transform in simulation space.
		This transform is relative, meaning that it is relative to a floating origin.
	*/
	CoreComponent::Transform getRelativeTransform() const;


	/* Gets the camera's absolute transform in simulation space.
		This transform is absolute, meaning that the data represents the camera's true position in simulation space, without additional manipulations (e.g., subtraction by a floating origin).
	*/
	CoreComponent::Transform getAbsoluteTransform() const;


	/* Fixes the camera to an entity (mesh) in the scene (or its own entity ID to enable free-fly mode).
		@param entityID: The entity's ID.
	*/
	void attachToEntity(EntityID entityID);

	/* Detaches the camera from any entity and revert back to free-fly mode. */
	void detachFromEntity();

	/* Is the camera in free-fly mode (i.e., not orbiting an entity)? */
	inline bool inFreeFlyMode() const { return m_inFreeFlyMode; }

	/* Gets the camera entity. */
	inline Entity getEntity() const { return m_camEntity; }

	/* Gets the orbited entity's position. */
	inline glm::dvec3 getOrbitedEntityPosition() const { return m_orbitedEntityPosition; }


	/* Should the camera revert back to its original free-fly position (True), or not (False)? */
	inline void revertPositionOnFreeFlySwitch(bool enabled) { m_revertPosition = enabled; }


	float movementSpeed = 1e6f;				// Movement speed in simulation space (m/s)
	float mouseSensitivity = 0.1f;			// Mouse sensitivity
	float zoom = 60.0f;						// Zoom (degrees)

private:
	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;

	std::unordered_map<int, Input::CameraMovement> m_keyToCamMovementBindings;

	// Camera orientation
	const glm::vec3 m_worldUp = SimulationConsts::UP_AXIS;
	glm::dvec3 m_position;
	glm::quat m_orientation;
	
		// Defaults
	glm::dvec3 m_defaultPosition;
	glm::quat m_defaultOrientation;

	float m_pitch = 0.0f;  // Used for clamping mouse pitch

	glm::vec3 m_front{};
	glm::vec3 m_localUp{};
	glm::vec3 m_right{};


	// Free-fly and attached controls
	Entity m_camEntity{};
	EntityID m_attachedEntityID{};

	bool m_inFreeFlyMode;
	bool m_revertPosition;				// Should the camera revert back to its saved free-fly position?		
	glm::vec3 m_freeFlyPosition;		// The camera's saved position in free-fly mode (to switch back to later)
	glm::quat m_freeFlyOrientation;

	float m_orbitRadius;	// Distance between camera and entity (in render space)

	std::unordered_map<EntityID, std::optional<glm::dvec3>> m_orbitedEntityLastPosition;		// The last positions of the entities being orbited (to perform linear interpolation)
	glm::dvec3 m_orbitedEntityPosition{};	// The position of the entity currently being orbited


	void reset();


	/* Resets the camera quaternion's roll. */
	void resetCameraQuatRoll(const glm::vec3 &forwardVector);


	/* Processes keyboard input.
		@param key: The pressed key.
		@param dt: Delta time.
	*/
	void processKeyboardInput(int key, double dt);


	/* Processes mouse input.
		@param deltaX: The x-axis offset of the mouse.
		@param deltaY: The y-axis offset of the mouse.
	*/
	void processMouseInput(float deltaX, float deltaY);


	/* Clamp pitch. */
	void clampPitch(float& angleY, const float pitchLimit);


	/* Processes mouse scroll.
		@param deltaY: The y-axis offset of the mouse.
	*/
	void processMouseScroll(float deltaY);
};