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
		updateGlobalTransform(entity, *frame);
		m_registry->updateComponent(entity, *frame);
	}

	std::cout;
}


void ReferenceFrameSystem::updateGlobalTransform(EntityID entityID, Component::ReferenceFrame& frame) {
	// If frame is the root frame, set global transform to local transform
	if (!frame.parentID.has_value()) {
		frame.globalTransform.position = frame.localTransform.position;
		frame.globalTransform.rotation = frame.localTransform.rotation;
		frame.globalTransform.scale = frame.localTransform.scale;

		return;
	}

	// Else, recursively compute the frame's global transform (assuming parent's global transform was already computed)
	const Component::ReferenceFrame* parent = &m_registry->getComponent<Component::ReferenceFrame>(frame.parentID.value());
	const double parentScale = parent->globalTransform.scale;


		// Order: Scale -> Rotate -> Translate
	glm::dvec3 scaledPosition = frame.localTransform.position * parentScale;
	glm::dvec3 rotatedPosition = parent->globalTransform.rotation * scaledPosition;

	frame.globalTransform.position = parent->globalTransform.position + rotatedPosition;
	frame.globalTransform.rotation = parent->globalTransform.rotation * frame.localTransform.rotation;  // NOTE: Quaternion multiplication is not commutative
	frame.globalTransform.scale = frame.localTransform.scale * parentScale;


	// Rescale global transform to account for different child-to-parent scale ratios (i.e., undo different scale influence on transforms)
	frame.globalTransform.position /= parentScale;
	frame.globalTransform.scale /= parentScale;


	// Ensure that the mesh with the global scale gets rendered
	frame.globalTransform.scale = SpaceUtils::GetRenderableScale(frame.globalTransform.scale);


	if (frame.globalTransform.scale == 0) {
		std::string errMsg = "Failed to update global transform for entity ID #" + std::to_string(entityID) + ": ";

		if (frame.localTransform.scale == 0)
			errMsg += "\n- Its local scale is 0";

		if (parentScale == 0)
			errMsg += "\n- Its parent (Entity ID #" + std::to_string(frame.parentID.value()) + ") has a scale value of 0";

		errMsg += "\n\nScale, globally or locally, must be greater than 0.";

		throw Log::RuntimeException(__FUNCTION__, __LINE__, errMsg);
	}
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
