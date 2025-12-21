#include "Elevator.h"
// #include "KinematicRigidBody.h"
#include "JoltPhysicsWorld.h"
#include "JoltRigidBody.h"
#include "JoltPhysicsEvents.h"
#include "JoltPhysicsUtils.h"
#include "globals.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Math/Transform.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/PhysicsUtils.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/Node.h>

// #include <Bullet/BulletDynamics/Dynamics/btRigidBody.h>
// #include <Bullet/LinearMath/btTransformUtil.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

using Urho3D::Node;
using Urho3D::Vector3;
using Urho3D::Quaternion;
using Urho3D::ToVector3;
using Urho3D::ToQuaternion;
using Urho3D::PhysicsWorld;
using Urho3D::RigidBody;
using Urho3D::E_POSTUPDATE;
using Urho3D::E_PHYSICSPRESTEP;
using Urho3D::E_NODECOLLISIONSTART;
namespace PostUpdate = Urho3D::PostUpdate;
namespace PhysicsPreStep = Urho3D::PhysicsPreStep;
namespace BeginFrame = Urho3D::BeginFrame;
namespace NodeCollisionStart = Urho3D::NodeCollisionStart;

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
    /*RigidBody * const rigidBody = node_->GetComponent<RigidBody>();
    rigidBody->getWorldTransform(_oldTransform);

    btRigidBody * const bulletBody = rigidBody->GetBody();
    bulletBody->setCollisionFlags(bulletBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT); // can be programmatically moved
    bulletBody->setActivationState(DISABLE_DEACTIVATION); // TODO is this necessary?
    bulletBody->setUserIndex(PhysicsUserIndex::Elevator);

    PhysicsWorld * const world = node_->GetScene()->GetComponent<PhysicsWorld>();
    // TODO call SubscribeToEvent() and UnsubscribeFromEvent() as needed so they aren't always firing
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(Elevator, HandlePostUpdate));
    // SubscribeToEvent(world, E_PHYSICSPREUPDATE, URHO3D_HANDLER(Elevator, HandlePhysicsPreUpdate));
    // SubscribeToEvent(world, E_PHYSICSPOSTUPDATE, URHO3D_HANDLER(Elevator, HandlePhysicsPostUpdate));
    SubscribeToEvent(world, E_PHYSICSPRESTEP, URHO3D_HANDLER(Elevator, HandlePhysicsPreStep));
    // SubscribeToEvent(world, E_PHYSICSPOSTSTEP, URHO3D_HANDLER(Elevator, HandlePhysicsPostStep));
    SubscribeToEvent(node_, E_NODECOLLISIONSTART, URHO3D_HANDLER(Elevator, HandleNodeCollisionStart));*/

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

/*void Elevator::HandlePostUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap &eventData)
{
    // std::cout << "Elevator::HandlePostUpdate" << std::endl;
    if (_state != State::Idle)
    {
        _accumulator += eventData[PostUpdate::P_TIMESTEP].GetFloat();

        // interpolate graphics version of transform using old physics transform and velocity/rotation
        KinematicRigidBody * const ourBody = static_cast<KinematicRigidBody*>(node_->GetComponent<RigidBody>());
        btRigidBody * const body = ourBody->GetBody();
        btTransform interpolatedTrans;
        btTransformUtil::integrateTransform(_oldTransform,
                                            body->getInterpolationLinearVelocity(), body->getInterpolationAngularVelocity(),
                                            _accumulator,
                                            interpolatedTrans);
        const Quaternion interpolatedRotation = ToQuaternion(interpolatedTrans.getRotation());
        const Vector3 interpolatedPosition = ToVector3(interpolatedTrans.getOrigin()) - interpolatedRotation * ourBody->GetCenterOfMass();

        // update the transform used by the graphics system
        ourBody->GetPhysicsWorld()->SetApplyingTransforms(true);
        node_->SetWorldPosition(interpolatedPosition);
        node_->SetWorldRotation(interpolatedRotation);
        ourBody->GetPhysicsWorld()->SetApplyingTransforms(false);
    }
}*/

