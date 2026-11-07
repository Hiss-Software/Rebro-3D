#pragma once

#include <QString>
#include <QVector>

#include "SceneObject.h"

// Reading and writing Rebro projects (.r3dp).
// Knows only about the object list — nothing about the viewport or Qt widgets.
namespace SceneDocument {

bool save(const QVector<SceneObject>& objects,
          const QString& path, QString* error = nullptr);

bool load(QVector<SceneObject>& outObjects,
          const QString& path, QString* error = nullptr);

} // namespace SceneDocument