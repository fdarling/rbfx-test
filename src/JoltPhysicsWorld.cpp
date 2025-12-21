#include "JoltPhysicsWorld.h"
#include "JoltContactListener.h"
#include "JoltDebugRenderer.h"
#include "JoltRigidBody.h"
#include "JoltPhysicsDefs.h"
#include "JoltPhysicsEvents.h"
#include "JoltPhysicsUtils.h"

#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/RenderAPI/DrawCommandQueue.h>
#include <Urho3D/RenderAPI/RenderContext.h>
#include <Urho3D/RenderAPI/RenderDevice.h>
#include <Urho3D/Graphics/GraphicsUtils.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
// #include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

// #include <Jolt/Jolt.h>
// #include <Jolt/RegisterTypes.h>
// #include <Jolt/Core/Factory.h>
// #include <Jolt/Core/TempAllocator.h>
// #include <Jolt/Core/JobSystemThreadPool.h>
// #include <Jolt/Physics/PhysicsSystem.h>
// #include <Jolt/Physics/Collision/Shape/BoxShape.h>
// #include <Jolt/Physics/Collision/Shape/SphereShape.h>
// #include <Jolt/Physics/Constraints/SixDOFConstraint.h>
// #include <Jolt/Physics/Body/BodyCreationSettings.h>
// #include <Jolt/Physics/Body/Body.h>

// constants
constexpr float FLOOR_WIDTH = 25.0;
constexpr float FLOOR_THICKNESS = 1.0;
constexpr float ELEVATOR_THICKNESS = 0.25;
constexpr float ELEVATOR_WIDTH = 6.0;
constexpr float JUMP_PAD_THICKNESS = 0.25;
constexpr float JUMP_PAD_WIDTH = 2.0;
constexpr float STACKED_BOX_THICKNESS = 0.125;
constexpr float STACKED_BOX_WIDTH = 2.0;
constexpr float SPHERE_DIAMETER = 1.0;
constexpr float BALL_DIAMETER = 0.5;
constexpr float BALL_INITIAL_SPEED = 15.0;
constexpr float ELEVATOR_TRAVEL_TIME = 4.0;
constexpr float ELEVATOR_PAUSE_TIME = 3.0;
constexpr float ELEVATOR_CYCLE_TIME = 2.0*ELEVATOR_TRAVEL_TIME + 2.0*ELEVATOR_PAUSE_TIME;
constexpr float ELEVATOR_LOWER_Y = ELEVATOR_THICKNESS/2.0;
constexpr float ELEVATOR_UPPER_Y = 22.0 - ELEVATOR_THICKNESS/2.0;
constexpr float LADDER_HEIGHT = 10.0;
constexpr float LADDER_WIDTH = 2.0;
constexpr float BALLOON_DIAMETER = 1.0;
constexpr float BALLOON_RADIUS = BALLOON_DIAMETER/2.0;
constexpr float MOUSELOOK_SENSITIVITY = 0.002f;
constexpr float WALK_SPEED = 20.0f;

static const std::size_t URHO3D_JOLT_PHYSICS_DEFAULT_TEMP_ALLOCATION_SIZE = 10 * 1024 * 1024;
static const uint cMaxBodies = 1024; // TODO increase to 65536
static const uint cNumBodyMutexes = 0;
static const uint cMaxBodyPairs = 1024; // TODO increase to 65536
static const uint cMaxContactConstraints = 1024; // TODO increase to 10240
static const float URHO3D_JOLTPHYSICS_DEFAULT_TIME_STEP = 1.0f / 60.0f;

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
        case JoltPhysicsLayers::NON_MOVING:
            return inObject2 == JoltPhysicsLayers::MOVING; // Non moving only collides with moving
        case JoltPhysicsLayers::MOVING:
            return true; // Moving collides with everything
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

