#pragma once

// #include <Urho3D/Core/Variant.h>
#include <Urho3D/Scene/Component.h>

class JoltDebugRenderer;

namespace Urho3D {
class VertexBuffer;
} // namespace Urho3D

namespace JPH {
class TempAllocatorImpl;
class JobSystemThreadPool;
class PhysicsSystem;
} // namespace JPH

class BPLayerInterfaceImpl;
class ObjectVsBroadPhaseLayerFilterImpl;
class ObjectLayerPairFilterImpl;

class JoltPhysicsWorld : public Urho3D::Component
{
    URHO3D_OBJECT(JoltPhysicsWorld, Urho3D::Component);
public:
    explicit JoltPhysicsWorld(Urho3D::Context *context);
    ~JoltPhysicsWorld() override;
    static void RegisterObject(Urho3D::Context *context);

    JPH::PhysicsSystem & GetPhysicsSystem() {return *physicsSystem_;}
    const JPH::PhysicsSystem & GetPhysicsSystem() const {return *physicsSystem_;}

    void Update(float timeStep);

    void DrawDebugGeometry(JoltDebugRenderer *debug, bool depthTest);
protected:
    void OnSceneSet(Urho3D::Scene *previousScene, Urho3D::Scene *scene) override;
private:
    // void HandleEndViewUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap &eventData);
    void HandleSceneSubsystemUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap &eventData);
private:
    JPH::TempAllocatorImpl *tempAllocator_;
    JPH::JobSystemThreadPool *threadPool_;
    BPLayerInterfaceImpl *layerInterface_;
    ObjectVsBroadPhaseLayerFilterImpl *objectVsBroadPhaseLayerFilter_;
    ObjectLayerPairFilterImpl *objectLayerPairFilter_;
    JPH::PhysicsSystem *physicsSystem_;
    float accumulator_;
    float fixedTimeStep_;
    // TODO MyBodyActivationListener
    // TODO MyContactListener
};

void URHO3D_API RegisterPhysicsLibrary(Urho3D::Context *context);
