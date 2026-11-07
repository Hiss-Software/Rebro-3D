#pragma once

#include <QString>
#include <QVector>

#include "SceneObject.h"

namespace MeshExporter {

// Export the scene to a file using the Assimp exporter.
// formatId: "obj", "gltf2", "glb2" (list of Assimp format hints).
// Object transforms are baked into the vertices.
bool exportToFile(const QVector<SceneObject>& objects,
                  const QString& path,
                  const QString& formatId,
                  QString* error = nullptr);

} // namespace MeshExporter