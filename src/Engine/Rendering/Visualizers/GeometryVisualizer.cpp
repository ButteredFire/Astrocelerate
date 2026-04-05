#include "GeometryVisualizer.hpp"


GeometryVisualizer::GeometryVisualizer(const Ctx::VkRenderDevice *renderDeviceCtx, const Ctx::VkWindow *windowCtx, const Ctx::OffscreenPipeline *offscreenData, const Geometry::GeometryData *geomData, const Buffer::BufferAlloc *globalVertBuffer, const Buffer::BufferAlloc *globalIdxBuffer, std::shared_ptr<VkBufferManager> bufMgr):
	m_renderDeviceCtx(renderDeviceCtx),
	m_windowCtx(windowCtx),
	m_offscreenData(offscreenData),
	m_geomData(geomData),
	m_globalVertBufAlloc(globalVertBuffer),
	m_globalIdxBufAlloc(globalIdxBuffer),
	m_bufManager(bufMgr) {

	m_cleanupManager	= ServiceLocator::GetService<CleanupManager>(__FUNCTION__);
	m_ecsRegistry		= ServiceLocator::GetService<ECSRegistry>(__FUNCTION__);

	m_offscreenPipeline			= m_offscreenData->pipeline;
	m_offscreenPipelineLayout	= m_offscreenData->pipelineLayout;
	m_offscreenRenderPass		= m_offscreenData->renderPass;
	m_materialDescriptorSet		= m_offscreenData->materialDescriptorSet;
	m_texArrayDescriptorSet		= m_offscreenData->texArrayDescriptorSet;
	m_perFrameDescriptorSets	= m_offscreenData->perFrameDescriptorSets;

	m_minUBOAlignment		= m_renderDeviceCtx->chosenDevice.properties.limits.minUniformBufferOffsetAlignment;
	m_alignedObjectUBOSize	= SystemUtils::Align(sizeof(Buffer::ObjectUBO), m_minUBOAlignment);
	m_alignedMaterialSize	= SystemUtils::Align(sizeof(Geometry::Material), m_minUBOAlignment);

	m_visualizerID = m_cleanupManager->createCleanupGroup();
}


GeometryVisualizer::~GeometryVisualizer() {
	m_cleanupManager->executeCleanupTask(m_visualizerID);
}


void GeometryVisualizer::init(std::array<VkCommandBuffer, SimulationConst::MAX_FRAMES_IN_FLIGHT> secondCmdBufs) {
	m_secondCmdBufs = std::move(secondCmdBufs);

	initResources();
}


void GeometryVisualizer::prepareFrame(uint32_t frameIdx, const Buffer::FramePacket &framePacket) {
	m_objectUBOs[frameIdx].meshRanges.clear();

	auto view = m_ecsRegistry->getView<CoreComponent::Transform, PhysicsComponent::RigidBody, RenderComponent::MeshRenderable>();

	for (auto &&[entity, transform, rigidBody, meshRenderable] : view) {
		Buffer::ObjectUBO ubo{};

		// Compute entity matrices
		glm::dvec3 renderPosition = SpaceUtils::ToRenderSpace_Position(transform.position - framePacket.camFloatingOrigin);

			// Scale in render space
		double renderScale = SpaceUtils::GetRenderableScale(SpaceUtils::ToRenderSpace_Scale(transform.scale)) * meshRenderable.visualScale;

			/* NOTE:
				Model matrices are constructed according to the Scale -> Rotate -> Translate (S-R-T) order.
				Specifically, a model matrix, M, is constructed like so:
						M = T * R * S * v_local (where v_local is the position of the vertex in local space).

				The reason why the construction looks backwards is because matrix multiplication is right-to-left for column vectors (the default in GLM). So, when we construct M as M = T * R * S * v_local, the operations are applied as:
						M = T * (R * (S * v_local)), hence S-R-T order.
			*/
		const glm::mat4 identityMat = glm::mat4(1.0f);
		glm::mat4 modelMatrix =
			glm::translate(identityMat, glm::vec3(renderPosition))
			* glm::mat4(glm::toMat4(transform.rotation))
			* glm::scale(identityMat, glm::vec3(static_cast<float>(renderScale)));

		ubo.modelMatrix = modelMatrix;
		ubo.normalMatrix = glm::transpose(glm::inverse(modelMatrix));


		// Write to mapped memory
		for (uint32_t meshIndex : meshRenderable.meshRange) {
			void *uboDst = SystemUtils::GetAlignedBufferOffset(m_alignedObjectUBOSize, m_objectUBOs[frameIdx].bufAlloc.mappedData, meshIndex);
			memcpy(uboDst, &ubo, sizeof(ubo));
		}

		m_objectUBOs[frameIdx].meshRanges.emplace_back(meshRenderable.meshRange);
	}
}


