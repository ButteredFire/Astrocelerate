/* ECSCore.hpp - Defines core features of the entity-component system.
*/

#pragma once

#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <climits>

#include <Core/LoggingManager.hpp>


using Entity = uint32_t;
constexpr Entity INVALID_ENTITY = UINT32_MAX;


class EntityManager {
public:
	EntityManager() : nextEntity(0) {};
	~EntityManager() = default;


	/* Creates a new entity. 
		@return The newly created entity.
	*/
	inline Entity createEntity() {
		Entity id = nextEntity++;
		activeEntities.insert(id);
		return id;
	};


	/* Destroys an entity.
		@param entity: The entity to be destroyed.
	*/
	inline void destroyEntity(Entity entity) {
		auto findPtr = activeEntities.find(entity);
		if (findPtr == activeEntities.end()) {
			throw Log::RuntimeException(__FUNCTION__, "Unable to destroy entity #" + std::to_string(entity) + ": Entity does not exist!");
		}

		activeEntities.erase(entity);
	}


	/* Gets all active entities. */
	inline const std::unordered_set<Entity>& getActiveEntities() const { return activeEntities; }

private:
	Entity nextEntity;
	std::unordered_set<Entity> activeEntities;
};



template<typename Component>
class ComponentArray {
public:
	/* Attaches an entity to the component array.
		@param entity: The entity to be attached.
		@param component: The component data.
	*/
	inline void attach(Entity entity, const Component& component) {
		components[entity] = component;
	}


	/* Detaches an entity from the component array.
		@param entity: The entity to be detached.
	*/
	inline void detach(Entity entity) {
		if (!contains(entity)) {
			throw Log::RuntimeException(__FUNCTION__, "Unable to detach entity #" + std::to_string(entity) + " from the component array!");
		}

		components.erase(entity);
	}
	

	/* Checks whether an entity exists in the component array.
		@param entity: The entity to be checked.
	*/
	inline bool contains(Entity entity) const {
		return components.find(entity) != components.end();
	}


	/* Queries an entity to get its component.
		@param entity: The entity to be queried.
	*/
	inline Component& getComponent(Entity entity) const {
		if (!contains(entity)) {
			throw Log::RuntimeException(__FUNCTION__, "Unable to get component from entity #" + std::to_string(entity) + ": Entity does not exist!");
		}

		return components.at(entity);
	}


	/* Gets all entity-component mappings. */
	inline const std::unordered_map<Entity, Component>& getAll() const { return components; };

private:
	std::unordered_map<Entity, Component> components;
};


template<typename... Components>
class View {
public:
	View(const ComponentArray<Components>... args) : components(args) {};
	
	template<typename LoopFunc>
	void forEach(LoopFunc func) const {
		for (const auto& [entity, component] : std::get<0>(components).data()) {
			if (std::get<ComponentArray<Components>>(components).contains(entity)) {
				func(entity, std::get<ComponentArray<Components>>(components).getComponent(entity));
			}
		}
	}

	
private:
	std::tuple<const ComponentArray<Components>...> components;
};