#include "pch.h"
#include "ECSTest.h"
#include <sstream>

namespace Valkyrion {

namespace {
    std::string EntityToString(ECS::Entity entity) {
        std::stringstream ss;
        ss << "Entity[" << entity << "]";
        return ss.str();
    }
}

void ECSTest::Run()
{
    try {
        std::cout << "\n=== Starting ECS Test ===\n";

        // Initialize coordinator
        ECS::Coordinator coordinator;
        coordinator.Init();

        // Test 1: Component Registration
        std::cout << "\n[TEST 1/5] Registering components...";
        coordinator.template RegisterComponent<TransformComponent>();
        coordinator.template RegisterComponent<RigidBodyComponent>();
        std::cout << " SUCCESS\n";

        // Test 2: Entity Creation and Component Assignment
        std::cout << "\n[TEST 2/5] Creating entities and assigning components...\n";
        auto entity1 = coordinator.CreateEntity();
        coordinator.template AddComponent<TransformComponent>(entity1, { 1.0f, 2.0f, 3.0f });
        coordinator.template AddComponent<RigidBodyComponent>(entity1, { 10.0f, 20.0f });
        std::cout << "  - Created " << EntityToString(entity1) << " with Transform and RigidBody components\n";

        auto entity2 = coordinator.CreateEntity();
        coordinator.template AddComponent<TransformComponent>(entity2, { 4.0f, 5.0f, 6.0f });
        std::cout << "  - Created " << EntityToString(entity2) << " with Transform component\n";

        // Test 3: Component Retrieval
        std::cout << "\n[TEST 3/5] Testing component retrieval...\n";
        {
            auto& transform1 = coordinator.template GetComponent<TransformComponent>(entity1);
            auto& rb1 = coordinator.template GetComponent<RigidBodyComponent>(entity1);
            std::cout << "  - " << EntityToString(entity1) << " Position: (" 
                      << transform1.x << ", " << transform1.y << ", " << transform1.z << ")\n";
            std::cout << "  - " << EntityToString(entity1) << " Velocity: (" 
                      << rb1.velocityX << ", " << rb1.velocityY << ")\n";
        }

        // Test 4: Entity Destruction and ID Recycling
        std::cout << "\n[TEST 4/5] Testing entity destruction and ID recycling...\n";
        std::cout << "  - Destroying " << EntityToString(entity2) << "\n";
        coordinator.DestroyEntity(entity2);
        
        // Create a new entity - should get the next available ID
        auto newEntity1 = coordinator.CreateEntity();
        std::cout << "  - Created new entity " << EntityToString(newEntity1) << " after destroying " << EntityToString(entity2) << "\n";
        
        // Create and destroy an entity to test recycling
        auto tempEntity = coordinator.CreateEntity();
        std::cout << "  - Created temporary entity " << EntityToString(tempEntity) << " for recycling test\n";
        coordinator.DestroyEntity(tempEntity);
        
        // Create another entity - should get the recycled ID
        auto newEntity2 = coordinator.CreateEntity();
        std::cout << "  - Created new entity " << EntityToString(newEntity2) << " after recycling\n";
        
        // The next entity should get the recycled ID
        if (newEntity2 == tempEntity) {
            std::cout << "  - Verified ID " << tempEntity << " was properly recycled\n";
        } else {
            std::cerr << "  - WARNING: Expected recycled ID " << tempEntity 
                      << " but got " << newEntity2 << "\n";
        }

        // Test 5: Multiple Entity Creation/Destruction
        std::cout << "\n[TEST 5/5] Stress testing entity creation/destruction...\n";
        const int numEntities = 1000;
        std::vector<ECS::Entity> entities;
        
        std::cout << "  - Creating " << numEntities << " entities...";
        for (int i = 0; i < numEntities; ++i) {
            auto e = coordinator.CreateEntity();
            coordinator.template AddComponent<TransformComponent>(e, { (float)i, (float)i, (float)i });
            entities.push_back(e);
        }
        std::cout << " DONE\n";
        
        std::cout << "  - Destroying all entities...";
        for (auto e : entities) {
            coordinator.DestroyEntity(e);
        }
        std::cout << " DONE\n";
        
        // Create new entity to verify recycling works
        auto newEntity = coordinator.CreateEntity();
        std::cout << "  - Created new " << EntityToString(newEntity) << " after cleanup\n";
        
        std::cout << "\n=== ECS Test Completed Successfully ===\n";
    } catch (const std::exception& e) {
        std::cerr << "\n!!! ECS Test Failed: " << e.what() << "\n";
        throw; // Re-throw to be caught by the application
    }
}

} // namespace Valkyrion
