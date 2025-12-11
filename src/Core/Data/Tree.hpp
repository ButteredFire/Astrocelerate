/* Tree - Implementation of an N-ary/Generic Tree.
*/
#pragma once

#include <set>
#include <map>
#include <mutex>
#include <vector>
#include <atomic>
#include <cstdint>
#include <optional>
#include <algorithm>
#include <functional>


using NodeID = uint32_t;

template<typename T>
class Tree {
private:
	struct Node {
		std::optional<NodeID> parent{};
		NodeID id{};
		std::set<NodeID> children{};

		T data = T();
	};

	std::map<NodeID, Node> m_nodeMap;
	std::vector<Node> m_rootNodes;

	std::mutex m_treeMutex;
	std::atomic<NodeID> m_currentID;

public:
	Tree() { m_currentID.store(0); };
	~Tree() = default;

	/* Adds a node to the tree.
		@param data: The data of the node.
		@param parent (default: std::nullopt): The parent node of the node to be added. If `parent` is `std::nullopt`, the node is treated as a root node.
	
		@return The ID of the node to be added.
	*/
	inline NodeID addNode(T data, std::optional<NodeID> parent = std::nullopt) {
		std::lock_guard<std::mutex> lock(m_treeMutex);

		Node node;
		node.id = m_currentID.load();
		node.data = data;

		if (parent.has_value()) {
			node.parent = parent;
			getNode(node.parent.value()).children.insert(node.id);
		}
		else
			m_rootNodes.emplace_back(node);

		m_nodeMap[node.id] = node;
		m_currentID++;

		return node.id;
	}


	/* Attaches a child node to a parent.
		@param id: The child node's ID.
		@param parentID: The parent node's ID.
	*/
	inline void attachNodeToParent(NodeID id, NodeID parentID) {
		Node &child = getNode(id), &parent = getNode(parentID);

		child.parent = parent.id;
		parent.children.insert(child.id);
	}


	/* Recursively gets a node and all of its children.
		@param startID (optional): The ID of the starting node. If `startID` is not provided, `getNodes` will return a merged result of potentially multiple sub-trees (with multiple roots).

		@return A vector of vector of nodes, `nodes`, where `nodes[depth]` yields a vector of all child nodes at a relative depth from the starting node (which is at a depth of 0).
	*/
	inline std::vector<std::vector<Node*>> getNodes(NodeID startID) {
		using VecLevels = std::vector<std::vector<Node*>>;

		std::lock_guard<std::mutex> lock(m_treeMutex);

		// This recursive function adds a node to the final vector of levels according to its level/depth
		static std::function<void(NodeID, VecLevels &, uint32_t)> processNode = [this](NodeID id, VecLevels &nodes, uint32_t depthFromStart) {
			Node *node = &getNode(id);

			if (depthFromStart >= nodes.size())
				nodes.push_back({});
			nodes[depthFromStart].push_back(node);

			for (auto &childID : node->children)
				processNode(childID, nodes, depthFromStart + 1);
		};

		VecLevels nodes;
		processNode(startID, nodes, 0);

		return nodes;
	}

	inline std::vector<std::vector<Node*>> getNodes() {
		using VecLevels = std::vector<std::vector<Node*>>;

		// Get all nodes, organized into trees according to the total number of root nodes
		std::vector<VecLevels> trees;
		uint32_t maxTreeDepth = 0;

		for (const auto &root : m_rootNodes) {
			trees.emplace_back(getNodes(root.id));
			maxTreeDepth = std::max<uint32_t>(maxTreeDepth, static_cast<uint32_t>(trees.back().size()));
		}

		// Merge the trees
		std::lock_guard<std::mutex> lock(m_treeMutex);

		VecLevels mergedTree;
		mergedTree.resize(maxTreeDepth);

		for (const auto &tree : trees)
			for (int i = 0; i < tree.size(); i++)
				mergedTree[i].insert(mergedTree[i].end(), tree[i].begin(), tree[i].end());

		return mergedTree;
	}


	/* Recursively deletes a node and all of its children.
		@param id: The ID of the node to be deleted.
	*/
	inline void deleteNode(NodeID id) {
		if (!m_nodeMap.count(id))
			return;

		getParentNode(id).children.erase(id); // Removes the node from its parent's list of children

		auto nodes = getNodes(id);

		// Delete the nodes, starting from the most distant children to the original node.
		std::lock_guard<std::mutex> lock(m_treeMutex);

		for (int i = nodes.size() - 1; i >= 0; i--)
			for (int j = nodes[i].size() - 1; j >= 0; j--)
				m_nodeMap.erase(nodes[i][j]->id);
	}


	/* Gets a node by its ID. */
	inline Node &getNode(NodeID id) { return m_nodeMap.at(id); }

	
	/* Gets the parent of a node.
		@param id: The ID of the node in question.

		@return The node's parent if it does exist, or the node itself otherwise.
	*/
	inline Node &getParentNode(NodeID id) {
		std::optional<NodeID> parentID = getNode(id).parent;
		if (!parentID.has_value())
			return getNode(id);
		return getNode(parentID.value());
	}

	inline size_t size() const { return static_cast<size_t>(m_currentID.load()); }
};
