#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <hdvw/vertex.hpp>
#include <hdvw/texture.hpp>

#include <memory>
#include <string_view>
#include <iostream>
#include <functional>

namespace hd {
    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<Texture> diffuse;
    };

    struct ModelCreateInfo {
        std::string_view filename;
        std::function<Texture(const char*)> textureConjure;
    };

    class Model_t;
    typedef std::shared_ptr<Model_t> Model;

    class Model_t {
        private:
            std::function<Texture(const char*)> textureConjure;

            void processNode(aiNode *node, const aiScene *scene) {
                meshes.reserve(meshes.size() + node->mNumMeshes);
                for(uint32_t i = 0; i < node->mNumMeshes; i++)
                {
                    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                    meshes.push_back(processMesh(mesh, scene));
                }

                for(unsigned int i = 0; i < node->mNumChildren; i++)
                {
                    processNode(node->mChildren[i], scene);
                }
            }

            Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
                Mesh ret;

                ret.vertices.reserve(mesh->mNumVertices);
                for(uint32_t i = 0; i < mesh->mNumVertices; i++)
                {
                    Vertex vertex;

                    vertex.pos.x = mesh->mVertices[i].x;
                    vertex.pos.y = mesh->mVertices[i].y;
                    vertex.pos.z = mesh->mVertices[i].z;

                    if (mesh->HasNormals()) {
                        vertex.normals.x = mesh->mNormals[i].x;
                        vertex.normals.y = mesh->mNormals[i].y;
                        vertex.normals.z = mesh->mNormals[i].z;
                    } else vertex.normals = glm::vec3(0.0f, 0.0f, 0.0f);

                    if(mesh->mTextureCoords[0]) {
                        vertex.texCoord.x = mesh->mTextureCoords[0][i].x; 
                        vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
                    } else vertex.texCoord = glm::vec2(0.0f, 0.0f);

                    ret.vertices.push_back(vertex);
                }

                ret.indices.reserve(mesh->mNumFaces * mesh->mFaces[0].mNumIndices);
                for(uint32_t i = 0; i < mesh->mNumFaces; i++) {
                    aiFace face = mesh->mFaces[i];

                    for(uint32_t j = 0; j < face.mNumIndices; j++)
                        ret.indices.push_back(face.mIndices[j]);        
                }

                aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

                auto diffuseCount = material->GetTextureCount(aiTextureType_DIFFUSE);
                ret.diffuse.reserve(diffuseCount);
                for (uint32_t i = 0; i < diffuseCount; i++) {
                    aiString filename;
                    material->GetTexture(aiTextureType_DIFFUSE, i, &filename);

                    ret.diffuse.push_back(textureConjure(filename.C_Str()));
                }

                return ret;
            }

        public:
            std::vector<Mesh> meshes;

            static Model conjure(ModelCreateInfo ci) {
                return std::make_shared<Model_t>(ci);
            }

            Model_t(ModelCreateInfo ci) {
                Assimp::Importer importer;
                const aiScene* scene = importer.ReadFile(ci.filename.data(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

                if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
                {
                    std::cerr << "ASSIMP:: " << importer.GetErrorString() << std::endl;
                    return;
                }

                textureConjure = ci.textureConjure;
                processNode(scene->mRootNode, scene);
            }
    };
}
