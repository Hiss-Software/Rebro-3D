#include "MeshRay.h"

#include <cmath>

namespace MeshRay {

bool rayTriangle(const glm::vec3& o, const glm::vec3& d,
                 const glm::vec3& v0, const glm::vec3& v1,
                 const glm::vec3& v2, float& tOut)
{
    glm::vec3 e1 = v1 - v0;
    glm::vec3 e2 = v2 - v0;
    glm::vec3 p = glm::cross(d, e2);
    float det = glm::dot(e1, p);
    if (std::abs(det) < 1e-9f) return false;
    float inv = 1.0f / det;

    glm::vec3 tv = o - v0;
    float u = glm::dot(tv, p) * inv;
    if (u < 0.0f || u > 1.0f) return false;

    glm::vec3 q = glm::cross(tv, e1);
    float v = glm::dot(d, q) * inv;
    if (v < 0.0f || u + v > 1.0f) return false;

    float t = glm::dot(e2, q) * inv;
    if (t <= 0.0f) return false;
    tOut = t;
    return true;
}

bool rayMesh(const MeshData& mesh,
             const glm::vec3& origLocal, const glm::vec3& dirLocal, Hit& out)
{
    bool found = false;
    float best = 1e30f;
    const auto& idx = mesh.indices;

    for (size_t f = 0; f + 2 < idx.size(); f += 3) {
        unsigned i0 = idx[f], i1 = idx[f + 1], i2 = idx[f + 2];
        float t = 0.0f;
        if (!rayTriangle(origLocal, dirLocal,
                         mesh.vertices[i0].pos,
                         mesh.vertices[i1].pos,
                         mesh.vertices[i2].pos, t))
            continue;
        if (t > 0.0f && t < best) {
            best = t;
            found = true;
            out.t = t;
            out.tri[0] = i0;
            out.tri[1] = i1;
            out.tri[2] = i2;
        }
    }
    if (found)
        out.point = origLocal + dirLocal * out.t;
    return found;
}

} // namespace MeshRay
