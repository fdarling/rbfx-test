#pragma once

#include <unordered_map>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>

// Urho3D forward declarations
namespace Urho3D {

class Scene;
class Node;
class Vector3;

} // namespace Urho3D

// custom forward declarations
class JoltRigidBody;

class Ladder
{
public:
    Ladder(Urho3D::Node *node);
    ~Ladder();

    Urho3D::Vector3 GetNormalForPoint(const Urho3D::Vector3 &pt) const;

    void ConstrainNode(Urho3D::Node *otherNode);
    void UnconstrainNode(Urho3D::Node *otherNode);

    Urho3D::Node * GetNode() {return node_;}
    const Urho3D::Node * GetNode() const {return node_;}
protected:
    typedef std::unordered_map<Urho3D::Node*, JPH::Ref<JPH::SixDOFConstraint>> ConstraintMap;
    Urho3D::Node *node_;
    JoltRigidBody *body_;
    ConstraintMap constrainedNodes_;
};
