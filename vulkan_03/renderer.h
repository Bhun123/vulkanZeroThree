#pragma once

#define DEBUG
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include <stb_image.h>
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>

#include "rendererStructs.h"

#include <functional>
#include <iostream>
#include <vector>
#include <exception>
#include <string>
#include <fstream>
#include <unordered_map>
#include <chrono>

const std::vector<Vertex> verticesSquare = {
{ { -0.5f, -0.5f, 0.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } },
{ { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } },
{ { 0.5f, 0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
{ { -0.5f, 0.5f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f } },

{ { -0.5f, -0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } },
{ { 0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } },
{ { 0.5f, 0.5f, -0.5f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
{ { -0.5f, 0.5f, -0.5f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f } }
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4,
	4,7,2
};

template <typename T> class vulkanObj;
class renderer
{
public:
	renderer();

	virtual ~renderer();

	
private:
	void initializeVulkan();
	void mainLoop();

	// vulkan objects setup
	void createWindow();
	void createInstance();
	void createDebug();
	void createDevice();

	void createSwapChain();
	void createRenderPass();
	void createFramebuffers();

	void createDescriptors();

	void createGraphicsPipeline();

	void createCommandPool();
	void createCommandBuffers();
	void createSemaphores();

	void createDepthBuffer();
	void createBuffers();
	void createImage();

	// vulkan draw frame
	void drawFrame();
	void updateMatrix();

	// setup helper functions
	std::vector<const char *> getInstanceExtensions();
	void createImageView(const vk::Image image, const vk::Format format, const vk::ImageAspectFlags aspectFlags, vk::UniqueImageView& imageView);
	std::vector<char> readShaderCode(const char * shaderPath);
	vk::ShaderModule createShaderModule(std::vector<char>& shaderCode);
	vk::CommandBuffer beginSingleTimeCommands() const;
	void endSingleTimeCommands(vk::CommandBuffer) const;

	// callbacks
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, const char*, const char*, void*);

	// external functions
	class CDdebugReportCallback
	{
	public:
		void create(const VkInstance& instance, const VkDebugReportCallbackCreateInfoEXT * pCreateInfo)
		{
			mInstance = instance;
			if (pfn_vkCreateDebugReportCallbackEXT(mInstance, pCreateInfo, nullptr, &mCallback) != VK_SUCCESS) throw std::runtime_error("error in debug callback creation");
		}
		~CDdebugReportCallback() {
			pfn_vkDestroyDebugReportCallbackEXT(mInstance, mCallback, nullptr);
		}
	private:
		VkDebugReportCallbackEXT mCallback; VkInstance mInstance;
		VkResult pfn_vkCreateDebugReportCallbackEXT(VkInstance, const VkDebugReportCallbackCreateInfoEXT*, const VkAllocationCallbacks*, VkDebugReportCallbackEXT*);
		void pfn_vkDestroyDebugReportCallbackEXT(VkInstance, VkDebugReportCallbackEXT, const VkAllocationCallbacks *);
	};
	class memoryBuffers
	{
	public:
		memoryBuffers(const renderer& p);
		~memoryBuffers();
		void createGPUBuffer(VkDeviceSize bufferSize, const void * bufferData, const std::string bufferName, VkBufferUsageFlagBits usage);
		void createGPUImage(VkDeviceSize imageSize, const void * imageData, uint32_t imageWidth, uint32_t imageHeight, const std::string imageName);
		void createUniformBuffer(VkDeviceSize bufferSize, const std::string bufferName);
		void createDepthBuffer(uint32_t imageWidth, uint32_t imageHeight, vk::Format depthFormat, const std::string imageName);

		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

		void init();
		VkBuffer & getBuffer(const std::string bufferName);

		std::unordered_map <std::string, bufferAllocation> buffers; // zvlastne, un map s structami( vkBuffer & vmaAllocation)
		std::unordered_map <std::string, imageAllocation> images;
	//private:
		const renderer & parent;
		VmaAllocator allocator;
		// zatial
		// bufferAllocation;
	};
	
	// other functions
	void information(const char * info);

	// glfw handles
	GLFWwindow * mWindow;

	// global vulkan handles
	vk::UniqueInstance mInstance;

#ifdef DEBUG
	CDdebugReportCallback mDebugCallback; // raii debug callback object
#endif // DEBUG
	vk::PhysicalDevice mPhysicalDevice;
	vk::UniqueDevice mDevice;
	vk::Queue mQueue;
	uint32_t mQueueFamilyIndex = -1;
	vk::SurfaceKHR mWindowSurface; // glfw will cleanup window surface

	vk::UniqueSwapchainKHR mSwapchain;
	std::vector<vk::UniqueImageView> mSwapchainImageViews;
	vk::Format mSwapchainImageFormat;
	vk::Extent2D mSwapchainExtent;

	vk::UniqueRenderPass mRenderPass;

	vk::UniqueCommandPool mCommandPool;

	vk::UniqueImageView mDepthBufferImageView;

	std::vector<vk::UniqueFramebuffer> mSwapchainFramebuffers;
	memoryBuffers buffers;

	vk::UniqueImageView mImageView;
	vk::UniqueSampler mImageSampler;

	vk::UniqueDescriptorSetLayout mDescriptorSetLayout;
	vk::UniqueDescriptorPool mDescriptorPool;
	std::vector<vk::DescriptorSet> mDescriptorSets;

	vk::UniquePipelineLayout mPipelineLayout;
	vk::UniquePipeline mPipeline;
	
	vk::UniqueSemaphore mSemaphore_NextImageAvailable;
	vk::UniqueSemaphore mSemaphore_RenderFinished;

	std::vector<vk::UniqueCommandBuffer> mCommandBuffers;
	
	// const
	const int WIN_WIDTH = 640, WIN_HEIGHT = 480;

	// random code lol
	void random();
};

template <typename T>
class vulkanObj
{
public:
	vulkanObj(T obj, std::function<void(T&)> deleter)
		:object(obj), deleter(deleter)
	{

	}

	~vulkanObj()
	{
		deleter(object);
	}

private:
	std::function<void(T&)> deleter;
	T object;
};

class vulkan_setup_error : public std::runtime_error
{
	typedef std::runtime_error _Mybase;
public:
	explicit vulkan_setup_error(const std::string& _Message)
		: _Mybase(_Message.c_str())
	{	// construct from message string
	}

	explicit vulkan_setup_error(const char *_Message)
		: _Mybase(_Message)
	{	// construct from message string
	}
};

class super
{
	virtual void aaa() = 0;
};
class sub : public super
{

};