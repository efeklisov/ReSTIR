#include <hdvw/pipeline.hpp>
using namespace hd;

DefaultPipeline_t::DefaultPipeline_t(DefaultPipelineCreateInfo ci) {
    _device = ci.device->raw();

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::Viewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = ci.extent.width;
    viewport.height = ci.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor = {};
    scissor.offset = vk::Offset2D{ 0, 0 };
    scissor.extent = ci.extent;

    vk::PipelineViewportStateCreateInfo viewportState = {};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = ci.cullMode;
    rasterizer.frontFace = ci.frontFace;
    rasterizer.polygonMode = ci.polygonMode;

    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    vk::PipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    vk::PipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    if (ci.checkDepth) {
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
    } else {
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
    }

    vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
        | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

    vk::PipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    /* vk::DynamicState dynamicStates[] = { */
    /*     vk::DynamicState::eViewport, */
    /*     vk::DynamicState::eLineWidth, */
    /* }; */

    /* vk::PipelineDynamicStateCreateInfo dynamicState{}; */
    /* dynamicState.dynamicStateCount = 2; */
    /* dynamicState.pDynamicStates = dynamicStates; */

    vk::GraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.stageCount = ci.shaderInfo.size();
    pipelineInfo.pStages = ci.shaderInfo.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    /* pipelineInfo.pDynamicState = &dynamicState; */
    pipelineInfo.layout = ci.pipelineLayout->raw();
    pipelineInfo.renderPass = ci.renderPass->raw();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    auto res = _device.createGraphicsPipeline(nullptr, pipelineInfo);
    if (res.result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create a pipeline");

    _pipeline = res.value;
}

vk::Pipeline DefaultPipeline_t::raw() {
    return _pipeline;
}

DefaultPipeline_t::~DefaultPipeline_t() {
    _device.destroy(_pipeline);
}

RaytraycingPipeline_t::RaytraycingPipeline_t(RaytraycingPipelineCreateInfo ci) {
    _device = ci.device->raw();

    vk::RayTracingPipelineCreateInfoKHR pipelineInfo{};
    pipelineInfo.setStages(ci.shaderInfos);
    pipelineInfo.setGroups(ci.shaderGroups);
    pipelineInfo.maxRecursionDepth = 1;
    pipelineInfo.layout = ci.pipelineLayout->raw();
    
    auto res = _device.createRayTracingPipelineKHR(nullptr, pipelineInfo, nullptr);
    if (res.result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create a raytraycing pipeline");

    _pipeline = res.value;
}

vk::Pipeline RaytraycingPipeline_t::raw() {
    return _pipeline;
}

RaytraycingPipeline_t::~RaytraycingPipeline_t() {
    _device.destroy(_pipeline);
}
