/* ECS.hpp - Defines public interfaces for ECSCore.hpp.
*/

#pragma once

#include "ECSCore.hpp"



class EntityManager {
public:
	EntityManager() {
		for (EntityID id = 0; id < MAX_ENTITIES; id++) {
			m_availableIDs.push(id);
		}

		Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
	};
	~EntityManager() = default;


	/* Creates a new entity.
		@return The newly created entity.
	*/
	inline Entity createEntity() {
		if (m_availableIDs.size() == 0) {
			throw Log::RuntimeException(__FUNCTION__, "Cannot create new entity: Entity count has reached the limit of " + std::to_string(MAX_ENTITIES) + " entities!");
		}

		EntityID newID = m_availableIDs.front();
		m_availableIDs.pop();

		Entity newEntity{};
		newEntity.id = newID;
		//newEntity.version = m_versions[newID];

		m_activeEntityIDs.push_back(newEntity.id);
		m_componentMasks.push_back(ComponentMask{});
		m_entityToIndexMap[newEntity.id] = (m_activeEntityIDs.size() - 1);

		return newEntity;
	};


	/* Destroys an entity.
		@param entity: The entity to be destroyed.
	*/
	inline void destroyEntity(Entity& entity) {
		size_t currentIndex = entity.id;
		size_t lastIndex = (m_activeEntityIDs.size() - 1);

		std::swap(m_activeEntityIDs[currentIndex], m_activeEntityIDs[lastIndex]);
		std::swap(m_componentMasks[currentIndex], m_componentMasks[lastIndex]);

		m_entityToIndexMap[m_activeEntityIDs[currentIndex]] = currentIndex;

		m_activeEntityIDs.pop_back();
		m_componentMasks.pop_back();
		m_entityToIndexMap.erase(entity.id);

		m_componentMasks[entity.id].reset();
		m_availableIDs.push(entity.id);
	}


	/* Gets active entity IDs. */
	inline std::vector<EntityID>& getAllEntityIDs() { return m_activeEntityIDs; }


	/* Gets active entity component masks. */
	inline std::vector<ComponentMask>& getAllComponentMasks() { return m_componentMasks; }

	/* Sets an entity's component mask.
		@param entity: The entity whose component mask needs to be set.
		@param mask: The mask to set the entity's component mask to.
	*/
	inline void setComponentMask(Entity& entity, ComponentMask mask) {
		m_componentMasks[entity.id] = mask;
	}


	inline const ComponentMask& getComponentMask(Entity& entity) const { return m_componentMasks[entity.id]; }

private:
	std::queue<EntityID> m_availableIDs;
	std::vector<EntityID> m_activeEntityIDs;
	std::vector<ComponentMask> m_componentMasks;
	std::unordered_map<EntityID, size_t> m_entityToIndexMap;
};



class ComponentManager {
public:
	ComponentManager() {
		Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
	}
	~ComponentManager() = default;


	/* Initializes a component array. 
		@tparam Component: The component type of the component array.
	*/
	template<typename Component>
	inline void initComponentArray() {
		const char* typeName = typeid(Component).name();

		if (m_componentTypes.find(typeName) != m_componentTypes.end()) {
			Log::print(Log::T_WARNING, __FUNCTION__, "Skipping initialization of component array of type " + enquote(typeName) + " as it has already been initialized.");

			return;
		}

		m_componentTypes[typeName] = ComponentTypeID::get<Component>();
		m_componentArrays[typeName] = std::make_shared<ComponentArray<Component>>();
	}


	/* Gets a component array.
		@tparam Component: The component type of the component array.

		@return A shared pointer to the component array.
	*/
	template<typename Component>
	std::shared_ptr<ComponentArray<Component>> getComponentArray() {
		const char* typeName = typeid(Component).name();

		// static_pointer_cast is like static_cast, but for shared pointers.
		return std::static_pointer_cast<ComponentArray<Component>>(m_componentArrays[typeName]);
	}


	/* Adds a component to a component array. 
		@tparam Component: The component type of the component array.
		
		@param entity: The entity owning the component instance to be added.
		@param component: The component instance.
	*/
	template<typename Component>
	inline void addComponent(Entity& entity, Component& component) {
		getComponentArray<Component>()->insert(entity, component);
	}


	/* Updates an existing component in a component array.
		@param entity: The entity owning the component to be updated.
		@param component: The new component data.
	*/
	template<typename Component>
	inline void updateComponent(Entity& entity, Component& component) {
		getComponentArray<Component>()->updateComponent(entity, component);
	}


	/* Removes a component from a component array.
		@tparam Component: The component type of the component array.

		@param entity: The entity owning the component instance to be removed.
	*/
	template<typename Component>
	inline void removeComponent(Entity& entity) {
		getComponentArray<Component>()->erase(entity);
	}


