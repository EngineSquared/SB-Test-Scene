#include <GLFW/glfw3.h>

#include <raylib.h>

#include "Registry.hpp"
#include <iostream>

#include "BoxCollider3D.hpp"
#include "Camera3D.hpp"
#include "RealTimeProvider.hpp"
#include "RealTimeUpdater.hpp"
#include "SoftBodyCollision.hpp"
#include "SoftBodyNode.hpp"
#include "SoftBodySpring.hpp"
#include "Transform.hpp"
#include "VelocityIntegration.hpp"

ES::Engine::Entity CreateParticle(ES::Engine::Registry &registry, const glm::vec3 &position, float mass = 1.f,
                                  float damping = 0.999f, float friction = 10.f, float elasticity = 0.1f)
{
    ES::Engine::Entity entity = registry.CreateEntity();

    registry.GetRegistry().emplace<ES::Plugin::Object::Component::Transform>(entity, position);
    registry.GetRegistry().emplace<ES::Plugin::Physics::Component::SoftBodyNode>(entity, mass, damping, friction, elasticity);

    return entity;
}

ES::Engine::Entity CreateSpring(ES::Engine::Registry &registry, ES::Engine::Entity nodeA, ES::Engine::Entity nodeB,
                                float stiffness = 1, float damping = 0.99f, float restLength = 1)
{
    ES::Engine::Entity entity = registry.CreateEntity();

    registry.GetRegistry().emplace<ES::Plugin::Physics::Component::SoftBodySpring>(entity, nodeA, nodeB, stiffness,
                                                                                   damping, restLength);

    return entity;
}

std::vector<ES::Engine::Entity> CreateBox(ES::Engine::Registry &registry, const glm::vec3 &position,
                                          const glm::vec3 &size)
{
    std::vector<ES::Engine::Entity> entities;
    entities.reserve(8);

    // Create particles for each corner of the box
    entities.push_back(CreateParticle(registry, position + glm::vec3(-size.x, -size.y, -size.z) / 2.f));
    entities.push_back(CreateParticle(registry, position + glm::vec3(size.x, -size.y, -size.z) / 2.f));
    entities.push_back(CreateParticle(registry, position + glm::vec3(size.x, -size.y, size.z) / 2.f));
    entities.push_back(CreateParticle(registry, position + glm::vec3(-size.x, -size.y, size.z) / 2.f));
    entities.push_back(CreateParticle(registry, position + glm::vec3(-size.x, size.y, -size.z) / 2.f));
    entities.push_back(CreateParticle(registry, position + glm::vec3(size.x, size.y, -size.z) / 2.f));
    entities.push_back(CreateParticle(registry, position + glm::vec3(size.x, size.y, size.z) / 2.f));
    entities.push_back(CreateParticle(registry, position + glm::vec3(-size.x, size.y, size.z) / 2.f));

    float stiffness = 1000.f;
    float damping = 0.95f;

    // Create springs for the bottom face
    CreateSpring(registry, entities[0], entities[1], stiffness, damping, size.x);
    CreateSpring(registry, entities[1], entities[2], stiffness, damping, size.z);
    CreateSpring(registry, entities[2], entities[3], stiffness, damping, size.x);
    CreateSpring(registry, entities[3], entities[0], stiffness, damping, size.z);

    // Create springs for the top face
    CreateSpring(registry, entities[4], entities[5], stiffness, damping, size.x);
    CreateSpring(registry, entities[5], entities[6], stiffness, damping, size.z);
    CreateSpring(registry, entities[6], entities[7], stiffness, damping, size.x);
    CreateSpring(registry, entities[7], entities[4], stiffness, damping, size.z);

    // Create vertical springs
    CreateSpring(registry, entities[0], entities[4], stiffness, damping, size.y);
    CreateSpring(registry, entities[1], entities[5], stiffness, damping, size.y);
    CreateSpring(registry, entities[2], entities[6], stiffness, damping, size.y);
    CreateSpring(registry, entities[3], entities[7], stiffness, damping, size.y);

    // 6 Face diagonal springs, mesh-like
    CreateSpring(registry, entities[0], entities[2], stiffness, damping, glm::sqrt(size.x * size.x + size.z * size.z));
    CreateSpring(registry, entities[1], entities[3], stiffness, damping, glm::sqrt(size.x * size.x + size.z * size.z));
    CreateSpring(registry, entities[4], entities[6], stiffness, damping, glm::sqrt(size.x * size.x + size.z * size.z));
    CreateSpring(registry, entities[5], entities[7], stiffness, damping, glm::sqrt(size.x * size.x + size.z * size.z));
    CreateSpring(registry, entities[0], entities[5], stiffness, damping, glm::sqrt(size.x * size.x + size.y * size.y));
    CreateSpring(registry, entities[1], entities[4], stiffness, damping, glm::sqrt(size.x * size.x + size.y * size.y));
    CreateSpring(registry, entities[2], entities[7], stiffness, damping, glm::sqrt(size.x * size.x + size.y * size.y));
    CreateSpring(registry, entities[3], entities[6], stiffness, damping, glm::sqrt(size.x * size.x + size.y * size.y));
    CreateSpring(registry, entities[0], entities[7], stiffness, damping, glm::sqrt(size.z * size.z + size.y * size.y));
    CreateSpring(registry, entities[1], entities[6], stiffness, damping, glm::sqrt(size.z * size.z + size.y * size.y));
    CreateSpring(registry, entities[2], entities[5], stiffness, damping, glm::sqrt(size.z * size.z + size.y * size.y));
    CreateSpring(registry, entities[3], entities[4], stiffness, damping, glm::sqrt(size.z * size.z + size.y * size.y));

    return entities;
}

