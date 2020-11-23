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
    struct Material {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        glm::vec3 emissive;
        glm::vec3 transparent;
        float shininess;
        float iorefraction;
        float dissolve;
        int shadingModel;
        int diffuseMapCount;
    };

    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<Texture> diffuse;
        Material material = {};
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

            void processNode(aiNode *node, const aiScene *scene);

            Mesh processMesh(aiMesh *mesh, const aiScene *scene);

        public:
            std::vector<Mesh> meshes;

            static Model conjure(ModelCreateInfo ci) {
                return std::make_shared<Model_t>(ci);
            }

            Model_t(ModelCreateInfo ci);
    };
}
