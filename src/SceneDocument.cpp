#include "SceneDocument.h"
#include "MeshData.h"

#include <QFile>
#include <QTextStream>
#include <QStringList>

#include <limits>

namespace {

void writeFloatLine(QTextStream& out, const char* key,
                    float x, float y, float z)
{
    out << key << ' '
        << x << ' ' << y << ' ' << z << '\n';
}

bool parseFloat3(const QStringList& tok, int base,
                 float& x, float& y, float& z)
{
    if (tok.size() < base + 3) return false;
    bool okX, okY, okZ;
    x = tok[base + 0].toFloat(&okX);
    y = tok[base + 1].toFloat(&okY);
    z = tok[base + 2].toFloat(&okZ);
    return okX && okY && okZ;
}

// Recalculate mesh bounds from its vertices
void computeBounds(const std::shared_ptr<MeshData>& mesh) {
    if (!mesh || mesh->vertices.empty()) return;
    glm::vec3 bmin( std::numeric_limits<float>::max());
    glm::vec3 bmax(-std::numeric_limits<float>::max());
    for (const MeshData::Vertex& v : mesh->vertices) {
        bmin = glm::min(bmin, v.pos);
        bmax = glm::max(bmax, v.pos);
    }
    mesh->boundsMin = bmin;
    mesh->boundsMax = bmax;
}

void writeInlineMesh(QTextStream& out, const std::shared_ptr<MeshData>& mesh) {
    if (!mesh || mesh->vertices.empty()) return;
    out << "verts " << mesh->vertices.size() << '\n';
    for (const MeshData::Vertex& v : mesh->vertices) {
        out << "v "
            << v.pos.x << ' ' << v.pos.y << ' ' << v.pos.z << ' '
            << v.normal.x << ' ' << v.normal.y << ' ' << v.normal.z << ' '
            << v.uv.x << ' ' << v.uv.y << '\n';
    }
    out << "tris " << (mesh->indices.size() / 3) << '\n';
    for (size_t f = 0; f + 2 < mesh->indices.size(); f += 3)
        out << "i " << mesh->indices[f] << ' '
            << mesh->indices[f + 1] << ' '
            << mesh->indices[f + 2] << '\n';
}

} // namespace

namespace SceneDocument {

bool save(const QVector<SceneObject>& objects,
          const QString& path, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = QString("Failed to open file for writing: %1").arg(path);
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out.setRealNumberPrecision(10); // otherwise geometry gets rounded to 6 digits
    out << "R3DP 2\n";
    out << "count " << objects.size() << '\n';

    for (const SceneObject& o : objects) {
        out << "object " << o.name << '\n';
        if (!o.meshPath.isEmpty()) {
            out << "mesh " << o.meshPath << '\n';
            if (o.meshEmbedded) out << "embedded 1\n";
        } else {
            out << "shape " << o.shapeId << '\n';
        }
        writeFloatLine(out, "pos", o.position.x, o.position.y, o.position.z);
        writeFloatLine(out, "rot", o.rotation.x, o.rotation.y, o.rotation.z);
        writeFloatLine(out, "scale", o.scale.x, o.scale.y, o.scale.z);
        writeFloatLine(out, "color",
                       o.material.diffuse.r, o.material.diffuse.g, o.material.diffuse.b);
        if (!o.material.texturePath.isEmpty())
            out << "tex " << o.material.texturePath << '\n';

        // Write primitive geometry (always) and edited imports inline.
        // The link to the source stays on the line above — as a reference, geometry matters more.
        const bool inlineMesh = (o.meshPath.isEmpty() || o.meshEmbedded)
                                && o.mesh && !o.mesh->vertices.empty();
        if (inlineMesh)
            writeInlineMesh(out, o.mesh);
    }
    return true;
}

bool load(QVector<SceneObject>& outObjects,
          const QString& path, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error)
            *error = QString("Failed to open file: %1").arg(path);
        return false;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    QString header = in.readLine().trimmed();
    if (!header.startsWith("R3DP")) {
        if (error)
            *error = QString("Not a Rebro project: %1").arg(path);
        return false;
    }
    QStringList headerTok = header.split(' ');
    bool versionOk = false;
    int version = headerTok.value(1).toInt(&versionOk);
    if (!versionOk || version < 1) {
        if (error)
            *error = QString("Invalid format version: %1").arg(header);
        return false;
    }

    outObjects.clear();
    SceneObject current;
    bool haveCurrent = false;

    // Inline geometry reading state
    std::shared_ptr<MeshData> mesh;
    bool inVerts = false;
    int vertsLeft = 0;
    bool inTris = false;
    int trisLeft = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList tok = line.split(' ', Qt::SkipEmptyParts);
        const QString key = tok.value(0);

        if (inVerts && key == "v" && vertsLeft > 0) {
            MeshData::Vertex v;
            float u = 0.0f, w = 0.0f;
            if (tok.size() >= 9 &&
                parseFloat3(tok, 1, v.pos.x, v.pos.y, v.pos.z) &&
                parseFloat3(tok, 4, v.normal.x, v.normal.y, v.normal.z)) {
                u = tok[7].toFloat();
                w = tok[8].toFloat();
                v.uv = { u, w };
                mesh->vertices.push_back(v);
                --vertsLeft;
                if (vertsLeft == 0) inVerts = false;
            }
            continue;
        }
        if (inTris && key == "i" && trisLeft > 0) {
            if (tok.size() >= 4) {
                mesh->indices.push_back(unsigned(tok[1].toInt()));
                mesh->indices.push_back(unsigned(tok[2].toInt()));
                mesh->indices.push_back(unsigned(tok[3].toInt()));
                --trisLeft;
                if (trisLeft == 0) inTris = false;
            }
            continue;
        }

        if (key == "object") {
            if (haveCurrent) {
                computeBounds(current.mesh);
                outObjects.push_back(current);
            }
            current = SceneObject{};
            current.name = line.mid(line.indexOf(' ') + 1).trimmed();
            haveCurrent = true;
        } else if (key == "mesh") {
            current.meshPath = line.mid(line.indexOf(' ') + 1).trimmed();
        } else if (key == "embedded") {
            current.meshEmbedded = (tok.value(1) == "1");
        } else if (key == "shape") {
            current.shapeId = tok.value(1);
        } else if (key == "verts") {
            mesh = std::make_shared<MeshData>();
            current.mesh = mesh; // inline mesh takes priority over shapeId and meshPath
            vertsLeft = tok.value(1).toInt();
            inVerts = vertsLeft > 0;
        } else if (key == "tris") {
            trisLeft = tok.value(1).toInt();
            inTris = trisLeft > 0;
        } else if (key == "pos") {
            parseFloat3(tok, 1,
                        current.position.x, current.position.y, current.position.z);
        } else if (key == "rot") {
            parseFloat3(tok, 1,
                        current.rotation.x, current.rotation.y, current.rotation.z);
        } else if (key == "scale") {
            parseFloat3(tok, 1,
                        current.scale.x, current.scale.y, current.scale.z);
        } else if (key == "color") {
            parseFloat3(tok, 1,
                        current.material.diffuse.r,
                        current.material.diffuse.g,
                        current.material.diffuse.b);
        } else if (key == "tex") {
            current.material.texturePath = line.mid(line.indexOf(' ') + 1).trimmed();
        } else {
            // Skip unknown keys — forward compatibility with future versions
        }
    }
    if (haveCurrent) {
        computeBounds(current.mesh);
        outObjects.push_back(current);
    }

    return true;
}

} // namespace SceneDocument