void GeometryVisualizer::render(uint32_t frameIdx) {
	//vkResetCommandBuffer(m_secondCmdBufs[frameIdx], 0);

	
	VkCommandBufferInheritanceInfo inheritanceInfo{};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.renderPass = m_offscreenRenderPass;
	inheritanceInfo.framebuffer = m_offscreenData->frameBuffers[frameIdx];
	inheritanceInfo.subpass = 0;

	VkCommandBufferBeginInfo cmdBufBeginInfo{};
	cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT; // RENDER_PASS_CONTINUE_BIT signals that the entire (secondary) command buffer will be executed within a single render pass instance
	cmdBufBeginInfo.pInheritanceInfo = &inheritanceInfo;


	vkBeginCommandBuffer(m_secondCmdBufs[frameIdx], &cmdBufBeginInfo);
	{
		vkCmdBindPipeline(m_secondCmdBufs[frameIdx], VK_PIPELINE_BIND_POINT_GRAPHICS, m_offscreenPipeline);


		// Specify viewport and scissor states (since they're dynamic states)
			// Viewport
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_windowCtx->extent.width);
		viewport.height = static_cast<float>(m_windowCtx->extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(m_secondCmdBufs[frameIdx], 0, 1, &viewport);

		// Scissor
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_windowCtx->extent;
		vkCmdSetScissor(m_secondCmdBufs[frameIdx], 0, 1, &scissor);



		// Bind vertex and index buffers
			// Vertex buffers
		VkBuffer vertexBuffers[] = {
			m_globalVertBufAlloc->buffer
		};
		VkDeviceSize vertexBufferOffsets[] = { 0 };
		vkCmdBindVertexBuffers(m_secondCmdBufs[frameIdx], 0, 1, vertexBuffers, vertexBufferOffsets);

		// Index buffer (note: you can only have 1 index buffer)
		VkBuffer indexBuffer = m_globalIdxBufAlloc->buffer;
		vkCmdBindIndexBuffer(m_secondCmdBufs[frameIdx], indexBuffer, 0, VK_INDEX_TYPE_UINT32);



		// Update global data
			// Textures array
		vkCmdBindDescriptorSets(m_secondCmdBufs[frameIdx], VK_PIPELINE_BIND_POINT_GRAPHICS, m_offscreenPipelineLayout, 2, 1, &m_texArrayDescriptorSet, 0, nullptr);



		// Update each mesh's UBOs
		for (const auto &indexRange : m_objectUBOs[frameIdx].meshRanges) {
			uint32_t vertexOffset = m_geomData->meshOffsets[indexRange.left].vertexOffset;

			for (uint32_t meshIndex : indexRange) {
				// Object UBO
				uint32_t objectUBOOffset = static_cast<uint32_t>(meshIndex * m_alignedObjectUBOSize);
				vkCmdBindDescriptorSets(m_secondCmdBufs[frameIdx],
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					m_offscreenPipelineLayout,
					0, 1, &m_objectUBOs[frameIdx].descriptorSet,
					1, &objectUBOOffset);


				// Material parameters UBO
				const Geometry::MeshOffset &meshOffset = m_geomData->meshOffsets[meshIndex];
				uint32_t meshMaterialOffset = static_cast<uint32_t>(meshOffset.materialIndex * m_alignedMaterialSize);

				vkCmdBindDescriptorSets(m_secondCmdBufs[frameIdx],
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					m_offscreenPipelineLayout,
					1, 1, &m_materialDescriptorSet,
					1, &meshMaterialOffset);


				// Draw call
				vkCmdDrawIndexed(m_secondCmdBufs[frameIdx], meshOffset.indexCount, 1, meshOffset.indexOffset, vertexOffset, 0);
			}
		}
	}
	vkEndCommandBuffer(m_secondCmdBufs[frameIdx]);
}


