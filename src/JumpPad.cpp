#include "JumpPad.h"
#include "globals.h"

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Physics/CollisionShape.h>

#include <Urho3D/ThirdParty/Bullet/BulletDynamics/Dynamics/btRigidBody.h>

using Urho3D::Vector3;
using Urho3D::RigidBody;
using Urho3D::CollisionShape;

JumpPad::JumpPad(Urho3D::Scene *scene, const Urho3D::Vector3 &pos, const Urho3D::Vector3 &size) :
    node_(nullptr)
{
    node_ = scene->CreateChild("JumpPad");
    // m_node->SetScale(Vector3(1.0, 1.0, 1.0));
    node_->SetPosition(pos);

    RigidBody * const rigidBody = node_->CreateComponent<RigidBody>();
    CollisionShape * const collisionShape = node_->CreateComponent<CollisionShape>();
    collisionShape->SetBox(size);

    btRigidBody * const bulletBody = rigidBody->GetBody();
    bulletBody->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE); // doesn't collide with anything!
    // bulletBody->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK); // for contact added callback to be called
    bulletBody->setUserIndex(PhysicsUserIndex::JumpPad);
}

JumpPad::~JumpPad()
{
    node_->Remove();
    node_ = nullptr;
}
