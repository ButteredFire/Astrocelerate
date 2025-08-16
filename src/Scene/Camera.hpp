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
	Camera(GLFWwindow* window, glm::dvec3 position, glm::quat orientation);
	~Camera() = default;

	friend class InputManager;

	/* Gets the camera's view matrix in render space. */
	glm::mat4 getRenderSpaceViewMatrix() const;


	/* Gets the camera's transform in global simulation space. */
	CoreComponent::Transform getGlobalTransform() const;


	/* Fixes the camera to an entity (mesh) in the scene (or its own entity ID to enable free-fly mode).
		@param entityID: The entity's ID.
	*/
	void attachToEntity(EntityID entityID);

	/* Detaches the camera from any entity and revert back to free-fly mode. */
	void detachFromEntity();

	/* Gets the camera entity. */
	Entity getEntity() const { return m_camEntity; }


	float movementSpeed = 1e6f;				// Movement speed in simulation space (m/s)
	float mouseSensitivity = 0.1f;			// Mouse sensitivity
	float zoom = 60.0f;						// Zoom (degrees)

private:
	GLFWwindow* m_window;
	std::shared_ptr<Registry> m_registry;
	std::shared_ptr<EventDispatcher> m_eventDispatcher;


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
	glm::vec3 m_freeFlyPosition;								// The camera's saved position in free-fly mode (to switch back to later)
	glm::quat m_freeFlyOrientation;

	float m_orbitRadius;	// Distance between camera and entity (in render space)
	std::unordered_map<EntityID, std::optional<glm::dvec3>> m_orbitedEntityLastPosition;		// The last positions of the entities being orbited (to perform linear interpolation)


	void reset();


	/* Updates the camera per frame.
		@param physicsUpdateTimeDiff (Default: 0): The difference between the current time and the time of the last physics update (for linear interpolation of entity positions between the two time points).
	*/
	void update(double physicsUpdateTimeDiff = 0);

	/* Processes keyboard input.
		@param direction: The direction the camera will go in.
		@param dt: Delta time.
	*/
	void processKeyboardInput(Input::CameraMovement direction, double dt);


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