#include "MeshData.h"

#include <cmath>
#include <algorithm>

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

void MeshData::recomputeBounds() {
    boundsMin = glm::vec3(0.0f);
    boundsMax = glm::vec3(0.0f);
    if (vertices.empty()) return;
    boundsMin = boundsMax = vertices.front().pos;
    for (const Vertex& v : vertices) {
        boundsMin = glm::min(boundsMin, v.pos);
        boundsMax = glm::max(boundsMax, v.pos);
    }
}

std::shared_ptr<MeshData> makeCubeMesh() {
    auto mesh = std::make_shared<MeshData>();
    const glm::vec3 p[8] = {
        {-1,-1,-1},{ 1,-1,-1},{ 1, 1,-1},{-1, 1,-1},
        {-1,-1, 1},{ 1,-1, 1},{ 1, 1, 1},{-1, 1, 1}
    };
    const int faces[6][4] = {
        {4,5,6,7},{0,3,2,1},{1,2,6,5},{0,4,7,3},{3,7,6,2},{0,1,5,4}
    };
    const glm::vec2 uv[4] = {{0,0},{1,0},{1,1},{0,1}};
    for (int f = 0; f < 6; ++f) {
        glm::vec3 a = p[faces[f][0]], b = p[faces[f][1]], c = p[faces[f][2]];
        glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
        for (int i = 0; i < 4; ++i)
            mesh->vertices.push_back({p[faces[f][i]], n, uv[i]});
    }
    for (int f = 0; f < 6; ++f) {
        unsigned base = unsigned(f * 4);
        mesh->indices.insert(mesh->indices.end(),
            {base, base + 1, base + 2, base, base + 2, base + 3});
    }
    mesh->recomputeBounds();
    return mesh;
}

std::shared_ptr<MeshData> makePlaneMesh(float size) {
    auto mesh = std::make_shared<MeshData>();
    float s = size * 0.5f;
    glm::vec3 n(0.0f, 1.0f, 0.0f);
    mesh->vertices = {
        {{-s, 0, -s}, n, {0, 0}}, {{ s, 0, -s}, n, {1, 0}},
        {{ s, 0,  s}, n, {1, 1}}, {{-s, 0,  s}, n, {0, 1}}
    };
    mesh->indices = {0, 1, 2, 0, 2, 3};
    mesh->recomputeBounds();
    return mesh;
}

std::shared_ptr<MeshData> makeSphereMesh(float radius, int stacks, int slices) {
    auto mesh = std::make_shared<MeshData>();
    for (int j = 0; j <= stacks; ++j) {
        float v = float(j) / float(stacks);
        float phi = v * kPi;
        for (int i = 0; i <= slices; ++i) {
            float u = float(i) / float(slices);
            float theta = u * 2.0f * kPi;
            glm::vec3 dir(std::sin(phi) * std::cos(theta),
                          std::cos(phi),
                          std::sin(phi) * std::sin(theta));
            mesh->vertices.push_back({dir * radius, dir, {u, 1.0f - v}});
        }
    }
    int row = slices + 1;
    for (int j = 0; j < stacks; ++j)
        for (int i = 0; i < slices; ++i) {
            unsigned a = unsigned(j * row + i);
            unsigned b = unsigned(j * row + i + 1);
            unsigned c = unsigned((j + 1) * row + i + 1);
            unsigned d = unsigned((j + 1) * row + i);
            mesh->indices.insert(mesh->indices.end(), {a, d, c, a, c, b});
        }
    mesh->recomputeBounds();
    return mesh;
}

std::shared_ptr<MeshData> makeCylinderMesh(float radius, float height, int segments) {
    auto mesh = std::make_shared<MeshData>();
    float h = height * 0.5f;

    // Side surface
    unsigned sideStart = unsigned(mesh->vertices.size());
    for (int i = 0; i <= segments; ++i) {
        float u = float(i) / float(segments);
        float a = u * 2.0f * kPi;
        float cx = std::cos(a), cz = std::sin(a);
        glm::vec3 n(cx, 0.0f, cz);
        mesh->vertices.push_back({{cx * radius,  h, cz * radius}, n, {u, 1}});
        mesh->vertices.push_back({{cx * radius, -h, cz * radius}, n, {u, 0}});
    }
    for (int i = 0; i < segments; ++i) {
        unsigned b0 = sideStart + unsigned(i * 2);
        unsigned t0 = b0 + 1;
        unsigned b1 = sideStart + unsigned((i + 1) * 2);
        unsigned t1 = b1 + 1;
        mesh->indices.insert(mesh->indices.end(), {b0, t0, t1, b0, t1, b1});
    }

    // Caps
    auto addCap = [&](float y, const glm::vec3& n, bool flip) {
        unsigned center = unsigned(mesh->vertices.size());
        mesh->vertices.push_back({{0, y, 0}, n, {0.5f, 0.5f}});
        unsigned first = unsigned(mesh->vertices.size());
        for (int i = 0; i <= segments; ++i) {
            float a = 2.0f * kPi * float(i) / float(segments);
            float cx = std::cos(a), cz = std::sin(a);
            mesh->vertices.push_back({{cx * radius, y, cz * radius}, n,
                                      {0.5f + 0.5f * cx, 0.5f + 0.5f * cz}});
        }
        for (int i = 0; i < segments; ++i) {
            if (flip)
                mesh->indices.insert(mesh->indices.end(),
                    {center, first + i + 1, first + i});
            else
                mesh->indices.insert(mesh->indices.end(),
                    {center, first + i, first + i + 1});
        }
    };
    addCap( h, glm::vec3(0,  1, 0), false);
    addCap(-h, glm::vec3(0, -1, 0), true);

    mesh->recomputeBounds();
    return mesh;
}

