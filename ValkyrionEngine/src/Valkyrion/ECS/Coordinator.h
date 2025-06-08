#pragma once
#include "Types.h"
#include "Component.h"
#include <memory>
#include <unordered_map>
#include <queue>

namespace Valkyrion::ECS
{
    class Coordinator
    {
    public:
        void Init()
        {
            // Reset entity tracking
            m_AvailableEntities = std::queue<Entity>();
            m_LivingEntityCount = 0;
            m_NextComponentType = 0;
            m_ComponentTypes.clear();
            m_ComponentArrays.clear();
            m_Signatures.fill(Signature{});
            
            // Start with the first entity ID
            m_AvailableEntities.push(0);
        }

        Entity CreateEntity()
        {
            assert(m_LivingEntityCount < MAX_ENTITIES && "Too many entities in existence.");

            // If we have no available IDs, create a new one
            if (m_AvailableEntities.empty()) {
                Entity newId = m_NextEntityId++;
                assert(newId < MAX_ENTITIES && "Too many entities in existence.");
                ++m_LivingEntityCount;
                return newId;
            }

            // Otherwise, reuse a recycled ID
            Entity id = m_AvailableEntities.front();
            m_AvailableEntities.pop();
            ++m_LivingEntityCount;
            return id;
        }

        void DestroyEntity(Entity entity)
        {
            assert(entity < MAX_ENTITIES && "Entity out of range.");

            // Remove all components from the entity
            for (auto const& pair : m_ComponentArrays)
            {
                auto const& component = pair.second;
                component->EntityDestroyed(entity);
            }

            // Invalidate the destroyed entity's signature
            m_Signatures[entity].reset();

            // Push the destroyed ID back into the queue for reuse
            m_AvailableEntities.push(entity);
            --m_LivingEntityCount;
            
            // If we've destroyed all entities, reset the queue to start from 0
            if (m_LivingEntityCount == 0) {
                m_AvailableEntities = std::queue<Entity>();
                m_AvailableEntities.push(0);
            }
        }

        template<typename T>
        void RegisterComponent()
        {
            const char* typeName = typeid(T).name();
            assert(m_ComponentTypes.find(typeName) == m_ComponentTypes.end() && "Registering component type more than once.");

            // Add this component type to the component type map
            m_ComponentTypes.insert({ typeName, m_NextComponentType });

            // Create a ComponentArray pointer and add it to the component arrays
            m_ComponentArrays.insert({ typeName, std::make_shared<ComponentArray<T>>() });

            ++m_NextComponentType;
        }

        template<typename T>
        void AddComponent(Entity entity, T component)
        {
            // Check if the component already exists on this entity
            if (HasComponent<T>(entity)) {
                // If it exists, just update the existing component
                GetComponent<T>(entity) = component;
            } else {
                // Otherwise, add the new component
                GetComponentArray<T>()->InsertData(entity, component);
                // Update the entity's signature
                auto typeName = typeid(T).name();
                auto componentType = m_ComponentTypes[typeName];
                m_Signatures[entity].set(componentType);
            }
        }

        template<typename T>
        void RemoveComponent(Entity entity)
        {
            GetComponentArray<T>()->RemoveData(entity);
        }

        template<typename T>
        bool HasComponent(Entity entity) const
        {
            auto typeName = typeid(T).name();
            auto it = m_ComponentTypes.find(typeName);
            if (it == m_ComponentTypes.end()) {
                return false;
            }
            return m_Signatures[entity].test(it->second);
        }

        template<typename T>
        T& GetComponent(Entity entity)
        {
            return GetComponentArray<T>()->GetData(entity);
        }

        template<typename T>
        ComponentType GetComponentType()
        {
            const char* typeName = typeid(T).name();
            assert(m_ComponentTypes.find(typeName) != m_ComponentTypes.end() && "Component not registered before use.");
            return m_ComponentTypes[typeName];
        }

        void SetSystemSignature(Signature signature)
        {
            m_Signature = signature;
        }

    private:
        // Map from type string to a component type
        std::unordered_map<const char*, ComponentType> m_ComponentTypes{};

        // Map from type string to a component array
        std::unordered_map<const char*, std::shared_ptr<IComponentArray>> m_ComponentArrays{};

        // The component type to be assigned to the next registered component
        ComponentType m_NextComponentType{};

        // Queue of unused entity IDs
        std::queue<Entity> m_AvailableEntities{};
        
        // Next available entity ID
        Entity m_NextEntityId{0};

        // Array of signatures where the index corresponds to the entity ID
        std::array<Signature, MAX_ENTITIES> m_Signatures{};

        // Total living entities - used to keep limits on how many exist
        uint32_t m_LivingEntityCount{};

        // The system signature
        Signature m_Signature;

        // Convenience function to get the statically cast pointer to the ComponentArray of type T
        template<typename T>
        std::shared_ptr<ComponentArray<T>> GetComponentArray()
        {
            const char* typeName = typeid(T).name();
            assert(m_ComponentTypes.find(typeName) != m_ComponentTypes.end() && "Component not registered before use.");
            return std::static_pointer_cast<ComponentArray<T>>(m_ComponentArrays[typeName]);
        }
    };
}
