/* Camera.hpp - Camera implementation.
*/

#pragma once

#include <External/GLFWVulkan.hpp>
#include <External/GLM.hpp>

#include <Core/Data/Constants.h>
#include <Core/Application/LoggingManager.hpp>
#include <Core/Engine/ECS.hpp>

#include <Core/Data/Input.hpp>

#include <Engine/Components/CommonComponents.hpp>
#include <Engine/Components/PhysicsComponents.hpp>

#include <Utils/SpaceUtils.hpp>

#include <Simulation/Systems/Time.hpp>


class InputManager;

class Camera {
public:
	Camera(Entity self, GLFWwindow* window, glm::dvec3 position, glm::quat orientation);
	~Camera() = default;

	friend class InputManager;

	/* Gets the camera's view matrix in render space. */
	glm::mat4 getRenderSpaceViewMatrix() const;


	/* Gets the camera's transform in global simulation space. */
	CommonComponent::Transform getGlobalTransform() const;


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

	const glm::vec3 m_worldUp = SimulationConsts::UP_AXIS;
	glm::dvec3 m_position;
	glm::quat m_orientation;

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
	float m_freeFlyPitch = 0.0f;

	float m_orbitRadius = 5.0f;									// Distance from the entity's center
	float m_orbitYaw = 0.0f;									// Current horizontal angle around the entity (radians)
	float m_orbitPitch = glm::radians(20.0f);					// Current vertical angle around the entity (radians)
	glm::vec3 m_attachmentOffset = glm::vec3(0.0, 2.0, 0.0);	// Offset from entity's origin


	void update();


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