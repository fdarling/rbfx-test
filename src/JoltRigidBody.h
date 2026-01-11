#pragma once

#include <Urho3D/Scene/Component.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Body/MotionQuality.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

namespace Urho3D
{
} // namespace Urho3D

class JoltPhysicsWorld;

class JoltRigidBody : public Urho3D::Component
{
    URHO3D_OBJECT(JoltRigidBody, Urho3D::Component);
public:
    using MotionType = JPH::EMotionType;
    using MotionQuality = JPH::EMotionQuality;
    using AllowedDOFs = JPH::EAllowedDOFs;
public:
    explicit JoltRigidBody(Urho3D::Context *context);
    ~JoltRigidBody() override;
    static void RegisterObject(Urho3D::Context *context);
    void ReleaseBody();

    JPH::BodyID GetBodyID() {return joltBodyId_;}
    JPH::BodyID GetBodyID() const {return joltBodyId_;}

    void ApplyForce(const Urho3D::Vector3 &force);
    void MoveKinematic(const Urho3D::Vector3 &pos, const Urho3D::Quaternion &rot, float deltaTime);

    Urho3D::Vector3 GetAngularVelocity() const;
    Urho3D::Vector3 GetLinearVelocity() const;
    Urho3D::Matrix3x4 GetWorldTransform() const;
    float GetMass() const;

    void SetAllowedDOFs(AllowedDOFs dofs);
    void SetFriction(float friction);
    void SetGravityFactor(float gravityFactor);
    void SetMotionType(MotionType motionType);
    void SetMotionQuality(MotionQuality motionQuality);
    void SetAngularVelocity(const Urho3D::Vector3 &velocity);
    void SetLinearVelocity(const Urho3D::Vector3 &velocity);
    void SetRestitution(float restitution);
protected:
    void OnSceneSet(Urho3D::Scene *previousScene, Urho3D::Scene *scene) override;
private:
    template <typename ValueType, typename BodyInterfaceType>
    ValueType BodyAttributeGetter(ValueType (BodyInterfaceType::*GetterFunc)(const JPH::BodyID &) const) const;
    template <typename ValueType, typename BodyInterfaceType, typename... Args, typename... RestArgs>
    void BodyAttributeSetter(
        ValueType JPH::BodyCreationSettings::*SettingPtr,
        void (BodyInterfaceType::*SetterFunc)(const JPH::BodyID &, ValueType, Args...),
        ValueType value,
        RestArgs&&... restArgs
    );
    // void HandleSceneSubsystemUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap &eventData);
    void AddBodyToWorld();
    void RemoveBodyFromWorld();
private:
    // Urho3D::SharedPtr<JoltPhysicsWorld> joltPhysicsWorld_;
    Urho3D::WeakPtr<JoltPhysicsWorld> joltPhysicsWorld_;
    JPH::BodyID joltBodyId_;
    JPH::BodyCreationSettings joltBodySettings_;
};
