/* Camera.hpp - Camera implementation.
*/

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm_config.hpp>

#include <Core/Constants.h>
#include <Core/LoggingManager.hpp>

#include <CoreStructs/Input.hpp>

#include <Engine/Components/WorldSpaceComponents.hpp>

#include <Systems/Time.hpp>


class InputManager;

class Camera {
public:
	Camera(GLFWwindow* window, glm::vec3 position, glm::quat orientation);
	~Camera() = default;

	friend class InputManager;

	/* Gets the camera's view matrix. */
	glm::mat4 getViewMatrix() const;


	/* Gets the camera's transform in global simualtion space. */
	Component::Transform getGlobalTransform() const;


	float movementSpeed = 20.0f;			// Movement speed in simulation space (m/s)
	float mouseSensitivity = 0.1f;			// Mouse sensitivity
	float zoom = 60.0f;						// Zoom (degrees)

private:
	GLFWwindow* m_window;

	const glm::vec3 m_worldUp = SimulationConsts::UP_AXIS;

	glm::vec3 m_position;
	glm::quat m_orientation;

	float m_pitch = 0.0f;  // Used for clamping mouse pitch

	glm::vec3 m_front{};
	glm::vec3 m_localUp{};
	glm::vec3 m_right{};


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