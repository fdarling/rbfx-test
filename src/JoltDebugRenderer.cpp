#include "JoltDebugRenderer.h"
#include "CreateMaterial.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/OcclusionBuffer.h>
#include <Urho3D/Graphics/OctreeQuery.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Node.h>

// #define MASSIVE_LOGGING

// Cap the amount of lines to prevent crash when eg. debug rendering large heightfields
static const unsigned MAX_LINES = 1000000;
// Cap the amount of triangles to prevent crash.
static const unsigned MAX_TRIANGLES = 100000;

static Urho3D::Color JoltToUrhoColor(JPH::ColorArg inColor)
{
    static const float DIVISOR = 255.0;
    return Urho3D::Color(static_cast<float>(inColor.r) / DIVISOR, static_cast<float>(inColor.g) / DIVISOR, static_cast<float>(inColor.b) / DIVISOR, static_cast<float>(inColor.a) / DIVISOR);
}

// TODO consolidate this
static Urho3D::Vector3 JoltVec3ToUrhoVector3(const JPH::Vec3 &inVec)
{
    return Urho3D::Vector3(reinterpret_cast<const float*>(&inVec));
}

class JoltRenderBatch : public JPH::RefTargetVirtual
{
public:
    JPH_OVERRIDE_NEW_DELETE

    JoltRenderBatch(const JPH::DebugRenderer::Vertex *inVertices, int inVertexCount, const JPH::uint32 *inIndices, int inIndexCount, Urho3D::Context *context);
    JoltRenderBatch(const JPH::DebugRenderer::Triangle *inTriangles, int inTriangleCount, Urho3D::Context *context);
    ~JoltRenderBatch() = default;
    void AddRef()
    {
        _refCount++;
    }
    void Release()
    {
        _refCount--;
        if (_refCount == 0)
            delete this;
    }
    int _refCount;
    Urho3D::SharedPtr<Urho3D::Geometry> geometry_;
};

static const int JOLT_VERTICES_PER_TRIANGLE = 3;
static const ea::vector<Urho3D::VertexElement> JOLT_VERTEX_ELEMENTS =
{
    Urho3D::VertexElement{Urho3D::TYPE_VECTOR3, Urho3D::SEM_POSITION},
    Urho3D::VertexElement{Urho3D::TYPE_VECTOR3, Urho3D::SEM_NORMAL},
    Urho3D::VertexElement{Urho3D::TYPE_VECTOR2, Urho3D::SEM_TEXCOORD},
    Urho3D::VertexElement{Urho3D::TYPE_UBYTE4_NORM, Urho3D::SEM_COLOR},
};

JoltRenderBatch::JoltRenderBatch(const JPH::DebugRenderer::Vertex *inVertices, int inVertexCount, const JPH::uint32 *inIndices, int inIndexCount, Urho3D::Context *context) : _refCount(0)
{
#ifdef MASSIVE_LOGGING
    URHO3D_LOGINFO("JoltRenderBatch::JoltRenderBatch() (vertex / indices version)");
#endif // MASSIVE_LOGGING
    // set up VertexBuffer
    Urho3D::SharedPtr<Urho3D::VertexBuffer> vertexBuffer = Urho3D::MakeShared<Urho3D::VertexBuffer>(context);
    vertexBuffer->SetShadowed(true);
    vertexBuffer->SetSize(inVertexCount, Urho3D::MASK_POSITION | Urho3D::MASK_NORMAL, false);
    // vertexBuffer->SetSize(inVertexCount, JOLT_VERTEX_ELEMENTS, false);
    ea::vector<Urho3D::Vector3> verts;
    verts.reserve(inVertexCount);
    for (int i = 0; i < inVertexCount; i++)
    {
        verts.emplace_back(reinterpret_cast<const float *>(&inVertices[i].mPosition)); // TODO JoltVec3ToUrhoVector3
        verts.emplace_back(reinterpret_cast<const float *>(&inVertices[i].mNormal)); // TODO JoltVec3ToUrhoVector3
    }
#ifdef USING_RBFX
    vertexBuffer->Update(verts.data());
    // vertexBuffer->Update(inVertices);
#else // U3D
    vertexBuffer->SetData(verts.data());
    // vertexBuffer->SetData(inVertices);
#endif

    // set up optional IndexBuffer
    Urho3D::SharedPtr<Urho3D::IndexBuffer> indexBuffer = Urho3D::MakeShared<Urho3D::IndexBuffer>(context);
    indexBuffer->SetShadowed(true);
    if (inIndices)
    {
        indexBuffer->SetSize(inIndexCount, true); // true for 32-bit indices, false for 16-bit indices?
#ifdef USING_RBFX
        indexBuffer->Update(inIndices);
#else // U3D
        indexBuffer->SetData(inIndices);
#endif
    }
    else
    {
        indexBuffer->SetSize(inVertexCount, true); // true for 32-bit indices, false for 16-bit indices?
        ea::vector<uint32_t> generatedIndices;
        generatedIndices.reserve(inVertexCount);
        for (int i = 0; i < inVertexCount; i++)
            generatedIndices.emplace_back(i);
#ifdef USING_RBFX
        indexBuffer->Update(generatedIndices.data());
#else // U3D
        indexBuffer->SetData(generatedIndices.data());
#endif
    }

    // set up Geometry
    geometry_ = Urho3D::MakeShared<Urho3D::Geometry>(context);
    geometry_->SetVertexBuffer(0, vertexBuffer);
    geometry_->SetIndexBuffer(indexBuffer);
    geometry_->SetDrawRange(Urho3D::TRIANGLE_LIST, 0, inIndices ? inIndexCount : inVertexCount);
}

