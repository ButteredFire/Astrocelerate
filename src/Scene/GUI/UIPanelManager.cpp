#include "UIPanelManager.hpp"

UIPanelManager::UIPanelManager(VulkanContext& context):
	m_vkContext(context) {

	m_garbageCollector = ServiceLocator::GetService<GarbageCollector>(__FUNCTION__);
	m_eventDispatcher = ServiceLocator::GetService<EventDispatcher>(__FUNCTION__);
	m_registry = ServiceLocator::GetService<Registry>(__FUNCTION__);


	bindPanelFlags();

	// TODO: Serialize the panel mask in the future to allow for config loading/ opening panels from the last session
	m_panelMask.reset();

	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_VIEWPORT, GUI::TOGGLE_ON);
	GUI::TogglePanel(m_panelMask, GUI::PanelFlag::PANEL_TELEMETRY, GUI::TOGGLE_ON);

	initPanelsFromMask(m_panelMask);
	
	bindEvents();

	Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
}


void UIPanelManager::bindEvents() {
	m_eventDispatcher->subscribe<Event::GUIContextIsValid>(
		[this](const Event::GUIContextIsValid& event) {
			initViewportTextureDescriptorSet();
		}
	);
}


void UIPanelManager::initPanelsFromMask(GUI::PanelMask& mask) {
	using namespace GUI;

	for (size_t i = 0; i < FLAG_COUNT; i++) {
		PanelFlag flag = PanelFlagsArray[i];

		if (IsPanelOpen(mask, flag)) {
			auto it = m_panelCallbacks.find(flag);

			if (it == m_panelCallbacks.end() || 
				(it != m_panelCallbacks.end() && it->second == nullptr)
				) {

				Log::Print(Log::T_ERROR, __FUNCTION__, "Cannot open panel: Initialization callback for panel flag #" + std::to_string(i) + " is not available!");

				continue;
			}
		}
	}
}


void UIPanelManager::updatePanels() {
	renderPanelsMenu();

	for (auto&& [flag, callback] : m_panelCallbacks) {
		if (GUI::IsPanelOpen(m_panelMask, flag))
			(this->*callback)();
	}
}


void UIPanelManager::bindPanelFlags() {
	using namespace GUI;

	m_panelCallbacks[PanelFlag::PANEL_VIEWPORT]				= &UIPanelManager::renderViewportPanel;
	m_panelCallbacks[PanelFlag::PANEL_TELEMETRY]			= &UIPanelManager::renderTelemetryPanel;
	m_panelCallbacks[PanelFlag::PANEL_ENTITY_INSPECTOR]		= &UIPanelManager::renderEntityInspectorPanel;
	m_panelCallbacks[PanelFlag::PANEL_SIMULATION_CONTROL]	= &UIPanelManager::renderSimulationControlPanel;
	m_panelCallbacks[PanelFlag::PANEL_RENDER_SETTINGS]		= &UIPanelManager::renderRenderSettingsPanel;
	m_panelCallbacks[PanelFlag::PANEL_ORBITAL_PLANNER]		= &UIPanelManager::renderOrbitalPlannerPanel;
	m_panelCallbacks[PanelFlag::PANEL_DEBUG_CONSOLE]		= &UIPanelManager::renderDebugConsole;
}


