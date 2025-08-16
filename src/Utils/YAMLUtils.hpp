/* YAMLUtils.hpp - Utilities pertaining to serialization, deserialization, and other types of data manipulation.
*/

#pragma once

#include <Core/Data/YAMLConversions.hpp>
#include <Core/Data/YAMLKeys.hpp>


namespace YAMLUtils {
    /* Attempts to populate a struct/primitive with data from a YAML file.
        @param data: A pointer to the target struct/primitive.
        @param entryKey: The key name for the entry.
        @param node: The YAML Node that contains the data.

        @return True if the operation is successful, False otherwise.
    */
    template<typename T>
    inline bool TryGetEntryData(T *data, const std::string &entryKey, const YAML::Node &node) {
        if (node[entryKey]) {
            *data = node[entryKey].as<T>();
            return true;
        }
        return false;
    }


    /* Populates an ECS component with data from a component node.
    
        This is a utility function for YAMLUtils::TryGetEntryData.
        
        @param data: A pointer to the ECS component.
        @param componentNode: The YAML Node that contains the data.

        @return True if the operation is successful, False otherwise.
    */
    template<typename T>
    inline bool GetComponentData(T *data, const YAML::Node &componentNode) {
        if (componentNode[YAMLScene::Entity_Components_Type_Data]) {
            *data = componentNode[YAMLScene::Entity_Components_Type_Data].as<T>();
            return true;
        }
        return false;
    }
}