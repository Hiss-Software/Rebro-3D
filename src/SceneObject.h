#pragma once

#include <memory>
#include <QString>

#include <glm/glm.hpp>

#include "MeshData.h"

struct Material {
    glm::vec3 diffuse{0.55f, 0.58f, 0.62f};
    QString texturePath;   // empty — color only
};

struct SceneObject {
    QString name;
    QString meshPath;   // non-empty — imported model
    QString shapeId;    // "cube"/"plane"/"sphere"/"cylinder"/"cone"/"torus" if not a model
    bool meshEmbedded = false; // mesh was edited and is stored inside .r3dp
    std::shared_ptr<MeshData> mesh;
    Material material;
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};   // Euler, degrees
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
};