JoltRenderBatch::JoltRenderBatch(const JPH::DebugRenderer::Triangle *inTriangles, int inTriangleCount, Urho3D::Context *context) : _refCount(0)
{
#ifdef MASSIVE_LOGGING
    URHO3D_LOGINFO("JoltRenderBatch::JoltRenderBatch() (Triangle version)");
#endif // MASSIVE_LOGGING
    // flatten triangles to vertices
    // TODO: eliminate duplicates?
    const int vertex_count = inTriangleCount * JOLT_VERTICES_PER_TRIANGLE;
    ea::vector<Urho3D::Vector3> verts;
    verts.reserve(vertex_count);
    for (int i = 0; i < inTriangleCount; ++i)
    {
        for (int j = 0; j < JOLT_VERTICES_PER_TRIANGLE; j++)
        {
            verts.emplace_back(reinterpret_cast<const float *>(&inTriangles[i].mV[j].mPosition)); // TODO JoltVec3ToUrhoVector3
            verts.emplace_back(reinterpret_cast<const float *>(&inTriangles[i].mV[j].mNormal)); // TODO JoltVec3ToUrhoVector3
        }
    }

    // set up VertexBuffer
    Urho3D::SharedPtr<Urho3D::VertexBuffer> vertexBuffer = Urho3D::MakeShared<Urho3D::VertexBuffer>(context);
    vertexBuffer->SetShadowed(true);
    vertexBuffer->SetSize(vertex_count, Urho3D::MASK_POSITION | Urho3D::MASK_NORMAL, false);
    // vertexBuffer->SetSize(vertex_count, JOLT_VERTEX_ELEMENTS, false);
#ifdef USING_RBFX
    vertexBuffer->Update(verts.data());
    // vertexBuffer->Update(inTriangles);
#else // U3D
    vertexBuffer->SetData(verts.data());
    // vertexBuffer->SetData(inTriangles);
#endif

    // generate IndexBuffer
    Urho3D::SharedPtr<Urho3D::IndexBuffer> indexBuffer = Urho3D::MakeShared<Urho3D::IndexBuffer>(context);
    indexBuffer->SetSize(vertex_count, true); // true for 32-bit indices, false for 16-bit indices?
    indexBuffer->SetShadowed(true);
    {
        ea::vector<uint32_t> generatedIndices;
        generatedIndices.reserve(vertex_count);
        for (int i = 0; i < vertex_count; i++)
            generatedIndices.emplace_back(i);
#ifdef USING_RBFX
        indexBuffer->Update(generatedIndices.data());
#else // U3D
        indexBuffer->SetData(generatedIndices.data());
#endif
    }

    // set up Geometry
    geometry_ = Urho3D::MakeShared<Urho3D::Geometry>(context);
    geometry_->SetVertexBuffer(0, vertexBuffer);
    geometry_->SetIndexBuffer(indexBuffer);
    geometry_->SetDrawRange(Urho3D::TRIANGLE_LIST, 0, vertex_count);
}

JoltDebugRenderer::JoltDebugRenderer(Urho3D::Context* context)
    : Urho3D::Drawable(context, Urho3D::DRAWABLE_GEOMETRY)
{
    extraVertexBuffer_ = Urho3D::MakeShared<Urho3D::VertexBuffer>(context);
    extraVertexBuffer_->SetDebugName("JoltDebugRenderer");
    // URHO3D_LOGINFO("JoltDebugRenderer constructor called");
    SetEnabled(true);
    Initialize(); // we are required to call this! Jolt will crash if you don't
    SubscribeToEvent(Urho3D::E_ENDFRAME, URHO3D_HANDLER(JoltDebugRenderer, HandleEndFrame));
}

