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
	EntityManager() : nextEntity(0) {
		Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
	};
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
	inline Component& getComponent(Entity entity) {
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


class View {
public:
	template<typename... Components>
	static auto getView(ComponentArray<Components>... args) {
		constexpr size_t argCount = sizeof...(args);
		if (argCount == 0) {
			throw Log::RuntimeException(__FUNCTION__, "No components are passed into view!");
		}

		return InternalView<Components...>(args...);
	}

private:

	template<typename... Components>
	class InternalView {
	public:
		InternalView(ComponentArray<Components>... args) : components(args...) {};

		template<typename LoopFunc>
		void forEach(LoopFunc func) {
			constexpr size_t tupleSize = std::tuple_size<decltype(components)>::value;
			if (tupleSize == 0) {
				Log::print(Log::T_WARNING, __FUNCTION__, "No detectable component tuple to be iterated over!");
				return;
			}

			auto& firstComponentArray = std::get<0>(components);

			for (auto& [entity, component] : firstComponentArray.getAll()) {
				auto componentArrayContainsEntity = [this, entity](auto&... arrays) { return (arrays.contains(entity) && ...); };

				if (std::apply(componentArrayContainsEntity, components)) {
					auto finalFunc = [this, func, entity](auto&... arrays) { func(entity, arrays.getComponent(entity)...); };
					std::apply(finalFunc, components);
				}
			}

		}

	private:
		std::tuple<ComponentArray<Components>...> components;
	};
};

