#include "Player.h"
#include "Ladder.h"
#include "JoltPhysicsWorld.h"
#include "JoltRigidBody.h"
#include "JoltCollisionShape.h"
#include "JoltPhysicsEvents.h"
#include "JoltPhysicsUtils.h"
#include "CreateMaterial.h"
#include "CreatePrimitives.h"
#include "globals.h"

#include <Urho3D/Core/Timer.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Scene/Scene.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>

using Urho3D::Time;
using Urho3D::Node;
using Urho3D::Vector3;
using Urho3D::StaticModel;
using Urho3D::BoundingBox;
using Urho3D::Color;
using Urho3D::Clamp;
using Urho3D::Quaternion;
using Urho3D::OUTSIDE;

Urho3D::SharedPtr<Urho3D::Model> Player::cylinderModel_;

Player::Player(Urho3D::Scene *scene, const Urho3D::Vector3 &pos) :
    Urho3D::Object(scene->GetContext()),
    node_(nullptr),
    walkDir_(Vector3::ZERO),
    ladder_(nullptr),
    ladderToGrab_(nullptr),
    onGround_(false),
    wantJump_(false)
{
    node_ = scene->CreateChild("Player");
    node_->SetPosition(pos);

    // possibly create and cache the model
    if (!cylinderModel_)
        cylinderModel_ = CreateCapsuleModel(scene->GetContext(), PLAYER_RADIUS, PLAYER_HEIGHT - PLAYER_RADIUS*2.0); // TODO support multiple contexts!

    // use the model
    StaticModel * const sm = node_->CreateComponent<StaticModel>();
    sm->SetModel(cylinderModel_);
    sm->SetMaterial(CreateMaterial(scene->GetContext(), Color(0.8, 0.8, 0.8)));
    sm->SetCastShadows(true);

    // create physics body
    JoltRigidBody * const body = node_->CreateComponent<JoltRigidBody>();
    body->SetMotionType(JoltRigidBody::MotionType::Dynamic);
    body->SetAllowedDOFs(JoltRigidBody::AllowedDOFs::TranslationX | JoltRigidBody::AllowedDOFs::TranslationY | JoltRigidBody::AllowedDOFs::TranslationZ);

    // create physics shape
    JoltCollisionShape * const shape = node_->CreateComponent<JoltCollisionShape>();
    shape->SetCapsule(PLAYER_RADIUS*2.0, PLAYER_HEIGHT - PLAYER_RADIUS*2.0);
    SubscribeToEvent(node_, E_JOLTNODECOLLISIONSTART, URHO3D_HANDLER(Player, HandleNodeCollisionStart));

    // TODO set mass, friction, etc.
    // body->SetMass(PLAYER_MASS);
    // body->SetFriction(0.8f);
    // body->SetLinearDamping(0.2f);
    // body->SetAngularDamping(0.2f);
}

Player::~Player()
{
    node_->Remove();
    node_ = nullptr;
}

static Vector3 adjustWalkDir(Player *player, const Vector3 &walkDir)
{
    // leave pitch unmodified if we aren't even on a ladder
    if (!player->IsOnLadder())
        return walkDir;

    // NOTE: on the ground and trying to leave the ladder case already handled outside this function!

    // check to see if we are at the top of the ladder (maximum altitude)
    const bool aboveLadderVertically = player->IsAboveLadderVertically();
    if (aboveLadderVertically)
    {
        // are we on top of the ladder (in the sense of it being a platform)?
        const bool aboveLadderHorizontally = player->IsAboveLadderHorizontally();
        if (aboveLadderHorizontally)
            return walkDir;

        // are we walking onto the top of the ladder?
        const bool walkingTowardsLadder = player->IsFacingLadder(walkDir);
        if (walkingTowardsLadder)
            return walkDir;
    }

    // we are not at the top of the ladder, or we are at the top but
    // trying to climb down not up
    const Vector3 ladderNormal = player->GetLadderNormal();
    const Vector3 rotAxis = ladderNormal.CrossProduct(Vector3::UP);
    const float normalDot = ladderNormal.DotProduct(walkDir);
    const Vector3 normalComponent = (ladderNormal*normalDot);
    if (normalComponent == Vector3::ZERO)
        return walkDir;
    const Vector3 verticalComponent = Vector3::UP*(walkDir.DotProduct(Vector3::UP));
    const float normalPitch = (verticalComponent + normalComponent).Angle(-ladderNormal);
    const float targetPitch = Clamp(2.0f*(normalPitch - 45.0f), -89.9f, 89.9f);
    const Vector3 newWalkDir = Quaternion(targetPitch - normalPitch, rotAxis).RotationMatrix()*walkDir;

    return newWalkDir;
};

