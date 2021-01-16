#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <external/vk_mem_alloc.h>

#include <iostream>
#include <vector>
#include <chrono>

#include <hdvw/window.hpp>
#include <hdvw/instance.hpp>
#include <hdvw/surface.hpp>
#include <hdvw/device.hpp>
#include <hdvw/allocator.hpp>
#include <hdvw/queue.hpp>
#include <hdvw/commandpool.hpp>
#include <hdvw/commandbuffer.hpp>
#include <hdvw/swapchain.hpp>
#include <hdvw/renderpass.hpp>
#include <hdvw/framebuffer.hpp>
#include <hdvw/shader.hpp>
#include <hdvw/pipelinelayout.hpp>
#include <hdvw/pipeline.hpp>
#include <hdvw/semaphore.hpp>
#include <hdvw/fence.hpp>
#include <hdvw/vertex.hpp>
#include <hdvw/databuffer.hpp>
#include <hdvw/texture.hpp>
#include <hdvw/descriptorlayout.hpp>
#include <hdvw/descriptorpool.hpp>
#include <hdvw/descriptorset.hpp>

#include <engine/blas.hpp>
#include <engine/tlas.hpp>
#include <engine/sbt.hpp>
#include <engine/model.hpp>
#include <engine/camera.hpp>

#define MAX_FRAMES_IN_FLIGHT 3

struct MVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct UniformData {
    glm::mat4 viewInverse;
    glm::mat4 projInverse;
    uint32_t frameIndex;
};

class App {
    private:
        bool framebufferResized = false;

        hd::Window window;
        hd::Instance instance;
        hd::Surface surface;
        hd::Device device;
        hd::Allocator allocator;
        hd::Queue graphicsQueue;
        hd::Queue presentQueue;
        hd::CommandPool graphicsPool;

        std::vector<hd::Semaphore> imageAvailable;
        std::vector<hd::Semaphore> renderFinished;
        std::vector<hd::Fence> inFlightFences;

        hd::Model scene;

        /* hd::Camera camera; */

        struct vram {
            using vram_vertices = hd::DataBuffer<hd::Vertex>;
            using vram_indices = hd::DataBuffer<uint32_t>;
            using vram_texture = hd::Texture;
            using vram_material = hd::DataBuffer<hd::Material>;

            std::vector<vram_vertices> vertices;
            std::vector<vram_indices> indices;
            std::vector<vram_texture> diffuse;
            std::vector<vram_material> materials;

            std::vector<hd::BLAS> blases;
            hd::TLAS tlas;
        } vram;

        hd::DescriptorLayout rayLayout;
        hd::PipelineLayout rayPipeLayout;
        hd::Pipeline rayPipeline;
        hd::SBT sbt;

