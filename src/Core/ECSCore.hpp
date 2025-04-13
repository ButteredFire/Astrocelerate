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
bool operator==(const Entity& lhs, const Entity& rhs) {
	return lhs.id == rhs.id;
}
namespace std {
	template<>
	struct hash<Entity> {
		std::size_t operator()(const Entity& e) const noexcept {
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
		components[entity] = component;
		entity.componentMask.set(componentID);
		entityManager->updateEntity(entity);

		Log::print(Log::T_DEBUG, __FUNCTION__, "Attached component #" + std::to_string(componentID) + " to entity #" + std::to_string(entity.id));
	}


	/* Detaches an entity from the component array.
		@param entity: The entity to be detached.
	*/
	inline void detach(Entity& entity) {
		if (!contains(entity)) {
			throw Log::RuntimeException(__FUNCTION__, "Unable to detach entity #" + std::to_string(entity.id) + " from the component array!");
		}

		components.erase(entity);
		entity.componentMask.reset(componentID);
		entityManager->updateEntity(entity);

		Log::print(Log::T_DEBUG, __FUNCTION__, "Detached component #" + std::to_string(componentID) + " from entity #" + std::to_string(entity.id));
	}
	

	/* Checks whether an entity exists in the component array.
		@param entity: The entity to be checked.
	*/
	inline bool contains(Entity& entity) {
		return components.find(entity) != components.end();
	}


	/* Queries an entity to get its component.
		@param entity: The entity to be queried.
	*/
	inline Component& getComponent(Entity& entity) {
		if (!contains(entity)) {
			throw Log::RuntimeException(__FUNCTION__, "Unable to get component from entity #" + std::to_string(entity.id) + ": Entity does not exist!");
		}

		return components.at(entity);
	}


	/* Gets all entity-component mappings. */
	inline const std::unordered_map<Entity, Component>& getAll() const { return components; };

private:
	std::shared_ptr<EntityManager> entityManager;

	const size_t componentID = ComponentTypeID::get<Component>();
	std::unordered_map<Entity, Component> components;
};



class View {
public:
	/* Creates a view of the component arrays.
		@param entityManager: Used to narrow down the view scope to entities that are only created by it.
		@param arrays: The component arrays to be viewed.
		@return A view of the component arrays.
	*/
	template<typename... Components>
	static auto getView(std::shared_ptr<EntityManager> entityManager, ComponentArray<Components>&... arrays) {
		constexpr size_t argCount = sizeof...(arrays);
		if (argCount == 0) {
			throw Log::RuntimeException(__FUNCTION__, "No components are passed into view!");
		}
		
		return InternalView<Components...>(entityManager, arrays...);
	}

private:

	template<typename... Components>
	class InternalView {
	public:
		InternalView(std::shared_ptr<EntityManager> entityManager, ComponentArray<Components>&... arrays) :
			entityManager(entityManager), componentArrays(std::tie(arrays...)) {
		
			// Queries entities to look for those with the required components
			requiredMask = buildRequiredMask();
			auto& allEntities = entityManager->getActiveEntities();
			matchingEntities.reserve(allEntities.size()); // Reserve for fast push-back operations

			for (auto& entity : allEntities) {
				if ((entity.componentMask & requiredMask) == requiredMask) {
					matchingEntities.push_back(entity);
				}
			}
		};


		class Iterator {
		public:
			Iterator(InternalView* view, size_t index) :
				view(view), index(index) {}

			auto operator*() {
				Entity& entity = view->matchingEntities[index];

				auto generateTuple = [&](ComponentArray<Components>&... arrays) {
					return std::tuple<Entity&, Components&...>(entity, arrays.getComponent(entity)...);
				};

				return std::apply(generateTuple, view->componentArrays);
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
		Iterator end() { return Iterator(this, matchingEntities.size()); }

	private:
		std::shared_ptr<EntityManager> entityManager;

		std::tuple<ComponentArray<Components>&...> componentArrays;

		std::vector<Entity> matchingEntities;
		ComponentMask requiredMask;

		inline ComponentMask& buildRequiredMask() {
			ComponentMask requiredMask;
			(requiredMask.set(ComponentTypeID::get<Components>()), ...); // NOTE: This is a fold expression.
			return requiredMask;
		}
	};
};
