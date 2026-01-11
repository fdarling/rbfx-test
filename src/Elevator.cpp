#include "Elevator.h"
#include "JoltPhysicsWorld.h"
#include "JoltRigidBody.h"
#include "JoltPhysicsEvents.h"
#include "JoltPhysicsUtils.h"
#include "globals.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Math/Transform.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/Node.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

using Urho3D::Node;
using Urho3D::Vector3;
using Urho3D::Quaternion;

static const float DEST_COOLDOWN = 2.0;
static const float MAX_ELEVATOR_TRAVEL = 40.0f;
static const float ELEVATOR_SPEED = 5.0f;

Elevator::Elevator(Urho3D::Node *node) :
    Urho3D::Object(node->GetContext()),
    node_(node),
    _state(State::Idle),
    _accumulator(0.0),
    _cooldown(0.0)
{
    JoltRigidBody * const rigidBody = node_->GetComponent<JoltRigidBody>();
    rigidBody->SetMotionType(JoltRigidBody::MotionType::Kinematic);

    JoltPhysicsWorld * const world = node_->GetScene()->GetComponent<JoltPhysicsWorld>();
    SubscribeToEvent(world, E_JOLTPHYSICSPRESTEP, URHO3D_HANDLER(Elevator, HandlePhysicsPreStep));
    SubscribeToEvent(node_, E_JOLTNODECOLLISIONSTART, URHO3D_HANDLER(Elevator, HandleNodeCollisionStart));
}

Elevator::~Elevator()
{
    node_->Remove();
    node_ = nullptr;
}

void Elevator::HandlePhysicsPreStep(Urho3D::StringHash eventType, Urho3D::VariantMap &eventData)
{
    // std::cout << "HandlePhysicsPreStep" << std::endl;
    if (_state != State::Idle)
    {
        const float timeStep = eventData[JoltPhysicsPreStep::P_TIMESTEP].GetFloat();
        _accumulator += timeStep;

        JoltRigidBody * const ourBody = static_cast<JoltRigidBody*>(node_->GetComponent<JoltRigidBody>());

        if (_state == State::Departing || _state == State::Returning)
        {
            // determine "t" parametric progress between start the end
            static const float TOTAL_TIME = MAX_ELEVATOR_TRAVEL/ELEVATOR_SPEED;
            const float interpT = Urho3D::Min(_accumulator/TOTAL_TIME, 1.0);

            // calculate interpolated transform
            const Urho3D::Transform transA = Urho3D::Transform::FromMatrix3x4(_state == State::Departing ? _startTrans : _endTrans);
            const Urho3D::Transform transB = Urho3D::Transform::FromMatrix3x4(_state == State::Departing ? _endTrans : _startTrans);
            const Urho3D::Transform interp = transA.Lerp(transB, interpT);

            // tell the physics system to move to the destination within a certain amount of time (one simulation step)
            // URHO3D_LOGINFO("interpT = {}, pos.y = {}, transA.y = {}, transB.y = {}, timeStep = {}", interpT, interp.position_.ToString(), transA.position_.y_, transB.position_.y_, timeStep);
            ourBody->MoveKinematic(interp.position_, interp.rotation_, timeStep);

            // check if we are finishing the move...
            if (interpT >= 1.0)
            {
                // advance the state
                if (_state == State::Departing)
                    _state = State::DestCooldown;
                else
                    _state = State::OriginCooldown;

                // wait now that we are at the end of a movement
                _cooldown = DEST_COOLDOWN;
            }
        }
        else if (_state == State::DestCooldown || _state == State::OriginCooldown)
        {
            // we aren't moving while we wait
            ourBody->SetLinearVelocity(Urho3D::Vector3::ZERO);
            ourBody->SetAngularVelocity(Urho3D::Vector3::ZERO);

            // the accumulator should be reset, so when we leave the state we start at the initial position
            _accumulator = 0.0;

            // count-down the wait time
            _cooldown -= timeStep;

            // check to see if the countdown has finished
            if (_cooldown <= 0.0)
            {
                // don't allow it to go negative
                _cooldown = 0.0;

                // advance the state
                if (_state == State::DestCooldown)
                    _state = State::Returning;
                else
                    _state = State::Idle;
            }
        }
        // std::cout << "Elevator::HandlePhysicsPreStep: pos = " << pos.ToString().c_str() << "; vel = " << vel.ToString().c_str() << std::endl;
        // std::cout << "Elevator::HandlePhysicsPreStep: pos = " << pos.ToString().c_str() << "; newPos = " << newPos.ToString().c_str() << std::endl;
    }
    else
    {
        JoltRigidBody * const ourBody = static_cast<JoltRigidBody*>(node_->GetComponent<JoltRigidBody>());
        // we aren't moving while we are idle
        ourBody->SetLinearVelocity(Urho3D::Vector3::ZERO);
        ourBody->SetAngularVelocity(Urho3D::Vector3::ZERO);
    }
}

void Elevator::HandleNodeCollisionStart(Urho3D::StringHash eventType, Urho3D::VariantMap &eventData)
{
    // URHO3D_LOGINFO("Elevator::HandleNodeCollisionStart()");
    if (_state != State::Idle)
        return;
    // Node * const otherNode = static_cast<Node*>(eventData[JoltNodeCollisionStart::P_OTHERNODE].GetPtr());
    JoltRigidBody * const otherBody = static_cast<JoltRigidBody*>(eventData[JoltNodeCollisionStart::P_OTHERBODY].GetPtr());
    if (otherBody)
    {
        // URHO3D_LOGINFO("Elevator::HandleNodeCollisionStart: Touched Elevator!");
        _state = State::Departing;
        _accumulator = 0.0;
        _cooldown = 0.0;
        _startTrans = node_->GetWorldTransform();
        _endTrans = _startTrans;
        _endTrans.SetTranslation(_startTrans.Translation() + Vector3::UP*MAX_ELEVATOR_TRAVEL);
    }
}
