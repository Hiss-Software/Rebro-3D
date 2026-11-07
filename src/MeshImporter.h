#pragma once

#include <memory>
#include <QString>

#include "MeshData.h"

namespace MeshImporter {

// Import a model (OBJ, FBX, glTF/GLB and anything Assimp supports).
bool loadFromFile(const QString& path,
                  std::shared_ptr<MeshData>& outMesh,
                  QString* error = nullptr);

} // namespace MeshImporter