void Player::Advance()
{
    // finally use the ladder (it was deferred in an event handler)
    if (ladderToGrab_)
    {
        Ladder * const ladder = ladderToGrab_;
        ladderToGrab_ = nullptr;
        GrabLadder(ladder);
    }

    // get the physics objects
    JoltRigidBody * const body = node_->GetComponent<JoltRigidBody>();
    JoltCollisionShape * const shape = node_->GetComponent<JoltCollisionShape>();
    JoltPhysicsWorld * const physicsWorld = node_->GetScene()->GetComponent<JoltPhysicsWorld>();
    if (!body || !physicsWorld)
        return;
    JPH::PhysicsSystem &physicsSystems = physicsWorld->GetPhysicsSystem();
    JPH::BodyInterface &body_interface = physicsSystems.GetBodyInterface();

    // get the body's transform
    const JPH::Mat44 centerOfMassTransform = body_interface.GetCenterOfMassTransform(body->GetBodyID());
    // const Urho3D::Matrix3x4 worldTransform = body->GetWorldTransform();

    // test if we are on the ground
    const JPH::NarrowPhaseQuery &query = physicsSystems.GetNarrowPhaseQuery();
    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    // perform collision test to detect standing on the ground
    query.CollideShape(
        &shape->GetShape(), // shape
        JPH::Vec3::sReplicate(1.0f), // scale TODO how am I supposed to differentiate this from the world transform?
        centerOfMassTransform, // center of mass transform
        JPH::CollideShapeSettings(), // collision settings
        JPH::Vec3::sZero(), // base offset?
        collector, // results destination
        JPH::BroadPhaseLayerFilter(),
        JPH::ObjectLayerFilter(),
        JPH::IgnoreSingleBodyFilter(body->GetBodyID())
    );

    // check collision results
    {
        bool onGround = false;
        if (collector.HadHit())
        {
            for (const JPH::CollideShapeResult &result : collector.mHits)
            {
                const JPH::Vec3 normalDir = -result.mPenetrationAxis.Normalized();
                const JPH::BodyID otherBodyID = result.mBodyID2;
                JoltRigidBody * const otherBody = reinterpret_cast<JoltRigidBody*>(body_interface.GetUserData(otherBodyID));
                Urho3D::Node * const otherNode = otherBody->GetNode();
                Ladder * const ladder = reinterpret_cast<Ladder*>(otherNode->GetVar("GameObjectPtr").GetVoidPtr());
                if (ladder)
                    continue;
                if (normalDir.GetY() > 0.4 && result.mPenetrationDepth >= 0.0)
                {
                    onGround = true;
                    break;
                }
            }
        }
        onGround_ = onGround;
    }

    // handle special ladder behavior
    if (IsOnLadder())
    {
        // if we are on the ground and generally walking away from the ladder, depart!
        if (IsOnGround() && walkDir_ != Vector3::ZERO && !IsFacingLadder(walkDir_))
        {
            // let go of the ladder
            GrabLadder(nullptr);
        }

        // otherwise, commute horizontal "into" / "out-of" the ladder movement into vertical motion
        const Vector3 adjustedDir = adjustWalkDir(this, flyDir_);

        // when on the ladder, we move at a constant speed (rather than accelerate)
        // body->Activate();
        body->SetLinearVelocity(adjustedDir*PLAYER_WALK_SPEED);
    }
    else if (walkDir_ != Vector3::ZERO)
    {
        const Vector3 currentVel = body->GetLinearVelocity();
        const Vector3 currentVelH = Vector3(currentVel.x_, 0, currentVel.z_);
        const Vector3 targetVelH = walkDir_ * PLAYER_WALK_SPEED;
        Vector3 deltaVel = targetVelH - currentVelH;
        const float timeStep = 1.0/60.0;
        const float maxDelta = PLAYER_WALK_ACCEL * timeStep;
        if (deltaVel.Length() > maxDelta)
            deltaVel = deltaVel.Normalized() * maxDelta;
        const float mass = body->GetMass();
        const Vector3 force = mass * (deltaVel / timeStep);

        if (force != Vector3::ZERO)
        {
            // body->Activate();
            body->ApplyForce(force);
            // std::cout << "force: (" << force.x_ << "," << force.y_ << "," << force.z_ << ")" << std::endl;
        }
    }
    if (wantJump_ && IsOnLadder())
    {
        // determine the jump-away direction
        const Vector3 v = ladder_->GetNormalForPoint(node_->GetPosition())*PLAYER_JUMP_VELOCITY;

        // let go of the ladder
        GrabLadder(nullptr);

        // jump away
        // body->Activate();
        body->SetLinearVelocity(v);
    }
    else if (wantJump_ && IsOnGround())
    {
        Vector3 v = body->GetLinearVelocity();
        v.y_ = PLAYER_JUMP_VELOCITY;
        // body->Activate();
        body->SetLinearVelocity(v);
    }
}