void UIPanelManager::initViewportTextureDescriptorSet() {
	// Descriptor pool
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE }
	};

	// Use the flag CREATE_UPDATE_AFTER_BIND_BIT to allow descriptor sets allocated from this pool to be updated (i.e., not immutable).
	// Since we must update the viewport texture descriptor set once per frame, we must have this flag and a corresponding flag for descriptor bindings (see below).
	VkDescriptorUtils::CreateDescriptorPool(m_vkContext, m_descriptorPool, poolSizes, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);


	// Descriptor set layout
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBinding.binding = 0;
	layoutBinding.descriptorCount = 1;
	layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorBindingFlags bindingFlags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT; // Binding count: 1
	VkDescriptorSetLayoutBindingFlagsCreateInfo layoutBindingFlagsInfo{};
	layoutBindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	layoutBindingFlagsInfo.bindingCount = 1;
	layoutBindingFlagsInfo.pBindingFlags = &bindingFlags;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &layoutBinding;
	layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	layoutInfo.pNext = &layoutBindingFlagsInfo; // Chain the binding flags create info

	VkResult layoutResult = vkCreateDescriptorSetLayout(m_vkContext.Device.logicalDevice, &layoutInfo, nullptr, &m_viewportTextureDescSetLayout);

	CleanupTask layoutTask{};
	layoutTask.caller = __FUNCTION__;
	layoutTask.objectNames = { VARIABLE_NAME(m_viewportTextureDescSetLayout) };
	layoutTask.vkObjects = { m_vkContext.Device.logicalDevice, m_viewportTextureDescSetLayout };
	layoutTask.cleanupFunc = [this]() {
		vkDestroyDescriptorSetLayout(m_vkContext.Device.logicalDevice, m_viewportTextureDescSetLayout, nullptr);
	};

	m_garbageCollector->createCleanupTask(layoutTask);


	LOG_ASSERT(layoutResult == VK_SUCCESS, "Failed to create viewport texture descriptor set layout!");


	// Descriptor set
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_viewportTextureDescSetLayout;

	VkResult allocResult = vkAllocateDescriptorSets(m_vkContext.Device.logicalDevice, &allocInfo, &m_viewportRenderTextureDescSet);

	CleanupTask allocTask{};
	allocTask.caller = __FUNCTION__;
	allocTask.objectNames = { VARIABLE_NAME(m_viewportRenderTextureDescSet) };
	allocTask.vkObjects = { m_vkContext.Device.logicalDevice, m_descriptorPool, m_viewportRenderTextureDescSet };
	allocTask.cleanupFunc = [this]() {
		vkFreeDescriptorSets(m_vkContext.Device.logicalDevice, m_descriptorPool, 1, &m_viewportRenderTextureDescSet);
	};

	m_garbageCollector->createCleanupTask(allocTask);
	

	LOG_ASSERT(allocResult == VK_SUCCESS, "Failed to allocate viewport render texture descriptor set!");
}


void UIPanelManager::renderPanelsMenu() {
	using namespace GUI;

	ImGui::Begin("Panels Menu");

	for (size_t i = 0; i < FLAG_COUNT; i++) {
		PanelFlag flag = PanelFlagsArray[i];
		bool isOpen = IsPanelOpen(m_panelMask, flag);

		ImGui::Checkbox(GetPanelName(flag), &isOpen);

		TogglePanel(m_panelMask, flag, isOpen ? TOGGLE_ON : TOGGLE_OFF);
	}

	ImGui::End();
}


void UIPanelManager::renderViewportPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_VIEWPORT;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) {
		Log::Print(Log::T_ERROR, __FUNCTION__, "The viewport must not be closed!");
		GUI::TogglePanel(m_panelMask, flag, GUI::TOGGLE_ON);
	}

	ImGui::Begin("Viewport");		// This panel must be docked

	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
	m_viewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	ImGui::Text("Panel size: (%.2f, %.2f)", viewportPanelSize.x, viewportPanelSize.y);


	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageView = m_vkContext.OffscreenResources.imageView;
	imageInfo.sampler = m_vkContext.OffscreenResources.sampler;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet imageDescSetWrite{};
	imageDescSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	imageDescSetWrite.dstBinding = 0;
	imageDescSetWrite.descriptorCount = 1;
	imageDescSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imageDescSetWrite.dstSet = m_viewportRenderTextureDescSet;
	imageDescSetWrite.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(m_vkContext.Device.logicalDevice, 1, &imageDescSetWrite, 0, nullptr);
	

	ImTextureID renderTextureID = (ImTextureID) m_viewportRenderTextureDescSet;
	ImGui::Image(renderTextureID, viewportPanelSize);

	ImGui::End();
}


