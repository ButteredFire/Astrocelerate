/* ECS.hpp - Defines public interfaces for ECSCore.hpp.
*/

#pragma once

#include <any>
#include <mutex>
#include <typeindex>

#include "ECSCore.hpp"


class EntityManager {
public:
	EntityManager() :
		m_nextEntity(0) {

		Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
	};
	~EntityManager() = default;


	// To make the non-copyable and non-movable nature of EntityManager clear, the copy, copy assignment, move, and move assignment operators are deleted.
	EntityManager(const EntityManager &) = delete;
	EntityManager &operator=(const EntityManager &) = delete;

	EntityManager(EntityManager &&) = delete;
	EntityManager &operator=(EntityManager &&) = delete;


	/* Creates a new entity.
		@return The newly created entity.
	*/
	inline Entity createEntity(const std::string &name) {
		EntityID newID = m_nextEntity++;

		Entity newEntity{};
		newEntity.id = newID;
		newEntity.name = name;
		//newEntity.version = m_versions[newID];

		// NOTE: For thread-safety, push_back, map insertions, and vector access would need synchronization (e.g., mutexes) if createEntity/destroyEntity are called concurrently.
		m_activeEntityIDs.push_back(newEntity.id);
		m_componentMasks.push_back(ComponentMask{});

		// Ensure vectors are large enough to hold newID if it's not sequential
		if (newID >= m_componentMasks.size()) {
			m_componentMasks.resize(newID + 1);
			// m_versions.resize(newID + 1);
		}

		m_entityIDToIndexMap[newEntity.id] = (m_activeEntityIDs.size() - 1);
		m_entityIDToEntityMap[newEntity.id] = newEntity;

		return newEntity;
	};


	/* Destroys an entity.
		@param entity: The entity to be destroyed.
	*/
	inline void destroyEntity(Entity entity) {
		auto it = m_entityIDToIndexMap.find(entity.id);
		if (it == m_entityIDToIndexMap.end()) {
			Log::Print(Log::T_WARNING, __FUNCTION__, "Cannot destroy entity " + enquote(entity.name) + " (ID #" + std::to_string(entity.id) + "): Entity does not exist!");
			return;
		}

		size_t indexToRemove = it->second;
		size_t lastIndex = (m_activeEntityIDs.size() - 1);

		if (indexToRemove != lastIndex) {
			EntityID lastEntityID = m_activeEntityIDs[lastIndex];

			std::swap(m_activeEntityIDs[indexToRemove], m_activeEntityIDs[lastIndex]);
			//std::swap(m_componentMasks[indexToRemove], m_componentMasks[lastIndex]);	// m_componentMasks is indexed by EntityID, so no need to swap the two here.

			m_entityIDToIndexMap[lastEntityID] = indexToRemove;
		}

		m_activeEntityIDs.pop_back();
		//m_componentMasks.pop_back();	// m_componentMasks is indexed by EntityID
		m_entityIDToIndexMap.erase(entity.id);
		m_entityIDToEntityMap.erase(entity.id);

		if (entity.id < m_componentMasks.size()) {
			m_componentMasks[entity.id].reset();
		}

		// m_versions[entity.id]++;
	}


	/* Gets all entities. */
	inline std::unordered_map<EntityID, Entity> &getAllEntities() { return m_entityIDToEntityMap; }


	/* Gets all active entity IDs. */
	inline std::vector<EntityID> &getAllEntityIDs() { return m_activeEntityIDs; }


	/* Gets active entity component masks. */
	inline std::vector<ComponentMask> &getAllComponentMasks() { return m_componentMasks; }

	/* Sets an entity's component mask.
		@param entityID: The ID of the entity whose component mask needs to be set.
		@param mask: The mask to set the entity's component mask to.
	*/
	inline void setComponentMask(EntityID entityID, ComponentMask mask) {
		m_componentMasks[entityID] = mask;
	}


	inline const ComponentMask &getComponentMask(EntityID entityID) const { return m_componentMasks[entityID]; }


	/* Resets THIS entity manager to its initial state. */
	inline void reset() {
		m_nextEntity = 0;
		m_activeEntityIDs.clear();
		m_componentMasks.clear();
		m_entityIDToIndexMap.clear();
		m_entityIDToEntityMap.clear();
		// NOTE: Clear any versioning here
	}

private:
	std::atomic<EntityID> m_nextEntity;

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
	
		@return True if the component exists, False otherwise.
	*/
	template<typename Component>
	inline bool entityHasComponent(EntityID entityID) {
		return getComponentArray<Component>()->contains(entityID);
	}