void GeometryVisualizer::initResources() {
	// Object UBOs (1 per frame for MAX_FRAMES_IN_FLIGHT frames total)
	{
		VkDeviceSize objectUBOBufSize = m_alignedObjectUBOSize * m_geomData->meshCount;
		for (int i = 0; i < m_objectUBOs.size(); i++) {
			// Create UBO
			m_objectUBOs[i].bufAlloc		= m_bufManager->allocate(objectUBOBufSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, Buffer::MemIntent::RAM_SEQ_ACCESS);
			m_objectUBOs[i].descriptorSet	= m_perFrameDescriptorSets[i];

			m_cleanupManager->addTaskDependency(m_objectUBOs[i].bufAlloc.resourceID, m_visualizerID);

			// Update UBO descriptor set
			VkDescriptorBufferInfo objectUBOInfo{};
			objectUBOInfo.buffer = m_objectUBOs[i].bufAlloc.buffer;
			objectUBOInfo.offset = 0; // Offset will be dynamic during draw calls
			objectUBOInfo.range = m_alignedObjectUBOSize;

			VkWriteDescriptorSet objectUBODescWrite{};
			objectUBODescWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			objectUBODescWrite.dstSet = m_objectUBOs[i].descriptorSet;
			objectUBODescWrite.dstBinding = ShaderConst::VERT_BIND_OBJECT_UBO;
			objectUBODescWrite.dstArrayElement = 0;
			objectUBODescWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			objectUBODescWrite.descriptorCount = 1;
			objectUBODescWrite.pBufferInfo = &objectUBOInfo;

			vkUpdateDescriptorSets(m_renderDeviceCtx->logicalDevice, 1, &objectUBODescWrite, 0, nullptr);
		}
	}



	// Material parameters UBO
	{
		// Create UBO
		VkDeviceSize matBufSize = m_alignedMaterialSize * m_geomData->meshMaterials.size();
		m_materialUBOAlloc = m_bufManager->allocate(matBufSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, Buffer::MemIntent::RAM_SEQ_ACCESS);

		m_cleanupManager->addTaskDependency(m_materialUBOAlloc.resourceID, m_visualizerID);

		// Populate UBO with all materials
		for (size_t i = 0; i < m_geomData->meshMaterials.size(); i++) {
			void *dataOffset = SystemUtils::GetAlignedBufferOffset(m_alignedMaterialSize, m_materialUBOAlloc.mappedData, i);
			memcpy(dataOffset, &m_geomData->meshMaterials[i], sizeof(Geometry::Material));
		}

		// Update UBO descriptor set
		VkDescriptorBufferInfo materialUBOInfo{};
		materialUBOInfo.buffer = m_materialUBOAlloc.buffer;
		materialUBOInfo.offset = 0;
		materialUBOInfo.range = m_alignedMaterialSize;

		VkWriteDescriptorSet materialUBOdescWrite{};
		materialUBOdescWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		materialUBOdescWrite.dstSet = m_materialDescriptorSet;
		materialUBOdescWrite.dstBinding = ShaderConst::FRAG_BIND_MATERIAL_PARAMETERS;
		materialUBOdescWrite.dstArrayElement = 0;
		materialUBOdescWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		materialUBOdescWrite.descriptorCount = 1;
		materialUBOdescWrite.pBufferInfo = &materialUBOInfo;

		vkUpdateDescriptorSets(m_renderDeviceCtx->logicalDevice, 1, &materialUBOdescWrite, 0, nullptr);
	}



	// Texture array: sampler and descriptor set is already managed and updated by TextureManager
}
