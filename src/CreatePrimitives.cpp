#include "CreatePrimitives.h"

#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Math/Sphere.h>

#include <cstdint>

using Urho3D::BoundingBox;
using Urho3D::Context;
using Urho3D::Cos;
using Urho3D::Geometry;
using Urho3D::IndexBuffer;
using Urho3D::MASK_NORMAL;
using Urho3D::MASK_POSITION;
using Urho3D::Model;
using Urho3D::SharedPtr;
using Urho3D::Sin;
using Urho3D::Sphere;
using Urho3D::TRIANGLE_LIST;
using Urho3D::Vector3;
using Urho3D::VertexBuffer;

#ifndef USING_RBFX
namespace ea {

template <typename T>
class vector : public Urho3D::PODVector<T>
{
public:
    void push_back(const T &value) {Urho3D::PODVector<T>::Push(value);}
    // void push_back(const T &&value) {Urho3D::PODVector<T>::Push(value);}
    std::size_t size() const {return Urho3D::PODVector<T>::Size();}
    T * data() {return Urho3D::PODVector<T>::Buffer();}
    const T * data() const {return const_cast<const T*>(Urho3D::PODVector<T>::Buffer());}
};

} // namespace ea

#endif // USING_RBFX

Urho3D::SharedPtr<Model> CreateSphereModel(Urho3D::Context *context, float radius, int stacks, int slices)
{
    SharedPtr<Model> model(new Model(context));
    SharedPtr<VertexBuffer> vb(new VertexBuffer(context));
    SharedPtr<IndexBuffer> ib(new IndexBuffer(context));
    SharedPtr<Geometry> geom(new Geometry(context));

    // Enable CPU-side data for physics
    vb->SetShadowed(true);
    ib->SetShadowed(true);

    // Calculate vertices, normals, and texture coordinates
    ea::vector<Vector3> vertices;
    ea::vector<Vector3> normals;
    // PODVector<Vector2> texCoords;
    for (int stack = 0; stack <= stacks; ++stack)
    {
        const float phi = static_cast<float>(stack) * 180.0f / stacks;
        const float sinPhi = Sin(phi);
        const float cosPhi = Cos(phi);
        // const float v = static_cast<float>(stack) / stacks; // Texture V coordinate

        for (int slice = 0; slice <= slices; ++slice)
        {
            const float theta = static_cast<float>(slice) * 360.0f / slices;
            // const float u = static_cast<float>(slice) / slices; // Texture U coordinate
            Vector3 vertex = Vector3(sinPhi * Cos(theta), cosPhi, sinPhi * Sin(theta)) * radius;
            vertices.push_back(vertex);
            normals.push_back(vertex.Normalized());
            // texCoords.push_back(Vector2(u, 1.0f - v)); // Flip V for correct orientation
        }
    }

    // Vertex buffer (position, normal, texcoord)
    unsigned vertexCount = vertices.size();
    vb->SetSize(vertexCount, MASK_POSITION | MASK_NORMAL, false); // Static buffer
    ea::vector<float> vertexData;
    for (unsigned i = 0; i < vertexCount; ++i)
    {
        vertexData.push_back(vertices[i].x_);
        vertexData.push_back(vertices[i].y_);
        vertexData.push_back(vertices[i].z_);
        vertexData.push_back(normals[i].x_);
        vertexData.push_back(normals[i].y_);
        vertexData.push_back(normals[i].z_);
        //vertexData.push_back(texCoords[i].x_);
        //vertexData.push_back(texCoords[i].y_);
    }
#ifdef USING_RBFX
    vb->Update(vertexData.data());
#else // USING_RBFX
    vb->SetData(vertexData.data());
#endif // USING_RBFX

    // Index buffer (counterclockwise winding)
    ea::vector<uint16_t> indices;
    for (uint16_t stack = 0; stack < stacks; ++stack)
    {
        const uint16_t topRow = stack * (slices + 1);
        const uint16_t bottomRow = (stack + 1) * (slices + 1);

        for (uint16_t slice = 0; slice < slices; ++slice)
        {
            // First triangle (counterclockwise)
            indices.push_back(topRow + slice);
            indices.push_back(topRow + slice + 1);
            indices.push_back(bottomRow + slice);

            // Second triangle (counterclockwise)
            indices.push_back(topRow + slice + 1);
            indices.push_back(bottomRow + slice + 1);
            indices.push_back(bottomRow + slice);
        }
    }

    const unsigned indexCount = indices.size();
    ib->SetSize(indexCount, false, false); // 16-bit indices, static
#ifdef USING_RBFX
    ib->Update(indices.data());
#else // USING_RBFX
    ib->SetData(indices.data());
#endif // USING_RBFX

    geom->SetVertexBuffer(0, vb);
    geom->SetIndexBuffer(ib);
    geom->SetDrawRange(TRIANGLE_LIST, 0, indexCount);

    model->SetNumGeometries(1);
    model->SetGeometry(0, 0, geom);

    // Set bounding box
    BoundingBox bb;
    bb.Define(Sphere(Vector3::ZERO, radius));
    model->SetBoundingBox(bb);

    return model;
}
