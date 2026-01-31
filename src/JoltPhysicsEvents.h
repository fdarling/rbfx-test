#pragma once

#include <Urho3D/Core/Object.h>

/// Physics world is about to be updated. There may be zero, one, or more physics steps coming.
URHO3D_EVENT(E_JOLTPHYSICSPREUPDATE, JoltPhysicsPreUpdate)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
};

/// Physics world has been updated. There may have been zero, one, or more physics steps evaluated.
/// Overtime indicates the amount of non-simulated time after latest step.
URHO3D_EVENT(E_JOLTPHYSICSPOSTUPDATE, JoltPhysicsPostUpdate)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
    URHO3D_PARAM(P_OVERTIME, Overtime);            // float
};

/// Physics world is about to be stepped.
URHO3D_EVENT(E_JOLTPHYSICSPRESTEP, JoltPhysicsPreStep)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
    URHO3D_PARAM(P_NETWORKFRAME, NetworkFrame);    // unsigned
}

/// Physics world has been stepped.
URHO3D_EVENT(E_JOLTPHYSICSPOSTSTEP, JoltPhysicsPostStep)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Physics collision started. Global event sent by the PhysicsWorld.
/*URHO3D_EVENT(E_JOLTPHYSICSCOLLISIONSTART, JoltPhysicsCollisionStart)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    URHO3D_PARAM(P_NODEA, NodeA);                  // Node pointer
    URHO3D_PARAM(P_NODEB, NodeB);                  // Node pointer
    URHO3D_PARAM(P_BODYA, BodyA);                  // RigidBody pointer
    URHO3D_PARAM(P_BODYB, BodyB);                  // RigidBody pointer
    URHO3D_PARAM(P_TRIGGER, Trigger);              // bool
    URHO3D_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector3), normal (Vector3), distance (float), impulse (float) for each contact
}

/// Physics collision ongoing. Global event sent by the PhysicsWorld.
URHO3D_EVENT(E_JOLTPHYSICSCOLLISION, JoltPhysicsCollision)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    URHO3D_PARAM(P_NODEA, NodeA);                  // Node pointer
    URHO3D_PARAM(P_NODEB, NodeB);                  // Node pointer
    URHO3D_PARAM(P_BODYA, BodyA);                  // RigidBody pointer. Maybe be nullptr for KinematicCharacterController
    URHO3D_PARAM(P_BODYB, BodyB);                  // RigidBody pointer
    URHO3D_PARAM(P_TRIGGER, Trigger);              // bool
    URHO3D_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector3), normal (Vector3), distance (float), impulse (float) for each contact
}

/// Physics collision ended. Global event sent by the PhysicsWorld.
URHO3D_EVENT(E_JOLTPHYSICSCOLLISIONEND, JoltPhysicsCollisionEnd)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    URHO3D_PARAM(P_NODEA, NodeA);                  // Node pointer
    URHO3D_PARAM(P_NODEB, NodeB);                  // Node pointer
    URHO3D_PARAM(P_BODYA, BodyA);                  // RigidBody pointer
    URHO3D_PARAM(P_BODYB, BodyB);                  // RigidBody pointer
    URHO3D_PARAM(P_TRIGGER, Trigger);              // bool
}*/

/// Node's physics collision started. Sent by scene nodes participating in a collision.
URHO3D_EVENT(E_JOLTNODECOLLISIONSTART, JoltNodeCollisionStart)
{
    URHO3D_PARAM(P_BODY, Body);                    // JoltRigidBody pointer
    URHO3D_PARAM(P_JOLTBODY, JoltBody);            // JPH::Body pointer
    URHO3D_PARAM(P_OTHERNODE, OtherNode);          // Node pointer
    URHO3D_PARAM(P_OTHERBODY, OtherBody);          // JoltRigidBody pointer
    URHO3D_PARAM(P_OTHERJOLTBODY, OtherJoltBody);  // JPH::Body pointer
    URHO3D_PARAM(P_CONTACTMANIFOLD, Trigger);      // JPH::ContactManifold
    URHO3D_PARAM(P_CONTACTSETTINGS, Contacts);     // JPH::ContactSettings
}

/// Node's physics collision ongoing. Sent by scene nodes participating in a collision.
URHO3D_EVENT(E_JOLTNODECOLLISION, JoltNodeCollision)
{
    URHO3D_PARAM(P_BODY, Body);                    // JoltRigidBody pointer
    URHO3D_PARAM(P_JOLTBODY, JoltBody);            // JPH::Body pointer
    URHO3D_PARAM(P_OTHERNODE, OtherNode);          // Node pointer
    URHO3D_PARAM(P_OTHERBODY, OtherBody);          // JoltRigidBody pointer
    URHO3D_PARAM(P_OTHERJOLTBODY, OtherJoltBody);  // JPH::Body pointer
    URHO3D_PARAM(P_CONTACTMANIFOLD, Trigger);      // JPH::ContactManifold
    URHO3D_PARAM(P_CONTACTSETTINGS, Contacts);     // JPH::ContactSettings
}

/// Node's physics collision ended. Sent by scene nodes participating in a collision.
/*URHO3D_EVENT(E_JOLTNODECOLLISIONEND, JoltNodeCollisionEnd)
{
    URHO3D_PARAM(P_BODY, Body);                    // RigidBody pointer
    URHO3D_PARAM(P_OTHERNODE, OtherNode);          // Node pointer
    URHO3D_PARAM(P_OTHERBODY, OtherBody);          // RigidBody pointer
    URHO3D_PARAM(P_TRIGGER, Trigger);              // bool
}*/
