#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Forces GLM to use the Vulkan [0; 1] depth range instead of OpenGL's [-1; 1]
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/epsilon.hpp>