        void init() {
            window = hd::Window_t::conjure({
                    .width = 1280,
                    .height = 720,
                    .title = "Ray traycing",
                    .cursorVisible = true,
                    .windowUser = this,
                    .framebufferSizeCallback = framebufferResizeCallback,
                    /* .cursorPosCallback = mousePositionCallback, */
                    });

            /* camera = hd::Camera_t::conjure({ */
            /*         .window = window, */
            /*         }); */

            auto instanceExtensions = window->getRequiredExtensions();
            instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

            instance = hd::Instance_t::conjure({
                    .applicationName = "Raytraycing",
                    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                    .engineName = "Hova's Engine",
                    .engineVersion = VK_MAKE_VERSION(2, 0, 0),
                    .apiVersion = VK_API_VERSION_1_2,
                    .validationLayers = { "VK_LAYER_KHRONOS_validation" },
                    .extensions = instanceExtensions,
                    });

            surface = hd::Surface_t::conjure({
                    .window = window,
                    .instance = instance,
                    });

            vk::PhysicalDeviceFeatures feats{};
            feats.samplerAnisotropy = true;

            /* vk::PhysicalDeviceVulkan12Features feats12{}; */
            /* feats12.bufferDeviceAddress = true; */
            /* feats12.runtimeDescriptorArray = true; */
            /* feats12.shaderSampledImageArrayNonUniformIndexing = true; */
            /* feats12.shaderStorageBufferArrayNonUniformIndexing = true; */

            vk::PhysicalDeviceDescriptorIndexingFeatures desc_feats{};
            desc_feats.runtimeDescriptorArray = true;
            desc_feats.shaderSampledImageArrayNonUniformIndexing = true;
            desc_feats.shaderStorageBufferArrayNonUniformIndexing = true;

            vk::PhysicalDeviceAccelerationStructureFeaturesKHR acc_feats{};
            acc_feats.accelerationStructure = true;
            acc_feats.pNext = &desc_feats;

            vk::PhysicalDeviceRayTracingPipelineFeaturesKHR ray_feats{};
            ray_feats.rayTracingPipeline = true;
            ray_feats.pNext = &acc_feats;

            vk::PhysicalDeviceBufferDeviceAddressFeatures buffer_feats{};
            buffer_feats.bufferDeviceAddress = true;
            buffer_feats.pNext = &ray_feats;

            vk::PhysicalDeviceFeatures2 all_feats{};
            all_feats.features = feats;
            all_feats.pNext = &buffer_feats;

            device = hd::Device_t::conjure({
                    .instance = instance,
                    .surface = surface,
                    .extensions = { 
                        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
                        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
                        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
                        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
                        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
                    },
                    .features = all_feats,
                    .validationLayers = { "VK_LAYER_KHRONOS_validation" },
                    });

            allocator = hd::Allocator_t::conjure({
                    .instance = instance,
                    .device = device,
                    });

            graphicsQueue = hd::Queue_t::conjure({
                    .device = device,
                    .type = hd::QueueType::eGraphics,
                    });

            presentQueue = hd::Queue_t::conjure({
                    .device = device,
                    .type = hd::QueueType::ePresent,
                    });

            graphicsPool = hd::CommandPool_t::conjure({
                    .device = device,
                    .family = hd::PoolFamily::eGraphics,
                    });

            imageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
            renderFinished.resize(MAX_FRAMES_IN_FLIGHT);
            inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

            for (uint32_t iter = 0; iter < MAX_FRAMES_IN_FLIGHT; iter++) {
                imageAvailable[iter] = hd::Semaphore_t::conjure({.device = device});
                renderFinished[iter] = hd::Semaphore_t::conjure({.device = device});
                inFlightFences[iter] = hd::Fence_t::conjure({.device = device});
            }

            scene = hd::Model_t::conjure({
                    "models/scene.obj",
                    [&](const char *filename){
                        return hd::Texture_t::conjure({
                                filename,
                                graphicsPool,
                                graphicsQueue,
                                allocator,
                                device,
                                }); },
                    });

            vram.vertices.reserve(scene->meshes.size());
            vram.indices.reserve(scene->meshes.size());
            vram.diffuse.reserve(scene->meshes.size());
            vram.blases.reserve(scene->meshes.size());

            std::vector<vk::AccelerationStructureInstanceKHR> instances;
            instances.reserve(scene->meshes.size());

            const std::array<std::array<float, 4>, 3> matrix {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
            };

            vk::TransformMatrixKHR vk_matrix(matrix);

            vk::AccelerationStructureInstanceKHR instanceInfo{};
            instanceInfo.transform = vk_matrix;
            instanceInfo.mask = 0xFF;
            instanceInfo.instanceShaderBindingTableRecordOffset = 0; // HitGroupId
            instanceInfo.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);

