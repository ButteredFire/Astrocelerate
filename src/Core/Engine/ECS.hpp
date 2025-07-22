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

		Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
	};
	~EntityManager() = default;


	/* Creates a new entity.
		@return The newly created entity.
	*/
	inline Entity createEntity(const std::string& name) {
		if (m_availableIDs.size() == 0) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot create new entity: Entity count has reached the limit of " + std::to_string(MAX_ENTITIES) + " entities!");
		}

		EntityID newID = m_availableIDs.front();
		m_availableIDs.pop();

		Entity newEntity{};
		newEntity.id = newID;
		newEntity.name = name;
		//newEntity.version = m_versions[newID];

		m_activeEntityIDs.push_back(newEntity.id);
		m_componentMasks.push_back(ComponentMask{});

		m_entityIDToIndexMap[newEntity.id] = (m_activeEntityIDs.size() - 1);
		m_entityIDToEntityMap[newEntity.id] = newEntity;

		return newEntity;
	};


	/* Destroys an entity.
		@param entity: The entity to be destroyed.
	*/
	inline void destroyEntity(Entity entity) {
		if (m_entityIDToIndexMap.find(entity.id) == m_entityIDToIndexMap.end()) {
			Log::Print(Log::T_WARNING, __FUNCTION__, "Cannot destroy entity #" + std::to_string(entity.id) + " as it does not exist.");
			return;
		}

		size_t currentIndex = entity.id;
		size_t lastIndex = (m_activeEntityIDs.size() - 1);

		std::swap(m_activeEntityIDs[currentIndex], m_activeEntityIDs[lastIndex]);
		std::swap(m_componentMasks[currentIndex], m_componentMasks[lastIndex]);

		m_entityIDToIndexMap[m_activeEntityIDs[currentIndex]] = currentIndex;

		m_activeEntityIDs.pop_back();
		m_componentMasks.pop_back();
		m_entityIDToIndexMap.erase(entity.id);

		m_componentMasks[entity.id].reset();
		m_availableIDs.push(entity.id);
	}


	/* Gets all entities. */
	inline std::unordered_map<EntityID, Entity>& getAllEntities() { return m_entityIDToEntityMap; }


	/* Gets all active entity IDs. */
	inline std::vector<EntityID>& getAllEntityIDs() { return m_activeEntityIDs; }


	/* Gets active entity component masks. */
	inline std::vector<ComponentMask>& getAllComponentMasks() { return m_componentMasks; }

	/* Sets an entity's component mask.
		@param entityID: The ID of the entity whose component mask needs to be set.
		@param mask: The mask to set the entity's component mask to.
	*/
	inline void setComponentMask(EntityID entityID, ComponentMask mask) {
		m_componentMasks[entityID] = mask;
	}


	inline const ComponentMask& getComponentMask(EntityID entityID) const { return m_componentMasks[entityID]; }

private:
	std::queue<EntityID> m_availableIDs;
	std::vector<EntityID> m_activeEntityIDs;
	std::vector<ComponentMask> m_componentMasks;

	std::unordered_map<EntityID, size_t> m_entityIDToIndexMap;
	std::unordered_map<EntityID, Entity> m_entityIDToEntityMap;
};



class ComponentManager {
public:
	ComponentManager() {
		Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
	}
	~ComponentManager() = default;


	/* Initializes a component array. 
		@tparam Component: The component type of the component array.
	*/
	template<typename Component>
	inline void initComponentArray() {
		const char* typeName = typeid(Component).name();

		if (m_componentTypes.find(typeName) != m_componentTypes.end()) {
			Log::Print(Log::T_WARNING, __FUNCTION__, "Skipping initialization of component array of type " + enquote(typeName) + " as it has already been initialized.");

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

		if (m_componentArrays.find(typeName) == m_componentArrays.end()) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot get component array of type " + enquote(typeName) + ": Component array does not exist!\nMake sure to initialize the component array first before performing operations on it.");
		}

		// static_pointer_cast is like static_cast, but for shared pointers.
		return std::static_pointer_cast<ComponentArray<Component>>(m_componentArrays[typeName]);
	}


	/* Adds a component to a component array. 
		@tparam Component: The component type of the component array.
		
		@param entityID: The ID of the entity owning the component instance to be added.
		@param component: The component instance.
	*/
	template<typename Component>
	inline void addComponent(EntityID entityID, Component component) {
		getComponentArray<Component>()->insert(entityID, component);
	}


	/* Updates an existing component in a component array.
		@param entityID: The ID of the entity owning the component to be updated.
		@param component: The new component data.
	*/
	template<typename Component>
	inline void updateComponent(EntityID entityID, Component component) {
		getComponentArray<Component>()->updateComponent(entityID, component);
	}


