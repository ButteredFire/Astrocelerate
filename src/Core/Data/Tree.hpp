#pragma once

#include <mutex>
#include <vector>
#include <atomic>
#include <cstdint>
#include <optional>
#include <algorithm>
#include <functional>
#include <unordered_set>
#include <unordered_map>


using NodeID = uint32_t;


/* Tree - Implementation of an N-ary/Generic Tree.
	@warning This implementation is not thread-safe.
*/
template<typename T>
class Tree {
private:
	struct Node {
		NodeID id{};
		std::unordered_set<NodeID> parents{};
		std::unordered_set<NodeID> children{};

		T data = T();
	};

	std::unordered_map<NodeID, Node> m_nodeMap;
	std::unordered_set<NodeID> m_rootNodeIDs;

	std::atomic<NodeID> m_currentID;

	using VecLevels = std::vector<std::vector<Node *>>;

	/* Adds a node to the final vector of levels according to its level/depth. */
	inline void processNode(NodeID id, VecLevels &nodes, uint32_t depthFromStart) {
		Node *node = &getNode(id);

		if (depthFromStart >= nodes.size())
			nodes.push_back({});
		nodes[depthFromStart].push_back(node);

		for (auto &childID : node->children)
			processNode(childID, nodes, depthFromStart + 1);
	}

public:
	Tree() { m_currentID.store(0); };
	~Tree() = default;

	/* Adds a node to the tree.
		@param data: The data of the node.
		@param parent (default: std::nullopt): The parent node of the node to be added. If `parent` is `std::nullopt`, the node is treated as a root node.

		@return The ID of the node to be added.
	*/
	inline NodeID addNode(T data, std::optional<NodeID> parent = std::nullopt) {
		Node node;
		node.id = m_currentID.fetch_add(1);  // Fetch current value, then add one
		node.data = data;

		if (parent.has_value()) {
			node.parents.insert(parent.value());
			getNode(parent.value()).children.insert(node.id);
		}
		else
			m_rootNodeIDs.insert(node.id);

		m_nodeMap[node.id] = node;

		return node.id;
	}


	/* Attaches a child node to a parent.
		@param id: The child node's ID.
		@param parentID: The parent node's ID.
	*/
	inline void attachNodeToParent(NodeID id, NodeID parentID) {
		Node &child = getNode(id), &parent = getNode(parentID);

		child.parents.insert(parent.id);
		parent.children.insert(child.id);
	}


	/* Recursively gets a node and all of its children.
		@param startID (optional): The ID of the starting node. If `startID` is not provided, `getNodes` will return a merged result of potentially multiple sub-trees (with multiple roots).

		@return A vector of vector of nodes, `nodes`, where `nodes[depth]` yields a vector of all child nodes at a relative depth from the starting node (which is at a depth of 0).
	*/
	inline VecLevels getNodes(NodeID startID) {
		if (!m_nodeMap.count(startID))
			return VecLevels();

		VecLevels nodes;
		processNode(startID, nodes, 0);

		return nodes;
	}

	inline VecLevels getNodes() {
		// Get all nodes, organized into trees according to the total number of root nodes
		std::vector<VecLevels> trees;
		uint32_t maxTreeDepth = 0;

		for (const auto &rootID : m_rootNodeIDs) {
			trees.emplace_back(getNodes(rootID));
			maxTreeDepth = std::max<uint32_t>(maxTreeDepth, static_cast<uint32_t>(trees.back().size()));
		}

		// Merge the trees
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
		auto nodes = getNodes(id);

		// Delete the nodes, starting from the most distant children to the original node.
		for (int i = nodes.size() - 1; i >= 0; i--)
			for (int j = nodes[i].size() - 1; j >= 0; j--) {
				NodeID id = nodes[i][j]->id;
				if (!m_nodeMap.count(id))
					return;

				for (const auto &parentID : nodes[i][j]->parents)
					getNode(parentID).children.erase(id); // Removes the node from its parent's list of children

				m_nodeMap.erase(id);

				if (nodes[i][j]->parents.empty())
					m_rootNodeIDs.erase(id);
			}
	}


	/* Gets a node by its ID. */
	inline Node &getNode(NodeID id) { return m_nodeMap.at(id); }

	inline size_t size() const { return static_cast<size_t>(m_currentID.load()); }
};
