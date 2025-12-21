#include "JoltRigidBody.h"
#include "JoltPhysicsWorld.h"
#include "JoltPhysicsDefs.h"
#include "JoltPhysicsUtils.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h> // TODO remove me
#include <Jolt/Physics/PhysicsSystem.h>

#include <functional>
#include <utility>
// #include <iostream>

JoltRigidBody::JoltRigidBody(Urho3D::Context *context) :
    Component(context)
{
    // URHO3D_LOGINFO("JoltRigidBody::JoltRigidBody()");
    joltBodySettings_.mAllowDynamicOrKinematic = true;
    joltBodySettings_.mUserData = reinterpret_cast<JPH::uint64>(this);
}

JoltRigidBody::~JoltRigidBody()
{
    // URHO3D_LOGINFO("JoltRigidBody::~JoltRigidBody()");
    ReleaseBody();
}

void JoltRigidBody::RegisterObject(Urho3D::Context *context)
{
    context->AddFactoryReflection<JoltRigidBody>(Urho3D::Category_Physics);
}

void JoltRigidBody::ReleaseBody()
{
    // URHO3D_LOGINFO("JoltRigidBody::ReleaseBody()");
    if (joltPhysicsWorld_ && !joltBodyId_.IsInvalid())
    {
        RemoveBodyFromWorld();

        JPH::BodyInterface &body_interface = joltPhysicsWorld_->GetPhysicsSystem().GetBodyInterface();
        body_interface.DestroyBody(joltBodyId_);
        joltBodyId_ = JPH::BodyID();
    }
}

void JoltRigidBody::MoveKinematic(const Urho3D::Vector3 &pos, const Urho3D::Quaternion &rot, float deltaTime)
{
    // TODO defer the movement until we have the ability to apply it!

    // make sure we have a body to update
    if (!joltPhysicsWorld_ || joltBodyId_.IsInvalid())
        return;

    // get the body interface
    JPH::BodyInterface &body_interface = joltPhysicsWorld_->GetPhysicsSystem().GetBodyInterface();

    // actually move the body
    body_interface.MoveKinematic(joltBodyId_, ToJoltVec3(pos), ToJoltQuat(rot), deltaTime);
}

template <typename ValueType, typename BodyInterfaceType>
ValueType JoltRigidBody::BodyAttributeGetter(ValueType (BodyInterfaceType::*GetterFunc)(const JPH::BodyID &) const) const
{
    // make sure we have a body to update
    if (!joltPhysicsWorld_ || joltBodyId_.IsInvalid())
        return ValueType(); // TODO support defining a default in the template, or in the arguments

    // get the body interface
    JPH::BodyInterface &body_interface = joltPhysicsWorld_->GetPhysicsSystem().GetBodyInterface();

    // call the appropriate body interface method
    return std::invoke(
        GetterFunc, // member function pointer
        body_interface, // object instance
        joltBodyId_ // first method argument
    );
}

template <typename ValueType, typename BodyInterfaceType, typename... Args, typename... RestArgs>
void JoltRigidBody::BodyAttributeSetter(
    ValueType JPH::BodyCreationSettings::*SettingPtr,
    void (BodyInterfaceType::*SetterFunc)(const JPH::BodyID &, ValueType, Args...),
    ValueType value,
    RestArgs&&... restArgs
)
{
    // bail if we already have this setting
    if ((joltBodySettings_.*SettingPtr) == value)
        return;

    // remember the setting
    joltBodySettings_.*SettingPtr = value;

    // make sure we have a body to update
    if (!joltPhysicsWorld_ || joltBodyId_.IsInvalid())
        return;

    // get the body interface
    JPH::BodyInterface &body_interface = joltPhysicsWorld_->GetPhysicsSystem().GetBodyInterface();

    // call the appropriate body interface method
    std::invoke(
        SetterFunc, // member function pointer
        body_interface, // object instance
        joltBodyId_, // first method argument
        joltBodySettings_.*SettingPtr, // second method argument
        std::forward<RestArgs>(restArgs)... // any other arguments
    );
}

Urho3D::Vector3 JoltRigidBody::GetAngularVelocity() const
{
    return ToVector3(BodyAttributeGetter(&JPH::BodyInterface::GetAngularVelocity));
}

