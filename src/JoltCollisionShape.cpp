#include "JoltCollisionShape.h"
#include "JoltRigidBody.h"
#include "JoltPhysicsWorld.h"
#include "JoltPhysicsUtils.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h> // for JPH::Ref
#include <Jolt/Physics/Body/BodyInterface.h> // for JPH::BodyInterface
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h> // for JPH::CapsuleShape
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h> // for JPH::ConvexHullShape and JPH::ConvexHullShapeSettings
#include <Jolt/Physics/Collision/Shape/EmptyShape.h> // for JPH::EmptyShape and JPH::EmptyShapeSettings
#include <Jolt/Physics/Collision/Shape/MeshShape.h> // for JPH::MeshShape and JPH::MeshShapeSettings
#include <Jolt/Physics/Collision/Shape/Shape.h> // for JPH::Shape
#include <Jolt/Physics/Collision/Shape/SphereShape.h> // for JPH::SphereShape
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h> // for JPH::StaticCompoundShape and JPH::StaticCompoundShapeSettings
#include <Jolt/Physics/Collision/Shape/ScaledShape.h> // for JPH::ScaledShape
#include <Jolt/Physics/PhysicsSystem.h> // for JPH::PhysicsSystem

#include <Jolt/Physics/Collision/Shape/BoxShape.h>

static ea::vector< JPH::Ref<JPH::Shape> > ModelToJoltConvexHulls(Urho3D::Model *model, unsigned lodLevel)
{
    ea::vector< JPH::Ref<JPH::Shape> > result;
    const unsigned numGeometries = model->GetNumGeometries();

    for (unsigned i = 0; i < numGeometries; ++i)
    {
        Urho3D::Geometry * const geometry = model->GetGeometry(i, lodLevel);
        if (!geometry)
        {
            URHO3D_LOGWARNING("Skipping null geometry for triangle mesh collision");
            continue;
        }

        ea::shared_array<unsigned char> vertexData;
        ea::shared_array<unsigned char> indexData;
        unsigned vertexSize = 0;
        unsigned indexSize = 0;
        const ea::vector<Urho3D::VertexElement> *elements = nullptr;

        geometry->GetRawDataShared(vertexData, vertexSize, indexData, indexSize, elements);
        if (!vertexData || !indexData || !elements || Urho3D::VertexBuffer::GetElementOffset(*elements, Urho3D::TYPE_VECTOR3, Urho3D::SEM_POSITION) != 0 || vertexSize < sizeof(JPH::Vec3))
        {
            URHO3D_LOGWARNING("Skipping geometry with no or unsuitable CPU-side geometry data for triangle mesh collision");
            continue;
        }

        // load vertices using indices
        JPH::Array<JPH::Vec3> points;
        {
            const unsigned char *vertex_src_ptr = vertexData.get();
            const unsigned char *index_src_ptr = &indexData[geometry->GetIndexStart() * indexSize];
            const unsigned index_count = geometry->GetIndexCount();
            const std::size_t indexCopySize = Urho3D::Min(indexSize, sizeof(JPH::uint32));
            points.reserve(index_count);
            ea::vector<bool> used_index(index_count, false);
            for (unsigned i = 0; i < index_count; i++)
            {
                // load the index
                JPH::uint32 index = 0;
                memcpy(&index, index_src_ptr, indexCopySize);
                index_src_ptr += indexSize;

                // skip indices we've already used
                if (used_index[index])
                    continue;
                used_index[index] = true;

                // load the vertex
                JPH::Vec3 point;
                memcpy(&point, vertex_src_ptr + index*vertexSize, sizeof(JPH::Vec3));
                points.emplace_back(std::move(point));
            }
        }

        JPH::ConvexHullShapeSettings convexHullSettings(points);
        JPH::Shape::ShapeResult shapeResult = convexHullSettings.Create();
        if (shapeResult.HasError())
        {
            URHO3D_LOGWARNING("Failed to create a Jolt Physics JPH::ConvexHullShape due to error: {}", shapeResult.GetError());
            continue;
        }
        JPH::Ref<JPH::Shape> shape = shapeResult.Get();
        result.push_back(shape);
    }

    return result;
}

