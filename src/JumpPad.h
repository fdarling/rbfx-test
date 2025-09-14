#pragma once

// forward declarations
namespace Urho3D {

class Scene;
class Node;
class Vector3;

} // namespace Urho3D

class JumpPad
{
public:
    JumpPad(Urho3D::Scene *scene, const Urho3D::Vector3 &pos, const Urho3D::Vector3 &size);
    ~JumpPad();
protected:
    Urho3D::Node *node_;
};