	/* Checks whether the component has been registered.
		@tparam Component: The component type of the component array.

		@return True if the component has been registered, False otherwise.
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

		m_requiredMask = buildComponentMask<Components...>();

		init();
	};

	~InternalView() = default;


	/* Refreshes the view to get new data. */
	inline void refresh() {
		init();
	}


	/* Gets the entities that are included in the view. */
	inline auto getMatchingEntities() { return m_matchingEntities; }


	/* Gets the unwrapped view data. */
	inline auto getData() { return m_matchingTuples; }


	/* Ignores a variable number of components.
		@tparam IgnoredComponents: The components whose entities having them attached are to be ignored in the view.
	*/
	template<typename... IgnoredComponents>
	inline void ignoreComponents() {
		m_ignoredMask = buildComponentMask<IgnoredComponents...>();
		init(m_matchingEntities);
	}


	/* Returns the number of entries queried. */
	inline size_t size() { return m_matchingTuples.size(); }


	class Iterator {
		public:
			Iterator(InternalView* view, size_t index) :
				m_view(view), m_index(index) {}
		
		
			// Dereferencing (used internally in range-based loops and structured bindings)
			auto operator*() {
				return m_view->m_matchingTuples[m_index];
			}

			// Prefix increment
			Iterator& operator++() {
				m_index++;
				return *this;
			}
		
			// Suffix increment
			Iterator operator++(int) {
				Iterator tmp = *this;
				++(*this);
				return tmp;
			}
		
			bool operator!=(const Iterator& other) const {
				return m_index != other.m_index;
			}
		
		private:
			InternalView* m_view;
			size_t m_index;
	};
		
		
	Iterator begin() { return Iterator(this, 0); }
	Iterator end()   { return Iterator(this, m_matchingEntities.size()); }

	// Accessing via index
	auto operator[](size_t idx) {
		LOG_ASSERT((idx >= 0 && idx < m_matchingTuples.size()), "Cannot retrieve entity data at index " + TO_STR(idx) + ": Index is out of bounds!");

		return m_matchingTuples[idx];
	}


private:
	EntityManager& entityManager;
	ComponentManager& componentManager;

	std::vector<EntityID> m_matchingEntities;
	std::vector<std::tuple<EntityID, Components...>> m_matchingTuples;

	std::vector<ComponentMask> m_entityComponentMasks;
	ComponentMask m_requiredMask, m_ignoredMask;


	/* Initializes the view.
		@param sourceEntities (optional): The source vector of entities used as data for updating matching entities from. If it is empty, the view will filter entities from all existing entities.
	*/
	inline void init(std::vector<EntityID> sourceEntities = {}) {
		std::vector<EntityID> entities = (sourceEntities.empty()) ? entityManager.getAllEntityIDs() : sourceEntities;
		m_entityComponentMasks = entityManager.getAllComponentMasks();

		updateMatchingEntities(entities);
	}


	/* Updates the list of entities according to filters (e.g., required/ignored masks).
		@param dirtyEntities: The vector of entities whose component data has changed since the last `updateMatchingEntities` call.
	*/
	inline void updateMatchingEntities(std::vector<EntityID> &dirtyEntities) {
		std::vector<std::tuple<EntityID, Components...>> tempTuples;
		tempTuples.reserve(dirtyEntities.size());

		for (size_t i = 0; i < dirtyEntities.size(); i++) {
			// If entity matches filters, update/add its values in m_matchingEntities and m_matchingTuples
			if (
				((m_entityComponentMasks[i] & m_requiredMask) == m_requiredMask) &&	// Entity mask must match the required mask
				((m_entityComponentMasks[i] & m_ignoredMask).none())				// Entity mask must NOT match the ignored mask
				) {

				auto entityData = constructTuple(dirtyEntities[i]);

				auto it = std::find(m_matchingEntities.begin(), m_matchingEntities.end(), dirtyEntities[i]);

				if (it != m_matchingEntities.end()) {
					// If it already exists in matchingEntities and matchingTuples, update
					int index = std::distance(m_matchingEntities.begin(), it);

					std::swap(m_matchingEntities[index], m_matchingEntities.back());

					std::swap(m_matchingTuples[index], m_matchingTuples.back());
					m_matchingTuples.back() = entityData;
				}
				else {
					// Else, add
					m_matchingEntities.push_back(dirtyEntities[i]);
					m_matchingTuples.push_back(entityData);
				}
			}
		}
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


	/* Constructs a tuple of the specified components for a specific matching entity.
		@param id: The ID of the entity.

		@return A corresponding tuple containing the entity ID and the specified components.
	*/
	auto constructTuple(EntityID id) {
		return std::tuple<EntityID, Components...>(
			id,
			componentManager.getComponentArray<Components>()->getComponent(id)...
		);
	}
};


/* A hashmap wrapper that stores View queries. */
class ViewMap {
public:
	/* Inserts a View into the map, or updates the map with a new View. */
	template <typename... Components>
	inline void insert(InternalView<Components...> view) {
		m_viewMap[typeid(InternalView<Components...>)] = std::move(view);
	}


