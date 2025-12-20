#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>

class JoltPhysicsWorld;

class JoltContactListener final : public JPH::ContactListener {
public:
    JoltContactListener(JoltPhysicsWorld *world);
    ~JoltContactListener();

    void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override;
    void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override;
private:
    JoltPhysicsWorld *physicsWorld_;
};
