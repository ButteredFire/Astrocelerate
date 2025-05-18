#include "ReferenceFrameSystem.hpp"


ReferenceFrameSystem::ReferenceFrameSystem() {
	m_registry = ServiceLocator::getService<Registry>(__FUNCTION__);

	Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void ReferenceFrameSystem::updateAllFrames() {
	static bool treeSorted = false;
	
	if (!treeSorted) {
		sortFrameTree();
		treeSorted = true;
	}

	for (auto& [entity, frame] : m_referenceFrames) {
		updateGlobalTransform(frame);
		m_registry->updateComponent(entity, *frame);
	}
}


void ReferenceFrameSystem::updateGlobalTransform(Component::ReferenceFrame* frame) {
	// If frame is the root frame, set global transform to local transform
	if (frame->parent == nullptr) {
		frame->globalPosition = frame->localPosition;
		frame->globalRotation = frame->localRotation;
		frame->globalScale = frame->localScale;

		return;
	}

	// Else, recursively compute the frame's global transform (assuming parent's global transform was already computed)
	const Component::ReferenceFrame* parent = frame->parent;
	const double parentScale = parent->globalScale;

		// Order: Scale -> Rotate -> Translate
	glm::dvec3 scaledPosition = frame->localPosition * parentScale;
	glm::dvec3 rotatedPosition = parent->globalRotation * scaledPosition;

	frame->globalPosition = parent->globalPosition + rotatedPosition;
	frame->globalRotation = parent->globalRotation * frame->localRotation;  // NOTE: Quaternion multiplication is not commutative
	frame->globalScale = frame->localScale * parentScale;
}


void ReferenceFrameSystem::sortFrameTree() {
	// sortFrameTree traverses the reference frame tree depth-first.

	m_referenceFrames.clear();

	auto view = m_registry->getView<Component::ReferenceFrame>();

	std::unordered_map<EntityID, Component::ReferenceFrame*> frameMap;
	std::unordered_map<Component::ReferenceFrame*, EntityID> reverseFrameMap;

	for (auto&& [entity, frame] : view) {
		frameMap[entity] = &frame;		// Alternatively: frameMap[entity] = std::addressof(frame);
		reverseFrameMap[&frame] = entity;
	}


	// Recursive DFS-like search and topological sort
	std::unordered_set<EntityID> visitedEntities;
	std::unordered_set<EntityID> recursionStack; // Used to detect cyclic dependencies

	std::function<void(EntityID)> visit = [&](EntityID entity) {
		// If a cyclic dependency is detected
		if (recursionStack.contains(entity)) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to sort reference frame tree: Cyclic dependency detected! (Entity ID involved: " + std::to_string(entity) + ")");
		}


		// Prematurely terminate search if entity already has parent
		if (visitedEntities.contains(entity))
			return;


		// Else, find the parent of the current entity and push its frame after visiting the parent.
		visitedEntities.insert(entity);
		recursionStack.insert(entity);

		auto* frame = frameMap[entity];
		auto it = reverseFrameMap.find(frame->parent);

		if (frame->parent && it != reverseFrameMap.end()) {
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
