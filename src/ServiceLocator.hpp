#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>


class ServiceLocator {
public:
	template<typename T>
	inline static void registerService(std::shared_ptr<T> service) {
		services[std::type_index(typeid(T))] = service;
	}

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