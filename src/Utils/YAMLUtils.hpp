/* YAMLUtils.hpp - Utilities pertaining to serialization, deserialization, and other types of data manipulation.
*/

#pragma once

#include <Core/Data/YAMLConversions.hpp>
#include <Core/Data/YAMLKeys.hpp>


namespace YAMLUtils {
    /* Populates an ECS component with data from a component node.
        @param componentNode: The YAML Node that contains the data.
        @param data: A reference to the ECS component.

        @return True if the operation is successful, False otherwise.
    */
    template<typename T>
    inline bool GetComponentData(const YAML::Node &componentNode, T &data) {
        if (componentNode["Data"]) {
            data = componentNode["Data"].as<T>();
            return true;
        }
        return false;
    }


    /* Extracts the entity name from a reference string (ref.EntityName).
        @param refStr: The reference string.
    
        @return The entity name.
    */
    inline std::string GetReferenceSubstring(const std::string &refStr) {
        return refStr.substr(YAMLKey::Ref.size(), (refStr.size() - YAMLKey::Ref.size()));
    }
}