#pragma once
#include "Types.h"
#include <unordered_map>
#include <memory>
#include <cassert>

namespace Valkyrion::ECS
{
    class IComponentArray
    {
    public:
        virtual ~IComponentArray() = default;
        virtual void EntityDestroyed(Entity entity) = 0;
    };

    template<typename T>
    class ComponentArray : public IComponentArray
    {
    public:
        void InsertData(Entity entity, T component)
        {
            assert(m_EntityToIndexMap.find(entity) == m_EntityToIndexMap.end() && "Component added to same entity more than once.");

            size_t newIndex = m_Size;
            m_EntityToIndexMap[entity] = newIndex;
            m_IndexToEntityMap[newIndex] = entity;
            m_ComponentArray[newIndex] = component;
            ++m_Size;
        }

        void RemoveData(Entity entity)
        {
            assert(m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end() && "Removing non-existent component.");

            size_t indexOfRemovedEntity = m_EntityToIndexMap[entity];
            size_t indexOfLastElement = m_Size - 1;
            
            // Move the last element into the place of the removed element
            if (indexOfRemovedEntity != indexOfLastElement)
            {
                Entity entityOfLastElement = m_IndexToEntityMap[indexOfLastElement];
                
                m_ComponentArray[indexOfRemovedEntity] = m_ComponentArray[indexOfLastElement];
                m_EntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
                m_IndexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;
            }

            // Remove the last element
            m_EntityToIndexMap.erase(entity);
            m_IndexToEntityMap.erase(indexOfLastElement);
            --m_Size;
        }

        T& GetData(Entity entity)
        {
            assert(m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end() && "Retrieving non-existent component.");
            return m_ComponentArray[m_EntityToIndexMap[entity]];
        }

        void EntityDestroyed(Entity entity) override
        {
            if (m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end())
            {
                RemoveData(entity);
            }
        }

    private:
        std::array<T, MAX_ENTITIES> m_ComponentArray;
        std::unordered_map<Entity, size_t> m_EntityToIndexMap;
        std::unordered_map<size_t, Entity> m_IndexToEntityMap;
        size_t m_Size{ 0 };
    };
}
