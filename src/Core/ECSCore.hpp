/* ECSCore.hpp - Defines core implementations of the entity-component system.
*/

#pragma once

#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <climits>
#include <bitset>
#include <queue>

#include <Core/LoggingManager.hpp>


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

	inline bool operator==(const Entity& other) const {
		return (id == other.id) &&
				(version == other.version);
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
		@param entity: The entity to be attached.
		@param component: The component data.
	*/
	inline void insert(Entity& entity, Component& component) {
		/* Flow of entity insertion:
		
			High-level flow:
				+ Add a new entity to the list of entity IDs, and its component to the list of components.
				+ Add the (new entity, new entity index) pair to entity-to-index map

			Implementation:
				+ Add component to components
				+ Add entity.id to entityIDs
				+ Map (entity.id in entityToArrayIndexMap) to arraySize
				+ arraySize += 1
		*/

		if (m_entityToArrayIndexMap.find(entity.id) != m_entityToArrayIndexMap.end()) {
			throw Log::RuntimeException(__FUNCTION__, "Cannot insert entity into component array: Entity already exists! (Entity ID: " + std::to_string(entity.id) + ")");
		}

		size_t newSize = arraySize++;
		
		m_components.push_back(component);
		m_entityIDs.push_back(entity.id);
		m_entityToArrayIndexMap[entity.id] = newSize;
	}


	/* Erases an entity from the component array.
		@param entity: The entity to be detached.
	*/
	inline void erase(Entity& entity) {
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

		if (m_entityToArrayIndexMap.find(entity.id) == m_entityToArrayIndexMap.end())
			return;

		size_t currentIndex = m_entityToArrayIndexMap[entity.id];
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
		m_entityToArrayIndexMap.erase(entity.id);

		arraySize--;
	}


	/* Gets a component from the component array.
		@param entity: The entity owning the requested component.

		@return The requested component.
	*/
	inline Component& getComponent(EntityID& entity) { return m_components[m_entityToArrayIndexMap[entity]]; }
	

	/* Checks whether a component exists in the component array.
		@param entity: The entity owning the component to be checked.

		@return True if the component exists, otherwise False.
	*/
	inline bool contains(Entity& entity) {
		return (m_entityToArrayIndexMap.find(entity.id) != m_entityToArrayIndexMap.end());
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



//class View {
//public:
//	/* Creates a view of the component arrays.
//		@param entityManager: Used to narrow down the view scope to entities that are only created by it.
//		@param arrays: The component arrays to be viewed.
//		@return A view of the component arrays.
//	*/
//	template<typename... Components>
//	static auto getView(std::shared_ptr<EntityManager> entityManager) {
//		constexpr size_t argCount = sizeof...(Components);
//		if (argCount == 0) {
//			throw Log::RuntimeException(__FUNCTION__, "No components are passed into view!");
//		}
//		
//		return InternalView<Components...>(entityManager);
//	}
//
//private:
//
//	template<typename... Components>
//	class InternalView {
//	public:
//		InternalView(std::shared_ptr<EntityManager> entityManager) :
//			entityManager(entityManager) {
//		
//			// Queries entities to look for those with the required components
//			requiredMask = buildComponentMask<Components...>();
//			auto allEntities = entityManager->getActiveEntities();
//			updateMatchingEntities(allEntities);
//		};
//
//
//		/* Ignores a variable number of components.
//			@tparam IgnoredComponents: The components whose entities having them attached are to be ignored in the view.
//		*/
//		template<typename... IgnoredComponents>
//		inline void ignoreComponents() {
//			ignoredMask = buildComponentMask<IgnoredComponents...>();
//			updateMatchingEntities(matchingEntities);
//		}
//
//
//		class Iterator {
//		public:
//			Iterator(InternalView* view, size_t index) :
//				view(view), index(index) {}
//
//
//			// Dereferencing
//			// This is necessary for compatibility with native range-based loops and structured bindings.
//			auto operator*() {
//				Entity& entity = view->matchingEntities[index];
//
//				auto generateTuple = [&](Components&... components) {
//					return std::tuple<Entity&, Components&...>(entity, components...);
//				};
//
//				return std::apply(generateTuple, view->matchingEntityComponents);
//			}
//
//
//			// Prefix increment
//			Iterator& operator++() {
//				index++;
//				return *this;
//			}
//
//
//			// Suffix increment
//			Iterator operator++(int) {
//				Iterator tmp = *this;
//				++(*this);
//				return tmp;
//			}
//
//
//			bool operator!=(const Iterator& other) const {
//				return index != other.index;
//			}
//
//		private:
//			InternalView* view;
//			size_t index;
//		};
//		
//
//		Iterator begin() { return Iterator(this, 0); }
//		Iterator end()   { return Iterator(this, matchingEntities.size()); }
//
//	private:
//		std::shared_ptr<EntityManager> entityManager;
//
//		std::vector<Entity> matchingEntities;
//		ComponentMask requiredMask;
//		ComponentMask ignoredMask;
//
//
//		/* Updates the list of entities according to filters (e.g., required/ignored masks).
//			@param sourceEntities: The source vector of entities used as data for updating matching entities from.
//		*/
//		inline void updateMatchingEntities(std::vector<Entity>& sourceEntities) {
//			std::vector<Entity> temp;
//			temp.reserve(sourceEntities.size());
//
//			for (auto& entity : sourceEntities) {
//				if (
//					((entity.componentMask & requiredMask) == requiredMask) &&	// Entity mask must match the required mask
//					((entity.componentMask & ignoredMask).none())				// Entity mask must NOT match the ignored mask
//					) {
//
//					temp.push_back(entity);
//				}
//			}
//
//			matchingEntities.swap(temp);
//		}
//
//		
//		/* Creates a component mask.
//			@tparam SpecifiedComponents: Components to be included in the mask.
//			
//			@return The newly created component mask.
//		*/
//		template<typename... SpecifiedComponents>
//		inline ComponentMask& buildComponentMask() {
//			ComponentMask mask;
//			(mask.set(ComponentTypeID::get<SpecifiedComponents>()), ...);
//			return mask;
//		}
//	};
//};