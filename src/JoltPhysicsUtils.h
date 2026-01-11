#pragma once

#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/Color.h>
#include <Urho3D/Math/Vector3.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Color.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Mat44.h>
#include <Jolt/Geometry/AABox.h>

inline JPH::Vec3 ToJoltVec3(const Urho3D::Vector3 &inVec)
{
    return JPH::Vec3(inVec.x_, inVec.y_, inVec.z_);
}

inline JPH::Quat ToJoltQuat(const Urho3D::Quaternion &inQuat)
{
    return JPH::Quat(inQuat.x_, inQuat.y_, inQuat.z_, inQuat.w_);
}

inline Urho3D::Color ToColor(JPH::ColorArg inColor)
{
    static const float DIVISOR = 256.0;
    static const float ADDEND = 0.5/DIVISOR;
    return Urho3D::Color(static_cast<float>(inColor.r) / DIVISOR + ADDEND, static_cast<float>(inColor.g) / DIVISOR + ADDEND, static_cast<float>(inColor.b) / DIVISOR + ADDEND, static_cast<float>(inColor.a) / DIVISOR + ADDEND);
}

inline Urho3D::Vector3 ToVector3(const JPH::Vec3 &inVec)
{
    return Urho3D::Vector3(reinterpret_cast<const float*>(&inVec));
}

inline Urho3D::Matrix4 ToMatrix4(const JPH::Mat44 &inMat)
{
    float rawMat[16];
    inMat.StoreFloat4x4(reinterpret_cast<JPH::Float4*>(rawMat));
    Urho3D::Matrix4 mat(rawMat);
    mat = mat.Transpose();
    return mat;
}

inline Urho3D::Quaternion ToQuaternion(const JPH::Quat &inQuat)
{
    return Urho3D::Quaternion(inQuat.GetW(), inQuat.GetX(), inQuat.GetY(), inQuat.GetZ());
}

inline Urho3D::BoundingBox ToBoundingBox(const JPH::AABox &inBB)
{
    return Urho3D::BoundingBox(
        Urho3D::Vector3(inBB.mMin.GetX(), inBB.mMin.GetY(), inBB.mMin.GetZ()),
        Urho3D::Vector3(inBB.mMax.GetX(), inBB.mMax.GetY(), inBB.mMax.GetZ())
    );
}