            for (uint32_t iter = 0; iter < scene->meshes.size(); iter++) {
                vram.vertices.push_back(hd::DataBuffer_t<hd::Vertex>::conjure({
                        .commandPool = graphicsPool,
                        .queue = graphicsQueue,
                        .allocator = allocator,
                        .device = device,
                        .data = scene->meshes[iter].vertices,
                        .usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
                        }));

                vram.indices.push_back(hd::DataBuffer_t<uint32_t>::conjure({
                        .commandPool = graphicsPool,
                        .queue = graphicsQueue,
                        .allocator = allocator,
                        .device = device,
                        .data = scene->meshes[iter].indices,
                        .usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
                        }));

                vram.diffuse.push_back(scene->meshes[iter].diffuse[0]);

                vram.materials.push_back(hd::DataBuffer_t<hd::Material>::conjure({
                        .commandPool = graphicsPool,
                        .queue = graphicsQueue,
                        .allocator = allocator,
                        .device = device,
                        .data = {scene->meshes[iter].material},
                        .usage = vk::BufferUsageFlagBits::eStorageBuffer,
                        }));

                vram.blases.push_back(hd::BLAS_t::conjure({
                        vram.vertices[iter],
                        vram.indices[iter],
                        graphicsPool,
                        graphicsQueue,
                        device,
                        allocator,
                        }));

                instanceInfo.instanceCustomIndex = iter; // InstanceId
                instanceInfo.accelerationStructureReference = vram.blases[iter]->address();
                instances.push_back(instanceInfo);
            }

            auto instbuffer = hd::DataBuffer_t<vk::AccelerationStructureInstanceKHR>::conjure({
                    .commandPool = graphicsPool,
                    .queue = graphicsQueue,
                    .allocator = allocator,
                    .device = device,
                    .data = instances,
                    .usage = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
                    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                    });

            vram.tlas = hd::TLAS_t::conjure({
                    instbuffer,
                    graphicsPool,
                    graphicsQueue,
                    device,
                    allocator,
                    });

            instbuffer.reset();

            vk::DescriptorSetLayoutBinding aStructLayoutBinding{};
            aStructLayoutBinding.binding = 0;
            aStructLayoutBinding.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
            aStructLayoutBinding.descriptorCount = 1;
            aStructLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR;

            vk::DescriptorSetLayoutBinding resImageLayoutBinding{};
            resImageLayoutBinding.binding = 1;
            resImageLayoutBinding.descriptorType = vk::DescriptorType::eStorageImage;
            resImageLayoutBinding.descriptorCount = 1;
            resImageLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

            vk::DescriptorSetLayoutBinding uniBufferBinding{};
            uniBufferBinding.binding = 2;
            uniBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
            uniBufferBinding.descriptorCount = 1;
            uniBufferBinding.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

            vk::DescriptorSetLayoutBinding textureBinding{};
            textureBinding.binding = 3;
            textureBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            textureBinding.descriptorCount = vram.diffuse.size();
            textureBinding.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

            vk::DescriptorSetLayoutBinding vertexBinding{};
            vertexBinding.binding = 4;
            vertexBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
            vertexBinding.descriptorCount = vram.vertices.size();
            vertexBinding.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

            vk::DescriptorSetLayoutBinding indicesBinding{};
            indicesBinding.binding = 5;
            indicesBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
            indicesBinding.descriptorCount = vram.indices.size();
            indicesBinding.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

            vk::DescriptorSetLayoutBinding materialBinding{};
            materialBinding.binding = 6;
            materialBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
            materialBinding.descriptorCount = vram.materials.size();
            materialBinding.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

            rayLayout = hd::DescriptorLayout_t::conjure({
                    .device = device,
                    .bindings = { aStructLayoutBinding, resImageLayoutBinding, uniBufferBinding, textureBinding, vertexBinding, indicesBinding, materialBinding },
                    });

            rayPipeLayout = hd::PipelineLayout_t::conjure({
                    .device = device,
                    .descriptorLayouts = { rayLayout->raw() },
                    });

            hd::Shader raygenShader = hd::Shader_t::conjure({
                    .device = device,
                    .filename = "shaders/raygen.rgen.spv",
                    .stage = vk::ShaderStageFlagBits::eRaygenKHR,
                    });

            hd::Shader missShader = hd::Shader_t::conjure({
                    .device = device,
                    .filename = "shaders/miss.rmiss.spv",
                    .stage = vk::ShaderStageFlagBits::eMissKHR,
                    });