void Player::SetWalkAndFlyDirections(const Urho3D::Vector3 &walkDir, const Urho3D::Vector3 &flyDir)
{
    walkDir_ = walkDir;
    flyDir_ = flyDir;
}

void Player::SetJumping(bool en)
{
    wantJump_ = en;
}

bool Player::IsFacingLadder(const Vector3 &faceDir) const
{
    if (!ladder_)
        return false;

    const Vector3 v = ladder_->GetNormalForPoint(node_->GetPosition());

    return faceDir.DotProduct(v) < 0.0f;
}

static const float OVERLAP_TOLERANCE = 0.05;
static const Vector3 HORIZONTAL_OVERLAP_TOLERANCE(OVERLAP_TOLERANCE, 0.0f, OVERLAP_TOLERANCE);

bool Player::IsAboveLadderVertically() const
{
    if (!ladder_)
        return false;
    JoltCollisionShape * const playerShape = node_->GetComponent<JoltCollisionShape>();
    JoltCollisionShape * const ladderShape = ladder_->GetNode()->GetComponent<JoltCollisionShape>();
    const BoundingBox playerBB = playerShape->GetWorldBoundingBox();
    const BoundingBox ladderBB = ladderShape->GetWorldBoundingBox();
    return playerBB.min_.y_ + OVERLAP_TOLERANCE >= ladderBB.max_.y_;
}

bool Player::IsAboveLadderHorizontally() const
{
    if (!ladder_)
        return false;
    JoltCollisionShape * const playerShape = node_->GetComponent<JoltCollisionShape>();
    JoltCollisionShape * const ladderShape = ladder_->GetNode()->GetComponent<JoltCollisionShape>();
    const BoundingBox playerBB = playerShape->GetWorldBoundingBox();
          BoundingBox ladderBB = ladderShape->GetWorldBoundingBox();

    // shrink ladder bounding box horizontally
    ladderBB.min_ += HORIZONTAL_OVERLAP_TOLERANCE;
    ladderBB.max_ -= HORIZONTAL_OVERLAP_TOLERANCE;

    // grow ladder bounding box vertically by height of player
    ladderBB.max_.y_ += playerBB.Size().y_;

    return ladderBB.IsInside(playerBB) != OUTSIDE;
}

Urho3D::Vector3 Player::GetLadderNormal() const
{
    if (!ladder_)
        return Vector3::ZERO;
    return ladder_->GetNormalForPoint(node_->GetPosition());
}

void Player::HandleNodeCollisionStart(Urho3D::StringHash eventType, Urho3D::VariantMap &eventData)
{
    Node * const nodeB = static_cast<Node*>(eventData[JoltNodeCollisionStart::P_OTHERNODE].GetPtr());
    JoltRigidBody * const bodyB = static_cast<JoltRigidBody*>(eventData[JoltNodeCollisionStart::P_OTHERBODY].GetPtr());
    if (nodeB && bodyB)
    {
        // TODO identify the object type before assuming, right now only the Ladder sets GameObjectPtr...
        Ladder * const ladder = reinterpret_cast<Ladder*>(nodeB->GetVar("GameObjectPtr").GetVoidPtr());
        if (ladder)
        {
            // we are not allowed to modify things during this event, defer using the ladder until later
            ladderToGrab_ = ladder;
        }
    }
}

void Player::GrabLadder(Ladder *ladder)
{
    // no change case
    if (ladder_ == ladder)
        return;

    // let go of old ladder
    if (ladder_)
        ladder_->UnconstrainNode(node_);

    // remember the ladder
    ladder_ = ladder;

    // access our physics body
    JoltRigidBody * const body = node_->GetComponent<JoltRigidBody>();

    // attach to the new ladder
    if (ladder)
        ladder->ConstrainNode(node_);

    // no gravity when on any ladder
    body->SetGravityFactor(ladder ? 0.0 : 1.0);
}
