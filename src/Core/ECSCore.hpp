/* ECSCore.hpp - Defines core features of the entity-component system.
*/

#pragma once

#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <climits>
#include <bitset>

#include <Core/LoggingManager.hpp>


// Component
constexpr uint32_t MAX_COMPONENTS_PER_ENTITY = 64;
using ComponentMask = std::bitset<MAX_COMPONENTS_PER_ENTITY>;

// Entity
using EntityID = uint32_t;
constexpr EntityID INVALID_ENTITY = UINT32_MAX;

struct Entity {
	EntityID id;
	ComponentMask componentMask;
};

	// Defines the == operator for Entity objects to compare based only on ID
	// (so that there won't be errors like `Entity{0, maskA} != Entity{0, maskB}`, i.e. the same entity but with different masks are treated as separate entities)
inline bool operator==(const Entity& lhs, const Entity& rhs) {
	return lhs.id == rhs.id;
}
namespace std {
	template<>
	struct hash<Entity> {
		inline std::size_t operator()(const Entity& e) const noexcept {
			return std::hash<EntityID>()(e.id);
		}
	};
}


/* Down the line, you might consider sparse sets if:
	- You’re simulating hundreds of thousands to millions of entities and need iteration speed gains.
	- You implement systems where the cache layout of components becomes a performance bottleneck.
	- You add system-level optimizations that require bulk access to contiguous data.
	- You allow custom or non-linear entity IDs (e.g. from networking or persistence layers).
	- You go multithreaded and want to parallelize system execution efficiently.
*/
class SparseSet {

};



class EntityManager {
public:
	EntityManager() : nextEntity(0) {
		Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
	};
	~EntityManager() = default;


	/* Creates a new entity. 
		@return The newly created entity.
	*/
	inline Entity createEntity() {
		Entity newEntity{};
		newEntity.id = nextEntity++;
		activeEntities.push_back(newEntity);
		return newEntity;
	};


	/* Destroys an entity.
		@param entity: The entity to be destroyed.
	*/
	inline void destroyEntity(Entity& entity) {
		auto findPtr = std::find(activeEntities.begin(), activeEntities.end(), entity);
		if (findPtr == activeEntities.end()) {
			throw Log::RuntimeException(__FUNCTION__, "Unable to destroy entity #" + std::to_string(entity.id) + ": Entity does not exist!");
		}

		activeEntities.erase(findPtr);
	}


	/* Updates an entity. 
		@param entity: The entity to be updated.
	*/
	inline void updateEntity(Entity& entity) {
		auto findPtr = std::find(activeEntities.begin(), activeEntities.end(), entity);
		if (findPtr == activeEntities.end()) {
			throw Log::RuntimeException(__FUNCTION__, "Unable to destroy entity #" + std::to_string(entity.id) + ": Entity does not exist!");
		}

		activeEntities[findPtr - activeEntities.begin()] = entity;
	}


	/* Gets all active entities. */
	inline const std::vector<Entity>& getActiveEntities() const { return activeEntities; }

private:
	EntityID nextEntity;
	std::vector<Entity> activeEntities;
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



template<typename Component>
class ComponentArray {
public:
	ComponentArray(std::shared_ptr<EntityManager> entityManager) : entityManager(entityManager) {};
	~ComponentArray() = default;

	/* Attaches an entity to the component array.
		@param entity: The entity to be attached.
		@param component: The component data.
	*/
	inline void attach(Entity& entity, const Component& component) {
		ecMap[entity] = component;
		components.push_back(component);

		entity.componentMask.set(componentID);
		entityManager->updateEntity(entity);
	}


	/* Detaches an entity from the component array.
		@param entity: The entity to be detached.
	*/
	inline void detach(Entity& entity) {
		if (!contains(entity)) {
			throw Log::RuntimeException(__FUNCTION__, "Unable to detach entity #" + std::to_string(entity.id) + " from the component array!");
		}

		ecMap.erase(entity);
		components.erase(std::find(components.begin(), components.end(), ecMap[entity]));

		entity.componentMask.reset(componentID);
		entityManager->updateEntity(entity);
	}
	

	/* Checks whether an entity exists in the component array.
		@param entity: The entity to be checked.
	*/
	inline bool contains(Entity& entity) {
		return ecMap.find(entity) != ecMap.end();
	}