            hd::Shader shadowShader = hd::Shader_t::conjure({
                    .device = device,
                    .filename = "shaders/shadow.rmiss.spv",
                    .stage = vk::ShaderStageFlagBits::eMissKHR,
                    });

            hd::Shader closestHitShader = hd::Shader_t::conjure({
                    .device = device,
                    .filename = "shaders/closesthit.rchit.spv",
                    .stage = vk::ShaderStageFlagBits::eClosestHitKHR,
                    });

            vk::RayTracingShaderGroupCreateInfoKHR raygenGroupCI{};
            raygenGroupCI.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
            raygenGroupCI.generalShader = 0;
            raygenGroupCI.closestHitShader = VK_SHADER_UNUSED_KHR;
            raygenGroupCI.anyHitShader = VK_SHADER_UNUSED_KHR;
            raygenGroupCI.intersectionShader = VK_SHADER_UNUSED_KHR;

            vk::RayTracingShaderGroupCreateInfoKHR missGroupCI{};
            missGroupCI.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
            missGroupCI.generalShader = 1;
            missGroupCI.closestHitShader = VK_SHADER_UNUSED_KHR;
            missGroupCI.anyHitShader = VK_SHADER_UNUSED_KHR;
            missGroupCI.intersectionShader = VK_SHADER_UNUSED_KHR;

            vk::RayTracingShaderGroupCreateInfoKHR shadowGroupCI{};
            shadowGroupCI.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
            shadowGroupCI.generalShader = 2;
            shadowGroupCI.closestHitShader = VK_SHADER_UNUSED_KHR;
            shadowGroupCI.anyHitShader = VK_SHADER_UNUSED_KHR;
            shadowGroupCI.intersectionShader = VK_SHADER_UNUSED_KHR;

            vk::RayTracingShaderGroupCreateInfoKHR closesthitGroupCI{};
            closesthitGroupCI.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
            closesthitGroupCI.generalShader = VK_SHADER_UNUSED_KHR;
            closesthitGroupCI.closestHitShader = 3;
            closesthitGroupCI.anyHitShader = VK_SHADER_UNUSED_KHR;
            closesthitGroupCI.intersectionShader = VK_SHADER_UNUSED_KHR;

            rayPipeline = hd::RaytraycingPipeline_t::conjure({
                    .pipelineLayout = rayPipeLayout,
                    .device = device,
                    .shaderInfos = { raygenShader->info(), missShader->info(), shadowShader->info(), closestHitShader->info() },
                    .shaderGroups = { raygenGroupCI, missGroupCI, shadowGroupCI, closesthitGroupCI },
                    });

