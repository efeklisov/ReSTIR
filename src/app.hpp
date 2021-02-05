#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <external/vk_mem_alloc.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <thread>

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
#include <engine/saveimg.hpp>

#define MAX_FRAMES_IN_FLIGHT 3

struct params_t {
    uint32_t N = 4;
    std::string method = "mis_orig";
    bool pseudoOffline = false;
    uint32_t frames = 6;
    bool capture = false;
};

struct MVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct UniformData {
    glm::mat4 viewInverse;
    glm::mat4 projInverse;
    uint32_t frameIndex;
    uint32_t N;
};

struct UniSizes {
    uint32_t meshesSize;
    uint32_t lightsSize;
};

struct PushWindowSize {
    uint32_t width;
    uint32_t height;
};

struct UniCount {
    uint32_t count;
};

class App {
    private:
        params_t params;

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

        struct vram {
            using vram_vertices = hd::DataBuffer<hd::Vertex>;
            using vram_indices  = hd::DataBuffer<uint32_t>;
            using vram_texture  = hd::Texture;
            using vram_material = hd::DataBuffer<hd::Material>;

            std::vector<vram_vertices> vertices;
            std::vector<vram_indices>  indices;
            std::vector<vram_texture>  diffuse;
            std::vector<vram_material> materials;
            hd::DataBuffer<hd::VRAM_Light> lights;
            hd::DataBuffer<UniSizes> uniSizes;

            std::vector<hd::BLAS> blases;
            hd::TLAS tlas;

            struct storage {
                hd::Image image;
                hd::ImageView view;

                hd::Image summ;
                hd::ImageView summView;
            } storage;

            hd::DataBuffer<UniformData> unibuffer;
            hd::DataBuffer<UniCount> uniCount;
        } vram;

        struct ram {
            hd::Image saveImage;
            hd::ImageView saveView;
        } ram;

        hd::DescriptorLayout compLayout;
        hd::PipelineLayout compPipeLayout;
        hd::Pipeline summPipeline;

        hd::DescriptorLayout rayLayout;
        hd::PipelineLayout rayPipeLayout;
        hd::Pipeline rayPipeline;
        hd::SBT sbt;

