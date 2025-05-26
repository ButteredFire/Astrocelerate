#include "ReferenceFrameSystem.hpp"


ReferenceFrameSystem::ReferenceFrameSystem() {
	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void ReferenceFrameSystem::updateAllFrames() {
	static bool treeSorted = false;
	
	if (!treeSorted) {
		sortFrameTree();
		treeSorted = true;
	}

	for (auto& [entity, frame] : m_referenceFrames) {
		frame->globalTransform = computeGlobalTransform(entity, *frame);
		m_registry->updateComponent(entity, *frame);
	}
}


Component::Transform ReferenceFrameSystem::computeGlobalTransform(EntityID entityID, Component::ReferenceFrame& frame) {
	Component::Transform globalTransform;

	// If frame is the root frame, set global transform to local transform
	if (!frame.parentID.has_value()) {
		globalTransform.position = frame.localTransform.position;
		globalTransform.rotation = frame.localTransform.rotation;

		return globalTransform;
	}

	// Else, recursively compute the frame's global transform (assuming parent's global transform was already computed)
	const Component::ReferenceFrame* parent = &m_registry->getComponent<Component::ReferenceFrame>(frame.parentID.value());

		// Order: Scale -> Rotate -> Translate
	glm::dvec3 rotatedPosition = parent->globalTransform.rotation * frame.localTransform.position;

	globalTransform.position = parent->globalTransform.position + rotatedPosition;
	globalTransform.rotation = glm::normalize(parent->globalTransform.rotation * frame.localTransform.rotation);  // NOTE: Quaternion multiplication is not commutative;; It is good practice to use normalize() after quaternion multiplication to prevent drift


	return globalTransform;
}


void ReferenceFrameSystem::sortFrameTree() {
	// sortFrameTree traverses the reference frame tree depth-first.

	m_referenceFrames.clear();

	auto view = m_registry->getView<Component::ReferenceFrame>();

	std::unordered_map<EntityID, Component::ReferenceFrame*> frameMap;
	std::unordered_map<Component::ReferenceFrame*, EntityID> reverseFrameMap;

	for (auto&& [entity, _] : view) {
		Component::ReferenceFrame* frame = &m_registry->getComponent<Component::ReferenceFrame>(entity);
		frameMap[entity] = frame;		// Alternatively: frameMap[entity] = std::addressof(frame);
		reverseFrameMap[frame] = entity;
	}


	// Recursive DFS-like search and topological sort
	std::unordered_set<EntityID> visitedEntities;
	std::unordered_set<EntityID> recursionStack; // Used to detect cyclic dependencies

	std::function<void(EntityID)> visit = [&](EntityID entity) {
		// If a cyclic dependency is detected
		if (recursionStack.contains(entity)) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to sort reference frame tree due to a cyclic dependency!\nEntry node of the cycle has entity ID #" + std::to_string(entity) + ".");
		}


		// Prematurely terminate search if entity already has parent
		if (visitedEntities.contains(entity))
			return;


		// Else, find the parent of the current entity and push its frame after visiting the parent.
		visitedEntities.insert(entity);
		recursionStack.insert(entity);

		auto* frame = frameMap[entity];

		if (frame->parentID.has_value()) {
			auto it = reverseFrameMap.find(
				&m_registry->getComponent<Component::ReferenceFrame>(frame->parentID.value())
			);

			if (it != reverseFrameMap.end())
				visit(it->second);
		}

		recursionStack.erase(entity);

		m_referenceFrames.push_back(std::make_pair(entity, frame));
	};


	// Visit all frames
	for (auto&& [entity, frame] : frameMap) {
		visit(entity);
	}
}