static ea::vector< JPH::Ref<JPH::Shape> > ModelToJoltMeshes(Urho3D::Model *model, unsigned lodLevel)
{
    ea::vector< JPH::Ref<JPH::Shape> > result;
    const unsigned numGeometries = model->GetNumGeometries();

    for (unsigned i = 0; i < numGeometries; ++i)
    {
        Urho3D::Geometry * const geometry = model->GetGeometry(i, lodLevel);
        if (!geometry)
        {
            URHO3D_LOGWARNING("Skipping null geometry for triangle mesh collision");
            continue;
        }

        ea::shared_array<unsigned char> vertexData;
        ea::shared_array<unsigned char> indexData;
        unsigned vertexSize = 0;
        unsigned indexSize = 0;
        const ea::vector<Urho3D::VertexElement> *elements = nullptr;

        geometry->GetRawDataShared(vertexData, vertexSize, indexData, indexSize, elements);
        if (!vertexData || !indexData || !elements || Urho3D::VertexBuffer::GetElementOffset(*elements, Urho3D::TYPE_VECTOR3, Urho3D::SEM_POSITION) != 0 || vertexSize < sizeof(JPH::Float3))
        {
            URHO3D_LOGWARNING("Skipping geometry with no or unsuitable CPU-side geometry data for triangle mesh collision");
            continue;
        }
        // URHO3D_LOGINFO("ModelToJoltMeshes(): vertexSize = {}", vertexSize);
        // URHO3D_LOGINFO("ModelToJoltMeshes(): indexSize = {}", indexSize);
        // URHO3D_LOGINFO("ModelToJoltMeshes(): geometry->GetIndexCount() = {}", geometry->GetIndexCount());

        // convert indices and vertices to Jolt Triangles
        JPH::TriangleList joltTriangles;
        {
            const unsigned char *vertex_src_ptr = vertexData.get();
            const unsigned char *index_src_ptr = &indexData[geometry->GetIndexStart() * indexSize];
            const std::size_t indexCopySize = Urho3D::Min(indexSize, sizeof(JPH::uint32));
            const unsigned numTriangles = geometry->GetIndexCount()/3;
            joltTriangles.reserve(numTriangles);
            for (unsigned i = 0; i < numTriangles; i++)
            {
                // load the index values
                JPH::uint32 indices[3] = {0, 0, 0};
                for (int j = 0; j < 3; j++)
                {
                    memcpy(indices + j, index_src_ptr, indexCopySize);
                    index_src_ptr += indexSize;
                }

                // load the vertices using those indices
                JPH::Triangle triangle;
                for (int j = 0; j < 3; j++)
                {
                    const JPH::uint32 index = indices[j];
                    memcpy(triangle.mV + j, vertex_src_ptr + index*vertexSize, sizeof(JPH::Float3));
                }
                joltTriangles.emplace_back(triangle);
            }
        }

        JPH::MeshShapeSettings meshSettings(joltTriangles);
        JPH::Shape::ShapeResult shapeResult = meshSettings.Create();
        if (shapeResult.HasError())
        {
            URHO3D_LOGWARNING("Failed to create a Jolt Physics JPH::MeshShape due to error: {}", shapeResult.GetError());
            continue;
        }
        JPH::Ref<JPH::Shape> shape = shapeResult.Get();
        result.push_back(shape);
    }

    return result;
}

JoltCollisionShape::JoltCollisionShape(Urho3D::Context *context) :
    Component(context),
    shapeData_(std::monostate{}),
    scale_(Urho3D::Vector3::ONE),
    position_(Urho3D::Vector3::ZERO),
    rotation_(Urho3D::Quaternion::IDENTITY),
    recreateShape_(true)
{
    URHO3D_LOGINFO("JoltCollisionShape::JoltCollisionShape()");
}

JoltCollisionShape::~JoltCollisionShape()
{
    URHO3D_LOGINFO("JoltCollisionShape::~JoltCollisionShape()");
}

void JoltCollisionShape::RegisterObject(Urho3D::Context *context)
{
    context->AddFactoryReflection<JoltCollisionShape>(Urho3D::Category_Physics);
}

void JoltCollisionShape::ApplyAttributes()
{
    URHO3D_LOGINFO("JoltCollisionShape::ApplyAttributes()");
    if (recreateShape_)
    {
        UpdateShape();
        NotifyRigidBody();
    }
}

void JoltCollisionShape::SetCapsule(float diameter, float height, const Urho3D::Vector3 &position, const Urho3D::Quaternion &rotation)
{
    shapeData_ = CapsuleShapeData{
        .diameter_{diameter},
        .height_{height}
    };
    scale_ = Urho3D::Vector3::ONE;
    position_ = position;
    rotation_ = rotation;

    UpdateShape();
    NotifyRigidBody();
}

