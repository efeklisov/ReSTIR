#include <model.hpp>

#include <fstream>

#include <external/rapidjson/document.h>

namespace hd {
    Mesh Model_t::processMesh(aiMesh *mesh, const aiScene *scene) {
        Mesh ret;

        ret.vertices.reserve(mesh->mNumVertices);
        for(uint32_t i = 0; i < mesh->mNumVertices; i++) {
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


    Model_t::Model_t(ModelCreateInfo const & ci) {
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

    std::vector<Light> Model_t::parseLights(std::string_view filename) {
        std::vector<Light> lights;

        std::ifstream file(filename.data(), std::ios_base::in);
        assert(!file.fail());

        std::stringstream fileStr;
        fileStr << file.rdbuf();
        file.close();

        rapidjson::Document sceneJSON;
        sceneJSON.Parse(fileStr.str().c_str());
        fileStr.clear();

        assert(sceneJSON.IsObject());

        const auto& lightsValue = sceneJSON["Lights"];
        assert(lightsValue.IsArray());

        lights.reserve(lightsValue.Size());
        for (const auto& lightInfo : lightsValue.GetArray()) {
            assert(lightInfo.IsObject());

            const auto& posInfo = lightInfo["pos"];
            assert(posInfo.IsArray());
            assert(posInfo.Size() == 3);

            glm::vec3 pos(0.0f);
            for (rapidjson::SizeType i = 0; i < posInfo.Size(); i++) {
                assert(posInfo[i].IsFloat());
                pos[i] = posInfo[i].GetFloat();
            }

            const auto& colorInfo = lightInfo["color"];
            assert(colorInfo.IsArray());
            assert(colorInfo.Size() == 3);

            glm::vec3 color(0.0f);
            for (rapidjson::SizeType i = 0; i < colorInfo.Size(); i++) {
                assert(colorInfo[i].IsFloat());
                color[i] = colorInfo[i].GetFloat();
            }

            const auto& intensityInfo = lightInfo["intensity"];
            assert(intensityInfo.IsFloat());

            float intensity = intensityInfo.GetFloat();

            const auto& dimsInfo = lightInfo["dims"];
            assert(dimsInfo.IsArray());
            assert(dimsInfo.Size() == 2);

            glm::vec2 dims(0.0f);
            for (rapidjson::SizeType i = 0; i < dimsInfo.Size(); i++) {
                assert(dimsInfo[i].IsFloat());
                dims[i] = dimsInfo[i].GetFloat();
            }

            const auto& rotateInfo = lightInfo["rotate"];
            assert(rotateInfo.IsArray());
            assert(rotateInfo.Size() == 3);

            glm::vec3 rotate(0.0f);
            for (rapidjson::SizeType i = 0; i < rotateInfo.Size(); i++) {
                assert(rotateInfo[i].IsFloat());
                rotate[i] = glm::radians(rotateInfo[i].GetFloat());
            }

            lights.push_back({
                    .pos = pos,
                    .color = color,
                    .intensity = intensity,
                    .dims = dims,
                    .rotate = rotate,
                    });
        }

        return lights;
    }

    LightPadInfo Model_t::generateLightPad(Light light) {
        auto translateAbs = glm::translate(glm::mat4(1.0f), light.pos);

        std::vector<glm::vec4> translateRel = {
            glm::vec4( light.dims[0] / 2, 0.0f,  light.dims[1] / 2, 1.0f),
            glm::vec4(-light.dims[0] / 2, 0.0f,  light.dims[1] / 2, 1.0f),
            glm::vec4( light.dims[0] / 2, 0.0f, -light.dims[1] / 2, 1.0f),
            glm::vec4(-light.dims[0] / 2, 0.0f, -light.dims[1] / 2, 1.0f),
        };

        auto rotate = glm::mat4_cast(glm::normalize(glm::quat(light.rotate)));

        std::vector<glm::vec3> generatedVertices(translateRel.size());
        for (uint32_t iter = 0; iter < translateRel.size(); iter++) {
            generatedVertices[iter] = translateAbs * rotate * translateRel[iter];
        }

        std::vector<glm::vec2> generatedTex = {
            glm::vec2(0.0, 0.0),
            glm::vec2(1.0, 0.0),
            glm::vec2(0.0, 1.0),
            glm::vec2(1.0, 1.0),
        };

        std::vector<uint32_t> generatedIndeces = {
            0, 1, 2,
            1, 2, 3,
            0, 2, 1,
            1, 3, 2,
        };

        glm::vec3 generatedNormal;
        {
            glm::vec3 vec1 = generatedVertices[1] - generatedVertices[0];
            glm::vec3 vec2 = generatedVertices[2] - generatedVertices[0];

            generatedNormal = glm::normalize(glm::cross(vec1, vec2));
        }

        LightPadInfo ret{};
        for (auto& elem: generatedIndeces) {
            Vertex vertex = {};

            vertex.pos = generatedVertices[elem];

            vertex.texCoord = generatedTex[elem];

            vertex.normals = generatedNormal;

            ret.vertices.push_back(vertex);
            ret.indices.push_back(ret.vertices.size() - 1);
        }

        ret.props.color = light.color;
        ret.props.intensity = light.intensity;
        ret.props.normal = generatedNormal;
        ret.props.a = generatedVertices[0];
        ret.props.ab = generatedVertices[1] - generatedVertices[0];
        ret.props.ac = generatedVertices[2] - generatedVertices[0];

        return ret;
    }
}
