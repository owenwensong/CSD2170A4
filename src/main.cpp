/*!*****************************************************************************
 * @file        main.cpp
 * @author      Sascha Willems
 * @co-author   William Zheng
 * @co-author   Owen Huang Wensong, w.huang, 390008220
 * @date        25 NOV 2022
 * @brief   
 *
 * @par Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*******************************************************************************/

#include "appBase.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION true

class VulkanExample : public VkAppBase
{
private:

public:

  // Resources for the graphics part of the example
  struct {
    VkDescriptorSetLayout descriptorSetLayout;	// Image display shader binding layout
    VkDescriptorSet descriptorSet;	            // Image display shader bindings
    VkPipeline pipelineFilled;						      // Filled pipeline
    VkPipeline pipelineWireframe;               // Wireframe pipeline
    VkPipelineLayout pipelineLayout;			      // Layout of the graphics pipeline
  } graphics;

  vks::Buffer uniformBufferVS;

  struct {
    glm::mat4 projection;
    glm::mat4 modelView;
  } uboVS;

  int vertexBufferSize;

  std::vector<std::string> shaderNames;

  VulkanExample() : VkAppBase(ENABLE_VALIDATION)
  {
    title = "Compute shader image processing";
    camera.type = Camera::CameraType::lookat;
    camera.setPosition(glm::vec3(0.0f, 0.0f, -2.0f));
    camera.setRotation(glm::vec3(0.0f));
    camera.setPerspective(60.0f, (float)width * 0.5f / (float)height, 1.0f, 256.0f);
  }

  ~VulkanExample()
  {
    // Graphics
    vkDestroyPipeline(device, graphics.pipelineFilled, nullptr);
    vkDestroyPipeline(device, graphics.pipelineWireframe, nullptr);
    vkDestroyPipelineLayout(device, graphics.pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, graphics.descriptorSetLayout, nullptr);

    uniformBufferVS.destroy();
  }

  // Enable physical device features required for this example
  virtual void getEnabledFeatures()
  {
    // Tessellation shader support is required for this example
    if (deviceFeatures.tessellationShader) {
      enabledFeatures.tessellationShader = VK_TRUE;
    }
    else {
      vks::tools::exitFatal("Selected GPU does not support tessellation shaders!", VK_ERROR_FEATURE_NOT_PRESENT);
    }
    // Fill mode non solid is required for wireframe display
    if (deviceFeatures.fillModeNonSolid) {
      enabledFeatures.fillModeNonSolid = VK_TRUE;
    }
    else {
      std::cerr << "wireframe not supported :(" << std::endl;
    }
  }

  // Prepare a texture target that is used to store compute shader calculations
  void prepareTextureTarget(vks::Texture* tex, uint32_t width, uint32_t height, VkFormat format)
  {
    VkFormatProperties formatProperties;

    // Get device properties for the requested texture format
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
    // Check if requested image format supports image storage operations
    assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);

    // Prepare blit target texture
    tex->width = width;
    tex->height = height;

    VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent = { width, height, 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    // Image will be sampled in the fragment shader and used as storage target in the compute shader
    imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    imageCreateInfo.flags = 0;
    // If compute and graphics queue family indices differ, we create an image that can be shared between them
    // This can result in worse performance than exclusive sharing mode, but save some synchronization to keep the sample simple
    std::vector<uint32_t> queueFamilyIndices;
    if (vulkanDevice->queueFamilyIndices.graphics != vulkanDevice->queueFamilyIndices.compute) {
      queueFamilyIndices = {
        vulkanDevice->queueFamilyIndices.graphics,
        vulkanDevice->queueFamilyIndices.compute
      };
      imageCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
      imageCreateInfo.queueFamilyIndexCount = 2;
      imageCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    }

    VkMemoryAllocateInfo memAllocInfo = vks::initializers::memoryAllocateInfo();
    VkMemoryRequirements memReqs;

    VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &tex->image));

    vkGetImageMemoryRequirements(device, tex->image, &memReqs);
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &tex->deviceMemory));
    VK_CHECK_RESULT(vkBindImageMemory(device, tex->image, tex->deviceMemory, 0));

    VkCommandBuffer layoutCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    tex->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    vks::tools::setImageLayout(
      layoutCmd, tex->image,
      VK_IMAGE_ASPECT_COLOR_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED,
      tex->imageLayout);

    vulkanDevice->flushCommandBuffer(layoutCmd, queue, true);

    // Create sampler
    VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler.addressModeV = sampler.addressModeU;
    sampler.addressModeW = sampler.addressModeU;
    sampler.mipLodBias = 0.0f;
    sampler.maxAnisotropy = 1.0f;
    sampler.compareOp = VK_COMPARE_OP_NEVER;
    sampler.minLod = 0.0f;
    sampler.maxLod = tex->mipLevels;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &tex->sampler));

    // Create image view
    VkImageViewCreateInfo view = vks::initializers::imageViewCreateInfo();
    view.image = VK_NULL_HANDLE;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = format;
    view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    view.image = tex->image;
    VK_CHECK_RESULT(vkCreateImageView(device, &view, nullptr, &tex->view));

    // Initialize a descriptor for later use
    tex->descriptor.imageLayout = tex->imageLayout;
    tex->descriptor.imageView = tex->view;
    tex->descriptor.sampler = tex->sampler;
    tex->device = vulkanDevice;
  }

  void loadAssets()
  {

  }

  void buildCommandBuffers()
  {
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    VkClearValue clearValues[2];
    clearValues[0].color = defaultClearColor;
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;
    //std::cout << "drawCmdBuffers size = " << drawCmdBuffers.size() << std::endl;
    for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
    {
      // Set target frame buffer
      renderPassBeginInfo.framebuffer = frameBuffers[i];

      VkCommandBuffer cmdBuf{ drawCmdBuffers[i] };

      VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuf, &cmdBufInfo));

      vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

      VkViewport viewport = vks::initializers::viewport(width * 0.5f, static_cast<float>(height), 0.0f, 1.0f);
      VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
      vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

      { // LEFT
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipelineWireframe);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipelineLayout, 0, 1, &graphics.descriptorSet, 0, nullptr);
        vkCmdDraw(cmdBuf, 3, 1, 0, 0);
      }

      viewport.x += viewport.width; // right side viewport rect min

      { // RIGHT
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipelineFilled);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipelineLayout, 0, 1, &graphics.descriptorSet, 0, nullptr);
        vkCmdDraw(cmdBuf, 3, 1, 0, 0);
      }
      
      //drawUI(drawCmdBuffers[i]);

      vkCmdEndRenderPass(cmdBuf);

      VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuf));
    }

  }

  void setupDescriptorPool()
  {
    std::vector<VkDescriptorPoolSize> poolSizes = {
      // Graphics pipelines uniform buffers
      vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
      // Graphics pipelines image samplers for displaying compute output image
      vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2),
      // Compute pipelines uses a storage image for image reads and writes
      // note: optimization, not creating new image to store YUV
      vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2),
      // Compute pipelines for Assignment 3 Part 2 require an SSBO
      vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1)
    };
    VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 3);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
  }

  void setupDescriptorSetLayout()
  {
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      // Binding 0: Vertex shader uniform buffer
      vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
    };

    VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &graphics.descriptorSetLayout));

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&graphics.descriptorSetLayout, 1);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &graphics.pipelineLayout));
  }

  void setupDescriptorSet()
  {
    VkDescriptorSetAllocateInfo allocInfo =
      vks::initializers::descriptorSetAllocateInfo(descriptorPool, &graphics.descriptorSetLayout, 1);

    // Graphics
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &graphics.descriptorSet));
    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      vks::initializers::writeDescriptorSet(graphics.descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBufferVS.descriptor)
    };
    vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
  }

  void preparePipelines()
  {
    VkPipelineVertexInputStateCreateInfo vertexInputState{ vks::initializers::pipelineVertexInputStateCreateInfo() };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState
    { vks::initializers::pipelineInputAssemblyStateCreateInfo(
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        0,
        VK_FALSE
    ) };

    VkPipelineRasterizationStateCreateInfo rasterizationState =
      vks::initializers::pipelineRasterizationStateCreateInfo(
        VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_NONE,
        VK_FRONT_FACE_COUNTER_CLOCKWISE,
        0);

    VkPipelineColorBlendAttachmentState blendAttachmentState =
      vks::initializers::pipelineColorBlendAttachmentState(
        0xf,
        VK_FALSE);

    VkPipelineColorBlendStateCreateInfo colorBlendState =
      vks::initializers::pipelineColorBlendStateCreateInfo(
        1,
        &blendAttachmentState);

    VkPipelineDepthStencilStateCreateInfo depthStencilState =
      vks::initializers::pipelineDepthStencilStateCreateInfo(
        VK_TRUE,
        VK_TRUE,
        VK_COMPARE_OP_LESS_OR_EQUAL);

    VkPipelineViewportStateCreateInfo viewportState =
      vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

    VkPipelineMultisampleStateCreateInfo multisampleState =
      vks::initializers::pipelineMultisampleStateCreateInfo(
        VK_SAMPLE_COUNT_1_BIT,
        0);

    std::vector<VkDynamicState> dynamicStateEnables = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState =
      vks::initializers::pipelineDynamicStateCreateInfo(
        dynamicStateEnables.data(),
        dynamicStateEnables.size(),
        0);

    // Rendering pipeline
    // Load shaders
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    shaderStages[0] = loadShader(getShadersPath() + "test/genTri.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(getShadersPath() + "test/genTri.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    VkGraphicsPipelineCreateInfo pipelineCreateInfo =
      vks::initializers::pipelineCreateInfo(
        graphics.pipelineLayout,
        renderPass,
        0);

    pipelineCreateInfo.pVertexInputState = &vertexInputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = shaderStages.size();
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.renderPass = renderPass;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphics.pipelineFilled));

    rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphics.pipelineWireframe));
  }

  // Prepare and initialize uniform buffer containing shader uniforms
  void prepareUniformBuffers()
  {
    // Vertex shader uniform buffer block
    VK_CHECK_RESULT(vulkanDevice->createBuffer(
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &uniformBufferVS,
      sizeof(uboVS)));

    // Map persistent
    VK_CHECK_RESULT(uniformBufferVS.map());

    updateUniformBuffers();
  }

  void updateUniformBuffers()
  {
    uboVS.projection = camera.matrices.perspective;
    uboVS.modelView = camera.matrices.view;
    memcpy(uniformBufferVS.mapped, &uboVS, sizeof(uboVS));
  }

  // Ignoring template 7: using in-queue execution barriers
  void draw()
  {
    VkAppBase::prepareFrame();

    VkPipelineStageFlags graphicsWaitStageMasks[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore graphicsWaitSemaphores[] = { semaphores.presentComplete };
    VkSemaphore graphicsSignalSemaphores[] = { semaphores.renderComplete };

    // Submit graphics commands
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = graphicsWaitSemaphores;
    submitInfo.pWaitDstStageMask = graphicsWaitStageMasks;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = graphicsSignalSemaphores;
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

    VkAppBase::submitFrame();
  }

  // Ignoring template 8: changes made directly to prepareCompute
  void prepare()
  {
    VkAppBase::prepare();
    loadAssets();
    prepareUniformBuffers();
    setupDescriptorSetLayout();
    preparePipelines();
    setupDescriptorPool();
    setupDescriptorSet();
    buildCommandBuffers();
    prepared = true;
  }

  virtual void render()
  {
    if (!prepared)
      return;
    draw();
    if (camera.updated) {
      updateUniformBuffers();
    }
  }

  virtual void OnUpdateUIOverlay(vks::UIOverlay* /*overlay*/)
  {
    
  }
};

VULKAN_EXAMPLE_MAIN()