void JoltCollisionShape::SetSphere(float diameter, const Urho3D::Vector3 &position, const Urho3D::Quaternion &rotation)
{
    shapeData_ = SphereShapeData{
        .diameter_{diameter}
    };
    scale_ = Urho3D::Vector3::ONE;
    position_ = position;
    rotation_ = rotation;

    UpdateShape();
    NotifyRigidBody();
}

void JoltCollisionShape::SetTriangleMesh(Urho3D::Model *model, unsigned lodLevel, const Urho3D::Vector3 &scale, const Urho3D::Vector3 &position, const Urho3D::Quaternion &rotation)
{
    URHO3D_LOGINFO("JoltCollisionShape::SetTriangleMesh()");
    shapeData_ = ModelShapeData{
        .model_{model},
        .lodLevel_ = lodLevel,
        .useConvexHull_ = false
    };
    scale_ = scale;
    position_ = position;
    rotation_ = rotation;

    UpdateShape();
    NotifyRigidBody();
}

void JoltCollisionShape::SetConvexHull(Urho3D::Model *model, unsigned lodLevel, const Urho3D::Vector3 &scale, const Urho3D::Vector3 &position, const Urho3D::Quaternion &rotation)
{
    URHO3D_LOGINFO("JoltCollisionShape::SetConvexHull()");
    shapeData_ = ModelShapeData{
        .model_{model},
        .lodLevel_ = lodLevel,
        .useConvexHull_ = true
    };
    scale_ = scale;
    position_ = position;
    rotation_ = rotation;

    UpdateShape();
    NotifyRigidBody();
}

void JoltCollisionShape::NotifyRigidBody()
{
    URHO3D_LOGINFO("JoltCollisionShape::NotifyRigidBody()");

    // sanity check, this shouldn't ever be the case! it means not even an EmptyShape was able to be created...
    if (!joltUnscaledShape_)
        return;

    // get the node
    Urho3D::Node * const node = GetNode();
    if (!node)
        return;

    // get JoltRigidBody component
    JoltRigidBody * const joltRigidBody = node->GetComponent<JoltRigidBody>();
    if (!joltRigidBody)
        return;

    // get the internal Jolt body ID
    JPH::BodyID bodyID = joltRigidBody->GetBodyID();
    if (bodyID.IsInvalid())
        return;

    // we need the "physics world" Component
    Urho3D::Scene * const scn = GetScene();
    if (!scn)
        return;
    JoltPhysicsWorld * const joltPhysicsWorld = scn->GetOrCreateComponent<JoltPhysicsWorld>();

    // get the body interface
    JPH::BodyInterface &body_interface = joltPhysicsWorld->GetPhysicsSystem().GetBodyInterface();

    // determine the node's scale
    const JPH::Vec3 scale = ToJoltVec3(node->GetWorldScale());
    if (scale != JPH::Vec3(1.0, 1.0, 1.0))
        joltScaledShape_ = new JPH::ScaledShape(joltUnscaledShape_, scale);

    // use the body interface to actually set the rigid body's shape
    body_interface.SetShape(bodyID, joltScaledShape_ ? joltScaledShape_ : joltUnscaledShape_, false, JPH::EActivation::Activate);
    URHO3D_LOGINFO("JoltCollisionShape::NotifyRigidBody(): called SetShape()!");
    // Remove the shape first to ensure it is not added twice
    /*compound->removeChildShape(shape_.get());

    if (IsEnabledEffective())
    {
        // Then add with updated offset
        Vector3 position = position_;
        // For terrains, undo the height centering performed automatically by Bullet
        if (shapeType_ == SHAPE_TERRAIN && geometry_)
        {
            auto* heightfield = static_cast<HeightfieldData*>(geometry_.Get());
            position.y_ += (heightfield->minHeight_ + heightfield->maxHeight_) * 0.5f;
        }

        btTransform offset;
        offset.setOrigin(ToBtVector3(node_->GetWorldScale() * position));
        offset.setRotation(ToBtQuaternion(rotation_));
        compound->addChildShape(offset, shape_.get());
    }

    // Finally tell the rigid body to update its mass
    if (updateMass)
        rigidBody_->UpdateMass();*/
}

void JoltCollisionShape::OnSceneSet(Urho3D::Scene *previousScene, Urho3D::Scene *scene)
{
    URHO3D_LOGINFO("JoltCollisionShape::OnSceneSet()");
    if (scene)
    {
        NotifyRigidBody();
    }
}

