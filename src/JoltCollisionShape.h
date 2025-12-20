#pragma once

#include <Urho3D/Scene/Component.h>
//#include <Urho3D/Container/Ptr.h> // for Urho3D::SharedPtr<>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h> // for JPH::Ref
#include <Jolt/Physics/Collision/Shape/Shape.h> // for JPH::Shape
#include <variant>
namespace Urho3D
{
} // namespace Urho3D

class JoltCollisionShape : public Urho3D::Component
{
    URHO3D_OBJECT(JoltCollisionShape, Urho3D::Component);
public:
    explicit JoltCollisionShape(Urho3D::Context *context);
    ~JoltCollisionShape() override;
    static void RegisterObject(Urho3D::Context *context);
    void ApplyAttributes() override;
    void SetCapsule(float diameter, float height, const Urho3D::Vector3 &position = Urho3D::Vector3::ZERO, const Urho3D::Quaternion &rotation = Urho3D::Quaternion::IDENTITY);
    void SetSphere(float diameter, const Urho3D::Vector3 &position = Urho3D::Vector3::ZERO, const Urho3D::Quaternion &rotation = Urho3D::Quaternion::IDENTITY);
    void SetTriangleMesh(Urho3D::Model *model, unsigned lodLevel = 0, const Urho3D::Vector3 &scale = Urho3D::Vector3::ONE, const Urho3D::Vector3 &position = Urho3D::Vector3::ZERO, const Urho3D::Quaternion &rotation = Urho3D::Quaternion::IDENTITY);
    //void SetCustomTriangleMesh(Urho3D::CustomGeometry *custom, const Urho3D::Vector3 &scale = Urho3D::Vector3::ONE, const Urho3D::Vector3 &position = Urho3D::Vector3::ZERO, const Urho3D::Quaternion &rotation = Urho3D::Quaternion::IDENTITY);
    void SetConvexHull(Urho3D::Model *model, unsigned lodLevel = 0, const Urho3D::Vector3 &scale = Urho3D::Vector3::ONE, const Urho3D::Vector3 &position = Urho3D::Vector3::ZERO, const Urho3D::Quaternion &rotation = Urho3D::Quaternion::IDENTITY);

    void NotifyRigidBody();
protected:
    void OnSceneSet(Urho3D::Scene *previousScene, Urho3D::Scene *scene) override;
private:
    void UpdateShape();
    // void HandleSceneSubsystemUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap &eventData);

    struct CapsuleShapeData
    {
        float diameter_;
        float height_;
    };
    struct SphereShapeData
    {
        float diameter_;
    };
    struct ModelShapeData
    {
        Urho3D::SharedPtr<Urho3D::Model> model_;
        unsigned lodLevel_;
        bool useConvexHull_;
    };

    std::variant<std::monostate, CapsuleShapeData, SphereShapeData, ModelShapeData> shapeData_;
    Urho3D::Vector3 scale_;
    Urho3D::Vector3 position_;
    Urho3D::Quaternion rotation_;
    bool recreateShape_;
    JPH::Ref<JPH::Shape> joltUnscaledShape_;
    JPH::Ref<JPH::Shape> joltScaledShape_;
};