JoltDebugRenderer::~JoltDebugRenderer() = default;

void JoltDebugRenderer::RegisterObject(Urho3D::Context* context)
{
    context->AddFactoryReflection<JoltDebugRenderer>(Urho3D::Category_Geometry);

    // URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
}

void JoltDebugRenderer::ProcessRayQuery(const Urho3D::RayOctreeQuery &query, ea::vector<Urho3D::RayQueryResult> &results)
{
    // For simplicity, use AABB raycast. For more accurate raycasting, would need to check each geometry.
    Urho3D::Drawable::ProcessRayQuery(query, results);
}

void JoltDebugRenderer::UpdateBatches(const Urho3D::FrameInfo &frame)
{
    URHO3D_PROFILE("JoltDebugUpdate");
#ifdef MASSIVE_LOGGING
    URHO3D_LOGINFO("JoltDebugRenderer::UpdateBatches called!");
#endif // MASSIVE_LOGGING

    const Urho3D::BoundingBox &worldBoundingBox = GetWorldBoundingBox();
    distance_ = frame.camera_->GetDistance(worldBoundingBox.Center());

    // TODO deal with lines_ and triangles_!

    // count total geometry/color combinations
    unsigned totalBatches = 0;
    for (const JoltBatchMap::value_type &entry : joltBatches_)
    {
        totalBatches += entry.second.instances_.size(); // number of colors per geometry
    }

    // resize batches
    batches_.clear();
    batches_.resize(totalBatches);
    // batches_.reserve(totalBatches);
#ifdef MASSIVE_LOGGING
    URHO3D_LOGINFO("totalBatches: {}", (int)totalBatches);
#endif // MASSIVE_LOGGING

    // URHO3D_LOGINFO("JoltDebugRenderer::UpdateBatches: batches_.size() = {}", batches_.size());
    unsigned batchIndex = 0;
    for (const JoltBatchMap::value_type &entry : joltBatches_)
    {
        for (const JoltDebugRenderBatchEntry::InstanceMap::value_type &instanceInfo : entry.second.instances_)
        {
            batches_[batchIndex].geometry_ = entry.second.geometry_;
            batches_[batchIndex].material_ = colorMap_[instanceInfo.first];
            batches_[batchIndex].distance_ = distance_;
            batches_[batchIndex].worldTransform_ = instanceInfo.second.data();
            batches_[batchIndex].numWorldTransforms_ = instanceInfo.second.size();
            batchIndex++;
        }
    }
}

Urho3D::Geometry * JoltDebugRenderer::GetLodGeometry(unsigned batchIndex, unsigned level)
{
    // URHO3D_LOGINFO("JoltDebugRenderer::GetLodGeometry called!");
    // TODO are we supposed to be accessing batches_ here? Or the precursor to it?
    if (batchIndex >= batches_.size())
        return nullptr;
    return batches_[batchIndex].geometry_;
}

unsigned JoltDebugRenderer::GetNumOccluderTriangles()
{
    return 0;
}

bool JoltDebugRenderer::DrawOcclusion(Urho3D::OcclusionBuffer *buffer)
{
    return true;
}

void JoltDebugRenderer::AddLine(const Urho3D::Vector3 &start, const Urho3D::Vector3 &end, const Urho3D::Color &color)
{
    AddLine(start, end, color.ToUInt());
}

void JoltDebugRenderer::AddLine(const Urho3D::Vector3 &start, const Urho3D::Vector3 &end, unsigned color)
{
    if (lines_.size() >= MAX_LINES)
        return;
    lines_.push_back(JoltDebugLine(start, end, color));
}

void JoltDebugRenderer::AddTriangle(const Urho3D::Vector3 &v1, const Urho3D::Vector3 &v2, const Urho3D::Vector3 &v3, const Urho3D::Color &color)
{
    AddTriangle(v1, v2, v3, color.ToUInt());
}

void JoltDebugRenderer::AddTriangle(const Urho3D::Vector3 &v1, const Urho3D::Vector3 &v2, const Urho3D::Vector3 &v3, unsigned color)
{
    if (triangles_.size() >= MAX_TRIANGLES)
        return;

    triangles_.push_back(JoltDebugTriangle(v1, v2, v3, color));
}

void JoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
#ifdef MASSIVE_LOGGING
    URHO3D_LOGINFO("JoltDebugRenderer::DrawLine()");
