/* SubpassBinder.hpp - Manages the binding of renderables to subpasses.  
*/  

#pragma once  

#include <map>
#include <unordered_map>
#include <vector>  
#include <type_traits>  
#include <stdexcept>  

#include <Core/Constants.h>  
#include <Core/LoggingManager.hpp>  

#include <Engine/Components/RenderComponents.hpp>


class SubpassBinder {  
public:  
   inline SubpassBinder() {
       
       createComponentSubpassMappings();
       m_components.reserve(m_subpassToComp.size());

       Log::Print(Log::T_DEBUG, __FUNCTION__, "Initialized.");
   }
   ~SubpassBinder() = default;


   template<typename Component>
   inline void linkComponent(Component* component) {
       size_t componentTypeHash = typeid(Component).hash_code();
       if (m_compToSubpass.find(componentTypeHash) == m_compToSubpass.end()) {
           throw Log::RuntimeException(__FUNCTION__, __LINE__, "Cannot link component of type " + enquote(typeid(Component).name()) + " to subpass: Component is not supported!");
       }

       m_components[m_compToSubpass[componentTypeHash]].push_back(static_cast<void*>(component));
   }


   inline std::map<SubpassConsts::Type, std::vector<void*>> getSubpassComponentMappings() {
       std::map<SubpassConsts::Type, std::vector<void*>> mapping;

       mapping[SubpassConsts::T_MAIN] = m_components[SubpassConsts::T_MAIN];
       mapping[SubpassConsts::T_IMGUI] = m_components[SubpassConsts::T_IMGUI];

       return mapping;
   }


private:
    std::vector<std::vector<void*>> m_components;

    std::map<SubpassConsts::Type, std::unordered_set<size_t>> m_subpassToComp;
    std::unordered_map<size_t, SubpassConsts::Type> m_compToSubpass;


    /* Maps components to subpasses via their hash codes, and vice versa. */
    inline void createComponentSubpassMappings() {
        size_t meshRenderable = typeid(RenderComponent::MeshRenderable).hash_code();
        size_t guiRenderable = typeid(RenderComponent::GUIRenderable).hash_code();

        m_subpassToComp[SubpassConsts::T_MAIN] = { meshRenderable };

        m_subpassToComp[SubpassConsts::T_IMGUI] = { guiRenderable };


        for (const auto& [type, hashes] : m_subpassToComp) {
            for (const auto& hash : hashes) {
                m_compToSubpass[hash] = type;
            }
        }
    }
};