        inline auto populateInitialVRAM(hd::Model scene, std::vector<hd::Light>& lights) {
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
                vram.vertices.push_back(hd::conjure<hd::Vertex>({
                        .commandPool = graphicsPool,
                        .queue = graphicsQueue,
                        .allocator = allocator,
                        .device = device,
                        .data = scene->meshes[iter].vertices,
                        .usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
                        }));

                vram.indices.push_back(hd::conjure<uint32_t>({
                        .commandPool = graphicsPool,
                        .queue = graphicsQueue,
                        .allocator = allocator,
                        .device = device,
                        .data = scene->meshes[iter].indices,
                        .usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
                        }));

                vram.diffuse.push_back(scene->meshes[iter].diffuse[0]);

                vram.materials.push_back(hd::conjure<hd::Material>({
                        .commandPool = graphicsPool,
                        .queue = graphicsQueue,
                        .allocator = allocator,
                        .device = device,
                        .data = {scene->meshes[iter].material},
                        .usage = vk::BufferUsageFlagBits::eStorageBuffer,
                        }));

                vram.blases.push_back(hd::conjure({
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

            std::vector<hd::VRAM_Light> vram_lights;
            vram_lights.reserve(lights.size());

            for (uint32_t iter = 0; iter < lights.size(); iter++){
                auto lightPad = hd::Model_t::generateLightPad(lights[iter]);
                vram_lights.push_back(lightPad.props);

                auto vram_vertices = hd::conjure<hd::Vertex>({
                        .commandPool = graphicsPool,
                        .queue = graphicsQueue,
                        .allocator = allocator,
                        .device = device,
                        .data = lightPad.vertices,
                        .usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
                        });

                auto vram_indices = hd::conjure<uint32_t>({
                        .commandPool = graphicsPool,
                        .queue = graphicsQueue,
                        .allocator = allocator,
                        .device = device,
                        .data = lightPad.indices,
                        .usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
                        });

                vram.blases.push_back(hd::conjure({
                        vram_vertices,
                        vram_indices,
                        graphicsPool,
                        graphicsQueue,
                        device,
                        allocator,
                        }));

                instanceInfo.instanceCustomIndex = scene->meshes.size() + iter; // InstanceId
                instanceInfo.accelerationStructureReference = vram.blases[vram.blases.size() - 1]->address();
                instances.push_back(instanceInfo);
            }

            vram.lights = hd::conjure<hd::VRAM_Light>({
                    .commandPool = graphicsPool,
                    .queue = graphicsQueue,
                    .allocator = allocator,
                    .device = device,
                    .data = vram_lights,
                    .usage = vk::BufferUsageFlagBits::eStorageBuffer,
                    });

            auto instbuffer = hd::conjure<vk::AccelerationStructureInstanceKHR>({
                    .commandPool = graphicsPool,
                    .queue = graphicsQueue,
                    .allocator = allocator,
                    .device = device,
                    .data = instances,
                    .usage = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
                    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                    });

            vram.tlas = hd::conjure({
                    instbuffer,
                    graphicsPool,
                    graphicsQueue,
                    device,
                    allocator,
                    });

            instbuffer.reset();

            UniSizes uniSizes{};
            uniSizes.meshesSize = scene->meshes.size();
            uniSizes.lightsSize = lights.size();

            vram.uniSizes = hd::conjure<UniSizes>({
                    .commandPool = graphicsPool,
                    .queue = graphicsQueue,
                    .allocator = allocator,
                    .device = device,
                    .data = {uniSizes},
                    .usage = vk::BufferUsageFlagBits::eUniformBuffer,
                    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                    });

        }

        constexpr auto selectFeatures() {
            struct features {
                vk::PhysicalDeviceFeatures feats{};
                vk::PhysicalDeviceDescriptorIndexingFeatures desc_feats{};
                vk::PhysicalDeviceAccelerationStructureFeaturesKHR acc_feats{};
                vk::PhysicalDeviceRayTracingPipelineFeaturesKHR ray_feats{};
                vk::PhysicalDeviceBufferDeviceAddressFeatures buffer_feats{};
                vk::PhysicalDeviceFeatures2 feats2{};

                auto res() {
                    return feats2;
                }
            } features;

            features.feats.samplerAnisotropy = true;

            features.desc_feats.runtimeDescriptorArray = true;
            features.desc_feats.shaderSampledImageArrayNonUniformIndexing = true;
            features.desc_feats.shaderStorageBufferArrayNonUniformIndexing = true;

            features.acc_feats.accelerationStructure = true;
            features.acc_feats.pNext = &features.desc_feats;

            features.ray_feats.rayTracingPipeline = true;
            features.ray_feats.pNext = &features.acc_feats;

            features.buffer_feats.bufferDeviceAddress = true;
            features.buffer_feats.pNext = &features.ray_feats;

            features.feats2.features = features.feats;
            features.feats2.pNext = &features.buffer_feats;

            return features;
        }

        constexpr auto populateCompLayout() {
            vk::DescriptorSetLayoutBinding resImageLayoutBinding{};
            resImageLayoutBinding.binding = 0;
            resImageLayoutBinding.descriptorType = vk::DescriptorType::eStorageImage;
            resImageLayoutBinding.descriptorCount = 1;
            resImageLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            vk::DescriptorSetLayoutBinding saveImageLayoutBinding{};
            saveImageLayoutBinding.binding = 1;
            saveImageLayoutBinding.descriptorType = vk::DescriptorType::eStorageImage;
            saveImageLayoutBinding.descriptorCount = 1;
            saveImageLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            vk::DescriptorSetLayoutBinding uniCountLayoutBinding{};
            uniCountLayoutBinding.binding = 2;
            uniCountLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
            uniCountLayoutBinding.descriptorCount = 1;
            uniCountLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            compLayout = hd::conjure({
                    .device = device,
                    .bindings = { 
                            resImageLayoutBinding, saveImageLayoutBinding, uniCountLayoutBinding,
                        },
                    });
        }

        constexpr auto populateRayLayout() {
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

            vk::DescriptorSetLayoutBinding lightsBinding{};
            lightsBinding.binding = 7;
            lightsBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
            lightsBinding.descriptorCount = 1;
            lightsBinding.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

            vk::DescriptorSetLayoutBinding uniSizesBinding{};
            uniSizesBinding.binding = 8;
            uniSizesBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
            uniSizesBinding.descriptorCount = 1;
            uniSizesBinding.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;

            rayLayout = hd::conjure({
                    .device = device,
                    .bindings = { 
                            aStructLayoutBinding, resImageLayoutBinding, uniBufferBinding, 
                            textureBinding, vertexBinding, indicesBinding, materialBinding,
                            lightsBinding, uniSizesBinding
                        },
                    });
        }

        inline auto populateRayPipeline() {
            hd::Shader raygenShader = hd::conjure({
                    .device = device,
                    .filename = "shaders/raygen.rgen.spv",
                    .stage = vk::ShaderStageFlagBits::eRaygenKHR,
                    });

            hd::Shader missShader = hd::conjure({
                    .device = device,
                    .filename = "shaders/miss.rmiss.spv",
                    .stage = vk::ShaderStageFlagBits::eMissKHR,
                    });

            hd::Shader shadowShader = hd::conjure({
                    .device = device,
                    .filename = "shaders/shadow.rmiss.spv",
                    .stage = vk::ShaderStageFlagBits::eMissKHR,
                    });

            std::stringstream rchit;
            rchit << "shaders/" << params.method << ".rchit.spv";
            hd::Shader closestHitShader = hd::conjure({
                    .device = device,
                    .filename = rchit.str().c_str(),
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

            rayPipeline = hd::conjure({
                    .pipelineLayout = rayPipeLayout,
                    .device = device,
                    .shaderInfos = { raygenShader->info(), missShader->info(), shadowShader->info(), closestHitShader->info() },
                    .shaderGroups = { raygenGroupCI, missGroupCI, shadowGroupCI, closesthitGroupCI },
                    });
        }

        auto init() {
            window = hd::conjure({
                    .width = 1280,
                    .height = 720,
                    .title = "Ray traycing",
                    .cursorVisible = true,
                    .windowUser = this,
                    .framebufferSizeCallback = framebufferResizeCallback,
                    });

            auto instanceExtensions = window->getRequiredExtensions();
            instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

            instance = hd::conjure({
                    .applicationName = "Raytraycing",
                    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                    .engineName = "Hova's Engine",
                    .engineVersion = VK_MAKE_VERSION(2, 0, 0),
                    .apiVersion = VK_API_VERSION_1_2,
                    .validationLayers = { "VK_LAYER_KHRONOS_validation" },
                    .extensions = instanceExtensions,
                    });

            surface = hd::conjure({
                    .window = window,
                    .instance = instance,
                    });

            device = hd::conjure({
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
                    .features = selectFeatures().res(),
                    .validationLayers = { "VK_LAYER_KHRONOS_validation" },
                    });

            allocator = hd::conjure({
                    .instance = instance,
                    .device = device,
                    });

            graphicsQueue = hd::conjure({
                    .device = device,
                    .type = hd::QueueType::eGraphics,
                    });

            presentQueue = hd::conjure({
                    .device = device,
                    .type = hd::QueueType::ePresent,
                    });

            graphicsPool = hd::conjure({
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

            // BEGIN RAM
            auto scene = hd::conjure({
                    "models/scene.obj",
                    [&](const char *filename){
                        return hd::conjure({
                                filename,
                                graphicsPool,
                                graphicsQueue,
                                allocator,
                                device,
                                }); },
                    });
            
            auto lights = hd::Model_t::parseLights("models/scene.json");
            // END RAM

            populateInitialVRAM(scene, lights);

            populateCompLayout();

            vk::PushConstantRange pushWindowSize{};
            pushWindowSize.stageFlags = vk::ShaderStageFlagBits::eCompute;
            pushWindowSize.offset = 0;
            pushWindowSize.size = sizeof(PushWindowSize);

            compPipeLayout = hd::conjure({
                    .device = device,
                    .descriptorLayouts = { compLayout->raw() },
                    .pushConstants = { pushWindowSize },
                    });

            hd::Shader compShader = hd::conjure({
                    .device = device,
                    .filename = "shaders/summ.comp.spv",
                    .stage = vk::ShaderStageFlagBits::eCompute,
                    });

            summPipeline = hd::conjure({
                .pipelineLayout = compPipeLayout,
                .device = device,
                .shaderInfo = compShader->info(),
            });

            populateRayLayout();

            rayPipeLayout = hd::conjure({
                    .device = device,
                    .descriptorLayouts = { rayLayout->raw() },
                    });

            populateRayPipeline();

            sbt = hd::conjure({
                    .pipeline = rayPipeline,
                    .device = device,
                    .allocator = allocator,
                    .raygenCount = 1,
                    .missCount = 2,
                    .hitCount = 1,
                    });
        }

        hd::SwapChain swapChain;

        hd::DescriptorPool rayDescriptorPool;
        hd::DescriptorSet summDescriptorSet;
        hd::DescriptorSet rayDescriptorSet;
        std::vector<hd::CommandBuffer> rayCmdBuffers;
        std::vector<hd::CommandBuffer> raySaveCmdBuffers;
        std::vector<hd::CommandBuffer> raySummCmdBuffers;
        std::vector<hd::Fence> inFlightImages;

        inline auto fillSummLayout() {
            std::vector<vk::WriteDescriptorSet> writes;
            writes.reserve(3);

            auto write = [&](uint32_t binding, vk::DescriptorType type, uint32_t index = 0) {
                vk::WriteDescriptorSet writeSet{};
                writeSet.dstBinding = binding;
                writeSet.dstArrayElement = index;
                writeSet.descriptorType = type;
                writeSet.descriptorCount = 1;
                writeSet.dstSet = summDescriptorSet->raw();

                return writeSet;
            };

            auto storageImageInfo = vram.storage.view->writeInfo(vk::ImageLayout::eGeneral);
            auto storageImageWrite = write(0, vk::DescriptorType::eStorageImage);
            storageImageWrite.setPImageInfo(&storageImageInfo);
            writes.push_back(storageImageWrite);

            auto storageSummInfo = vram.storage.summView->writeInfo(vk::ImageLayout::eGeneral);
            auto storageSummWrite = write(1, vk::DescriptorType::eStorageImage);
            storageSummWrite.setPImageInfo(&storageSummInfo);
            writes.push_back(storageSummWrite);

            auto uniCountInfo = vram.uniCount->writeInfo();
            auto uniCountWrite = write(2, vk::DescriptorType::eUniformBuffer);
            uniCountWrite.setPBufferInfo(&uniCountInfo);
            writes.push_back(uniCountWrite);

            device->raw().updateDescriptorSets(writes, nullptr);
        }

        inline auto fillRayLayout() {
            std::vector<vk::WriteDescriptorSet> writes;
            writes.reserve(5 + vram.vertices.size());

            auto write = [&](uint32_t binding, vk::DescriptorType type, uint32_t index = 0) {
                vk::WriteDescriptorSet writeSet{};
                writeSet.dstBinding = binding;
                writeSet.dstArrayElement = index;
                writeSet.descriptorType = type;
                writeSet.descriptorCount = 1;
                writeSet.dstSet = rayDescriptorSet->raw();

                return writeSet;
            };

            auto aStructDI = vram.tlas->writeInfo();
            auto aStructWrite = write(0, vk::DescriptorType::eAccelerationStructureKHR);
            aStructWrite.setPNext(&aStructDI);
            writes.push_back(aStructWrite);

            auto storageImageInfo = vram.storage.view->writeInfo(vk::ImageLayout::eGeneral);
            auto storageImageWrite = write(1, vk::DescriptorType::eStorageImage);
            storageImageWrite.setPImageInfo(&storageImageInfo);
            writes.push_back(storageImageWrite);

            auto unibufferInfo = vram.unibuffer->writeInfo();
            auto unibufferWrite = write(2, vk::DescriptorType::eUniformBuffer);
            unibufferWrite.setPBufferInfo(&unibufferInfo);
            writes.push_back(unibufferWrite);

            auto lightsInfo = vram.lights->writeInfo();
            auto lightsWrite = write(7, vk::DescriptorType::eStorageBuffer);
            lightsWrite.setPBufferInfo(&lightsInfo);
            writes.push_back(lightsWrite);

            auto uniSizesInfo = vram.uniSizes->writeInfo();
            auto uniSizesWrite = write(8, vk::DescriptorType::eUniformBuffer);
            uniSizesWrite.setPBufferInfo(&uniSizesInfo);
            writes.push_back(uniSizesWrite);

            std::vector<vk::DescriptorImageInfo>  imgInfos;
            std::vector<vk::DescriptorBufferInfo> vtxInfos;
            std::vector<vk::DescriptorBufferInfo> idxInfos;
            std::vector<vk::DescriptorBufferInfo> matInfos;

            imgInfos.reserve(vram.diffuse.size());
            vtxInfos.reserve(vram.vertices.size());
            idxInfos.reserve(vram.indices.size());
            matInfos.reserve(vram.materials.size());

            for (uint32_t iter = 0; iter < vram.vertices.size(); iter++) {
                imgInfos.push_back(vram.diffuse[iter]->writeInfo(vk::ImageLayout::eShaderReadOnlyOptimal));
                auto imgWrite = write(3, vk::DescriptorType::eCombinedImageSampler, iter);
                imgWrite.setPImageInfo(&imgInfos[iter]);
                writes.push_back(imgWrite);

                vtxInfos.push_back(vram.vertices[iter]->writeInfo());
                auto vtxBufferWrite = write(4, vk::DescriptorType::eStorageBuffer, iter);
                vtxBufferWrite.setPBufferInfo(&vtxInfos[iter]);
                writes.push_back(vtxBufferWrite);

                idxInfos.push_back(vram.indices[iter]->writeInfo());
                auto idxBufferWrite = write(5, vk::DescriptorType::eStorageBuffer, iter);
                idxBufferWrite.setPBufferInfo(&idxInfos[iter]);
                writes.push_back(idxBufferWrite);

                matInfos.push_back(vram.materials[iter]->writeInfo());
                auto matBufferWrite = write(6, vk::DescriptorType::eStorageBuffer, iter);
                matBufferWrite.setPBufferInfo(&matInfos[iter]);
                writes.push_back(matBufferWrite);
            }

            device->raw().updateDescriptorSets(writes, nullptr);
        }

        uint32_t globalFrameCount = 0;

        auto setup() {
            swapChain = hd::conjure(hd::SwapChainCreateInfo{
                    .window = window,
                    .surface = surface,
                    .allocator = allocator,
                    .device = device,
                    .presentMode = vk::PresentModeKHR::eMailbox,
                    });

            vram.storage.image = hd::conjure({
                    .allocator = allocator,
                    .extent = swapChain->extent(),
                    .format = swapChain->format(),
                    .imageUsage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage,
                    .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                    });

            {
                auto cmd = graphicsPool->singleTimeBegin();
                cmd->transitionImageLayout({
                        .image = vram.storage.image,
                        .layout = vk::ImageLayout::eGeneral,
                        });
                graphicsPool->singleTimeEnd(cmd, graphicsQueue);
            }

            vram.storage.view = hd::conjure({
                    .image = vram.storage.image->raw(),
                    .device = device,
                    .format = vram.storage.image->format(),
                    .range = vram.storage.image->range(),
                    .type = vk::ImageViewType::e2D,
                    });

            vram.storage.summ = hd::conjure({
                    .allocator = allocator,
                    .extent = swapChain->extent(),
                    .format = swapChain->format(),
                    .imageUsage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst,
                    .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                    });

            {
                auto cmd = graphicsPool->singleTimeBegin();
                cmd->transitionImageLayout({
                        .image = vram.storage.summ,
                        .layout = vk::ImageLayout::eGeneral,
                        });

                vk::ClearColorValue color = std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
                vk::ImageSubresourceRange range = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

                cmd->raw().clearColorImage(vram.storage.summ->raw(), vk::ImageLayout::eGeneral, color, range);
                graphicsPool->singleTimeEnd(cmd, graphicsQueue);
            }

            vram.storage.summView = hd::conjure({
                    .image = vram.storage.summ->raw(),
                    .device = device,
                    .format = vram.storage.summ->format(),
                    .range = vram.storage.summ->range(),
                    .type = vk::ImageViewType::e2D,
                    });

            ram.saveImage = hd::conjure({
                    .allocator = allocator,
                    .extent = swapChain->extent(),
                    .format = swapChain->format(),
                    .tiling = vk::ImageTiling::eLinear,
                    .imageUsage = vk::ImageUsageFlagBits::eTransferDst,
                    .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
                    });

            {
                auto cmd = graphicsPool->singleTimeBegin();
                cmd->transitionImageLayout({
                        .image = ram.saveImage,
                        .layout = vk::ImageLayout::eGeneral,
                        });
                graphicsPool->singleTimeEnd(cmd, graphicsQueue);
            }

            ram.saveView = hd::conjure({
                    .image = ram.saveImage->raw(),
                    .device = device,
                    .format = ram.saveImage->format(),
                    .range = ram.saveImage->range(),
                    .type = vk::ImageViewType::e2D,
                    });

            UniformData uniData{};

            vram.unibuffer = hd::conjure<UniformData>({
                    .commandPool = graphicsPool,
                    .queue = graphicsQueue,
                    .allocator = allocator,
                    .device = device,
                    .data = {uniData},
                    .usage = vk::BufferUsageFlagBits::eUniformBuffer,
                    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                    });

            UniCount uniCount{};

            vram.uniCount = hd::conjure<UniCount>({
                    .commandPool = graphicsPool,
                    .queue = graphicsQueue,
                    .allocator = allocator,
                    .device = device,
                    .data = {uniCount},
                    .usage = vk::BufferUsageFlagBits::eUniformBuffer,
                    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                    });

            rayDescriptorPool = hd::conjure({
                    .device = device,
                    .layouts = {{rayLayout, 1}, {compLayout, 1}},
                    .instances = 1,
                   });

            summDescriptorSet = rayDescriptorPool->allocate(1, compLayout).at(0);
            rayDescriptorSet = rayDescriptorPool->allocate(1, rayLayout).at(0);

            fillSummLayout();
            fillRayLayout();

            vk::ImageSubresourceRange sRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

            auto fillCmdBuffer = [&](hd::CommandBuffer buffer, uint32_t i) {
                buffer->raw().bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rayPipeline->raw());
                buffer->raw().bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, rayPipeLayout->raw(), 0, rayDescriptorSet->raw(), nullptr);

                vk::StridedDeviceAddressRegionKHR callableShaderSBTEntry{};

                buffer->raw().traceRaysKHR(
                        sbt->raygen().region,
                        sbt->miss().region,
                        sbt->hit().region,
                        callableShaderSBTEntry,
                        swapChain->extent().width,
                        swapChain->extent().height,
                        1
                        );

                buffer->transitionImageLayout({
                        swapChain->colorAttachment(i)->raw(),
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eTransferDstOptimal,
                        sRange,
                        });

                buffer->transitionImageLayout({
                        vram.storage.image->raw(),
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

                buffer->raw().copyImage(
                        vram.storage.image->raw(), vk::ImageLayout::eTransferSrcOptimal, 
                        swapChain->colorAttachment(i)->raw(), vk::ImageLayout::eTransferDstOptimal, 
                        copyRegion
                        );

                buffer->transitionImageLayout({
                        swapChain->colorAttachment(i)->raw(),
                        vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::ePresentSrcKHR,
                        sRange,
                        });

                buffer->transitionImageLayout({
                        vram.storage.image->raw(),
                        vk::ImageLayout::eTransferSrcOptimal,
                        vk::ImageLayout::eGeneral,
                        sRange,
                        });
            };

            const struct PushWindowSize dims = {
                vram.storage.image->extent().width,
                vram.storage.image->extent().height,
            };

            rayCmdBuffers = graphicsPool->allocate(swapChain->length());
            raySaveCmdBuffers = graphicsPool->allocate(swapChain->length());
            raySummCmdBuffers = graphicsPool->allocate(swapChain->length());
            for (uint32_t i = 0; i < swapChain->length(); i++) {
                {
                    auto buffer = rayCmdBuffers[i];
                    buffer->begin();
                    fillCmdBuffer(buffer, i);
                    buffer->end();
                }

                if (!params.capture)
                    continue;

                {
                    auto buffer = raySummCmdBuffers[i];
                    buffer->begin();
                    fillCmdBuffer(buffer, i);

                    buffer->barrier({vk::AccessFlagBits{0}, vk::AccessFlagBits{0},
                            vk::PipelineStageFlagBits::eRayTracingShaderKHR, vk::PipelineStageFlagBits::eComputeShader});

                    buffer->raw().pushConstants(compPipeLayout->raw(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(PushWindowSize), &dims);

                    buffer->raw().bindDescriptorSets(vk::PipelineBindPoint::eCompute, compPipeLayout->raw(), 0, summDescriptorSet->raw(), nullptr);
                    buffer->raw().bindPipeline(vk::PipelineBindPoint::eCompute, summPipeline->raw());
                    buffer->raw().dispatch(swapChain->extent().width / 16, swapChain->extent().height / 16, 1);

                    buffer->end();
                }

                {
                    auto buffer = raySaveCmdBuffers[i];
                    buffer->begin();
                    fillCmdBuffer(buffer, i);

                    buffer->transitionImageLayout({
                            vram.storage.summ->raw(),
                            vk::ImageLayout::eGeneral,
                            vk::ImageLayout::eTransferSrcOptimal,
                            sRange,
                            });

                    vk::ImageCopy copyRegion{};
                    copyRegion.srcSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 };
                    copyRegion.setSrcOffset({ 0, 0, 0 });
                    copyRegion.dstSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 };
                    copyRegion.setDstOffset({ 0, 0, 0 });
                    copyRegion.setExtent({ vram.storage.summ->extent().width, vram.storage.summ->extent().height, 1 });

                    buffer->raw().copyImage(
                            vram.storage.summ->raw(), vk::ImageLayout::eTransferSrcOptimal, 
                            ram.saveImage->raw(), vk::ImageLayout::eGeneral, 
                            copyRegion
                            );

                    buffer->transitionImageLayout({
                            vram.storage.summ->raw(),
                            vk::ImageLayout::eTransferSrcOptimal,
                            vk::ImageLayout::eGeneral,
                            sRange,
                            });

                    buffer->end();
                }
            }

            inFlightImages.resize(swapChain->length(), nullptr);
        }

        void cleanupRender() {
            device->waitIdle();

            rayCmdBuffers.clear();
            vram.unibuffer.reset();
            ram.saveView.reset();
            ram.saveImage.reset();
            vram.storage.view.reset();
            vram.storage.image.reset();
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
            const float cameraSpeed = 0.15f;
            const float cameraRotateSpeed = 0.06f;

            static float rotateYAngle = glm::pi<float>();
            static float rotateZAngle = -glm::half_pi<float>();

            static glm::vec3 cameraPos = glm::vec3(13.5f, -3.0f, 0.0f);
            static glm::vec3 cameraForward = glm::vec3(0.0f, 0.0f, 1.0f);
            static glm::vec3 cameraLeft = glm::vec3(1.0f, 0.0f, 0.0f);
            static glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

            auto perspective = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 512.0f);
            auto translate = glm::translate(glm::mat4(1.0f), cameraPos);
            auto rotateY = glm::rotate(glm::mat4(1.0f), rotateYAngle, glm::vec3(0.0f, 0.0f, 1.0f));
            auto rotateZ = glm::rotate(glm::mat4(1.0f), rotateZAngle, glm::vec3(0.0f, 1.0f, 0.0f));

            UniformData uniData {};
            uniData.projInverse = glm::inverse(perspective * glm::mat4(1.0f));
            uniData.viewInverse = glm::inverse(rotateZ * rotateY * translate * glm::mat4(1.0f));
            uniData.frameIndex = globalFrameCount;
            uniData.N = params.N;

            {
                auto data = vram.unibuffer->map();
                memcpy(data, &uniData, sizeof(UniformData));
                vram.unibuffer->unmap();
            }

            cameraForward = glm::normalize(glm::vec3(uniData.viewInverse[2]));
            cameraLeft = glm::normalize(glm::vec3(uniData.viewInverse[0]));
            cameraUp = glm::normalize(glm::vec3(uniData.viewInverse[1]));

            if (glfwGetKey(window->raw(), GLFW_KEY_W) == GLFW_PRESS)
                cameraPos += cameraForward * cameraSpeed;
            if (glfwGetKey(window->raw(), GLFW_KEY_S) == GLFW_PRESS)
                cameraPos += cameraForward * -cameraSpeed;
            if (glfwGetKey(window->raw(), GLFW_KEY_A) == GLFW_PRESS)
                cameraPos += cameraLeft * cameraSpeed;
            if (glfwGetKey(window->raw(), GLFW_KEY_D) == GLFW_PRESS)
                cameraPos += cameraLeft * -cameraSpeed;
            if (glfwGetKey(window->raw(), GLFW_KEY_SPACE) == GLFW_PRESS)
                cameraPos += cameraUp * cameraSpeed;
            if (glfwGetKey(window->raw(), GLFW_KEY_BACKSPACE) == GLFW_PRESS)
                cameraPos += cameraUp * -cameraSpeed;

            if (glfwGetKey(window->raw(), GLFW_KEY_Q) == GLFW_PRESS)
                rotateZAngle += -cameraRotateSpeed;
            if (glfwGetKey(window->raw(), GLFW_KEY_E) == GLFW_PRESS)
                rotateZAngle += cameraRotateSpeed;

            UniCount uniCount{};
            uniCount.count = (globalFrameCount == params.frames) ? params.frames : 1;

            {
                auto data = vram.uniCount->map();
                memcpy(data, &uniCount, sizeof(UniCount));
                vram.uniCount->unmap();
            }
        }

        uint32_t currentFrame = 0;
        void update() {
            static uint32_t screenshotFrame = -1;
            inFlightFences[currentFrame]->wait();

            if ((currentFrame == screenshotFrame) && params.capture) {
                if (params.pseudoOffline) {
                    hd::saveImg(ram.saveImage, device, allocator, params.method, params.N, globalFrameCount - swapChain->length());
                    exit(0);
                }
                std::thread{hd::saveImg, ram.saveImage, device, allocator, params.method, params.N, globalFrameCount - swapChain->length()}.detach();
                screenshotFrame = -1;
            }

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

                std::vector<vk::CommandBuffer> raw;

                if ((globalFrameCount == params.frames) && params.capture) {
                    raw.push_back(raySaveCmdBuffers[imageIndex]->raw());
                    screenshotFrame = currentFrame;
                } else if ((globalFrameCount < params.frames) && params.capture) {
                    raw.push_back(raySummCmdBuffers[imageIndex]->raw());
                } else
                    raw.push_back(rayCmdBuffers[imageIndex]->raw());

                vk::SubmitInfo submitInfo = {};
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphores;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = raw.size();
                submitInfo.pCommandBuffers = raw.data();
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

    public:
        App(params_t _params) : params(_params) {}

        void run() {
            init();
            setup();
            loop();
        }
};
