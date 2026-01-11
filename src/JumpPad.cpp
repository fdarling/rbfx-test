#include "JumpPad.h"
#include "JoltPhysicsEvents.h"
#include "JoltPhysicsUtils.h"
#include "JoltPhysicsWorld.h"
#include "JoltRigidBody.h"
#include "globals.h"

#include <Urho3D/Scene/Node.h>

using Urho3D::Node;
using Urho3D::Vector3;

JumpPad::JumpPad(Urho3D::Node *node) :
    Urho3D::Object(node->GetContext()),
    node_(node)
{
    SubscribeToEvent(node_, E_JOLTNODECOLLISIONSTART, URHO3D_HANDLER(JumpPad, HandleNodeCollision));
    JoltPhysicsWorld * const physicsWorld = node_->GetScene()->GetComponent<JoltPhysicsWorld>();
    SubscribeToEvent(physicsWorld, E_JOLTPHYSICSPOSTSTEP, URHO3D_HANDLER(JumpPad, HandlePhysicsPostStep));
}

JumpPad::~JumpPad()
{
    node_->Remove();
    node_ = nullptr;
}

void JumpPad::HandleNodeCollision(Urho3D::StringHash eventType, Urho3D::VariantMap &eventData)
{
    // URHO3D_LOGINFO("JumpPad::HandleNodeCollision()");
    // Node * const otherNode = static_cast<Node*>(eventData[JoltNodeCollisionStart::P_OTHERNODE].GetPtr());
    JoltRigidBody * const otherBody = static_cast<JoltRigidBody*>(eventData[JoltNodeCollisionStart::P_OTHERBODY].GetPtr());
    if (!otherBody)
        return;
    toLaunch_.emplace_back(otherBody);
}

void JumpPad::HandlePhysicsPostStep(Urho3D::StringHash eventType, Urho3D::VariantMap &eventData)
{
    // process all deferred bodies that touched the launch pad
    for (Urho3D::WeakPtr<JoltRigidBody> &otherBody : toLaunch_)
    {
        // make sure the weak pointer is still valid
        if (!otherBody)
            continue;

        // alter the vertical velocity of the object that touched the jump pad
        Vector3 vel = otherBody->GetLinearVelocity();
        vel.y_ = 10.0;
        otherBody->SetLinearVelocity(vel);
    }

    // clear the list of deferred bodies now that we've handled them
    toLaunch_.clear();
}
