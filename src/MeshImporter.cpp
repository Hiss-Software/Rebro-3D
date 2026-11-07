#include "MeshImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace MeshImporter {

bool loadFromFile(const QString& path,
                  std::shared_ptr<MeshData>& outMesh,
                  QString* error)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path.toStdString(),
        aiProcess_Triangulate
        | aiProcess_GenSmoothNormals
        | aiProcess_JoinIdenticalVertices);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE
        || !scene->mRootNode)
    {
        if (error)
            *error = QString("Failed to import %1: %2")
                         .arg(path, importer.GetErrorString());
        return false;
    }
    if (scene->mNumMeshes == 0) {
        if (error)
            *error = QString("File contains no geometry: %1").arg(path);
        return false;
    }

    auto mesh = std::make_shared<MeshData>();

    unsigned vertexOffset = 0;
    for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* src = scene->mMeshes[m];
        bool hasUV = src->HasTextureCoords(0);

        mesh->vertices.reserve(mesh->vertices.size() + src->mNumVertices);
        for (unsigned v = 0; v < src->mNumVertices; ++v) {
            glm::vec3 pos(src->mVertices[v].x,
                          src->mVertices[v].y,
                          src->mVertices[v].z);
            glm::vec3 normal(0.0f, 1.0f, 0.0f);
            if (src->HasNormals())
                normal = glm::normalize(glm::vec3(
                    src->mNormals[v].x, src->mNormals[v].y, src->mNormals[v].z));
            glm::vec2 uv(0.0f);
            if (hasUV)
                uv = glm::vec2(src->mTextureCoords[0][v].x,
                               src->mTextureCoords[0][v].y);
            mesh->vertices.push_back({pos, normal, uv});
        }

        for (unsigned f = 0; f < src->mNumFaces; ++f) {
            const aiFace& face = src->mFaces[f];
            for (unsigned i = 0; i < face.mNumIndices; ++i)
                mesh->indices.push_back(vertexOffset + face.mIndices[i]);
        }
        vertexOffset += src->mNumVertices;
    }

    mesh->recomputeBounds();
    outMesh = mesh;
    return true;
}

} // namespace MeshImporter