void JoltCollisionShape::UpdateShape()
{
    URHO3D_LOGINFO("JoltCollisionShape::UpdateShape()");
    /*if (!physicsWorld_)
    {
        retryCreation_ = true;
        return;
    }*/
    ea::vector< JPH::Ref<JPH::Shape> > shapes;
    // TODO use std::visit instead of std::holds_alternative
    if (std::holds_alternative<ModelShapeData>(shapeData_))
    {
        ModelShapeData &modelShapeData = std::get<ModelShapeData>(shapeData_);
        if (modelShapeData.model_->GetNumGeometries())
        {
            if (modelShapeData.useConvexHull_)
                shapes = ModelToJoltConvexHulls(modelShapeData.model_, modelShapeData.lodLevel_);
            else
                shapes = ModelToJoltMeshes(modelShapeData.model_, modelShapeData.lodLevel_);
        }
    }
    else if (std::holds_alternative<CapsuleShapeData>(shapeData_))
    {
        CapsuleShapeData &capsuleShapeData = std::get<CapsuleShapeData>(shapeData_);
        // shapes.push_back(new JPH::CapsuleShape(capsuleShapeData.height_/2.0, capsuleShapeData.diameter_/2.0));
        shapes.push_back(new JPH::CapsuleShape(capsuleShapeData.height_/2.0*1.25, capsuleShapeData.diameter_/2.0*1.25)); // HACK make capsule bigger so we can see it outside of the graphical representation
    }
    else if (std::holds_alternative<SphereShapeData>(shapeData_))
    {
        SphereShapeData &sphereShapeData = std::get<SphereShapeData>(shapeData_);
        shapes.push_back(new JPH::SphereShape(sphereShapeData.diameter_/2.0));
        // shapes.push_back(new JPH::SphereShape(sphereShapeData.diameter_/2.0*1.25)); // HACK make sphere bigger so we can see it outside of the graphical representation
    }
    else
        return;
    // shapes.push_back(new JPH::BoxShape(JPH::Vec3(1.0, 1.0, 1.0))); // HACK for testing
    if (shapes.size() == 0)
    {
        URHO3D_LOGINFO("JoltCollisionShape::UpdateShape(): (as empty shape)");
        JPH::EmptyShapeSettings emptyShapeSettings;
        JPH::Shape::ShapeResult shapeResult = emptyShapeSettings.Create();
        if (shapeResult.HasError())
        {
            URHO3D_LOGWARNING("Failed to create a Jolt Physics JPH::EmptyShape due to error: {}", shapeResult.GetError());
            joltUnscaledShape_ = nullptr;
            joltScaledShape_ = nullptr;
        }
        else
        {
            JPH::Ref<JPH::Shape> emptyShape = shapeResult.Get();
            joltUnscaledShape_ = emptyShape;
            joltScaledShape_ = nullptr;
        }
    }
    else if (shapes.size() == 1)
    {
        URHO3D_LOGINFO("JoltCollisionShape::UpdateShape(): (as single shape)");
        joltUnscaledShape_ = shapes[0];
        joltScaledShape_ = nullptr;
    }
    else if (shapes.size() >= 2)
    {
        URHO3D_LOGINFO("JoltCollisionShape::UpdateShape(): (as compound shape)");
        JPH::StaticCompoundShapeSettings compoundSettings;
        for (std::size_t i = 0; i < shapes.size(); i++)
        {
            compoundSettings.AddShape(JPH::Vec3::sZero(), JPH::Quat::sIdentity(), shapes[i]);
        }
        JPH::Shape::ShapeResult shapeResult = compoundSettings.Create();
        if (shapeResult.HasError())
        {
            URHO3D_LOGWARNING("Failed to create a Jolt Physics JPH::StaticCompoundShape due to error: {}", shapeResult.GetError());
            joltUnscaledShape_ = nullptr;
            joltScaledShape_ = nullptr;
        }
        else
        {
            JPH::Ref<JPH::Shape> compoundShape = shapeResult.Get();
            joltUnscaledShape_ = compoundShape;
            joltScaledShape_ = nullptr;
        }
    }

    // TODO monitor for changes to the model
    // SubscribeToEvent(model_, E_RELOADFINISHED, URHO3D_HANDLER(CollisionShape, HandleModelReloadFinished));
    recreateShape_ = false;
}