	/* Gets a component from a component array.
		@tparam Component: The component type of the component array.

		@param entity: The entity owning the requested component.

		@return The requested component.
	*/
	template<typename Component>
	inline Component& getComponent(Entity& entity) {
		return getComponentArray<Component>()->getComponent(entity.id);
	}


	/* Checks whether a component exists in the component array.
		@tparam Component: The component type of the component array.

		@param entity: The entity owning the component to be checked.
	
		@return True if the component exists, otherwise False.
	*/
	template<typename Component>
	inline bool containsComponent(Entity& entity) {
		return getComponentArray<Component>()->contains(entity);
	}

private:
	std::unordered_map<const char*, size_t> m_componentTypes;
	std::unordered_map<const char*, std::shared_ptr<void>> m_componentArrays;
};



template<typename... Components>
class InternalView {
public:
	InternalView(EntityManager& entityManager, ComponentManager& componentManager):
		entityManager(entityManager),
		componentManager(componentManager) {

		m_matchingEntities = entityManager.getAllEntityIDs();
		m_entityComponentMasks = entityManager.getAllComponentMasks();

		m_requiredMask = buildComponentMask<Components...>();
		updateMatchingEntities(m_matchingEntities);
	};

	~InternalView() = default;


	/* Ignores a variable number of components.
		@tparam IgnoredComponents: The components whose entities having them attached are to be ignored in the view.
	*/
	template<typename... IgnoredComponents>
	inline void ignoreComponents() {
		m_ignoredMask = buildComponentMask<IgnoredComponents...>();
		updateMatchingEntities(m_matchingEntities);
	}


	class Iterator {
		public:
			Iterator(InternalView* view, size_t index) :
				view(view), index(index) {}
		
		
			// Dereferencing
			// This is necessary for compatibility with native range-based loops and structured bindings.
			auto operator*() {
				EntityID& entity = view->m_matchingEntities[index];
		
				return std::tuple<EntityID&, Components&...>(
					entity,
					view->componentManager.getComponentArray<Components>()->getComponent(entity)...
				);
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
	Iterator end()   { return Iterator(this, m_matchingEntities.size()); }


private:
	EntityManager& entityManager;
	ComponentManager& componentManager;

	std::vector<EntityID> m_matchingEntities;
	std::vector<ComponentMask> m_entityComponentMasks;

	ComponentMask m_requiredMask, m_ignoredMask;


	/* Updates the list of entities according to filters (e.g., required/ignored masks).
		@param sourceEntities: The source vector of entities used as data for updating matching entities from.
	*/
	inline void updateMatchingEntities(std::vector<EntityID>& sourceEntities) {
		std::vector<EntityID> temp;
		temp.reserve(sourceEntities.size());


		for (size_t i = 0; i < sourceEntities.size(); i++) {
			if (
				((m_entityComponentMasks[i] & m_requiredMask) == m_requiredMask) &&	// Entity mask must match the required mask
				((m_entityComponentMasks[i] & m_ignoredMask).none())				// Entity mask must NOT match the ignored mask
				) {
	
				temp.push_back(sourceEntities[i]);
			}

		}
	
		m_matchingEntities.swap(temp);
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



class Registry {
public:
	Registry() {
		Log::print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
	}
	~Registry() = default;

	inline Entity createEntity() {
		return entityManager.createEntity();
	}


	inline void destroyEntity(Entity& entity) {
		entityManager.destroyEntity(entity);
	}


	template<typename Component>
	inline void initComponentArray() {
		componentManager.initComponentArray<Component>();
	}


	template<typename Component>
	inline void addComponent(Entity& entity, Component& component) {
		componentManager.addComponent<Component>(entity, component);

		ComponentMask mask = entityManager.getComponentMask(entity);
		mask.set(ComponentTypeID::get<Component>());

		entityManager.setComponentMask(entity, mask);
	}


	template<typename Component>
	inline void updateComponent(Entity& entity, Component& component) {
		componentManager.updateComponent(entity, component);
	}


	template<typename Component>
	inline Component& getComponent(Entity& entity) {
		return componentManager.getComponent<Component>(entity);
	}


	template<typename Component>
	inline bool hasComponent(Entity& entity) {
		return componentManager.containsComponent<Component>(entity);
	}


	template<typename... Components>
	auto getView() {
		constexpr size_t argCount = sizeof...(Components);
		if (argCount == 0) {
			throw Log::RuntimeException(__FUNCTION__, "No components are passed into view!");
		}

		return InternalView<Components...>(entityManager, componentManager);
	}

private:
	EntityManager entityManager;
	ComponentManager componentManager;
};