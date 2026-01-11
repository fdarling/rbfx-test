#include "Ball.h"
#include "JoltRigidBody.h"
#include "JoltCollisionShape.h"
#include "CreateMaterial.h"
#include "CreatePrimitives.h"
#include "globals.h"

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Scene/Scene.h>

using Urho3D::Vector3;
using Urho3D::StaticModel;

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
    JoltRigidBody * const body = node_->CreateComponent<JoltRigidBody>();
    body->SetMotionType(JoltRigidBody::MotionType::Dynamic);
    body->SetMotionQuality(JoltRigidBody::MotionQuality::LinearCast);

    // create physics shape
    JoltCollisionShape * const shape = node_->CreateComponent<JoltCollisionShape>();
    shape->SetSphere(BALL_DIAMETER);
    body->SetFriction(0.5);
    body->SetRestitution(0.1);
    body->SetLinearVelocity(vel);

    // TODO set mass, friction, etc.
    // JPH::MassProperties msp;
    // msp.ScaleToMass(1.0);
    // ball_settings.mMassPropertiesOverride = msp;
    // ball_settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    // TODO friction, was 0.5 with Bullet
    // ball_settings.mLinearDamping = 0.01f; // was 0.0 with Bullet
    // ball_settings.mAngularDamping = 0.01f; // was 0.2 with Bullet
}

Ball::~Ball()
{
    node_->Remove();
    node_ = nullptr;
}
