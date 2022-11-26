/*!*****************************************************************************
 * @file        main.cpp
 * @author      Sascha Willems
 * @co-author   William Zheng
 * @co-author   Owen Huang Wensong, w.huang, 390008220
 * @date        25 NOV 2022
 * @brief       Assignment 4 entry point. This file is a modified version of
 *              Assignment 3 part 2 main file.
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

  vks::Buffer UBOGlobal_Device;

  // use same uniform buffer for all shader stages out of laziness
  struct UBOGlobal
  {
    glm::mat4 m_View;
    glm::mat4 m_Proj;
    glm::vec4 m_Center{ 0.0f, -0.25f, 0.0f, 1.0f }; // center of the ellipsoid
    // x: a param, y: b param, z: c param, w: tessellation level
    glm::vec4 m_ScaleAndTeslvl{ 0.25f, 0.5f, 0.25f, 64.0f };
  } UBOGlobal_Host;

  VulkanExample() : VkAppBase(ENABLE_VALIDATION)
  {
    title = "CSD2170 Assignment 4 | Tessellation | Owen Huang Wensong";
    camera.type = Camera::CameraType::lookat;
    camera.setPosition(glm::vec3(0.0f, 0.0f, -2.0f));
    camera.setRotation(glm::vec3(0.0f));
    camera.setPerspective(60.0f, width * 0.5f / static_cast<float>(height), 1.0f, 256.0f);
  }

  ~VulkanExample()
  {
    // Graphics
    vkDestroyPipeline(device, graphics.pipelineFilled, nullptr);
    vkDestroyPipeline(device, graphics.pipelineWireframe, nullptr);
    vkDestroyPipelineLayout(device, graphics.pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, graphics.descriptorSetLayout, nullptr);

    UBOGlobal_Device.destroy();
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
        vkCmdDraw(cmdBuf, 1, 1, 0, 0);
      }

      viewport.x += viewport.width; // right side viewport rect min

      { // RIGHT
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipelineFilled);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipelineLayout, 0, 1, &graphics.descriptorSet, 0, nullptr);
        vkCmdDraw(cmdBuf, 1, 1, 0, 0);
      }
      
      //drawUI(cmdBuf);

      vkCmdEndRenderPass(cmdBuf);

      VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuf));
    }

  }

  void setupDescriptorPool()
  {
    std::vector<VkDescriptorPoolSize> poolSizes = {
      // Graphics pipelines uniform buffers
      vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
    };
    VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
  }

  void setupDescriptorSetLayout()
  {
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      // Binding 0: Vertex shader uniform buffer
      vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, 0)
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
      vks::initializers::writeDescriptorSet(graphics.descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &UBOGlobal_Device.descriptor)
    };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
  }

  void preparePipelines()
  {
    VkPipelineVertexInputStateCreateInfo vertexInputState{ vks::initializers::pipelineVertexInputStateCreateInfo() };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState
    { vks::initializers::pipelineInputAssemblyStateCreateInfo(
        VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
        0,
        VK_FALSE
    ) };

    VkPipelineTessellationStateCreateInfo tessellationState{ vks::initializers::pipelineTessellationStateCreateInfo(1) };

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
        static_cast<uint32_t>(dynamicStateEnables.size()),
        0);

    // Load shaders
    std::array<VkPipelineShaderStageCreateInfo, 4> shaderStages
    {
      loadShader(getShadersPath() + "ellipsoid.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
      loadShader(getShadersPath() + "ellipsoid.tesc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT),
      loadShader(getShadersPath() + "ellipsoid.tese.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT),
      loadShader(getShadersPath() + "ellipsoid.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo =
      vks::initializers::pipelineCreateInfo(
        graphics.pipelineLayout,
        renderPass,
        0);

    pipelineCreateInfo.pVertexInputState = &vertexInputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pTessellationState = &tessellationState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
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
      &UBOGlobal_Device,
      sizeof(UBOGlobal)));

    // Map persistent
    VK_CHECK_RESULT(UBOGlobal_Device.map());

    updateUniformBuffers();
  }

  void updateUniformBuffers()
  {
    UBOGlobal_Host.m_View = camera.matrices.view;
    UBOGlobal_Host.m_Proj = camera.matrices.perspective;
    memcpy(UBOGlobal_Device.mapped, &UBOGlobal_Host, sizeof(UBOGlobal));
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

  virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay)
  {
    if (overlay->header("Settings"))
    {
      if (overlay->inputFloat("Center X", &UBOGlobal_Host.m_Center.x, 0.125f, 3))
      {
        updateUniformBuffers();
      }
      if (overlay->inputFloat("Center Y", &UBOGlobal_Host.m_Center.y, 0.125f, 3))
      {
        updateUniformBuffers();
      }
      if (overlay->inputFloat("Center Z", &UBOGlobal_Host.m_Center.z, 0.125f, 3))
      {
        updateUniformBuffers();
      }
      if (overlay->inputFloat("ellipsoid a", &UBOGlobal_Host.m_ScaleAndTeslvl.x, 0.125f, 3))
      {
        updateUniformBuffers();
      }
      if (overlay->inputFloat("ellipsoid b", &UBOGlobal_Host.m_ScaleAndTeslvl.y, 0.125f, 3))
      {
        updateUniformBuffers();
      }
      if (overlay->inputFloat("ellipsoid c", &UBOGlobal_Host.m_ScaleAndTeslvl.z, 0.125f, 3))
      {
        updateUniformBuffers();
      }
      if (overlay->inputFloat("tessellation strength", &UBOGlobal_Host.m_ScaleAndTeslvl.w, 1.0, 3))
      {
        updateUniformBuffers();
      }
    }
  }
};

VULKAN_EXAMPLE_MAIN()