Urho3D::Vector3 JoltRigidBody::GetLinearVelocity() const
{
    return ToVector3(BodyAttributeGetter(&JPH::BodyInterface::GetLinearVelocity));
}

void JoltRigidBody::SetAllowedDOFs(AllowedDOFs dofs)
{
    // bail if we already have this setting
    if ((joltBodySettings_.mAllowedDOFs) == dofs)
        return;

    // remember the setting
    joltBodySettings_.mAllowedDOFs = dofs;

    // make sure we have a body to update
    if (!joltPhysicsWorld_ || joltBodyId_.IsInvalid())
        return;

    // try to lock the body (in might be deleted?)
    const JPH::BodyLockInterfaceLocking &lock_interface = joltPhysicsWorld_->GetPhysicsSystem().GetBodyLockInterface();
    JPH::BodyLockWrite lock(lock_interface, joltBodyId_);
    if (!lock.Succeeded())
        return;

    // make sure the body even has motion properties
    JPH::Body &body = lock.GetBody();
    if (!body.IsDynamic())
        return;

    // update the motion properties
    JPH::MotionProperties * const motionProps = body.GetMotionProperties();
    motionProps->SetMassProperties(dofs, joltBodySettings_.GetMassProperties());
}

void JoltRigidBody::SetFriction(float friction)
{
    BodyAttributeSetter(
        &JPH::BodyCreationSettings::mFriction,
        &JPH::BodyInterface::SetFriction,
        friction
    );
}

void JoltRigidBody::SetMotionType(MotionType motionType)
{
    BodyAttributeSetter(
        &JPH::BodyCreationSettings::mMotionType,
        &JPH::BodyInterface::SetMotionType,
        motionType,
        JPH::EActivation::Activate
    );
    const JPH::ObjectLayer layer = (joltBodySettings_.mMotionType == MotionType::Static) ? JoltPhysicsLayers::NON_MOVING : JoltPhysicsLayers::MOVING;
    BodyAttributeSetter(
        &JPH::BodyCreationSettings::mObjectLayer,
        &JPH::BodyInterface::SetObjectLayer,
        layer
    );
}

void JoltRigidBody::SetMotionQuality(MotionQuality motionQuality)
{
    BodyAttributeSetter(
        &JPH::BodyCreationSettings::mMotionQuality,
        &JPH::BodyInterface::SetMotionQuality,
        motionQuality
    );
}

void JoltRigidBody::SetAngularVelocity(const Urho3D::Vector3 &velocity)
{
    // TODO stash velocity for deferred application
    if (!joltPhysicsWorld_ || joltBodyId_.IsInvalid())
        return;
    JPH::BodyInterface &body_interface = joltPhysicsWorld_->GetPhysicsSystem().GetBodyInterface();
    body_interface.SetAngularVelocity(joltBodyId_, ToJoltVec3(velocity));
}

void JoltRigidBody::SetLinearVelocity(const Urho3D::Vector3 &velocity)
{
    // TODO stash velocity for deferred application
    if (!joltPhysicsWorld_ || joltBodyId_.IsInvalid())
        return;
    JPH::BodyInterface &body_interface = joltPhysicsWorld_->GetPhysicsSystem().GetBodyInterface();
    body_interface.SetLinearVelocity(joltBodyId_, ToJoltVec3(velocity));
}

void JoltRigidBody::SetRestitution(float restitution)
{
    BodyAttributeSetter(
        &JPH::BodyCreationSettings::mRestitution,
        &JPH::BodyInterface::SetRestitution,
        restitution
    );
}

