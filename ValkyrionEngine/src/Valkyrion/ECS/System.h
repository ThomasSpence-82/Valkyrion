#pragma once
#include "Types.h"
#include <set>

namespace Valkyrion::ECS
{
    class System
    {
    public:
        std::set<Entity> m_Entities;
    };
}