void UIPanelManager::renderTelemetryPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_TELEMETRY;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag));

	auto view = m_registry->getView<Component::RigidBody, Component::ReferenceFrame>();
	size_t entityCount = 0;

	for (const auto& [entity, rigidBody, refFrame] : view) {
		// As the content is dynamically generated, we need each iteration to have its ImGui ID to prevent conflicts.
		// Since entity IDs are always unique, we can use them as ImGui IDs.
		ImGui::PushID(static_cast<int>(entity));


		ImGui::Text("Entity ID #%d", entity);


		// --- Rigid-body Entity Debug Info ---
		if (ImGui::CollapsingHeader("Rigid-body Entity Debug Info")) {
			float velocityAbs = glm::length(rigidBody.velocity);
			ImGui::Text("\tVelocity:");
			ImGui::Text("\t\tVector: (x: %.2f, y: %.2f, z: %.2f)", rigidBody.velocity.x, rigidBody.velocity.y, rigidBody.velocity.z);
			ImGui::Text("\t\tAbsolute: |v| ~= %.4f m/s", velocityAbs);


			float accelerationAbs = glm::length(rigidBody.acceleration);
			ImGui::Text("\tAcceleration:");
			ImGui::Text("\t\tVector: (x: %.2f, y: %.2f, z: %.2f)", rigidBody.acceleration.x, rigidBody.acceleration.y, rigidBody.acceleration.z);
			ImGui::Text("\t\tAbsolute: |a| ~= %.4f m/s^2", accelerationAbs);


				// Display mass: scientific notation for large values, fixed for small
			if (std::abs(rigidBody.mass) >= 1e6f) {
				ImGui::Text("\tMass: %.2e kg", rigidBody.mass);
			}
			else {
				ImGui::Text("\tMass: %.2f kg", rigidBody.mass);
			}

		}


		// --- Reference Frame Entity Debug Info ---
		if (ImGui::CollapsingHeader("Reference Frame Entity Debug Info")) {
			// Parent ID
			if (refFrame.parentID.has_value()) {
				ImGui::Text("\tParent Entity ID: %d", refFrame.parentID.value());
			}
			else {
				ImGui::Text("\tParent Entity ID: None");
			}

			ImGui::Text("\t\tScale (simulation): %.10f m (radius)",
				refFrame.scale);
			ImGui::Text("\t\tScale (render): %.10f m (radius)",
				SpaceUtils::GetRenderableScale(SpaceUtils::ToRenderSpace(refFrame.scale)));

			// Local Transform
			ImGui::Text("\tLocal Transform:");
			ImGui::Text("\t\tPosition: (x: %.2f, y: %.2f, z: %.2f)",
				refFrame.localTransform.position.x,
				refFrame.localTransform.position.y,
				refFrame.localTransform.position.z);
			ImGui::Text("\t\t\tMagnitude: ||vec|| ~= %.2f m", glm::length(refFrame.localTransform.position));

			ImGui::Text("\t\tRotation: (x: %.2f, y: %.2f, z: %.2f, w: %.2f)",
				refFrame.localTransform.rotation.x,
				refFrame.localTransform.rotation.y,
				refFrame.localTransform.rotation.z,
				refFrame.localTransform.rotation.w);

			// Global Transform
			ImGui::Text("\tGlobal Transform (normalized):");
			ImGui::Text("\t\tPosition: (x: %.2f, y: %.2f, z: %.2f)",
				refFrame.globalTransform.position.x,
				refFrame.globalTransform.position.y,
				refFrame.globalTransform.position.z);
			ImGui::Text("\t\t\tMagnitude: ||vec|| ~= %.2f m", glm::length(refFrame.globalTransform.position));

			ImGui::Text("\t\tRotation: (x: %.2f, y: %.2f, z: %.2f, w: %.2f)",
				refFrame.globalTransform.rotation.x,
				refFrame.globalTransform.rotation.y,
				refFrame.globalTransform.rotation.z,
				refFrame.globalTransform.rotation.w);

		}


		if (entityCount < view.size() - 1) {
			ImGui::Separator();
		}
		entityCount++;


		ImGui::PopID();
	}

	ImGui::End();
}


void UIPanelManager::renderEntityInspectorPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_ENTITY_INSPECTOR;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag));
	
	ImGui::Text("Pushing the boundaries of space exploration, one line of code at a time.");
	
	ImGui::End();
}


void UIPanelManager::renderSimulationControlPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_SIMULATION_CONTROL;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag));

	ImGui::Text("Pushing the boundaries of space exploration, one line of code at a time.");

	ImGui::End();
}


void UIPanelManager::renderRenderSettingsPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_RENDER_SETTINGS;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag));

	ImGui::Text("Pushing the boundaries of space exploration, one line of code at a time.");

	ImGui::End();
}


void UIPanelManager::renderOrbitalPlannerPanel() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_ORBITAL_PLANNER;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag));

	ImGui::Text("Pushing the boundaries of space exploration, one line of code at a time.");

	ImGui::End();
}


void UIPanelManager::renderDebugConsole() {
	const GUI::PanelFlag flag = GUI::PanelFlag::PANEL_DEBUG_CONSOLE;
	if (!GUI::IsPanelOpen(m_panelMask, flag)) return;

	ImGui::Begin(GUI::GetPanelName(flag));

	ImGui::Text("Pushing the boundaries of space exploration, one line of code at a time.");

	ImGui::End();
}
