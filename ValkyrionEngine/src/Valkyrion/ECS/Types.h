#pragma once
#include <cstdint>
#include <bitset>
#include <array>

namespace Valkyrion::ECS
{
    using Entity = uint32_t;
    const Entity MAX_ENTITIES = 5000;

    using ComponentType = uint8_t;
    const ComponentType MAX_COMPONENTS = 32;

    using Signature = std::bitset<MAX_COMPONENTS>;
}
