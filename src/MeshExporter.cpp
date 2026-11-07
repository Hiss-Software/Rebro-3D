#include "MeshExporter.h"

#include <assimp/scene.h>
#include <assimp/Exporter.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <algorithm> // for std::copy

namespace {

glm::mat4 modelMatrix(const SceneObject& obj) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), obj.position);
    m = glm::rotate(m, glm::radians(obj.rotation.x), glm::vec3(1,0,0));
    m = glm::rotate(m, glm::radians(obj.rotation.y), glm::vec3(0,1,0));
    m = glm::rotate(m, glm::radians(obj.rotation.z), glm::vec3(0,0,1));
    return glm::scale(m, obj.scale);
}

// Recalculate normals based on triangle geometry.
// This ignores stale normals in the mesh, ensuring clean exports.
// We pass by value to avoid mutating the editor's scene state.
void recalcNormals(MeshData mesh) {
    for (auto& v : mesh.vertices)
        v.normal = glm::vec3(0.0f);

    for (size_t f = 0; f + 2 < mesh.indices.size(); f += 3) {
        unsigned a = mesh.indices[f];
        unsigned b = mesh.indices[f + 1];
        unsigned c = mesh.indices[f + 2];
        if (a >= mesh.vertices.size() ||
            b >= mesh.vertices.size() ||
            c >= mesh.vertices.size())
            continue;

        const glm::vec3& p0 = mesh.vertices[a].pos;
        const glm::vec3& p1 = mesh.vertices[b].pos;
        const glm::vec3& p2 = mesh.vertices[c].pos;
        glm::vec3 n = glm::cross(p1 - p0, p2 - p0);
        mesh.vertices[a].normal += n;
        mesh.vertices[b].normal += n;
        mesh.vertices[c].normal += n;
    }

    for (auto& v : mesh.vertices) {
        float len = glm::length(v.normal);
        v.normal = (len > 1e-8f) ? v.normal / len : glm::vec3(0.0f, 1.0f, 0.0f);
    }
}

} // namespace

namespace MeshExporter {

bool exportToFile(const QVector<SceneObject>& objects,
                  const QString& path,
                  const QString& formatId,
                  QString* error)
{
    if (objects.isEmpty()) {
        if (error) *error = "Scene is empty — nothing to export";
        return false;
    }

    std::vector<aiVector3D> verts;
    std::vector<aiVector3D> normals;
    std::vector<aiVector3D> uvs;
    std::vector<unsigned> indexBuffer;
    bool hasUV = false;

    for (const SceneObject& o : objects) {
        if (!o.mesh) continue;

        // Create a copy to recalculate normals without altering the scene
        MeshData meshCopy = *o.mesh;
        recalcNormals(meshCopy);

        glm::mat4 m = modelMatrix(o);
        // Normal matrix: correct for non-uniform scaling
        glm::mat3 nrm = glm::mat3(glm::transpose(glm::inverse(m)));

        unsigned offset = unsigned(verts.size());
        for (const MeshData::Vertex& v : meshCopy.vertices) {
            glm::vec4 p = m * glm::vec4(v.pos, 1.0f);
            verts.push_back(aiVector3D(p.x, p.y, p.z));
            glm::vec3 n = glm::normalize(nrm * v.normal);
            normals.push_back(aiVector3D(n.x, n.y, n.z));
            uvs.push_back(aiVector3D(v.uv.x, v.uv.y, 0.0f));
            if (v.uv.x != 0.0f || v.uv.y != 0.0f) hasUV = true;
        }
        for (unsigned idx : meshCopy.indices)
            indexBuffer.push_back(offset + idx);
    }

    if (verts.empty()) {
        if (error) *error = "No geometry in scene to export";
        return false;
    }

    const unsigned numFaces = unsigned(indexBuffer.size() / 3);

    aiScene* scene = new aiScene();
    scene->mNumMeshes = 1;
    scene->mMeshes = new aiMesh*[1];
    aiMesh* mesh = new aiMesh();

    mesh->mNumVertices = unsigned(verts.size());
    mesh->mVertices = new aiVector3D[verts.size()];
    mesh->mNormals = new aiVector3D[normals.size()];
    std::copy(verts.begin(), verts.end(), mesh->mVertices);
    std::copy(normals.begin(), normals.end(), mesh->mNormals);

    if (hasUV) {
        mesh->mTextureCoords[0] = new aiVector3D[uvs.size()];
        std::copy(uvs.begin(), uvs.end(), mesh->mTextureCoords[0]);
        mesh->mNumUVComponents[0] = 2;
    }

    mesh->mNumFaces = numFaces;
    mesh->mFaces = new aiFace[numFaces];
    for (unsigned f = 0; f < numFaces; ++f) {
        aiFace& face = mesh->mFaces[f];
        face.mNumIndices = 3;
        face.mIndices = new unsigned[3];
        face.mIndices[0] = indexBuffer[f * 3 + 0];
        face.mIndices[1] = indexBuffer[f * 3 + 1];
        face.mIndices[2] = indexBuffer[f * 3 + 2];
    }
    mesh->mMaterialIndex = 0;
    mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
    scene->mMeshes[0] = mesh;

    scene->mNumMaterials = 1;
    scene->mMaterials = new aiMaterial*[1];
    scene->mMaterials[0] = new aiMaterial();
    aiColor3D diffuse(0.55f, 0.58f, 0.62f);
    scene->mMaterials[0]->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_DIFFUSE);

    scene->mRootNode = new aiNode();
    scene->mRootNode->mNumMeshes = 1;
    scene->mRootNode->mMeshes = new unsigned[1];
    scene->mRootNode->mMeshes[0] = 0;

    Assimp::Exporter exporter;
    aiReturn res = exporter.Export(scene, formatId.toStdString(),
                                   path.toStdString(), 0, 0);
    
    // Note: Assimp documentation suggests you can use aiReleaseImport or delete scene.
    // However, using Assimp::Exporter::Export, the scene is owned by the caller.
    delete scene;

    if (res != aiReturn_SUCCESS) {
        if (error)
            *error = QString("Export to %1 failed: %2")
                         .arg(path, QString::fromStdString(exporter.GetErrorString()));
        return false;
    }
    return true;
}

} // namespace MeshExporter