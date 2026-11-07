#pragma once

#include <memory>
#include <vector>
#include <QString>

#include <glm/glm.hpp>

struct MeshData {
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    std::vector<Vertex> vertices;
    std::vector<unsigned> indices;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};

    void recomputeBounds();
};

std::shared_ptr<MeshData> makeCubeMesh();
std::shared_ptr<MeshData> makePlaneMesh(float size = 4.0f);
std::shared_ptr<MeshData> makeSphereMesh(float radius = 1.0f, int stacks = 24, int slices = 32);
std::shared_ptr<MeshData> makeCylinderMesh(float radius = 1.0f, float height = 2.0f, int segments = 32);
std::shared_ptr<MeshData> makeConeMesh(float radius = 1.0f, float height = 2.0f, int segments = 32);
std::shared_ptr<MeshData> makeTorusMesh(float R = 1.0f, float r = 0.3f, int major = 48, int minor = 24);

// By primitive ID; unknown ID falls back to a cube
std::shared_ptr<MeshData> makePrimitiveMesh(const QString& shapeId);