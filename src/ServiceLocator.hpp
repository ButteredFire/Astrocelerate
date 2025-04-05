#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>


class ServiceLocator {
public:
	/* Appends a service to the service registry, or modifies an existing service in the service registry.
		@param service: A shared pointer to the service. If the service to be registered already exists, the currently mapped service in the service registry will be updated.
	*/
	template<typename T>
	inline static void registerService(std::shared_ptr<T> service) {
		std::type_index serviceTypeIdx = std::type_index(typeid(T));
		if (services.find(serviceTypeIdx) != services.end()) {
			Log::print(Log::T_WARNING, __FUNCTION__, "Service of type " + enquote(serviceTypeIdx.name()) + " already exists! Overwriting existing service...");
		}

		services[serviceTypeIdx] = service;
	}

	/* Gets a service from the registry.
		@tparam T: the service type.
		
		@return A shared pointer to the service.
	*/
	template<typename T>
	inline static std::shared_ptr<T> getService() {
		auto ptrIt = services.find(std::type_index(typeid(T)));
		if (ptrIt == services.end()) {
			throw Log::RuntimeException(__FUNCTION__, "Failed to find service!");
		}

		return std::static_pointer_cast<T>(ptrIt->second);
	}


private:
	static inline std::unordered_map<std::type_index, std::shared_ptr<void>> services;
};