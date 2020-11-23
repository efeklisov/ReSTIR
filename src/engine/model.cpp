#include <model.hpp>

namespace hd {
    void Model_t::processNode(aiNode *node, const aiScene *scene) {
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

    Mesh Model_t::processMesh(aiMesh *mesh, const aiScene *scene) {
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

            if (mesh->HasTextureCoords(0)) {
                vertex.texCoord.x = mesh->mTextureCoords[0][i].x; 
                vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
            } else vertex.texCoord = glm::vec2(0.0f, 0.0f);

            if (mesh->HasTangentsAndBitangents()) {
                vertex.tangent.x = mesh->mTangents[i].x;
                vertex.tangent.y = mesh->mTangents[i].y;
                vertex.tangent.z = mesh->mTangents[i].z;

                vertex.bitangent.x = mesh->mBitangents[i].x;
                vertex.bitangent.y = mesh->mBitangents[i].y;
                vertex.bitangent.z = mesh->mBitangents[i].z;
            } else {
                vertex.tangent = glm::vec3(0.0f, 0.0f, 0.0f);
                vertex.bitangent = glm::vec3(0.0f, 0.0f, 0.0f);
            }

            ret.vertices.push_back(vertex);
        }

        ret.indices.reserve(mesh->mNumFaces * mesh->mFaces[0].mNumIndices);
        for(uint32_t i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];

            for(uint32_t j = 0; j < face.mNumIndices; j++)
                ret.indices.push_back(face.mIndices[j]);        
        }

        ret.material = {};
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        aiColor3D ambient(0.0f, 0.0f, 0.0f);
        if (material->Get(AI_MATKEY_COLOR_AMBIENT, ambient) == AI_SUCCESS) {
            ret.material.ambient.x = ambient.r;
            ret.material.ambient.y = ambient.g;
            ret.material.ambient.z = ambient.b;
        } else ret.material.ambient = glm::vec3(0.0f, 0.0f, 0.0f);

        aiColor3D diffuse(0.0f, 0.0f, 0.0f);
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS) {
            ret.material.diffuse.x = diffuse.r;
            ret.material.diffuse.y = diffuse.g;
            ret.material.diffuse.z = diffuse.b;
        } else ret.material.diffuse = glm::vec3(0.0f, 0.0f, 0.0f);

        aiColor3D specular(0.0f, 0.0f, 0.0f);
        if (material->Get(AI_MATKEY_COLOR_SPECULAR, specular) == AI_SUCCESS) {
            ret.material.specular.x = specular.r;
            ret.material.specular.y = specular.g;
            ret.material.specular.z = specular.b;
        } else ret.material.specular = glm::vec3(0.0f, 0.0f, 0.0f);

        aiColor3D emissive(0.0f, 0.0f, 0.0f);
        if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS) {
            ret.material.emissive.x = emissive.r;
            ret.material.emissive.y = emissive.g;
            ret.material.emissive.z = emissive.b;
        } else ret.material.emissive = glm::vec3(0.0f, 0.0f, 0.0f);

        aiColor3D transparent(0.0f, 0.0f, 0.0f);
        if (material->Get(AI_MATKEY_COLOR_TRANSPARENT, transparent) == AI_SUCCESS) {
            ret.material.transparent.x = transparent.r;
            ret.material.transparent.y = transparent.g;
            ret.material.transparent.z = transparent.b;
        } else ret.material.transparent = glm::vec3(0.0f, 0.0f, 0.0f);

        if (material->Get(AI_MATKEY_SHININESS_STRENGTH, ret.material.shininess) != AI_SUCCESS)
            ret.material.shininess = 1.0f;

        if (material->Get(AI_MATKEY_REFRACTI, ret.material.iorefraction) != AI_SUCCESS)
            ret.material.iorefraction = 0.0f;

        if (material->Get(AI_MATKEY_OPACITY, ret.material.dissolve) != AI_SUCCESS)
            ret.material.dissolve = 0.0f;

        if (material->Get(AI_MATKEY_SHADING_MODEL, ret.material.shadingModel) != AI_SUCCESS)
            ret.material.shadingModel = 0;

        ret.material.diffuseMapCount = material->GetTextureCount(aiTextureType_DIFFUSE);
        ret.diffuse.reserve(ret.material.diffuseMapCount);
        for (uint32_t i = 0; i < ret.material.diffuseMapCount; i++) {
            aiString filename;
            material->GetTexture(aiTextureType_DIFFUSE, i, &filename);

            ret.diffuse.push_back(textureConjure(filename.C_Str()));
        }

        return ret;
    }

    Model_t::Model_t(ModelCreateInfo ci) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(ci.filename.data(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cerr << "ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }

        textureConjure = ci.textureConjure;
        processNode(scene->mRootNode, scene);
    }
}