namespace BroadPhaseLayers
{
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr uint NUM_LAYERS(2);
} // namespace BroadPhaseLayers

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[JoltPhysicsLayers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[JoltPhysicsLayers::MOVING] = BroadPhaseLayers::MOVING;
    }

    virtual uint GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < JoltPhysicsLayers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char * GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer)
        {
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:  return "NON_MOVING";
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:      return "MOVING";
        default:                                                    JPH_ASSERT(false); return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[JoltPhysicsLayers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
        case JoltPhysicsLayers::NON_MOVING:
            return inLayer2 == BroadPhaseLayers::MOVING;
        case JoltPhysicsLayers::MOVING:
            return true;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

JoltPhysicsWorld::JoltPhysicsWorld(Urho3D::Context *context) :
    Component(context),
    tempAllocator_(nullptr),
    threadPool_(nullptr),
    layerInterface_(nullptr),
    objectVsBroadPhaseLayerFilter_(nullptr),
    objectLayerPairFilter_(nullptr),
    physicsSystem_(nullptr),
    accumulator_(0.0),
    fixedTimeStep_(URHO3D_JOLTPHYSICS_DEFAULT_TIME_STEP),
    simulatedSteps_(0)
{
    URHO3D_LOGINFO("JoltPhysicsWorld::JoltPhysicsWorld()");
    tempAllocator_ = new JPH::TempAllocatorImpl(URHO3D_JOLT_PHYSICS_DEFAULT_TEMP_ALLOCATION_SIZE);
    threadPool_ = new JPH::JobSystemSingleThreaded(JPH::cMaxPhysicsJobs);
    // threadPool_ = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);
    layerInterface_ = new BPLayerInterfaceImpl();
    objectVsBroadPhaseLayerFilter_ = new ObjectVsBroadPhaseLayerFilterImpl();
    objectLayerPairFilter_ = new ObjectLayerPairFilterImpl();
    contactListener_ = std::make_unique<JoltContactListener>(this);
    physicsSystem_ = new JPH::PhysicsSystem();
    physicsSystem_->Init(cMaxBodies,
                         cNumBodyMutexes,
                         cMaxBodyPairs,
                         cMaxContactConstraints,
                         *layerInterface_,
                         *objectVsBroadPhaseLayerFilter_,
                         *objectLayerPairFilter_);
    physicsSystem_->SetContactListener(contactListener_.get());
}

JoltPhysicsWorld::~JoltPhysicsWorld()
{
    URHO3D_LOGINFO("JoltPhysicsWorld::~JoltPhysicsWorld()");
    if (physicsSystem_)
        delete physicsSystem_;
    if (objectLayerPairFilter_)
        delete objectLayerPairFilter_;
    if (objectVsBroadPhaseLayerFilter_)
        delete objectVsBroadPhaseLayerFilter_;
    if (layerInterface_)
        delete layerInterface_;
    if (threadPool_)
        delete threadPool_;
    if (tempAllocator_)
        delete tempAllocator_;
}

void JoltPhysicsWorld::RegisterObject(Urho3D::Context *context)
{
    URHO3D_LOGINFO("JoltPhysicsWorld::RegisterObject()");
    context->AddFactoryReflection<JoltPhysicsWorld>(Urho3D::Category_Subsystem);

    JPH::RegisterDefaultAllocator(); // must be called once, and before any other Jolt Physics API calls
    JPH::Factory::sInstance = new JPH::Factory(); // TODO destroy this at some point...
    JPH::RegisterTypes();
}

void JoltPhysicsWorld::Update(float timeStep)
{
#ifdef MASSIVE_LOGGING
    URHO3D_LOGINFO("JoltPhysicsWorld::Update({})", timeStep);
#endif // MASSIVE_LOGGING

    // determine how many cycles we are committing to simulating
    accumulator_ += timeStep;
    const int cCollisionSteps = accumulator_ / fixedTimeStep_;
    if (!cCollisionSteps) // TODO should we inhibit PreUpdate/PostUpdate when there aren't any steps being simulated?
        return;
    // const float simulatedTime = static_cast<float>(cCollisionSteps)*fixedTimeStep_;

    {
        URHO3D_PROFILE("JoltUpdateSystem");
        PreUpdate(fixedTimeStep_);
        for (int step = 0; step < cCollisionSteps; step++)
        {
            PreStep(fixedTimeStep_);
            // actually simulate. NOTE: in order to have per-substep callbacks, we
            // cannot have Jolt perform more that one substep per function call
            physicsSystem_->Update(fixedTimeStep_, 1, tempAllocator_, threadPool_);
            accumulator_ -= fixedTimeStep_;
            simulatedSteps_++;
            PostStep(fixedTimeStep_);
        }
        PostUpdate(fixedTimeStep_, simulatedSteps_*fixedTimeStep_ + accumulator_);
    }

    // TODO interpolation!

    // update the Urho3D nodes
    {
        URHO3D_PROFILE("JoltSyncBodies");
        JPH::BodyIDVector body_ids;
        physicsSystem_->GetActiveBodies(JPH::EBodyType::RigidBody, body_ids);
        // physicsSystem_->GetBodies(body_ids);
        const JPH::BodyLockInterfaceLocking &lock_interface = physicsSystem_->GetBodyLockInterface();
        // std::cout << "=== Active Bodies (" << body_ids.size() << ") ===\n";
        for (JPH::BodyID body_id : body_ids)
        {
            // Lock the body for read access (thread-safe)
            JPH::BodyLockRead lock(lock_interface, body_id);
            if (!lock.Succeeded())
                continue; // Body was removed during iteration (rare)

            const JPH::Body &body = lock.GetBody();
            JoltRigidBody * const associatedBody = reinterpret_cast<JoltRigidBody*>(body.GetUserData());
            if (!associatedBody)
                continue;
            Urho3D::Node * const node = associatedBody->GetNode();
            if (!node)
                continue;

#if 0
            // position / rotation method
            const JPH::Vec3 joltPos = body.GetPosition();
            const JPH::Quat joltRot = body.GetRotation();
            const Urho3D::Vector3 oldPos = node->GetWorldPosition();
            const Urho3D::Vector3 newPos(joltPos.GetX(), joltPos.GetY(), joltPos.GetZ());
            const Urho3D::Quaternion oldRot = node->GetWorldRotation();
            const Urho3D::Quaternion newRot = JoltQuatToUrhoQuaternion(joltRot);
            if (newPos == oldPos && oldRot == newRot)
                continue;
            node->SetWorldPosition(newPos);
            node->SetWorldRotation(newRot);
#else
            // transform method
            const JPH::Mat44 joltTrans = body.GetWorldTransform();
            const Urho3D::Matrix3x4 oldTrans = node->GetWorldTransform();
            Urho3D::Vector3 oldPos;
            Urho3D::Vector3 oldScale; // reused later
            Urho3D::Quaternion oldRot;
            oldTrans.Decompose(oldPos, oldRot, oldScale);
            const Urho3D::Matrix3x4 newTransWithScale(ToMatrix4(joltTrans));
            Urho3D::Vector3 newPos;
            Urho3D::Vector3 newScale; // ignored, will always be identity!
            Urho3D::Quaternion newRot;
            newTransWithScale.Decompose(newPos, newRot, newScale);
            const Urho3D::Matrix3x4 newTrans(newPos, newRot, oldScale);
            if (newTrans == oldTrans)
                continue;
            node->SetWorldTransform(newTrans);
#endif
        }
    }
}

void JoltPhysicsWorld::DrawDebugGeometry(JoltDebugRenderer *debug, bool depthTest)
{
    URHO3D_PROFILE("JoltDebugDraw");
#ifdef MASSIVE_LOGGING
    URHO3D_LOGINFO("JoltPhysicsWorld::DrawDebugGeometry()");
#endif // MASSIVE_LOGGING

    JPH::BodyManager::DrawSettings draw_settings;
    draw_settings.mDrawShape = true;
    draw_settings.mDrawShapeWireframe = true;
    draw_settings.mDrawBoundingBox = false;
#ifdef MASSIVE_LOGGING
    URHO3D_LOGINFO("JoltPhysicsWorld::DrawDebugGeometry (starting to draw...)");
#endif // MASSIVE_LOGGING
    // debug->SetCameraPos(); // TODO where do we get the current camera information?
    physicsSystem_->DrawBodies(draw_settings, debug);
#ifdef MASSIVE_LOGGING
    URHO3D_LOGINFO("JoltPhysicsWorld::DrawDebugGeometry (...finished drawing!)");
#endif // MASSIVE_LOGGING
}

void JoltPhysicsWorld::OnSceneSet(Urho3D::Scene *previousScene, Urho3D::Scene *scene)
{
#ifdef MASSIVE_LOGGING
    URHO3D_LOGINFO("JoltPhysicsWorld::OnSceneSet() scene = {}, GetScene() = {}", (void*)scene, (void*)GetScene());
#endif // MASSIVE_LOGGING
    // Subscribe to the scene subsystem update, which will trigger the physics simulation step
    if (scene)
    {
        // scene_ = GetScene();
        SubscribeToEvent(GetScene(), Urho3D::E_SCENESUBSYSTEMUPDATE, URHO3D_HANDLER(JoltPhysicsWorld, HandleSceneSubsystemUpdate));
        // SubscribeToEvent(Urho3D::E_ENDVIEWUPDATE, URHO3D_HANDLER(JoltPhysicsWorld, HandleEndViewUpdate));
        // SubscribeToEvent(Urho3D::E_ENDFRAME, URHO3D_HANDLER(JoltPhysicsWorld, HandleEndViewUpdate));
        // SubscribeToEvent(GetScene(), Urho3D::E_BEGINRENDERING, URHO3D_HANDLER(JoltPhysicsWorld, HandleEndViewUpdate));
        // SubscribeToEvent(Urho3D::E_ENDRENDERING, URHO3D_HANDLER(JoltPhysicsWorld, HandleEndViewUpdate));
    }
    else
    {
        UnsubscribeFromEvent(Urho3D::E_SCENESUBSYSTEMUPDATE);
        // UnsubscribeFromEvent(Urho3D::E_ENDVIEWUPDATE);
        // UnsubscribeFromEvent(Urho3D::E_ENDFRAME);
        // UnsubscribeFromEvent(Urho3D::E_BEGINRENDERING);
        // UnsubscribeFromEvent(Urho3D::E_ENDRENDERING);
    }
}

// void JoltPhysicsWorld::HandleEndViewUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap &eventData)
// {
    // URHO3D_LOGINFO("JoltPhysicsWorld::HandleEndViewUpdate()");
// }

void JoltPhysicsWorld::HandleSceneSubsystemUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap &eventData)
{
    Update(eventData[Urho3D::SceneSubsystemUpdate::P_TIMESTEP].GetFloat());
}

void JoltPhysicsWorld::PreUpdate(float timeStep)
{
    using namespace JoltPhysicsPreUpdate;
    URHO3D_PROFILE("JoltPreUpdate");
    Urho3D::VariantMap &eventData = GetEventDataMap();
    eventData[P_WORLD] = this;
    eventData[P_TIMESTEP] = timeStep;
    SendEvent(E_JOLTPHYSICSPREUPDATE, eventData);
}

void JoltPhysicsWorld::PostUpdate(float timeStep, float overtime)
{
    using namespace JoltPhysicsPostUpdate;
    URHO3D_PROFILE("JoltPostUpdate");
    Urho3D::VariantMap &eventData = GetEventDataMap();
    eventData[P_WORLD] = this;
    eventData[P_TIMESTEP] = timeStep;
    eventData[P_OVERTIME] = overtime;
    SendEvent(E_JOLTPHYSICSPOSTUPDATE, eventData);
}

void JoltPhysicsWorld::PreStep(float timeStep)
{
    using namespace JoltPhysicsPreStep;
    URHO3D_PROFILE("JoltPreStep");
    Urho3D::VariantMap &eventData = GetEventDataMap();
    eventData[P_WORLD] = this;
    eventData[P_TIMESTEP] = timeStep;
    SendEvent(E_JOLTPHYSICSPRESTEP, eventData);
}

void JoltPhysicsWorld::PostStep(float timeStep)
{
    using namespace JoltPhysicsPostStep;
    URHO3D_PROFILE("JoltPostStep");
    Urho3D::VariantMap &eventData = GetEventDataMap();
    eventData[P_WORLD] = this;
    eventData[P_TIMESTEP] = timeStep;
    SendEvent(E_JOLTPHYSICSPOSTSTEP, eventData);
}
