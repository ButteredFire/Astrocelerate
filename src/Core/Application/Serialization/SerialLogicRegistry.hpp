#pragma once

#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>

#include <yaml-cpp/yaml.h>


#include "ParseContexts.hpp"


/* Stores the deserialization and serialization logic for file parsing/saving. */
class SerialLogicRegistry {
public:
	~SerialLogicRegistry() = default;
	
	/* Function signature for deserialization */
	using DeserialLogic = std::function<void(IParseCtx *)>;


	/* Sets the deserialization logic for a particular type.
		@param typeName: The name of the type, as defined in the target file.
		@param deserialLogic: The deserialization lambda.
	*/
	inline void setDeserialLogic(const std::string &typeName, DeserialLogic deserialLogic) {
		m_loaders[typeName] = std::move(deserialLogic);
	}


	/* Deserializes data.
		@param typeName: The name of the type, as defined in the target file.
		@param ctx: The pointer to the parse context.
	*/
	inline void deserialize(const std::string &typeName, IParseCtx *ctx) {
		if (auto it = m_loaders.find(typeName); it != m_loaders.end())
			it->second(ctx);
	}

private:
	std::unordered_map<std::string, DeserialLogic> m_loaders;
};