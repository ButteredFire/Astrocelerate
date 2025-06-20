/* ECSCore.hpp - Defines core implementations of the entity-component system.
*/

#pragma once

#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <climits>
#include <bitset>
#include <queue>

#include <Core/Application/LoggingManager.hpp>


// Component
constexpr uint32_t MAX_COMPONENTS_PER_ENTITY = 64;
using ComponentMask = std::bitset<MAX_COMPONENTS_PER_ENTITY>;

// Entity
using EntityID = uint32_t;
using EntityVersion = uint32_t;

constexpr EntityID INVALID_ENTITY = UINT32_MAX;
constexpr EntityID MAX_ENTITIES = 1e5;

struct Entity {
	EntityID id;
	EntityVersion version;

	std::string name;

	inline bool operator==(const Entity& other) const {
		return (id == other.id) &&
				(version == other.version) &&
				(name == other.name);
	}
};

namespace std {
	template<>
	struct hash<Entity> {
		inline std::size_t operator()(const Entity& e) const noexcept {
			return std::hash<EntityID>()(e.id) ^
					std::hash<EntityVersion>()(e.version);
		}
	};
}



/* ComponentArray is a bidirectional sparse set implementation. */
template<typename Component>
class ComponentArray {
public:
	ComponentArray() {};
	~ComponentArray() = default;

	/* Inserts an entity into the component array.
		@param entityID: The ID of the entity to be attached.
		@param component: The component data.
	*/
	inline void insert(EntityID entityID, Component component) {
		/* Flow of entity insertion:
		
			High-level flow:
				+ Add a new entity to the list of entity IDs, and its component to the list of components.
				+ Add the (new entity, new entity index) pair to entity-to-index map

			Implementation:
				+ Add component to components
				+ Add entityID to entityIDs
				+ Map (entityID in entityToArrayIndexMap) to arraySize
				+ arraySize += 1
		*/

		if (m_entityToArrayIndexMap.find(entityID) != m_entityToArrayIndexMap.end()) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot insert entity into component array: Entity already exists! (Entity ID: " + std::to_string(entityID) + ")");
		}

		size_t newSize = arraySize++;
		
		m_components.push_back(component);
		m_entityIDs.push_back(entityID);
		m_entityToArrayIndexMap[entityID] = newSize;
	}


	/* Erases an entity from the component array.
		@param entityID: The ID of the entity to be detached.
	*/
	inline void erase(EntityID entityID) {
		/* Flow of entity erasure:

			High-level flow: Swap specified entity's position with last entity's position.
				=> Specified entity is now at the last position in the component array => O(1) deletion.
				=> Last entity is now at the specified entity's position in the component array => Essentially "replaces" deleted entity with last entity.

			Implementation:
				+ Save specified entity index;
				+ Swap specified entity with last entity;
					=> "last entity" is now "specified entity";
					   "specified entity" is now "last entity". (pay attention to quotation marks...)

				+ In entity-to-index map, "specified entity" still maps to last entity
					=> Update "specified entity" to map to saved index;

				+ Delete last element from "components" and "entityIDs";
				+ Delete ("last entity", "last entity" index) pair from entity-to-index map.
		*/

		if (m_entityToArrayIndexMap.find(entityID) == m_entityToArrayIndexMap.end())
			return;

		size_t currentIndex = m_entityToArrayIndexMap[entityID];
		size_t lastIndex = arraySize - 1;

		std::swap(m_components[currentIndex], m_components[lastIndex]);
		std::swap(m_entityIDs[currentIndex], m_entityIDs[lastIndex]);

		/* At this point:
			+ (Entity + Component) pair at current index is now the pair at last index;
			+ (Entity + Component) pair at last index is now the pair at current index;

			=> Deleting (Entity + Component) pair at last index IS TECHNICALLY deleting the specified entity.
		*/

		m_entityToArrayIndexMap[m_entityIDs[currentIndex]] = currentIndex;

		m_components.pop_back();
		m_entityIDs.pop_back();
		m_entityToArrayIndexMap.erase(entityID);

		arraySize--;
	}


	/* Updates a component in the component array.
		@param entityID: The ID of the entity owning the component to be updated.
		@param component: The new component data.
	*/
	inline void updateComponent(EntityID entityID, Component component) {
		if (m_entityToArrayIndexMap.find(entityID) == m_entityToArrayIndexMap.end()) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot update component of type " + enquote(typeid(Component).name()) + " for entity #" + std::to_string(entityID) + ": Entity does not exist!");
		}

		size_t index = m_entityToArrayIndexMap[entityID];
		m_components[index] = component;
	}


	/* Gets a component from the component array.
		@param entityID: The ID of the entity owning the requested component.

		@return The requested component.
	*/
	inline Component& getComponent(EntityID entityID) { return m_components[m_entityToArrayIndexMap[entityID]]; }
	

	/* Checks whether a component exists in the component array.
		@param entityID: The ID of the entity owning the component to be checked.

		@return True if the component exists, otherwise False.
	*/
	inline bool contains(EntityID entityID) {
		return (m_entityToArrayIndexMap.find(entityID) != m_entityToArrayIndexMap.end());
	}

private:
	std::vector<Component> m_components;							// Dense array
	std::vector<EntityID> m_entityIDs;								// Used like a reverse map (Array Index -> Entity ID)
	std::unordered_map<EntityID, size_t> m_entityToArrayIndexMap;	// Sparse map (Entity ID -> Array Index)

	size_t arraySize = 0;
};



class ComponentTypeID {
public:
	/* Generates a unique ID for a component type.
		@return A unique ID for the component type.
	*/
	template<typename Component>
	inline static size_t get() {
		// Each get<Component>() instantiation has its own static currentID, initialized only once and shared across calls for that specific type.
		// This ensures every component type gets a unique, persistent ID.
		static size_t currentID = nextID++;
		return currentID;
	}

private:
	inline static size_t nextID = 0;
};
