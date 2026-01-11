#include "Ladder.h"
#include "JoltPhysicsWorld.h"
#include "JoltCollisionShape.h"
#include "JoltRigidBody.h"
#include "globals.h"

#include <Urho3D/Scene/Scene.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <array> // for std::array<>
#include <algorithm> // for std::max_element()
#include <limits> // for std::numeric_limits<>

using Urho3D::Vector3;
using Urho3D::BoundingBox;

Ladder::Ladder(Urho3D::Node *node) :
    node_(node),
    body_(node_->GetComponent<JoltRigidBody>())
{
    node_->SetVar("GameObjectPtr", this);
}

Ladder::~Ladder()
{
    constrainedNodes_.clear();
    node_->Remove();
    node_ = nullptr;
    body_ = nullptr;
}

Vector3 Ladder::GetNormalForPoint(const Urho3D::Vector3 &pt) const
{
    // calculate "cylindrical" normal
    Vector3 v = pt - node_->GetPosition();
    v.y_ = 0.0; // remove the vertical component
    if (v == Vector3::ZERO)
        return Vector3::ZERO;
    v.Normalize();

    // find which cardinal direction best matches the vector
    // TODO: take the node's rotation into consideration...
    static const std::array<Vector3, 4> CARDINAL_DIRECTIONS{
        Vector3::LEFT,
        Vector3::RIGHT,
        Vector3::FORWARD,
        Vector3::BACK
    };
    std::array<float, 4> dotProducts;
    std::transform(CARDINAL_DIRECTIONS.begin(), CARDINAL_DIRECTIONS.end(), dotProducts.begin(), [&] (const Vector3 &dir) {
        return v.DotProduct(dir);
    });
    std::array<float, 4>::const_iterator bestIt = std::max_element(dotProducts.cbegin(), dotProducts.cend());
    std::size_t bestIndex = std::distance(dotProducts.cbegin(), bestIt);

    // use that cardinal direction
    return CARDINAL_DIRECTIONS[bestIndex];
}

static Urho3D::BoundingBox GetLocalAABB(Urho3D::CollisionShape *collisionShape)
{
    btCollisionShape * const shape = collisionShape->GetCollisionShape();
    btTransform localTransform;
    localTransform.setIdentity();
    btVector3 aabbMin;
    btVector3 aabbMax;
    shape->getAabb(localTransform, aabbMin, aabbMax);
    return BoundingBox(Vector3(aabbMin), Vector3(aabbMax));
}

static void SetConstraintFromAABB(JPH::SixDOFConstraintSettings &settings, const JPH::AABox &aabb)
{
    settings.mLimitMin[JPH::SixDOFConstraintSettings::EAxis::TranslationX] = aabb.mMin.GetX();
    settings.mLimitMax[JPH::SixDOFConstraintSettings::EAxis::TranslationX] = aabb.mMax.GetX();
    settings.mLimitMin[JPH::SixDOFConstraintSettings::EAxis::TranslationY] = aabb.mMin.GetY();
    settings.mLimitMax[JPH::SixDOFConstraintSettings::EAxis::TranslationY] = aabb.mMax.GetY();
    settings.mLimitMin[JPH::SixDOFConstraintSettings::EAxis::TranslationZ] = aabb.mMin.GetZ();
    settings.mLimitMax[JPH::SixDOFConstraintSettings::EAxis::TranslationZ] = aabb.mMax.GetZ();
}

void Ladder::ConstrainNode(Urho3D::Node *otherNode)
{
    // make sure we didn't already constrain it
    if (constrainedNodes_.find(otherNode) != constrainedNodes_.end())
        return;

    // make sure the node has a physics body
    JoltRigidBody * const rigidBodyB = otherNode->GetComponent<JoltRigidBody>();
    JoltPhysicsWorld * const physicsWorld = node_->GetScene()->GetComponent<JoltPhysicsWorld>();
    JoltCollisionShape * const physicsShape = node_->GetComponent<JoltCollisionShape>();
    if (!rigidBodyB || !physicsWorld || !physicsShape)
        return;
    JPH::PhysicsSystem &physicsSystem = physicsWorld->GetPhysicsSystem();
    JPH::BodyInterface &body_interface = physicsSystem.GetBodyInterface();

    // get the body IDs used for the constraint
    const JPH::BodyID bodyIdA = body_->GetBodyID();
    const JPH::BodyID bodyIdB = rigidBodyB->GetBodyID();

    // partial XYZ volume around "ladder"
    static const btScalar TOLERANCE = 0.001;
    const JPH::Vec3 expansion3D(PLAYER_RADIUS + TOLERANCE, PLAYER_HEIGHT/2.0 + TOLERANCE, PLAYER_RADIUS + TOLERANCE);
    JPH::AABox constraintBB = physicsShape->GetShape().GetWorldSpaceBounds(JPH::Mat44::sIdentity(), JPH::Vec3::sReplicate(1.0f));
    constraintBB.ExpandBy(expansion3D);

    // create the constraint for limit the distance from the ladder
    JPH::SixDOFConstraintSettings constraint_settings;
    constexpr float LADDER_MARGIN = 0.001;
    constraint_settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
    SetConstraintFromAABB(constraint_settings, constraintBB);
    JPH::Ref<JPH::SixDOFConstraint> constraint = static_cast<JPH::SixDOFConstraint *>(body_interface.CreateConstraint(&constraint_settings, bodyIdA, bodyIdB));
    physicsWorld->GetPhysicsSystem().AddConstraint(constraint); // TODO is this necessary?

    // remember that we constrained it
    constrainedNodes_.insert({otherNode, constraint});
}

void Ladder::UnconstrainNode(Urho3D::Node *otherNode)
{
    // get the physics system
    JoltPhysicsWorld * const physicsWorld = node_->GetScene()->GetComponent<JoltPhysicsWorld>();
    if (!physicsWorld)
        return;
    JPH::PhysicsSystem &physicsSystem = physicsWorld->GetPhysicsSystem();

    // make sure we actually have it
    ConstraintMap::iterator it = constrainedNodes_.find(otherNode);
    if (it == constrainedNodes_.end())
        return;

    // remove the constraint from the system
    physicsSystem.RemoveConstraint(it->second);

    // forget the constraint
    constrainedNodes_.erase(it);
}
