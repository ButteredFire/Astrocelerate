/* ReferenceFrameSystem.hpp - Implementation of the reference frame system.
	The reference frame system allows for extreme transforms without loss of precision (e.g., from micro movements of boosters on spacecraft to the Solar system).
	Conceptually, the reference frame system works under the same principle as the scene graph, but it is used in both simulation and render space, not just the latter.
*/

#pragma once

#include <algorithm>
#include <External/GLM.hpp>

#include <Core/Engine/ECS.hpp>
#include <Core/Application/LoggingManager.hpp>
#include <Core/Engine/ServiceLocator.hpp>

#include <Engine/Components/PhysicsComponents.hpp>
#include <Engine/Components/RenderComponents.hpp>
#include <Engine/Components/TelemetryComponents.hpp>

#include <Utils/SpaceUtils.hpp>


class ReferenceFrameSystem {
public:
	ReferenceFrameSystem();
	~ReferenceFrameSystem() = default;


	/* Updates all reference frames. */
	void updateAllFrames();

private:
	std::shared_ptr<Registry> m_registry;
	std::vector<std::pair<EntityID, PhysicsComponent::ReferenceFrame*>> m_referenceFrames;

	EntityID m_renderSpaceID{};

	/* Computes the absolute transforms of all reference frames. */
	void computeGlobalTransforms();

	/* Sorts the reference frame tree. This is done only once, at the start of a simulation. */
	void sortFrameTree();
};