	/* Gets a cached View from the map.
		@tparam Components...: The combination of component types required to perform the operation.
	*/
	template<typename... Components>
	inline auto& get() {
		return std::any_cast<InternalView<Components...>&>(
			m_viewMap.at(typeid(InternalView<Components...>))
		);
	}


	/* Determines whether the map contains a cached View.
		@tparam Components...: The combination of component types required to perform the operation.
	*/
	template<typename... Components>
	inline bool contains() {
		return m_viewMap.find(typeid(InternalView<Components...>)) != m_viewMap.end();
	}


	/* Clears the map. */
	inline void clear() { m_viewMap.clear(); }

private:
	std::unordered_map<std::type_index, std::any> m_viewMap;
};



class Registry {
public:
	Registry() {
		init();
		Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
	}
	~Registry() = default;


	inline Entity createEntity(const std::string& name = "Unknown entity") {
		std::lock_guard<std::mutex> lock(m_entityMutex);
		return entityManager.createEntity(name);
	}


	inline Entity getEntity(EntityID entityID) {
		std::lock_guard<std::mutex> lock(m_entityMutex);
		return entityManager.getAllEntities()[entityID];
	}


	inline bool hasEntity(EntityID entityID) {
		std::lock_guard<std::mutex> lock(m_entityMutex);
		return entityManager.getAllEntities().count(entityID);
	}


	inline void destroyEntity(Entity& entity) {
		std::lock_guard<std::mutex> lock(m_entityMutex);
		entityManager.destroyEntity(entity);
	}


	template<typename Component>
	inline void initComponentArray() {
		std::lock_guard<std::mutex> lock(m_componentMutex);
		componentManager.initComponentArray<Component>();
	}


	template<typename Component>
	inline void addComponent(EntityID entityID, Component component) {
		std::lock_guard<std::mutex> lock(m_componentMutex);

		componentManager.addComponent<Component>(entityID, component);

		ComponentMask mask = entityManager.getComponentMask(entityID);
		mask.set(ComponentTypeID::get<Component>());

		entityManager.setComponentMask(entityID, mask);
	}


	template<typename Component>
	inline void removeComponent(EntityID entityID) {
		std::lock_guard<std::mutex> lock(m_componentMutex);
		componentManager.removeComponent(entityID);
	}


	template<typename Component>
	inline void updateComponent(EntityID entityID, Component component) {
		std::lock_guard<std::mutex> lock(m_componentMutex);
		componentManager.updateComponent(entityID, component);
	}


    template<typename Component>
    inline Component& getComponent(EntityID entityID) {
       if (!componentManager.entityHasComponent<Component>(entityID)) {
           throw Log::RuntimeException(__FUNCTION__, __LINE__, "Entity " + enquote(getEntity(entityID).name) + " (ID #" + std::to_string(entityID) + ") does not have the component " + enquote(typeid(Component).name()) + "!");
       }

       return componentManager.getComponent<Component>(entityID);
    }


	template<typename Component>
	inline bool hasComponent(EntityID entityID) {
		return componentManager.entityHasComponent<Component>(entityID);
	}


	inline void clear() {
		// NOTE: Assigning a new object automatically destroys the old class instance and constructs/moves the new one
		entityManager.reset();
		componentManager = ComponentManager();

		init();

		Log::Print(Log::T_INFO, __FUNCTION__, "Entity-Component Registry has been cleared.");
	}


	template<typename... Components>
	inline auto getView() {
		std::lock_guard<std::mutex> lock(m_viewMutex);

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


	/* Gets the entity of the render space. */
	inline Entity getRenderSpaceEntity() const { return m_renderSpace; }

private:
	EntityManager entityManager;
	ComponentManager componentManager;

	std::mutex m_entityMutex;
	std::mutex m_componentMutex;
	std::mutex m_viewMutex;

	Entity m_renderSpace;


	inline void init() {
		// Null placeholder entity
		Entity nullEntity = createEntity("null");
	
	
		// Global reference frame
		m_renderSpace = createEntity("Scene");
	}
};
