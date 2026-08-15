#pragma once

#include <Scripting/AsTLTypes.hpp>
#include <Scripting/Compiler/GraphNodeRegistry.hpp>


namespace Compiler {
	class MockNodeRegistry : public IGraphNodeRegistry {
	public:
		MockNodeRegistry() {
			m_nodeTable = transferTable();
			m_nodeLookup = transferLookup();
		};
		~MockNodeRegistry() = default;

		const NodeTableT& getNodeTable() const override { return m_nodeTable; };
		const NodeLookupT& getNodeLookup() const override { return m_nodeLookup; };

		void addOrSet(const GraphNodeDescriptor& descriptor) override {
			addOrModifyInternal(m_nodeTable, m_nodeLookup, descriptor);
		};

		void addOrSet(GraphNodeDescriptor&& descriptor) override {
			addOrModifyInternal(m_nodeTable, m_nodeLookup, std::forward<GraphNodeDescriptor>(descriptor));
		};

		bool contains(const std::string& nodeFuncName) const override { return m_nodeLookup.contains(nodeFuncName); };

		bool contains(AsTL::IDX nodeFuncIdx) const override { return m_nodeTable.contains(nodeFuncIdx); };

		const std::pair<AsTL::IDX, const GraphNodeDescriptor&> getInfo(const std::string& nodeFuncName) const override {
			AsTL::IDX index = m_nodeLookup.at(nodeFuncName);
			return {
				index,
				m_nodeTable.at(index)
			};
		};

		const GraphNodeDescriptor& getInfo(AsTL::IDX nodeFuncIdx) const override {
			return m_nodeTable.at(nodeFuncIdx);
		};

	private:
		NodeTableT m_nodeTable{};
		NodeLookupT m_nodeLookup{};
	};
}
