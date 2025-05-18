/* ReferenceFrameSystem.hpp - Implementation of the reference frame system.
	The reference frame system allows for extreme transforms without loss of precision (e.g., from micro movements of boosters on spacecraft to the Solar system).
	Conceptually, the reference frame system works under the same principle as the scene graph, but it is used in both simulation and render space, not just the latter.
*/

#pragma once

#include <glm_config.hpp>

#include <Core/ECS.hpp>
#include <Core/LoggingManager.hpp>
#include <Core/ServiceLocator.hpp>

#include <Engine/Components/WorldSpaceComponents.hpp>


class ReferenceFrameSystem {
public:
	ReferenceFrameSystem();
	~ReferenceFrameSystem() = default;


	/* Updates all reference frames. */
	void updateAllFrames();


	/* Updates the absolute transform of a reference frame. */
	void updateGlobalTransform(Component::ReferenceFrame* frame);


	/* Sorts the reference frame tree. This is done only once, at the start of a simulation. */
	void sortFrameTree();

private:
	std::shared_ptr<Registry> m_registry;

	std::vector<std::pair<EntityID, Component::ReferenceFrame*>> m_referenceFrames;
};