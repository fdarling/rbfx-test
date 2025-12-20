#include "JoltContactListener.h"
#include "JoltPhysicsWorld.h"
#include "JoltRigidBody.h"
#include "JoltPhysicsEvents.h"

// #include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/Body.h>

#include <Urho3D/Core/Thread.h>

JoltContactListener::JoltContactListener(JoltPhysicsWorld *world) :
    physicsWorld_(world)
{
    const ThreadID current_id = Urho3D::Thread::GetCurrentThreadID();
    URHO3D_LOGINFO("JoltContactListener::JoltContactListener() is running on thread ID: " + ea::to_string(current_id) + ", main?: " + (Urho3D::Thread::IsMainThread() ? "yes" : "no"));
}

JoltContactListener::~JoltContactListener()
{
}

void JoltContactListener::OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings)
{
    using namespace JoltNodeCollisionStart;
    Urho3D::Node *nodeA = nullptr;
    Urho3D::Node *nodeB = nullptr;
    JoltRigidBody * const rigidBodyA = reinterpret_cast<JoltRigidBody*>(inBody1.GetUserData());
    JoltRigidBody * const rigidBodyB = reinterpret_cast<JoltRigidBody*>(inBody2.GetUserData());
    if (rigidBodyA)
        nodeA = rigidBodyA->GetNode();
    if (rigidBodyB)
        nodeB = rigidBodyA->GetNode();

    // Urho3D::VariantMap &eventData = physicsWorld_->GetEventDataMap();
    // eventData[P_WORLD] = physicsWorld_;
    // eventData[P_NODEA] = nodeA;
    // eventData[P_NODEB] = nodeB;
    // eventData[P_BODYA] = rigidBodyA;
    // eventData[P_BODYB] = rigidBodyB;
    // SendEvent(E_JOLTNODECOLLISIONSTART, eventData);
    URHO3D_LOGINFO("JoltContactListener::OnContactAdded() nodeA = {}, nodeB = {}", (void*)nodeA, (void*)nodeB);
    Urho3D::VariantMap &eventData = physicsWorld_->GetEventDataMap();
    const ThreadID current_id = Urho3D::Thread::GetCurrentThreadID();
    if (nodeA)
    {
        eventData[P_BODY] = rigidBodyA;
        eventData[P_OTHERNODE] = nodeB;
        eventData[P_OTHERBODY] = rigidBodyB;
        URHO3D_LOGINFO("This function (OnContactAdded, nodeA portion) is running on thread ID: " + ea::to_string(current_id) + ", main?: " + (Urho3D::Thread::IsMainThread() ? "yes" : "no"));
        nodeA->SendEvent(E_JOLTNODECOLLISIONSTART, eventData);
    }
    if (nodeB)
    {
        eventData[P_BODY] = rigidBodyB;
        eventData[P_OTHERNODE] = nodeA;
        eventData[P_OTHERBODY] = rigidBodyA;
        URHO3D_LOGINFO("This function (OnContactAdded, nodeB portion) is running on thread ID: " + ea::to_string(current_id) + ", main?: " + (Urho3D::Thread::IsMainThread() ? "yes" : "no"));
        nodeB->SendEvent(E_JOLTNODECOLLISIONSTART, eventData);
    }
}

void JoltContactListener::OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings)
{
    using namespace JoltNodeCollision;
    Urho3D::Node *nodeA = nullptr;
    Urho3D::Node *nodeB = nullptr;
    JoltRigidBody * const rigidBodyA = reinterpret_cast<JoltRigidBody*>(inBody1.GetUserData());
    JoltRigidBody * const rigidBodyB = reinterpret_cast<JoltRigidBody*>(inBody2.GetUserData());
    if (rigidBodyA)
        nodeA = rigidBodyA->GetNode();
    if (rigidBodyB)
        nodeB = rigidBodyA->GetNode();

    Urho3D::VariantMap &eventData = physicsWorld_->GetEventDataMap();
    if (nodeA)
    {
        eventData[P_BODY] = rigidBodyA;
        eventData[P_OTHERNODE] = nodeB;
        eventData[P_OTHERBODY] = rigidBodyB;
        nodeA->SendEvent(E_JOLTNODECOLLISIONSTART, eventData);
    }
    if (nodeB)
    {
        eventData[P_BODY] = rigidBodyB;
        eventData[P_OTHERNODE] = nodeA;
        eventData[P_OTHERBODY] = rigidBodyA;
        nodeB->SendEvent(E_JOLTNODECOLLISION, eventData);
    }
}
