#pragma once

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <array>

namespace hd {
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normals;
        glm::vec2 texCoord;
        glm::vec3 tangent;
        glm::vec3 bitangent;

        static const uint32_t numEntries = 5;

        static vk::VertexInputBindingDescription getBindingDescription() {
            vk::VertexInputBindingDescription bindingDescription = {};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = vk::VertexInputRate::eVertex;

            return bindingDescription;
        }

        static std::array<vk::VertexInputAttributeDescription, numEntries> getAttributeDescriptions() {
            std::array<vk::VertexInputAttributeDescription, numEntries> attributeDescriptions = {};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
            attributeDescriptions[1].offset = offsetof(Vertex, normals);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = vk::Format::eR32G32B32Sfloat;
            attributeDescriptions[3].offset = offsetof(Vertex, tangent);

            attributeDescriptions[4].binding = 0;
            attributeDescriptions[4].location = 4;
            attributeDescriptions[4].format = vk::Format::eR32G32B32Sfloat;
            attributeDescriptions[4].offset = offsetof(Vertex, bitangent);

            return attributeDescriptions;
        }

        bool operator==(const Vertex& other) const {
            return pos == other.pos 
                && normals == other.normals 
                && texCoord == other.texCoord
                && tangent == other.tangent
                && bitangent == other.bitangent;
        }
    };
}

namespace std {
    template<> struct hash<hd::Vertex> {
        size_t operator()(hd::Vertex const& vertex) const {
            constexpr auto shift = [](auto left, auto right) {
                return (hash<decltype(left)>()(left)) ^ (hash<decltype(right)>()(right) << 1);
            };

            constexpr auto shiftr = [](auto left, auto right) {
                return (left >> 1) ^ (hash<decltype(right)>()(right) << 1);
            };

            return shiftr(shiftr(shiftr(shift(vertex.pos, vertex.normals), vertex.texCoord), vertex.tangent), vertex.bitangent);
        }
    };
}
