#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>

#include <Core/LoggingManager.hpp>


class ServiceLocator {
public:
	/* Appends a service to the service registry, or modifies an existing service in the service registry.
		@param service: A shared pointer to the service. If the service to be registered already exists, the currently mapped service in the service registry will be updated.
	*/
	template<typename T>
	inline static void RegisterService(std::shared_ptr<T> service) {
		std::type_index serviceTypeIdx = std::type_index(typeid(T));
		if (m_services.find(serviceTypeIdx) != m_services.end()) {
			Log::print(Log::T_WARNING, __FUNCTION__, "Service of type " + enquote(serviceTypeIdx.name()) + " already exists! Overwriting existing service...");
		}

		m_services[serviceTypeIdx] = service;
	}

	/* Gets a service from the registry.
		@tparam T: The service type.
		@param caller: The origin from which the call to this function is made.
		
		@return A shared pointer to the service.
	*/
	template<typename T>
	inline static std::shared_ptr<T> GetService(const char* caller) {
		std::type_index serviceTypeIdx = std::type_index(typeid(T));
		auto ptrIt = m_services.find(serviceTypeIdx);
		if (ptrIt == m_services.end()) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to find service of type " + enquote(serviceTypeIdx.name()) + "!"
				+ '\n' + "Service retrieval requested from " + std::string(caller) + ".");
		}

		return std::static_pointer_cast<T>(ptrIt->second);
	}


private:
	static inline std::unordered_map<std::type_index, std::shared_ptr<void>> m_services;
};