	/* Removes a component from a component array.
		@tparam Component: The component type of the component array.

		@param entityID: The ID of the entity owning the component instance to be removed.
	*/
	template<typename Component>
	inline void removeComponent(EntityID entityID) {
		getComponentArray<Component>()->erase(entityID);
	}


	/* Gets a component from a component array.
		@tparam Component: The component type of the component array.

		@param entityID: The ID of the entity owning the requested component.

		@return The requested component.
	*/
	template<typename Component>
	inline Component& getComponent(EntityID entityID) {
		return getComponentArray<Component>()->getComponent(entityID);
	}


	/* Checks whether an entity has a component.
		@tparam Component: The component type of the component array.

		@param entity: The ID of the entity owning the component to be checked.
	
		@return True if the component exists, otherwise False.
	*/
	template<typename Component>
	inline bool entityHasComponent(EntityID entityID) {
		return getComponentArray<Component>()->contains(entityID);
	}


	/* Checks whether the component has been registered.
		@tparam Component: The component type of the component array.

		@return True if the component has been registered, otherwise False.
	*/
	template<typename Component>
	inline bool arrayHasComponent() {
		const char* typeName = typeid(Component).name();
		return (m_componentArrays.find(typeName) != m_componentArrays.end());
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


	/* Returns the number of entries queried. */
	inline size_t size() { return m_matchingEntities.size(); }


	class Iterator {
		public:
			Iterator(InternalView* view, size_t index) :
				view(view), index(index) {}
		
		
			// Dereferencing
			// This is necessary for compatibility with native range-based loops and structured bindings.
			auto operator*() {
				EntityID entityID = view->m_matchingEntities[index];
		
				return std::tuple<EntityID, Components...>(
					entityID,
					view->componentManager.getComponentArray<Components>()->getComponent(entityID)...
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
	inline ComponentMask buildComponentMask() {
		ComponentMask mask{};
		(mask.set(ComponentTypeID::get<SpecifiedComponents>()), ...);
		return mask;
	}
};



class Registry {
public:
	Registry() {
		Entity nullEntity = createEntity("null");
		Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
	}
	~Registry() = default;


	inline Entity createEntity(const std::string& name = "Unknown entity") {
		return entityManager.createEntity(name);
	}


	inline Entity getEntity(EntityID entityID) {
		return entityManager.getAllEntities()[entityID];
	}


	inline bool hasEntity(EntityID entityID) {
		return entityManager.getAllEntities().count(entityID);
	}


	inline void destroyEntity(Entity& entity) {
		entityManager.destroyEntity(entity);
	}


	template<typename Component>
	inline void initComponentArray() {
		componentManager.initComponentArray<Component>();
	}


	template<typename Component>
	inline void addComponent(EntityID entityID, Component component) {
		componentManager.addComponent<Component>(entityID, component);

		ComponentMask mask = entityManager.getComponentMask(entityID);
		mask.set(ComponentTypeID::get<Component>());

		entityManager.setComponentMask(entityID, mask);
	}


	template<typename Component>
	inline void removeComponent(EntityID entityID) {
		componentManager.removeComponent(entityID);
	}


	template<typename Component>
	inline void updateComponent(EntityID entityID, Component component) {
		componentManager.updateComponent(entityID, component);
	}


    template<typename Component>
    inline Component& getComponent(EntityID entityID) {
       if (!componentManager.entityHasComponent<Component>(entityID)) {
           throw Log::RuntimeException(__FUNCTION__, __LINE__, "Entity #" + std::to_string(entityID) + " does not have the component " + enquote(typeid(Component).name()) + "!");
       }

       return componentManager.getComponent<Component>(entityID);
    }


	template<typename Component>
	inline bool hasComponent(EntityID entityID) {
		return componentManager.entityHasComponent<Component>(entityID);
	}


	inline void clear() {
		// NOTE: Assigning a new object automatically destroys the old class instance and constructs/moves the new one
		entityManager = EntityManager();
		componentManager = ComponentManager();
	}


	template<typename... Components>
	auto getView() {
		constexpr size_t argCount = sizeof...(Components);
		if (argCount == 0) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot get view: No components are passed into view!");
		}

		/* NOTE: C++20 allows the use of lambda expressions for iterating over parameter packs:
			`[&]<typename T>() {...}`     is a generic lambda;
			`.template operator()<Ts>()`  explicitly calls the lambda's templated call operator for each type in `Ts...`;
			`(expr, ...)`				  is a (unary right) fold expression that expands the call for each type in the parameter pack.
		*/
		([&]<typename Component>() {
			if (!componentManager.arrayHasComponent<Component>()) {
				throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot get view: The component " + enquote(typeid(Component).name()) + " has not been registered!");
			}
		}.template operator()<Components>(), ...);

		return InternalView<Components...>(entityManager, componentManager);
	}

private:
	EntityManager entityManager;
	ComponentManager componentManager;
};