void JoltRigidBody::OnSceneSet(Urho3D::Scene *previousScene, Urho3D::Scene *scene)
{
    // URHO3D_LOGINFO("JoltRigidBody::OnSceneSet({}, {})", (void*)previousScene, (void*)scene);
    if (scene)
    {
        Urho3D::Node * const node = GetNode();
        if (scene == node)
            URHO3D_LOGWARNING(GetTypeName() + " should not be created to the root scene node");

        const JPH::Vec3 joltPos = ToJoltVec3(node->GetWorldPosition());
        const JPH::Quat joltRot = ToJoltQuat(node->GetWorldRotation());
        // std::cout << "joltPos: " << joltPos << " joltRot: " << joltRot << std::endl;

        joltPhysicsWorld_ = scene->GetOrCreateComponent<JoltPhysicsWorld>();

        // create empty default shape for the body
        JPH::EmptyShapeSettings emptyShapeSettings;
        JPH::Shape::ShapeResult shapeResult = emptyShapeSettings.Create();
        JPH::Ref<JPH::Shape> emptyShape = shapeResult.Get();

        JPH::BodyInterface &body_interface = joltPhysicsWorld_->GetPhysicsSystem().GetBodyInterface();
        joltBodySettings_.SetShape(emptyShape);
        joltBodySettings_.mPosition = joltPos;
        joltBodySettings_.mRotation = joltRot;
        joltBodySettings_.mObjectLayer = (joltBodySettings_.mMotionType == MotionType::Static) ? JoltPhysicsLayers::NON_MOVING : JoltPhysicsLayers::MOVING;
        const JPH::Body * const body = body_interface.CreateBody(joltBodySettings_);
        if (!body)
        {
            URHO3D_LOGWARNING("Failed to create Jolt Physics JPH::Body object!");
            return;
        }
        joltBodyId_ = body->GetID();

        // physicsWorld_->AddRigidBody(this);

        AddBodyToWorld();
    }
    else
    {
        ReleaseBody();

        // if (physicsWorld_)
            // physicsWorld_->RemoveRigidBody(this);
    }
}

void JoltRigidBody::AddBodyToWorld()
{
    // URHO3D_LOGINFO("JoltRigidBody::AddBodyToWorld()");
    if (!joltPhysicsWorld_)
        return;
    if (joltBodyId_.IsInvalid())
        return;
    JPH::BodyInterface &body_interface = joltPhysicsWorld_->GetPhysicsSystem().GetBodyInterface();
    body_interface.AddBody(joltBodyId_, JPH::EActivation::Activate);// TODO which activation setting?

    /*if (body_)
        RemoveBodyFromWorld();
    else
    {
        // Correct inertia will be calculated below
        btVector3 localInertia(0.0f, 0.0f, 0.0f);
        body_ = ea::make_unique<btRigidBody>(mass_, this, shiftedCompoundShape_.get(), localInertia);
        body_->setUserPointer(this);

        // Check if CollisionShapes already exist in the node and add them to the compound shape.
        // Do not update mass yet, but do it once all shapes have been added
        ea::vector<CollisionShape*> shapes;
        node_->GetComponents<CollisionShape>(shapes);
        for (auto i = shapes.begin(); i != shapes.end(); ++i)
            (*i)->NotifyRigidBody(false);

        // Check if this node contains Constraint components that were waiting for the rigid body to be created, and signal them
        // to create themselves now
        ea::vector<Constraint*> constraints;
        node_->GetComponents<Constraint>(constraints);
        for (auto i = constraints.begin(); i != constraints.end(); ++i)
            (*i)->CreateConstraint();
    }

    UpdateMass();
    UpdateGravity();

    int flags = body_->getCollisionFlags();
    if (trigger_)
        flags |= btCollisionObject::CF_NO_CONTACT_RESPONSE;
    else
        flags &= ~btCollisionObject::CF_NO_CONTACT_RESPONSE;
    if (kinematic_)
        flags |= btCollisionObject::CF_KINEMATIC_OBJECT;
    else
        flags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
    body_->setCollisionFlags(flags);
    body_->forceActivationState(kinematic_ ? DISABLE_DEACTIVATION : ISLAND_SLEEPING);

    if (!IsEnabledEffective())
        return;

    btDiscreteDynamicsWorld* world = physicsWorld_->GetWorld();
    world->addRigidBody(body_.get(), (short)collisionLayer_, (short)collisionMask_);*/
}

void JoltRigidBody::RemoveBodyFromWorld()
{
    // URHO3D_LOGINFO("JoltRigidBody::RemoveBodyFromWorld()");
    if (!joltPhysicsWorld_)
        return;
    if (joltBodyId_.IsInvalid()) // && inWorld_
        return;
    JPH::BodyInterface &body_interface = joltPhysicsWorld_->GetPhysicsSystem().GetBodyInterface();
    if (body_interface.IsAdded(joltBodyId_))
        body_interface.RemoveBody(joltBodyId_);
    //inWorld_ = false;
}
