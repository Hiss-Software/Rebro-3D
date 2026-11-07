#pragma once

#include <glm/glm.hpp>

#include "MeshData.h"

namespace MeshRay {

struct Hit {
    float t = 0.0f;
    unsigned tri[3] = {0, 0, 0};
    glm::vec3 point{0.0f};   // in mesh local coordinates
};

// Ray-triangle intersection (Möller–Trumbore). t > 0 — front hits only
bool rayTriangle(const glm::vec3& o, const glm::vec3& d,
                 const glm::vec3& v0, const glm::vec3& v1,
                 const glm::vec3& v2, float& tOut);

// Ray in mesh local coordinates: iterate triangles, return the closest hit
bool rayMesh(const MeshData& mesh,
             const glm::vec3& origLocal, const glm::vec3& dirLocal, Hit& out);

} // namespace MeshRay