void Elevator::HandlePhysicsPreStep(Urho3D::StringHash eventType, Urho3D::VariantMap &eventData)
{
    // std::cout << "HandlePhysicsPreStep" << std::endl;
    if (_state != State::Idle)
    {
        const float timeStep = eventData[PhysicsPreStep::P_TIMESTEP].GetFloat();
        _accumulator += timeStep;

        // KinematicRigidBody * const ourBody = static_cast<KinematicRigidBody*>(node_->GetComponent<RigidBody>());
        JoltRigidBody * const ourBody = static_cast<JoltRigidBody*>(node_->GetComponent<JoltRigidBody>());
        // JoltPhysicsWorld * const world = static_cast<JoltPhysicsWorld*>(node_->GetScene()->GetComponent<JoltPhysicsWorld>());

        // save current (will become "old") physics transform for graphics interpolation purposes
        // ourBody->getWorldTransform(_oldTransform); // we save this for interpolation purposes

        if (_state == State::Departing || _state == State::Returning)
        {
            // calculate the direction of motion and velocity
            /*const Vector3 startToEndVec = Vector3(_endTrans.getOrigin() - _startTrans.getOrigin()).Normalized();
            Vector3 newVel = ((_state == State::Departing) ? startToEndVec : -startToEndVec)*ELEVATOR_SPEED;

            // calculate new physics transform
            btTransform newPhysicsTrans;
            newPhysicsTrans.setIdentity();
            newPhysicsTrans.setOrigin(_oldTransform.getOrigin() + ToBtVector3(newVel)*timeStep);
            newPhysicsTrans.setRotation(_oldTransform.getRotation());

            // cap the velocity / target position to not overshoot
            const btTransform &targetTrans = (_state == State::Departing) ? _endTrans : _startTrans;
            const btScalar targetRelativeY = (newPhysicsTrans.getOrigin().y() - targetTrans.getOrigin().y());
            const bool would_overshoot = (newVel.y_ > 0.0) ? (targetRelativeY >= 0.0) : (targetRelativeY <= 0.0);
            if (would_overshoot)
            {
                newPhysicsTrans = targetTrans;
                newVel = Vector3(targetTrans.getOrigin() - _oldTransform.getOrigin())/timeStep;
                if (_state == State::Departing)
                    _state = State::DestCooldown;
                else
                    _state = State::OriginCooldown;
                _cooldown = DEST_COOLDOWN;
            }

            // update the transform that is *used by physics*!
            ourBody->setOverrideTransform(newPhysicsTrans);
            // update the rest of the physics state
            ourBody->Activate();
            ourBody->SetLinearVelocity(newVel);

            // EVIL HACK to effectively get per-substep kinematic body information into Bullet, rather than per "full step"
            btRigidBody * const body = ourBody->GetBody();
            body->getInterpolationWorldTransform() = _oldTransform;
            body->saveKinematicState(timeStep);*/

            // determine "t" parametric progress between start the end
            static const float TOTAL_TIME = MAX_ELEVATOR_TRAVEL/ELEVATOR_SPEED;
            const float interpT = Urho3D::Min(_accumulator/TOTAL_TIME, 1.0);

            // calculate interpolated transform
            const Urho3D::Transform transA = Urho3D::Transform::FromMatrix3x4(_state == State::Departing ? _startTrans : _endTrans);
            const Urho3D::Transform transB = Urho3D::Transform::FromMatrix3x4(_state == State::Departing ? _endTrans : _startTrans);
            const Urho3D::Transform interp = transA.Lerp(transB, interpT);

            // tell the physics system to move to the destination within a certain amount of time (one simulation step)
            // world->GetPhysicsSystem().GetBodyInterface().MoveKinematic(ourBody->GetBodyID(), ToJoltVec3(interp.position_), ToJoltQuat(interp.rotation_), timeStep);
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
    /*// Node * const otherNode = static_cast<Node*>(eventData[NodeCollisionStart::P_OTHERNODE].GetPtr());
    RigidBody * const otherBody = static_cast<RigidBody*>(eventData[NodeCollisionStart::P_OTHERBODY].GetPtr());
    if (otherBody && otherBody->GetBody()->getUserIndex() == PhysicsUserIndex::Player)
    {
        // std::cout << "TOUCHED ELEVATOR" << std::endl;
        _state = State::Departing;
        _accumulator = 0.0;
        KinematicRigidBody * const ourBody = static_cast<KinematicRigidBody*>(node_->GetComponent<RigidBody>());
        ourBody->getWorldTransform(_startTrans);
        _endTrans = _startTrans;
        _endTrans.setOrigin(_startTrans.getOrigin() + ToBtVector3(Vector3::UP*MAX_ELEVATOR_TRAVEL));
    }*/
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
