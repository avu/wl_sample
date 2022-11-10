#pragma once

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <array>
#include <unordered_map>
#include <numeric>
#include <ctime>
#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>
#include <sys/stat.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <numeric>
#include <array>

#include "vulkan/vulkan.h"
#include "vulkan/vulkan_wayland.h"

#include "vulkan_swap_chain.h"
#include "vulkan_device.h"



class VulkanBase
{
private:
	void createPipelineCache();
	void createCommandPool();
	void createSynchronizationPrimitives();
	void initSwapchain();
	void setupSwapChain();
	void createCommandBuffers();
	void destroyCommandBuffers();
protected:
	std::string getShadersPath() const;

	VkInstance instance;
	std::vector<std::string> supportedInstanceExtensions;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	VkPhysicalDeviceFeatures enabledFeatures{};
	std::vector<const char*> enabledDeviceExtensions;
	void* deviceCreatepNextChain = nullptr;
	VkDevice device;
	VkQueue queue;
	VkFormat depthFormat;
	VkCommandPool cmdPool;
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo;
	std::vector<VkCommandBuffer> drawCmdBuffers;
	VkRenderPass renderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer>frameBuffers;
	uint32_t currentBuffer = 0;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	std::vector<VkShaderModule> shaderModules;
	VkPipelineCache pipelineCache;
	vulkan_swap_chain swapChain;

	struct {
		// Swap chain image presentation
		VkSemaphore presentComplete;
		// Command buffer submission and execution
		VkSemaphore renderComplete;
	} semaphores;
	std::vector<VkFence> waitFences;
public:
	bool prepared = false;
	uint32_t width = 1280;
	uint32_t height = 720;

	vulkan_device *vulkanDevice;

    glm::mat4 perspective;
    glm::mat4 view;
	glm::vec2 mousePos;

	std::string title = "Vulkan Example";
	std::string name = "vulkanExample";
	struct {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	} depthStencil;

	wl_display *display = nullptr;
	wl_registry *registry = nullptr;
	wl_compositor *compositor = nullptr;
	struct xdg_wm_base *shell = nullptr;
	wl_seat *seat = nullptr;
	wl_surface *surface = nullptr;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
	bool quit = false;
	bool configured = false;


	VulkanBase();
	virtual ~VulkanBase();
	bool initVulkan();

	struct xdg_surface *setupWindow();
	void initWaylandConnection();
	static void registryGlobalCb(void *data, struct wl_registry *registry,
			uint32_t name, const char *interface, uint32_t version);
	void registryGlobal(struct wl_registry *registry, uint32_t name,
			const char *interface, uint32_t version);
	static void registryGlobalRemoveCb(void *data, struct wl_registry *registry,
			uint32_t name);

	virtual VkResult createInstance();
	virtual void render() = 0;
	virtual void buildCommandBuffers();
	virtual void setupDepthStencil();
	virtual void setupFrameBuffer();
	virtual void setupRenderPass();
    virtual void prepare();
    void renderLoop();
};