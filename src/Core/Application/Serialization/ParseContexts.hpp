/* ParseContexts - Defines parse contexts for different files (e.g., JSON, YAML).
*/

#pragma once

#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>


#include <Core/Data/Mapping/YAMLKeys.hpp>

#include <Engine/Registry/ECS/ECSCore.hpp>


struct IParseCtx {
	virtual ~IParseCtx() = default; // This is necessary for proper cleanup
};


struct JSONParseCtx : public IParseCtx {
	nlohmann::json *data;
};


struct YAMLParseCtx : public IParseCtx {
	std::unordered_map<std::string, EntityID> *sceneEntities;
	std::unordered_map<std::string, YAML::Node> *selfComponents;

	EntityID entityID;
	std::string entityName;
	std::string currentComponentType;
};
