#pragma once

#include <Valkyrion/ECS/Coordinator.h>
#include <Valkyrion/ECS/Component.h>
#include <Valkyrion/ECS/System.h>
#include <Valkyrion/ECS/Types.h>
#include <iostream>

// Define test components
struct TransformComponent
{
    float x, y, z;
};

struct RigidBodyComponent
{
    float velocityX, velocityY;
};

namespace Valkyrion {

class ECSTest
{
public:
    static void Run();
};

} // namespace Valkyrion