            sbt = hd::SBT_t::conjure({
                    .pipeline = rayPipeline,
                    .device = device,
                    .allocator = allocator,
                    .raygenCount = 1,
                    .missCount = 2,
                    .hitCount = 1,
                    });
        }

        hd::SwapChain swapChain;
        struct storage {
            hd::Image image;
            hd::ImageView view;
        } storage;
        hd::DataBuffer<UniformData> unibuffer;
        hd::DescriptorPool rayDescriptorPool;
        hd::DescriptorSet rayDescriptorSet;
        std::vector<hd::CommandBuffer> rayCmdBuffers;
        std::vector<hd::Fence> inFlightImages;

        uint32_t globalFrameCount = 0;

        void setup() {
            swapChain = hd::SwapChain_t::conjure(hd::SwapChainCreateInfo{
                    .window = window,
                    .surface = surface,
                    .allocator = allocator,
                    .device = device,
                    .presentMode = vk::PresentModeKHR::eMailbox,
                    });

            storage.image = hd::Image_t::conjure({
                    .allocator = allocator,
                    .extent = swapChain->extent(),
                    .format = swapChain->format(),
                    .imageUsage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage,
                    .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                    });

            {
                auto cmd = graphicsPool->singleTimeBegin();
                cmd->transitionImageLayout({
                        .image = storage.image,
                        .layout = vk::ImageLayout::eGeneral,
                        });
                graphicsPool->singleTimeEnd(cmd, graphicsQueue);
            }

            storage.view = hd::ImageView_t::conjure({
                    .image = storage.image->raw(),
                    .device = device,
                    .format = storage.image->format(),
                    .range = storage.image->range(),
                    .type = vk::ImageViewType::e2D,
                    });

            const float aspect = static_cast<float>(swapChain->extent().width) / static_cast<float>(swapChain->extent().height);

            auto perspective = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 512.0f);
            auto translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, -3.5f));
            auto rotate = glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
            auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.15f, 0.15f, 0.15f));

            UniformData uniData {};
            uniData.projInverse = glm::inverse(perspective * glm::mat4(1.0f));
            uniData.viewInverse = glm::inverse(translate * scale * rotate * glm::mat4(1.0f));
            /* uniData.projInverse = camera->projI; */
            /* uniData.viewInverse = camera->viewI; */
            uniData.frameIndex = globalFrameCount;

            unibuffer = hd::DataBuffer_t<UniformData>::conjure({
                    .commandPool = graphicsPool,
                    .queue = graphicsQueue,
                    .allocator = allocator,
                    .device = device,
                    .data = {uniData},
                    .usage = vk::BufferUsageFlagBits::eUniformBuffer,
                    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                    });

            rayDescriptorPool = hd::DescriptorPool_t::conjure({
                    .device = device,
                    .layouts = {{rayLayout, 1}},
                    .instances = 1,
                   });

            rayDescriptorSet = rayDescriptorPool->allocate(1, rayLayout).at(0);

            {
                std::vector<vk::WriteDescriptorSet> writes;
                writes.reserve(3 + vram.vertices.size());

                auto topLevelAStruct = vram.tlas->raw();
                vk::WriteDescriptorSetAccelerationStructureKHR aStructDI{};
                aStructDI.setAccelerationStructures(topLevelAStruct);

                vk::WriteDescriptorSet aStuctWrite{};
                aStuctWrite.pNext = &aStructDI;
                aStuctWrite.dstSet = rayDescriptorSet->raw();
                aStuctWrite.dstBinding = 0;
                aStuctWrite.descriptorCount = 1;
                aStuctWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;

                writes.push_back(aStuctWrite);

                vk::DescriptorImageInfo storageImageInfo{};
                storageImageInfo.imageView = storage.view->raw();
                storageImageInfo.imageLayout = vk::ImageLayout::eGeneral;

                vk::WriteDescriptorSet storageImageWrite{};
                storageImageWrite.dstBinding = 1;
                storageImageWrite.dstArrayElement = 0;
                storageImageWrite.descriptorType = vk::DescriptorType::eStorageImage;
                storageImageWrite.descriptorCount = 1;
                storageImageWrite.dstSet = rayDescriptorSet->raw();
                storageImageWrite.setPImageInfo(&storageImageInfo);

                writes.push_back(storageImageWrite);

                vk::DescriptorBufferInfo unibufferInfo{};
                unibufferInfo.buffer = unibuffer->raw();
                unibufferInfo.offset = 0;
                unibufferInfo.range = sizeof(UniformData);

                vk::WriteDescriptorSet unibufferWrite = {};
                unibufferWrite.dstBinding = 2;
                unibufferWrite.dstArrayElement = 0;
                unibufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
                unibufferWrite.descriptorCount = 1;
                unibufferWrite.dstSet = rayDescriptorSet->raw();
                unibufferWrite.setPBufferInfo(&unibufferInfo);

                writes.push_back(unibufferWrite);

                std::vector<vk::DescriptorImageInfo> imageInfos;
                imageInfos.reserve(vram.diffuse.size());
                std::vector<vk::DescriptorBufferInfo> vertexInfos;
                vertexInfos.reserve(vram.vertices.size());
                std::vector<vk::DescriptorBufferInfo> indexInfos;
                indexInfos.reserve(vram.indices.size());
                std::vector<vk::DescriptorBufferInfo> materialInfos;
                materialInfos.reserve(vram.materials.size());

                for (uint32_t iter = 0; iter < vram.vertices.size(); iter++) {
                    vk::DescriptorImageInfo textureInfo{};
                    textureInfo.imageView = vram.diffuse[iter]->view();
                    textureInfo.sampler = vram.diffuse[iter]->sampler();
                    textureInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

                    imageInfos.push_back(textureInfo);

                    vk::WriteDescriptorSet textureWrite{};
                    textureWrite.dstBinding = 3;
                    textureWrite.dstArrayElement = iter;
                    textureWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
                    textureWrite.descriptorCount = 1;
                    textureWrite.dstSet = rayDescriptorSet->raw();
                    textureWrite.setPImageInfo(&imageInfos[iter]);

                    writes.push_back(textureWrite);

                    vk::DescriptorBufferInfo vertexBufferInfo{};
                    vertexBufferInfo.buffer = vram.vertices[iter]->raw();
                    vertexBufferInfo.offset = 0;
                    vertexBufferInfo.range = vram.vertices[iter]->size();

                    vertexInfos.push_back(vertexBufferInfo);

                    vk::WriteDescriptorSet vertexBufferWrite = {};
                    vertexBufferWrite.dstBinding = 4;
                    vertexBufferWrite.dstArrayElement = iter;
                    vertexBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
                    vertexBufferWrite.descriptorCount = 1;
                    vertexBufferWrite.dstSet = rayDescriptorSet->raw();
                    vertexBufferWrite.setPBufferInfo(&vertexInfos[iter]);

                    writes.push_back(vertexBufferWrite);

                    vk::DescriptorBufferInfo indexBufferInfo{};
                    indexBufferInfo.buffer = vram.indices[iter]->raw();
                    indexBufferInfo.offset = 0;
                    indexBufferInfo.range = vram.indices[iter]->size();

                    indexInfos.push_back(indexBufferInfo);

                    vk::WriteDescriptorSet indexBufferWrite = {};
                    indexBufferWrite.dstBinding = 5;
                    indexBufferWrite.dstArrayElement = iter;
                    indexBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
                    indexBufferWrite.descriptorCount = 1;
                    indexBufferWrite.dstSet = rayDescriptorSet->raw();
                    indexBufferWrite.setPBufferInfo(&indexInfos[iter]);

                    writes.push_back(indexBufferWrite);

                    vk::DescriptorBufferInfo materialBufferInfo{};
                    materialBufferInfo.buffer = vram.materials[iter]->raw();
                    materialBufferInfo.offset = 0;
                    materialBufferInfo.range = vram.materials[iter]->size();

                    materialInfos.push_back(materialBufferInfo);

                    vk::WriteDescriptorSet materialBufferWrite = {};
                    materialBufferWrite.dstBinding = 6;
                    materialBufferWrite.dstArrayElement = iter;
                    materialBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
                    materialBufferWrite.descriptorCount = 1;
                    materialBufferWrite.dstSet = rayDescriptorSet->raw();
                    materialBufferWrite.setPBufferInfo(&materialInfos[iter]);

                    writes.push_back(materialBufferWrite);
                }

                device->raw().updateDescriptorSets(writes, nullptr);
            }

            vk::ImageSubresourceRange sRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
            rayCmdBuffers = graphicsPool->allocate(swapChain->length());
            for (uint32_t i = 0; i < swapChain->length(); i++) {
                rayCmdBuffers[i]->begin();
                rayCmdBuffers[i]->raw().bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rayPipeline->raw());
                rayCmdBuffers[i]->raw().bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, rayPipeLayout->raw(), 0, rayDescriptorSet->raw(), nullptr);

                vk::StridedDeviceAddressRegionKHR raygenShaderSBTEntry{};
                raygenShaderSBTEntry.deviceAddress = sbt->raygen().address;
                raygenShaderSBTEntry.stride = device->_rayTracingProperties.shaderGroupHandleSize;
                raygenShaderSBTEntry.size = device->_rayTracingProperties.shaderGroupHandleSize * sbt->raygen().count;

                vk::StridedDeviceAddressRegionKHR missShaderSBTEntry{};
                missShaderSBTEntry.deviceAddress = sbt->miss().address;
                missShaderSBTEntry.stride = device->_rayTracingProperties.shaderGroupHandleSize;
                missShaderSBTEntry.size = device->_rayTracingProperties.shaderGroupHandleSize * sbt->miss().count;

                vk::StridedDeviceAddressRegionKHR hitShaderSBTEntry{};
                hitShaderSBTEntry.deviceAddress = sbt->hit().address;
                hitShaderSBTEntry.stride = device->_rayTracingProperties.shaderGroupHandleSize;
                hitShaderSBTEntry.size = device->_rayTracingProperties.shaderGroupHandleSize * sbt->hit().count;

                vk::StridedDeviceAddressRegionKHR callableShaderSBTEntry{};

                rayCmdBuffers[i]->raw().traceRaysKHR(
                        raygenShaderSBTEntry,
                        missShaderSBTEntry,
                        hitShaderSBTEntry,
                        callableShaderSBTEntry,
                        swapChain->extent().width,
                        swapChain->extent().height,
                        1
                        );

                rayCmdBuffers[i]->transitionImageLayout({
                        swapChain->colorAttachment(i)->raw(),
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eTransferDstOptimal,
                        sRange,
                        });

                rayCmdBuffers[i]->transitionImageLayout({
                        storage.image->raw(),
                        vk::ImageLayout::eGeneral,
                        vk::ImageLayout::eTransferSrcOptimal,
                        sRange,
                        });

                vk::ImageCopy copyRegion{};
                copyRegion.srcSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 };
                copyRegion.setSrcOffset({ 0, 0, 0 });
                copyRegion.dstSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 };
                copyRegion.setDstOffset({ 0, 0, 0 });
                copyRegion.setExtent({ swapChain->extent().width, swapChain->extent().height, 1 });

                rayCmdBuffers[i]->raw().copyImage(
                        storage.image->raw(), vk::ImageLayout::eTransferSrcOptimal, 
                        swapChain->colorAttachment(i)->raw(), vk::ImageLayout::eTransferDstOptimal, 
                        copyRegion
                        );

                rayCmdBuffers[i]->transitionImageLayout({
                        swapChain->colorAttachment(i)->raw(),
                        vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::ePresentSrcKHR,
                        sRange,
                        });

                rayCmdBuffers[i]->transitionImageLayout({
                        storage.image->raw(),
                        vk::ImageLayout::eTransferSrcOptimal,
                        vk::ImageLayout::eGeneral,
                        sRange,
                        });

                rayCmdBuffers[i]->end();
            }

            inFlightImages.resize(swapChain->length(), nullptr);
        }

        void cleanupRender() {
            device->waitIdle();

            rayCmdBuffers.clear();
            unibuffer.reset();
            storage.view.reset();
            storage.image.reset();
            swapChain.reset();

            device->updateSurfaceInfo();
        }

        void loop() {
            while (!window->shouldClose()) {
                window->pollEvents();
                update();
            }

            device->waitIdle();
        }

        void updateUnibuffer() {
            const float aspect = static_cast<float>(swapChain->extent().width) / static_cast<float>(swapChain->extent().height);

            static glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
            static const float cameraSpeed = 0.05f;

            if (glfwGetKey(window->raw(), GLFW_KEY_W) == GLFW_PRESS)
                cameraPos += glm::vec3(0.0f, 0.0f, 1.0f) * cameraSpeed;
            if (glfwGetKey(window->raw(), GLFW_KEY_S) == GLFW_PRESS)
                cameraPos += glm::vec3(0.0f, 0.0f, -1.0f) * cameraSpeed;
            if (glfwGetKey(window->raw(), GLFW_KEY_A) == GLFW_PRESS)
                cameraPos += glm::vec3(1.0f, 0.0f, 0.0f) * cameraSpeed;
            if (glfwGetKey(window->raw(), GLFW_KEY_D) == GLFW_PRESS)
                cameraPos += glm::vec3(-1.0f, 0.0f, 0.0f) * cameraSpeed;
            if (glfwGetKey(window->raw(), GLFW_KEY_SPACE) == GLFW_PRESS)
                cameraPos += glm::vec3(0.0f, 1.0f, 0.0f) * cameraSpeed;
            if (glfwGetKey(window->raw(), GLFW_KEY_BACKSPACE) == GLFW_PRESS)
                cameraPos += glm::vec3(0.0f, -1.0f, 0.0f) * cameraSpeed;

            auto perspective = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 512.0f);
            auto translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, -3.5f) + cameraPos);
            auto rotate = glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
            auto rotateZ = glm::rotate(glm::mat4(1.0f), glm::pi<float>() / 2, glm::vec3(0.0f, 1.0f, 0.0f));
            auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.15f, 0.15f, 0.15f));

            UniformData uniData {};
            uniData.projInverse = glm::inverse(perspective * glm::mat4(1.0f));
            uniData.viewInverse = glm::inverse(translate * scale * rotate * rotateZ * glm::mat4(1.0f));
            /* uniData.projInverse = camera->projI; */
            /* uniData.viewInverse = camera->viewI; */
            uniData.frameIndex = globalFrameCount;

            auto data = unibuffer->map();
            memcpy(data, &uniData, sizeof(UniformData));
            unibuffer->unmap();
        }

        uint32_t currentFrame = 0;
        void update() {
            inFlightFences[currentFrame]->wait();

            static auto startTime = std::chrono::high_resolution_clock::now();

            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

            startTime = currentTime;

            /* camera->processInput(); */
            /* camera->update(deltaTime); */
            updateUnibuffer();

            uint32_t imageIndex;
            {
                auto result = device->acquireNextImage(swapChain->raw(), imageAvailable[currentFrame]->raw());

                if (result.result == vk::Result::eErrorOutOfDateKHR) {
                    cleanupRender();
                    setup();
                } else if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR)
                    throw std::runtime_error("Failed to acquire the next image");
                imageIndex = result.value;
            }

            if (inFlightImages[imageIndex] != nullptr)
                inFlightImages[imageIndex]->wait();
            inFlightImages[imageIndex] = inFlightFences[currentFrame];

            {
                vk::Semaphore waitSemaphores[] = { imageAvailable[currentFrame]->raw() };
                vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
                vk::Semaphore signalSemaphores[] = { renderFinished[currentFrame]->raw() };

                auto raw = rayCmdBuffers[imageIndex]->raw();

                vk::SubmitInfo submitInfo = {};
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphores;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &raw;
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                inFlightFences[currentFrame]->reset();
                graphicsQueue->submit(submitInfo, inFlightFences[currentFrame]);
            }

            {
                vk::SwapchainKHR swapChains[] = { swapChain->raw() };

                vk::Semaphore waitSemaphores[] = { renderFinished[currentFrame]->raw() };

                vk::PresentInfoKHR presentInfo{};
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = waitSemaphores;
                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = swapChains;
                presentInfo.pImageIndices = &imageIndex;
                presentInfo.pResults = nullptr;

                vk::Result result = presentQueue->present(presentInfo);

                if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized) {
                    framebufferResized = false;
                    cleanupRender();
                    setup();
                } else if (result != vk::Result::eSuccess)
                    throw std::runtime_error("Failed to present the image to the swapChain");
            }

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            globalFrameCount++;
        }

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
            auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
            app->framebufferResized = true;
        }

        /* static void mousePositionCallback(GLFWwindow* window, double xpos, double ypos) { */
        /*     auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window)); */
        /*     app->camera->processMouse(xpos, ypos); */
        /* } */

    public:
        void run() {
            init();
            setup();
            loop();
        }
};