#endif // MASSIVE_LOGGING
    const Urho3D::Vector3 start(reinterpret_cast<const float*>(&inFrom)); // TODO JoltVec3ToUrhoVector3
    const Urho3D::Vector3 end(reinterpret_cast<const float*>(&inTo)); // TODO JoltVec3ToUrhoVector3
    const Urho3D::Color color = JoltToUrhoColor(inColor);
    AddLine(start, end, color.ToUInt());
}

void JoltDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow)
{
#ifdef MASSIVE_LOGGING
    URHO3D_LOGINFO("JoltDebugRenderer::DrawTriangle()");
#endif // MASSIVE_LOGGING
    /*glColor4ub(inColor.r, inColor.g, inColor.b, inColor.a);
    glBegin(GL_TRIANGLES);
    glVertex3d(inV1.GetX(), inV1.GetY(), inV1.GetZ());
    glVertex3d(inV2.GetX(), inV2.GetY(), inV2.GetZ());
    glVertex3d(inV3.GetX(), inV3.GetY(), inV3.GetZ());
    glEnd();*/
}

void JoltDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight)
{
#ifdef MASSIVE_LOGGING
    URHO3D_LOGINFO("JoltDebugRenderer::DrawText3D()");
#endif // MASSIVE_LOGGING
}

JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(const JPH::DebugRenderer::Vertex *inVertices, int inVertexCount, const JPH::uint32 *inIndices, int inIndexCount)
{
    return new JoltRenderBatch(inVertices, inVertexCount, inIndices, inIndexCount, context_);
}

JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(const JPH::DebugRenderer::Triangle *inTriangles, int inTriangleCount)
{
    return new JoltRenderBatch(inTriangles, inTriangleCount, context_);
}

void JoltDebugRenderer::DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox &inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const JPH::DebugRenderer::GeometryRef &inGeometry, JPH::DebugRenderer::ECullMode inCullMode, JPH::DebugRenderer::ECastShadow inCastShadow, JPH::DebugRenderer::EDrawMode inDrawMode)
{
#ifdef MASSIVE_LOGGING
    URHO3D_LOGINFO("JoltDebugRenderer::DrawGeometry()");
#endif // MASSIVE_LOGGING
    // get the matrix data for later conversion to Urho3D::Matrix3x4
    // TODO consolidate this code
    float rawMat[16];
    inModelMatrix.StoreFloat4x4(reinterpret_cast<JPH::Float4*>(rawMat));
    Urho3D::Matrix4 mat(rawMat);
    mat = mat.Transpose();

    // get the relevant vertex buffer object
    const JPH::Vec3 cameraPos(*reinterpret_cast<const JPH::Float3*>(&cameraPos_));
    const JPH::DebugRenderer::LOD &lod = inGeometry->GetLOD(cameraPos, inWorldSpaceBounds, inLODScaleSq);
    JoltRenderBatch * const batch = reinterpret_cast<JoltRenderBatch*>(lod.mTriangleBatch.GetPtr());

    // create Urho3D color
    const Urho3D::Color color = JoltToUrhoColor(inModelColor);

    // convert Urho3D color to uint32_t
    const uint32_t colorKey = color.ToUInt();

    // ensure there is material for this color
    Urho3D::SharedPtr<Urho3D::Material> &colorMaterial = colorMap_[colorKey];
    if (!colorMaterial)
        colorMaterial = CreateWireframeMaterial(context_, color);

    // locate the appropriate batch
    JoltDebugRenderBatchEntry &entry = joltBatches_[batch->geometry_];

    // we need to set the geometry member (at least the first time)
    entry.geometry_ = batch->geometry_;

    // locate the per-color instances and append a new instance
    JoltDebugRenderBatchEntry::InstanceMap::mapped_type &instances = entry.instances_[colorKey];
    instances.emplace_back(mat);
}

void JoltDebugRenderer::OnWorldBoundingBoxUpdate()
{
#ifdef MASSIVE_LOGGING
    // URHO3D_LOGINFO("JoltDebugRenderer::OnWorldBoundingBoxUpdate()");
#endif // MASSIVE_LOGGING
    worldBoundingBox_ = Urho3D::BoundingBox(std::numeric_limits<float>::min(), std::numeric_limits<float>::max());
}

void JoltDebugRenderer::HandleEndFrame(Urho3D::StringHash eventType, Urho3D::VariantMap &eventData)
{
#ifdef MASSIVE_LOGGING
    URHO3D_LOGINFO("JoltDebugRenderer::HandleEndFrame()");
#endif // MASSIVE_LOGGING
    joltBatches_.clear();
    lines_.clear();
    triangles_.clear();
    batches_.clear();
    OnMarkedDirty(node_);
}
