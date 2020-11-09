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

#define MAX_FRAMES_IN_FLIGHT 3

struct MVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct UniformData {
		glm::mat4 viewInverse;
		glm::mat4 projInverse;
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

        hd::Model cube;

        hd::DataBuffer<hd::Vertex> vertexBuffer;
        hd::DataBuffer<uint32_t> indexBuffer;

        hd::BLAS blas;
        hd::TLAS tlas;
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
                    });

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

            vk::PhysicalDeviceRayTracingFeaturesKHR ray_feats{};
            ray_feats.rayTracing = true;

            vk::PhysicalDeviceVulkan12Features feats12{};
            feats12.bufferDeviceAddress = true;
            feats12.pNext = &ray_feats;

            vk::PhysicalDeviceFeatures2 all_feats{};
            all_feats.features = feats;
            all_feats.pNext = &feats12;

            device = hd::Device_t::conjure({
                    .instance = instance,
                    .surface = surface,
                    .extensions = { 
                        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
                        VK_KHR_RAY_TRACING_EXTENSION_NAME, // BETA
                        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, // BETA
                        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME, // BETA
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

            cube = hd::Model_t::conjure({
                    "models/cube.obj",
                    [&](const char *filename){
                        return hd::Texture_t::conjure({
                                filename,
                                graphicsPool,
                                graphicsQueue,
                                allocator,
                                device,
                                }); },
                    });

            vertexBuffer = hd::DataBuffer_t<hd::Vertex>::conjure({
                    .commandPool = graphicsPool,
                    .queue = graphicsQueue,
                    .allocator = allocator,
                    .device = device,
                    .data = cube->meshes[0].vertices,
                    .usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
                    });

            indexBuffer = hd::DataBuffer_t<uint32_t>::conjure({
                    .commandPool = graphicsPool,
                    .queue = graphicsQueue,
                    .allocator = allocator,
                    .device = device,
                    .data = cube->meshes[0].indices,
                    .usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
                    });

            blas = hd::BLAS_t::conjure({
                    vertexBuffer,
                    indexBuffer,
                    graphicsPool,
                    graphicsQueue,
                    device,
                    allocator,
                    });

            const std::array<std::array<float, 4>, 3> matrix {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
            };

            vk::TransformMatrixKHR vk_matrix(matrix);

            vk::AccelerationStructureInstanceKHR inst{};
            inst.transform = vk_matrix;
            inst.instanceCustomIndex = 0;
            inst.mask = 0xFF;
            inst.instanceShaderBindingTableRecordOffset = 0;
            inst.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
            inst.accelerationStructureReference = blas->address();

            auto instbuffer = hd::DataBuffer_t<vk::AccelerationStructureInstanceKHR>::conjure({
                    .commandPool = graphicsPool,
                    .queue = graphicsQueue,
                    .allocator = allocator,
                    .device = device,
                    .data = {inst},
                    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                    });

            tlas = hd::TLAS_t::conjure({
                    instbuffer,
                    blas,
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
            aStructLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

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
            textureBinding.descriptorCount = 1;
            textureBinding.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

            vk::DescriptorSetLayoutBinding vertexBinding{};
            vertexBinding.binding = 4;
            vertexBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
            vertexBinding.descriptorCount = 1;
            vertexBinding.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

            vk::DescriptorSetLayoutBinding indicesBinding{};
            indicesBinding.binding = 5;
            indicesBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
            indicesBinding.descriptorCount = 1;
            indicesBinding.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

            rayLayout = hd::DescriptorLayout_t::conjure({
                    .device = device,
                    .bindings = { aStructLayoutBinding, resImageLayoutBinding, uniBufferBinding, textureBinding, vertexBinding, indicesBinding },
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

            vk::RayTracingShaderGroupCreateInfoKHR closesthitGroupCI{};
            closesthitGroupCI.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
            closesthitGroupCI.generalShader = VK_SHADER_UNUSED_KHR;
            closesthitGroupCI.closestHitShader = 2;
            closesthitGroupCI.anyHitShader = VK_SHADER_UNUSED_KHR;
            closesthitGroupCI.intersectionShader = VK_SHADER_UNUSED_KHR;

            rayPipeline = hd::RaytraycingPipeline_t::conjure({
                    .pipelineLayout = rayPipeLayout,
                    .device = device,
                    .shaderInfos = { raygenShader->info(), missShader->info(), closestHitShader->info() },
                    .shaderGroups = { raygenGroupCI, missGroupCI, closesthitGroupCI },
                    });

            sbt = hd::SBT_t::conjure({
                    .pipeline = rayPipeline,
                    .device = device,
                    .allocator = allocator,
                    .groupCount = 3,
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
            auto translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.5f));
            auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.15f, 0.15f, 0.15f));

            UniformData uniData {};
            uniData.projInverse = glm::inverse(perspective * glm::mat4(1.0f));
            uniData.viewInverse = glm::inverse(translate * scale * glm::mat4(1.0f));

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

            auto topLevelAStruct = tlas->raw();
            vk::WriteDescriptorSetAccelerationStructureKHR aStructDI{};
            aStructDI.setAccelerationStructures(topLevelAStruct);

            vk::WriteDescriptorSet aStuctWrite{};
            aStuctWrite.pNext = &aStructDI;
            aStuctWrite.dstSet = rayDescriptorSet->raw();
            aStuctWrite.dstBinding = 0;
            aStuctWrite.descriptorCount = 1;
            aStuctWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;

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

            vk::DescriptorImageInfo textureInfo{};
            textureInfo.imageView = cube->meshes[0].diffuse[0]->view();
            textureInfo.sampler = cube->meshes[0].diffuse[0]->sampler();
            textureInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

            vk::WriteDescriptorSet textureWrite{};
            textureWrite.dstBinding = 3;
            textureWrite.dstArrayElement = 0;
            textureWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            textureWrite.descriptorCount = 1;
            textureWrite.dstSet = rayDescriptorSet->raw();
            textureWrite.setPImageInfo(&textureInfo);

            vk::DescriptorBufferInfo vertexBufferInfo{};
            vertexBufferInfo.buffer = vertexBuffer->raw();
            vertexBufferInfo.offset = 0;
            vertexBufferInfo.range = vertexBuffer->size();

            vk::WriteDescriptorSet vertexBufferWrite = {};
            vertexBufferWrite.dstBinding = 4;
            vertexBufferWrite.dstArrayElement = 0;
            vertexBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
            vertexBufferWrite.descriptorCount = 1;
            vertexBufferWrite.dstSet = rayDescriptorSet->raw();
            vertexBufferWrite.setPBufferInfo(&vertexBufferInfo);

            vk::DescriptorBufferInfo indexBufferInfo{};
            indexBufferInfo.buffer = indexBuffer->raw();
            indexBufferInfo.offset = 0;
            indexBufferInfo.range = indexBuffer->size();

            vk::WriteDescriptorSet indexBufferWrite = {};
            indexBufferWrite.dstBinding = 5;
            indexBufferWrite.dstArrayElement = 0;
            indexBufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
            indexBufferWrite.descriptorCount = 1;
            indexBufferWrite.dstSet = rayDescriptorSet->raw();
            indexBufferWrite.setPBufferInfo(&indexBufferInfo);

            device->raw().updateDescriptorSets({aStuctWrite, storageImageWrite, unibufferWrite, textureWrite, vertexBufferWrite, indexBufferWrite}, nullptr);

            vk::ImageSubresourceRange sRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
            rayCmdBuffers = graphicsPool->allocate(swapChain->length());
            for (uint32_t i = 0; i < swapChain->length(); i++) {
                rayCmdBuffers[i]->begin();
                rayCmdBuffers[i]->raw().bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rayPipeline->raw());
                rayCmdBuffers[i]->raw().bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, rayPipeLayout->raw(), 0, rayDescriptorSet->raw(), nullptr);

                const vk::DeviceSize sbtSize = device->_rayTracingProperties.shaderGroupBaseAlignment * (vk::DeviceSize) sbt->size();

                vk::StridedBufferRegionKHR raygenShaderSBTEntry{};
                raygenShaderSBTEntry.buffer = sbt->raw();
                raygenShaderSBTEntry.offset = static_cast<vk::DeviceSize>(device->_rayTracingProperties.shaderGroupBaseAlignment * 0);
                raygenShaderSBTEntry.stride = device->_rayTracingProperties.shaderGroupBaseAlignment;
                raygenShaderSBTEntry.size = sbtSize;

                vk::StridedBufferRegionKHR missShaderSBTEntry{};
                missShaderSBTEntry.buffer = sbt->raw();
                missShaderSBTEntry.offset = static_cast<vk::DeviceSize>(device->_rayTracingProperties.shaderGroupBaseAlignment * 1);
                missShaderSBTEntry.stride = device->_rayTracingProperties.shaderGroupBaseAlignment;
                missShaderSBTEntry.size = sbtSize;

                vk::StridedBufferRegionKHR hitShaderSBTEntry{};
                hitShaderSBTEntry.buffer = sbt->raw();
                hitShaderSBTEntry.offset = static_cast<VkDeviceSize>(device->_rayTracingProperties.shaderGroupBaseAlignment * 2);
                hitShaderSBTEntry.stride = device->_rayTracingProperties.shaderGroupBaseAlignment;
                hitShaderSBTEntry.size = sbtSize;

                vk::StridedBufferRegionKHR callableShaderSBTEntry{};

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
            static auto startTime = std::chrono::high_resolution_clock::now();

            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

            const float aspect = static_cast<float>(swapChain->extent().width) / static_cast<float>(swapChain->extent().height);

            auto perspective = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 512.0f);
            auto translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.5f));
            auto rotateY = glm::rotate(glm::mat4(1.0f), time * 2.5f, glm::vec3(0.0f, 1.0f, 0.0f));
            auto rotateX = glm::rotate(glm::mat4(1.0f), time * 2.5f, glm::vec3(1.0f, 0.0f, 0.0f));
            auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.15f, 0.15f, 0.15f));

            UniformData uniData {};
            uniData.projInverse = glm::inverse(perspective * glm::mat4(1.0f));
            uniData.viewInverse = glm::inverse(translate * rotateX * rotateY * scale * glm::mat4(1.0f));

            auto data = unibuffer->map();
            memcpy(data, &uniData, sizeof(UniformData));
            unibuffer->unmap();
        }

        uint32_t currentFrame = 0;
        void update() {
            inFlightFences[currentFrame]->wait();

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
        }

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
            auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
            app->framebufferResized = true;
        }


    public:
        void run() {
            init();
            setup();
            loop();
        }
};