	/* Queries an entity to get its component.
		@param entity: The entity to be queried.
	*/
	inline Component& getComponent(Entity& entity) {
		if (!contains(entity)) {
			throw Log::RuntimeException(__FUNCTION__, "Unable to get component from entity #" + std::to_string(entity.id) + ": Entity does not exist!");
		}

		return ecMap.at(entity);
	}


	/* Gets all components in the component array. */
	inline const std::vector<Component>& getAllComponents() const { return components; }


	/* Gets all entity-component mappings. */
	inline const std::unordered_map<Entity, Component>& getECMappings() const { return ecMap; }

private:
	std::shared_ptr<EntityManager> entityManager;

	const size_t componentID = ComponentTypeID::get<Component>();
	std::unordered_map<Entity, Component> ecMap;

	std::vector<Component> components;
};



class View {
public:
	/* Creates a view of the component arrays.
		@param entityManager: Used to narrow down the view scope to entities that are only created by it.
		@param arrays: The component arrays to be viewed.
		@return A view of the component arrays.
	*/
	template<typename... Components>
	static auto getView(std::shared_ptr<EntityManager> entityManager) {
		constexpr size_t argCount = sizeof...(Components);
		if (argCount == 0) {
			throw Log::RuntimeException(__FUNCTION__, "No components are passed into view!");
		}
		
		return InternalView<Components...>(entityManager);
	}

private:

	template<typename... Components>
	class InternalView {
	public:
		InternalView(std::shared_ptr<EntityManager> entityManager) :
			entityManager(entityManager) {
		
			// Queries entities to look for those with the required components
			requiredMask = buildComponentMask<Components...>();
			auto allEntities = entityManager->getActiveEntities();
			updateMatchingEntities(allEntities);
		};


		/* Ignores a variable number of components.
			@tparam IgnoredComponents: The components whose entities having them attached are to be ignored in the view.
		*/
		template<typename... IgnoredComponents>
		inline void ignoreComponents() {
			ignoredMask = buildComponentMask<IgnoredComponents...>();
			updateMatchingEntities(matchingEntities);
		}


		class Iterator {
		public:
			Iterator(InternalView* view, size_t index) :
				view(view), index(index) {}


			// Dereferencing
			// This is necessary for compatibility with native range-based loops and structured bindings.
			auto operator*() {
				Entity& entity = view->matchingEntities[index];

				auto generateTuple = [&](Components&... components) {
					return std::tuple<Entity&, Components&...>(entity, components...);
				};

				return std::apply(generateTuple, view->matchingEntityComponents);
			}


			// Prefix increment
			Iterator& operator++() {
				index++;
				return *this;
			}


			// Suffix increment
			Iterator operator++(int) {
				Iterator tmp = *this;
				++(*this);
				return tmp;
			}


			bool operator!=(const Iterator& other) const {
				return index != other.index;
			}

		private:
			InternalView* view;
			size_t index;
		};
		

		Iterator begin() { return Iterator(this, 0); }
		Iterator end()   { return Iterator(this, matchingEntities.size()); }

	private:
		std::shared_ptr<EntityManager> entityManager;

		std::vector<Entity> matchingEntities;
		ComponentMask requiredMask;
		ComponentMask ignoredMask;


		/* Updates the list of entities according to filters (e.g., required/ignored masks).
			@param sourceEntities: The source vector of entities used as data for updating matching entities from.
		*/
		inline void updateMatchingEntities(std::vector<Entity>& sourceEntities) {
			std::vector<Entity> temp;
			temp.reserve(sourceEntities.size());

			for (auto& entity : sourceEntities) {
				if (
					((entity.componentMask & requiredMask) == requiredMask) &&	// Entity mask must match the required mask
					((entity.componentMask & ignoredMask).none())				// Entity mask must NOT match the ignored mask
					) {

					temp.push_back(entity);
				}
			}

			matchingEntities.swap(temp);
		}

		
		/* Creates a component mask.
			@tparam SpecifiedComponents: Components to be included in the mask.
			
			@return The newly created component mask.
		*/
		template<typename... SpecifiedComponents>
		inline ComponentMask& buildComponentMask() {
			ComponentMask mask;
			(mask.set(ComponentTypeID::get<SpecifiedComponents>()), ...);
			return mask;
		}
	};
};
