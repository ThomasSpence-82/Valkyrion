#include "Valkyrion/ECS/Coordinator.h"
#include "Valkyrion/ECS/TestComponents.h"
#include <iostream>

int main()
{
    std::cout << "Starting ECS Test...\n";

    Valkyrion::ECS::Coordinator coordinator;
    coordinator.Init();

    // Register components
    std::cout << "Registering components...\n";
    coordinator.RegisterComponent<TransformComponent>();
    coordinator.RegisterComponent<RigidBodyComponent>();

    // Create entities
    std::cout << "Creating entities...\n";
    auto entity1 = coordinator.CreateEntity();
    coordinator.AddComponent<TransformComponent>(entity1, { 1.0f, 2.0f, 3.0f });
    coordinator.AddComponent<RigidBodyComponent>(entity1, { 10.0f, 20.0f });

    auto entity2 = coordinator.CreateEntity();
    coordinator.AddComponent<TransformComponent>(entity2, { 4.0f, 5.0f, 6.0f });

    // Test component retrieval
    std::cout << "Testing component retrieval...\n";
    auto& transform1 = coordinator.GetComponent<TransformComponent>(entity1);
    std::cout << "Entity 1 Position: (" << transform1.x << ", " << transform1.y << ", " << transform1.z << ")\n";

    auto& rb1 = coordinator.GetComponent<RigidBodyComponent>(entity1);
    std::cout << "Entity 1 Velocity: (" << rb1.velocityX << ", " << rb1.velocityY << ")\n";

    // Test removing a component
    std::cout << "Removing transform component from entity 1...\n";
    coordinator.RemoveComponent<TransformComponent>(entity1);
    
    std::cout << "Test completed successfully!\n";
    
    // Uncomment the line below to test the assertion
    // auto& transform1Again = coordinator.GetComponent<TransformComponent>(entity1);

    return 0;
}