void RotateBoxFromPitchYawRoll(ES::Engine::Registry &registry, std::vector<ES::Engine::Entity> &boxEntities,
                               float pitch, float yaw, float roll)
{
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(pitch), glm::vec3(1, 0, 0));
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(yaw), glm::vec3(0, 1, 0));
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(roll), glm::vec3(0, 0, 1));

    for (auto entity : boxEntities)
    {
        auto &transform = registry.GetRegistry().get<ES::Plugin::Object::Component::Transform>(entity);
        glm::vec3 position = transform.position;
        glm::vec3 rotatedPosition = glm::vec3(rotationMatrix * glm::vec4(position, 1));
        transform.position = rotatedPosition;
    }
}

void DummyBoxRenderer(ES::Engine::Registry &registry)
{
    // View of node springs
    auto springView = registry.GetRegistry().view<ES::Plugin::Physics::Component::SoftBodySpring>();

    for (auto entity : springView)
    {
        auto &spring = springView.get<ES::Plugin::Physics::Component::SoftBodySpring>(entity);
        auto &nodeA = registry.GetRegistry().get<ES::Plugin::Physics::Component::SoftBodyNode>(spring.nodeA);
        auto &nodeB = registry.GetRegistry().get<ES::Plugin::Physics::Component::SoftBodyNode>(spring.nodeB);
        auto &transformA = registry.GetRegistry().get<ES::Plugin::Object::Component::Transform>(spring.nodeA);
        auto &transformB = registry.GetRegistry().get<ES::Plugin::Object::Component::Transform>(spring.nodeB);

        Vector3 rbAvec = {transformA.position.x, transformA.position.y, transformA.position.z};
        Vector3 rbBvec = {transformB.position.x, transformB.position.y, transformB.position.z};

        DrawLine3D(rbAvec, rbBvec, RED);
    }

    // View of box colliders
    auto boxColliderView =
        registry.GetRegistry()
            .view<ES::Plugin::Collision::Component::BoxCollider3D, ES::Plugin::Object::Component::Transform>();

    for (auto entity : boxColliderView)
    {
        auto &boxCollider = boxColliderView.get<ES::Plugin::Collision::Component::BoxCollider3D>(entity);
        auto &transform = boxColliderView.get<ES::Plugin::Object::Component::Transform>(entity);

        Vector3 position = {transform.position.x, transform.position.y, transform.position.z};
        Vector3 size = {boxCollider.size.x, boxCollider.size.y, boxCollider.size.z};

        DrawCubeV(position, size, GRAY);
    }

    DrawGrid(50, 1.0f);
}

int main()
{
    ES::Engine::Registry registry;

    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Soft Body Physics - POC");
    DisableCursor();
    SetTargetFPS(60);

    Camera3D raylibCam = {
        {10, 10, 10},
        {0,  0,  0 },
        {0,  1,  0 },
        45.0f,
        CAMERA_PERSPECTIVE
    };

    registry.RegisterResource<ES::Plugin::Time::Resource::RealTimeProvider>(
        ES::Plugin::Time::Resource::RealTimeProvider());
    registry.RegisterSystem(ES::Plugin::Physics::System::VelocityIntegration);
    registry.RegisterSystem(ES::Plugin::Collision::System::SoftBodyCollision);
    registry.RegisterSystem(ES::Plugin::Time::System::RealTimeUpdater);

    auto softBodyBox = CreateBox(registry, glm::vec3(0, 7, 0), glm::vec3(1, 1, 1));
    RotateBoxFromPitchYawRoll(registry, softBodyBox, 20, 45, 0);

    ES::Engine::Entity ground = registry.CreateEntity();
    registry.GetRegistry().emplace<ES::Plugin::Object::Component::Transform>(ground, glm::vec3(0, 0, 0));
    registry.GetRegistry().emplace<ES::Plugin::Collision::Component::BoxCollider3D>(ground, glm::vec3(20, 1, 20));

    while (!WindowShouldClose())
    {
        registry.RunSystems();

        BeginDrawing();
        ClearBackground(BLACK);
        UpdateCamera(&raylibCam, CAMERA_FREE);

        BeginMode3D(raylibCam);

        DummyBoxRenderer(registry);

        EndMode3D();
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
