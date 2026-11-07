#pragma once

#include <QString>
#include <QRegularExpression>
#include <glm/glm.hpp>

#include <cmath>

namespace MainWindowUtil {

constexpr char kFileFilter[] = "Rebro Projects (*.r3dp);;All Files (*)";
constexpr char kModelFilter[] =
    "Models (*.obj *.fbx *.glb *.gltf);;All Files (*)";
constexpr char kExportFilter[] =
    "Wavefront OBJ (*.obj);;glTF 2.0 (*.gltf);;GLB (*.glb)";
constexpr char kTextureFilter[] =
    "Images (*.png *.jpg *.jpeg *.bmp);;All Files (*)";

inline QString ensureExtension(QString path, const QString& ext) {
    if (!path.endsWith("." + ext, Qt::CaseInsensitive))
        path += "." + ext;
    return path;
}

inline bool parseHexColor(const QString& text, glm::vec3& out) {
    static const QRegularExpression re("^#?([0-9A-Fa-f]{6})$");
    auto m = re.match(text.trimmed());
    if (!m.hasMatch()) return false;
    QString hex = m.captured(1);
    bool r = hex.mid(0, 2).toInt(nullptr, 16);
    bool g = hex.mid(2, 2).toInt(nullptr, 16);
    bool b = hex.mid(4, 2).toInt(nullptr, 16);
    out = glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f);
    return true;
}

inline QString colorToHex(const glm::vec3& c) {
    return QString("#%1%2%3")
        .arg(int(std::round(c.r * 255.0f)), 2, 16, QChar('0'))
        .arg(int(std::round(c.g * 255.0f)), 2, 16, QChar('0'))
        .arg(int(std::round(c.b * 255.0f)), 2, 16, QChar('0')).toUpper();
}

} // namespace MainWindowUtil