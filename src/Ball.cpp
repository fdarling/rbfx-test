#include "Ball.h"
#include "JoltRigidBody.h"
#include "JoltCollisionShape.h"
#include "CreateMaterial.h"
#include "CreatePrimitives.h"
#include "globals.h"

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
// #include <Urho3D/Physics/RigidBody.h>
// #include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Scene/Scene.h>

using Urho3D::Vector3;
using Urho3D::StaticModel;
// using Urho3D::RigidBody;
// using Urho3D::CollisionShape;

Urho3D::SharedPtr<Urho3D::Model> Ball::sphereModel_;

static const float BALL_DIAMETER = BALL_RADIUS*2.0;

Ball::Ball(Urho3D::Scene *scene, const Urho3D::Vector3 &pos, const Urho3D::Vector3 &vel, const Urho3D::Color &color) :
    node_(nullptr)
{
    node_ = scene->CreateChild("Ball");
    // node_->SetScale(Vector3(1.0f, 1.0f, 1.0f));
    node_->SetPosition(pos);

    // possibly create and cache the model
    if (!sphereModel_)
        sphereModel_ = CreateSphereModel(scene->GetContext()); // TODO support multiple contexts!

    // use the model
    StaticModel * const sm = node_->CreateComponent<StaticModel>();
    sm->SetModel(sphereModel_);
    sm->SetMaterial(CreateMaterial(scene->GetContext(), color));
    sm->SetCastShadows(true);

    // AddText3DLabel(node_, "Ball");

    // create physics body
    /*RigidBody * const body = node_->CreateComponent<RigidBody>();
    body->SetMass(1.0f);
    body->SetFriction(0.5f);
    body->SetLinearDamping(0.0f);
    body->SetAngularDamping(0.2f);
    body->SetLinearVelocity(vel);
    body->SetCcdRadius(BALL_RADIUS*0.98); // TODO it is supposed to be smaller, right?
    body->SetCcdMotionThreshold(1e-7); // TODO why this number?

    // create physics shape
    CollisionShape * const shape = node_->CreateComponent<CollisionShape>();
    shape->SetSphere(BALL_DIAMETER);
    shape->SetMargin(0.001);*/
    
    JoltRigidBody * const body = node_->CreateComponent<JoltRigidBody>();
    body->SetMotionType(JoltRigidBody::MotionType::Dynamic);
    body->SetMotionQuality(JoltRigidBody::MotionQuality::LinearCast);
    JoltCollisionShape * const shape = node_->CreateComponent<JoltCollisionShape>();
    shape->SetSphere(BALL_DIAMETER);
    body->SetFriction(0.5);
    body->SetRestitution(0.1);
    body->SetLinearVelocity(vel);
    // TODO set the mass?
    // JPH::MassProperties msp;
    // msp.ScaleToMass(1.0); // kg
    // bullet_settings.mMassPropertiesOverride = msp;
    // bullet_settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    // bullet_settings.mLinearDamping = 0.01f;
    // bullet_settings.mAngularDamping = 0.01f;
}

Ball::~Ball()
{
    node_->Remove();
    node_ = nullptr;
}