std::shared_ptr<MeshData> makeConeMesh(float radius, float height, int segments) {
    auto mesh = std::make_shared<MeshData>();
    float h = height * 0.5f;

    // Side: apex + base ring
    unsigned apexIdx = unsigned(mesh->vertices.size());
    mesh->vertices.push_back({{0, h, 0}, {0, 1, 0}, {0.5f, 1.0f}});
    unsigned ringStart = unsigned(mesh->vertices.size());
    for (int i = 0; i <= segments; ++i) {
        float u = float(i) / float(segments);
        float a = u * 2.0f * kPi;
        float cx = std::cos(a), cz = std::sin(a);
        glm::vec3 n = glm::normalize(glm::vec3(cx * h, radius, cz * h));
        mesh->vertices.push_back({{cx * radius, -h, cz * radius}, n, {u, 0}});
    }
    for (int i = 0; i < segments; ++i)
        mesh->indices.insert(mesh->indices.end(),
            {apexIdx, ringStart + i, ringStart + i + 1});

    // Base
    unsigned capCenter = unsigned(mesh->vertices.size());
    mesh->vertices.push_back({{0, -h, 0}, {0, -1, 0}, {0.5f, 0.5f}});
    unsigned capStart = unsigned(mesh->vertices.size());
    for (int i = 0; i <= segments; ++i) {
        float a = 2.0f * kPi * float(i) / float(segments);
        mesh->vertices.push_back({{std::cos(a) * radius, -h, std::sin(a) * radius},
                                  {0, -1, 0},
                                  {0.5f + 0.5f * std::cos(a), 0.5f + 0.5f * std::sin(a)}});
    }
    for (int i = 0; i < segments; ++i)
        mesh->indices.insert(mesh->indices.end(),
            {capCenter, capStart + i + 1, capStart + i});

    mesh->recomputeBounds();
    return mesh;
}

std::shared_ptr<MeshData> makeTorusMesh(float R, float r, int major, int minor) {
    auto mesh = std::make_shared<MeshData>();
    for (int j = 0; j <= minor; ++j) {
        float v = float(j) / float(minor);
        float phi = v * 2.0f * kPi;
        for (int i = 0; i <= major; ++i) {
            float u = float(i) / float(major);
            float theta = u * 2.0f * kPi;
            float cp = std::cos(phi), sp = std::sin(phi);
            float ct = std::cos(theta), st = std::sin(theta);
            glm::vec3 pos((R + r * cp) * ct, r * sp, (R + r * cp) * st);
            glm::vec3 n(cp * ct, sp, cp * st);
            mesh->vertices.push_back({pos, n, {u, v}});
        }
    }
    int row = major + 1;
    for (int j = 0; j < minor; ++j)
        for (int i = 0; i < major; ++i) {
            unsigned a = unsigned(j * row + i);
            unsigned b = unsigned(j * row + i + 1);
            unsigned c = unsigned((j + 1) * row + i + 1);
            unsigned d = unsigned((j + 1) * row + i);
            mesh->indices.insert(mesh->indices.end(), {a, d, c, a, c, b});
        }
    mesh->recomputeBounds();
    return mesh;
}

std::shared_ptr<MeshData> makePrimitiveMesh(const QString& shapeId) {
    if (shapeId == "plane")    return makePlaneMesh();
    if (shapeId == "sphere")   return makeSphereMesh();
    if (shapeId == "cylinder") return makeCylinderMesh();
    if (shapeId == "cone")     return makeConeMesh();
    if (shapeId == "torus")    return makeTorusMesh();
    return makeCubeMesh();
}