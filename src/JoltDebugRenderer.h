#pragma once

#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Math/Matrix3x4.h>

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRenderer.h>

/// Debug rendering line.
struct JoltDebugLine
{
    /// Construct undefined.
    JoltDebugLine() = default;

    /// Construct with start and end positions and color.
    JoltDebugLine(const Urho3D::Vector3 &start, const Urho3D::Vector3 &end, unsigned color) :
        start_(start),
        end_(end),
        color_(color)
    {
    }

    /// Start position.
    Urho3D::Vector3 start_;
    /// End position.
    Urho3D::Vector3 end_;
    /// Color.
    unsigned color_{};
};

/// Debug render triangle.
struct JoltDebugTriangle
{
    /// Construct undefined.
    JoltDebugTriangle() = default;

    /// Construct with start and end positions and color.
    JoltDebugTriangle(const Urho3D::Vector3 &v1, const Urho3D::Vector3 &v2, const Urho3D::Vector3 &v3, unsigned color) :
        v1_(v1),
        v2_(v2),
        v3_(v3),
        color_(color)
    {
    }

    /// Vertex a.
    Urho3D::Vector3 v1_;
    /// Vertex b.
    Urho3D::Vector3 v2_;
    /// Vertex c.
    Urho3D::Vector3 v3_;
    /// Color.
    unsigned color_{};
};

struct JoltDebugRenderBatchEntry
{
    // SharedPtr<Material> material_;
    Urho3D::SharedPtr<Urho3D::Geometry> geometry_;
    typedef ea::unordered_map<uint32_t, ea::vector<Urho3D::Matrix3x4> > InstanceMap;
    InstanceMap instances_;
};

class JoltDebugRenderer : public Urho3D::Drawable, public JPH::DebugRenderer
{
    URHO3D_OBJECT(JoltDebugRenderer, Urho3D::Drawable);
    // JPH_OVERRIDE_NEW_DELETE
public:
    /// Construct.
    explicit JoltDebugRenderer(Urho3D::Context *context);
    /// Destruct.
    ~JoltDebugRenderer() override;

    /// Register object factory.
    static void RegisterObject(Urho3D::Context *context);

    /// Process octree raycast. May be called from a worker thread.
    void ProcessRayQuery(const Urho3D::RayOctreeQuery &query, ea::vector<Urho3D::RayQueryResult> &results) override;
    // void UpdateGeometry(const Urho3D::FrameInfo &frame) override;
    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    void UpdateBatches(const Urho3D::FrameInfo &frame) override;
    // Urho3D::UpdateGeometryType GetUpdateGeometryType() override {return Urho3D::UPDATE_MAIN_THREAD;}
    /// Return the geometry for a specific LOD level.
    Urho3D::Geometry * GetLodGeometry(unsigned batchIndex, unsigned level) override;
    /// Return number of occlusion geometry triangles.
    unsigned GetNumOccluderTriangles() override;
    /// Draw to occlusion buffer. Return true if did not run out of triangles.
    bool DrawOcclusion(Urho3D::OcclusionBuffer *buffer) override;

    void SetCameraPos(const Urho3D::Vector3 &pos) {cameraPos_ = pos;}

    /// Add a line.
    void AddLine(const Urho3D::Vector3 &start, const Urho3D::Vector3 &end, const Urho3D::Color &color);
    /// Add a line with color already converted to unsigned.
    void AddLine(const Urho3D::Vector3 &start, const Urho3D::Vector3 &end, unsigned color);
    /// Add a solid triangle.
    void AddTriangle(const Urho3D::Vector3 &v1, const Urho3D::Vector3 &v2, const Urho3D::Vector3 &v3, const Urho3D::Color &color);
    /// Add a solid triangle with color already converted to unsigned.
    void AddTriangle(const Urho3D::Vector3 &v1, const Urho3D::Vector3 &v2, const Urho3D::Vector3 &v3, unsigned color);

    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
    void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override;
    void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, JPH::ColorArg inColor, float inHeight) override;
    JPH::DebugRenderer::Batch CreateTriangleBatch(const JPH::DebugRenderer::Vertex *inVertices, int inVertexCount, const JPH::uint32 *inIndices, int inIndexCount) override;
    JPH::DebugRenderer::Batch CreateTriangleBatch(const JPH::DebugRenderer::Triangle *inTriangles, int inTriangleCount) override;
    void DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox &inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const JPH::DebugRenderer::GeometryRef &inGeometry, JPH::DebugRenderer::ECullMode inCullMode=JPH::DebugRenderer::ECullMode::CullBackFace, JPH::DebugRenderer::ECastShadow inCastShadow=JPH::DebugRenderer::ECastShadow::On, JPH::DebugRenderer::EDrawMode inDrawMode=JPH::DebugRenderer::EDrawMode::Solid) override;
protected:
    /// Recalculate the world-space bounding box.
    void OnWorldBoundingBoxUpdate() override;
private:
    void HandleEndFrame(Urho3D::StringHash eventType, Urho3D::VariantMap &eventData);
    /// Geometry entries.
    typedef ea::unordered_map<Urho3D::Geometry*, JoltDebugRenderBatchEntry> JoltBatchMap;
    typedef ea::unordered_map<uint32_t, Urho3D::SharedPtr<Urho3D::Material> > ColorMaterialMap;
    JoltBatchMap joltBatches_;
    ColorMaterialMap colorMap_;
    Urho3D::Vector3 cameraPos_;
    ea::vector<JoltDebugLine> lines_;
    ea::vector<JoltDebugTriangle> triangles_;
    Urho3D::SharedPtr<Urho3D::VertexBuffer> extraVertexBuffer_;
};
