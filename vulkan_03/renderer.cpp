#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "renderer.h"

renderer::renderer() : buffers(*this)
{
	
}

renderer::~renderer()
{
	glfwDestroyWindow(mWindow);
}

void renderer::initialize()
{
	try
	{
		//createWindow();
		createInstance();
		createDebug();
		createDevice();
		createSwapChain();
		createRenderPass();

		createCommandPool();
		createDepthBuffer();

		createFramebuffers();

		createImage();

		createDescriptors();
		createGraphicsPipeline();
		
		createBuffers();
		
		createCommandBuffers();
		createSemaphores();
	}
	catch (vulkan_setup_error& e)
	{
		std::cout << "ERROR(vulkan setup error): " << e.what() << std::endl;
		throw std::exception("vulkan setup failed");
	}
}

void renderer::frame()
{
	/*while (!glfwWindowShouldClose(mWindow))
	{
		glfwPollEvents();

		drawFrame();

		updateMatrix();

		vkDeviceWaitIdle(mDevice.get());
	}*/
	drawFrame();

	vkDeviceWaitIdle(mDevice.get());
}

void renderer::createWindow()
{
	if (glfwInit() == GLFW_FALSE) throw std::runtime_error("failed to create window!");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mWindow = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "vulkanEngine", nullptr, nullptr);

	glfwSetWindowUserPointer(mWindow, this);
}

void renderer::createInstance()
{
	auto vkMakeVersion = [](const uint32_t major, const uint32_t minor, const uint32_t patch)->uint32_t { return ((major) << 22) | ((minor) << 12) | (patch); };

	auto applicationInfo = vk::ApplicationInfo()
		.setApiVersion(vkMakeVersion(1,0,61))
		.setApplicationVersion(vkMakeVersion(0, 1, 0))
		.setPApplicationName("vulkan test 3")
		.setPEngineName("no engine");
		
	auto instanceInfo = vk::InstanceCreateInfo()
		.setPApplicationInfo(&applicationInfo);
	
#ifdef DEBUG
	const std::vector<const char *> enabledLayers{ "VK_LAYER_LUNARG_standard_validation" };
	instanceInfo
		.setEnabledLayerCount( static_cast<uint32_t>(enabledLayers.size()) )
		.setPpEnabledLayerNames(enabledLayers.data());
#endif // DEBUG

	auto instanceExtensions = getInstanceExtensions();
	instanceInfo
		.setEnabledExtensionCount(static_cast<uint32_t>(instanceExtensions.size()))
		.setPpEnabledExtensionNames(instanceExtensions.data());

	mInstance = vk::createInstanceUnique(instanceInfo); 
}

void renderer::createDebug()
{
#ifdef DEBUG
	VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfo = {};
	debugReportCallbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
	debugReportCallbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_INFORMATION_BIT_EXT; //| VK_DEBUG_REPORT_DEBUG_BIT_EXT;
	debugReportCallbackInfo.pfnCallback = debugCallback;

	mDebugCallback.create(mInstance.get(), &debugReportCallbackInfo);
#endif // DEBUG
}
void renderer::createDevice()
{
	auto physicalDevices = mInstance.get().enumeratePhysicalDevices();
	
	//select first device for now
	mPhysicalDevice = physicalDevices[0];

	//select queue family (first queue family with presentation and graphics support)
	auto DeviceQueueProperties = mPhysicalDevice.getQueueFamilyProperties();

	for (uint32_t i = 0; i < DeviceQueueProperties.size(); ++i )
	{
		if ((DeviceQueueProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) && glfwGetPhysicalDevicePresentationSupport(mInstance.get(), mPhysicalDevice, i))
		{
			mQueueFamilyIndex = i;
		}
	}
	if (mQueueFamilyIndex == -1) throw vulkan_setup_error("suitable queue family on GPU 0 not found.");

	float queuePriorities[]{ 1.0f };
	auto deviceQueueInfo = vk::DeviceQueueCreateInfo()
		.setQueueCount(1)
		.setQueueFamilyIndex(mQueueFamilyIndex)
		.setPQueuePriorities(queuePriorities);

	// we need extensions for swap chain enabled
	const std::vector<const char *> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	auto features = vk::PhysicalDeviceFeatures()
		.setFillModeNonSolid(true);

	auto deviceInfo = vk::DeviceCreateInfo()
		.setQueueCreateInfoCount(1)
		.setPQueueCreateInfos(&deviceQueueInfo)
		.setEnabledExtensionCount(static_cast<uint32_t>(deviceExtensions.size()))
		.setPpEnabledExtensionNames(deviceExtensions.data())
		.setPEnabledFeatures(&features);

	mDevice = mPhysicalDevice.createDeviceUnique(deviceInfo);

	mQueue = mDevice.get().getQueue(mQueueFamilyIndex, 0);

	// zatial sem
	buffers.init();
}

void renderer::createSwapChain()
{
	{
		VkSurfaceKHR surface;
		glfwCreateWindowSurface(mInstance.get(), mWindow, nullptr, &surface);
		mWindowSurface = surface;
	}
	
	if ( !mPhysicalDevice.getSurfaceSupportKHR(mQueueFamilyIndex, mWindowSurface) ) throw vulkan_setup_error("mWindowSurface not supported.");

	auto surfaceCapabilities = mPhysicalDevice.getSurfaceCapabilitiesKHR(mWindowSurface);
	auto surfaceFormats = mPhysicalDevice.getSurfaceFormatsKHR(mWindowSurface);
	auto surfacePresentModes = mPhysicalDevice.getSurfacePresentModesKHR(mWindowSurface);

	auto chooseSurfaceFormat = [&surfaceFormats] {
		// we need VK_FORMAT_B8G8R8A8_UNORM and VK_COLORSPACE_SRGB_NONLINEAR_KHR (if not found return first vector entry)
		for (const auto & surfaceFormat : surfaceFormats)
		{
			if ((surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) && (surfaceFormat.format == vk::Format::eB8G8R8A8Unorm))
			{
				return surfaceFormat;
			}
		}
		// or return default
		return surfaceFormats[0];
	};
	auto choosePresentMode = [&surfacePresentModes] {
		auto bestMode = vk::PresentModeKHR::eFifo;
		for (const auto & presentMode : surfacePresentModes)
		{
			if (presentMode == vk::PresentModeKHR::eMailbox)
			{
				return presentMode;
			}
			else if (presentMode == vk::PresentModeKHR::eImmediate)
			{
				bestMode = presentMode;
			}
		}
		return bestMode;
	};

	// standard code for choosing image count
	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
	{
		imageCount = surfaceCapabilities.maxImageCount;
	}

	auto surfaceFormat = chooseSurfaceFormat();

	auto swapchainInfo = vk::SwapchainCreateInfoKHR()
		.setMinImageCount(imageCount)
		.setImageColorSpace(surfaceFormat.colorSpace)
		.setImageFormat(surfaceFormat.format)
		.setSurface(mWindowSurface)
		.setImageExtent(surfaceCapabilities.currentExtent)
		.setImageArrayLayers(1)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
		.setImageSharingMode(vk::SharingMode::eExclusive)
		.setQueueFamilyIndexCount(0)
		.setPQueueFamilyIndices(nullptr)
		.setPreTransform(surfaceCapabilities.currentTransform)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setPresentMode(choosePresentMode())
		.setClipped(VK_TRUE);

	mSwapchain = mDevice.get().createSwapchainKHRUnique(swapchainInfo);

	auto swapChainImages = mDevice.get().getSwapchainImagesKHR(mSwapchain.get());

	// create swap chain image views
	mSwapchainImageViews.resize(swapChainImages.size());
	for (uint32_t i = 0; i < swapChainImages.size(); ++i)
	{
		createImageView(swapChainImages[i], surfaceFormat.format, vk::ImageAspectFlagBits::eColor, mSwapchainImageViews[i]);
	}

	mSwapchainImageFormat = surfaceFormat.format;
	mSwapchainExtent = surfaceCapabilities.currentExtent;

}

void renderer::createRenderPass()
{
	auto colorAttachement = vk::AttachmentDescription()
		.setFormat(mSwapchainImageFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	auto colorAttachementRef = vk::AttachmentReference()
		.setAttachment(0)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	
	auto depthAttachemnt = vk::AttachmentDescription()
		.setFormat(vk::Format::eD32Sfloat)  // does not need to be supperted, needs a proper function to check support
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	auto depthAttachementRef = vk::AttachmentReference()
		.setAttachment(1)
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
		
	auto subpass = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(1)
		.setPColorAttachments(&colorAttachementRef)
		.setPDepthStencilAttachment(&depthAttachementRef);

	std::array<vk::AttachmentDescription, 2> attachements = { colorAttachement, depthAttachemnt };

	auto renderPassInfo = vk::RenderPassCreateInfo()
		.setSubpassCount(1)
		.setPSubpasses(&subpass)
		.setAttachmentCount(static_cast<uint32_t>(attachements.size()))
		.setPAttachments(attachements.data());

	mRenderPass = mDevice.get().createRenderPassUnique(renderPassInfo);
}

void renderer::createFramebuffers()
{
	mSwapchainFramebuffers.resize(mSwapchainImageViews.size());
	for (size_t i = 0; i < mSwapchainFramebuffers.size(); ++i)
	{
		std::array<vk::ImageView, 2> attachments = { mSwapchainImageViews[i].get(), mDepthBufferImageView.get() };

		auto framebufferInfo = vk::FramebufferCreateInfo()
			.setRenderPass(mRenderPass.get())
			.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
			.setPAttachments(attachments.data())
			.setWidth(mSwapchainExtent.width)
			.setHeight(mSwapchainExtent.height)
			.setLayers(1); // refers to the number of layers in image arrays. Our swap chain images are single images, so the number of layers is 1
		mSwapchainFramebuffers[i] = mDevice.get().createFramebufferUnique(framebufferInfo);
	}
}

void renderer::createDescriptors()
{
	// descriptor set layout
	auto descriptorSetLayoutBinding = vk::DescriptorSetLayoutBinding()
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);
	auto samplerLayoutBinding = vk::DescriptorSetLayoutBinding()
		.setBinding(1)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	std::array<vk::DescriptorSetLayoutBinding, 2> layoutBindings = { descriptorSetLayoutBinding, samplerLayoutBinding };
	auto layoutCreateInfo = vk::DescriptorSetLayoutCreateInfo()
		.setBindingCount(static_cast<uint32_t>(layoutBindings.size()))
		.setPBindings(layoutBindings.data());

	mDescriptorSetLayout = mDevice.get().createDescriptorSetLayoutUnique(layoutCreateInfo);

	// uniform buffer
	buffers.createUniformBuffer(sizeof(glm::mat4), "MVP uniform buffer");

	// descriptor pool
	std::array<vk::DescriptorPoolSize, 2> poolSizes;
	poolSizes[0] = vk::DescriptorPoolSize()
		.setType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(1);
	poolSizes[1] = vk::DescriptorPoolSize()
		.setType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(1);

	auto poolCreateInfo = vk::DescriptorPoolCreateInfo()
		.setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
		.setPPoolSizes(poolSizes.data())
		.setMaxSets(1);

	mDescriptorPool = mDevice.get().createDescriptorPoolUnique(poolCreateInfo);

	// descriptor set
	auto descriptorSetAllocInfo = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(mDescriptorPool.get())
		.setDescriptorSetCount(1)
		.setPSetLayouts(&mDescriptorSetLayout.get());

	mDescriptorSets = mDevice.get().allocateDescriptorSets(descriptorSetAllocInfo);

	auto descriptorBufferInfo = vk::DescriptorBufferInfo()
		.setBuffer(buffers.getBuffer("MVP uniform buffer"))
		.setOffset(0)
		.setRange(sizeof(glm::mat4));
	auto DescriptorImageInfo = vk::DescriptorImageInfo()
		.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
		.setImageView(mImageView.get())
		.setSampler(mImageSampler.get());

	std::array<vk::WriteDescriptorSet, 2> descriptorWrites;
	descriptorWrites[0] = vk::WriteDescriptorSet()
		.setDstSet(mDescriptorSets[0])
		.setDstBinding(0)
		.setDstArrayElement(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setPBufferInfo(&descriptorBufferInfo);
	descriptorWrites[1] = vk::WriteDescriptorSet()
		.setDstSet(mDescriptorSets[0])
		.setDstBinding(1)
		.setDstArrayElement(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setPImageInfo(&DescriptorImageInfo);

	mDevice.get().updateDescriptorSets(descriptorWrites, nullptr);
}

void renderer::createGraphicsPipeline()
{
	auto fragCode = readShaderCode("shaders/frag.spv");
	auto vertCode = readShaderCode("shaders/vert.spv");
	auto shaderFrag = createShaderModule(fragCode);
	auto shaderVert = createShaderModule(vertCode);

	auto fragShaderStageInfo = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(shaderFrag)
		.setPName("main");
	auto vertShaderStageInfo = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(shaderVert)
		.setPName("main");
	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// fixed pipeline functions
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	auto vertexInputStateInfo = vk::PipelineVertexInputStateCreateInfo()
		.setVertexBindingDescriptionCount(1)
		.setPVertexBindingDescriptions(&bindingDescription)
		.setVertexAttributeDescriptionCount(static_cast<uint32_t>(attributeDescriptions.size()))
		.setPVertexAttributeDescriptions(attributeDescriptions.data());

	auto inputAssemblyStateInfo = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(vk::PrimitiveTopology::eTriangleList)
		.setPrimitiveRestartEnable(VK_FALSE);

	auto viewport = vk::Viewport()
		.setX(0.0f)
		.setY(0.0f)
		.setWidth(static_cast<float>(mSwapchainExtent.width))
		.setHeight(static_cast<float>(mSwapchainExtent.height))
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);

	auto scissor = vk::Rect2D()
		.setOffset({ 0, 0 })
		.setExtent(mSwapchainExtent);

	auto viewportStateInfo = vk::PipelineViewportStateCreateInfo()
		.setViewportCount(1)
		.setPViewports(&viewport)
		.setScissorCount(1)
		.setPScissors(&scissor);

	auto rasterizerInfo = vk::PipelineRasterizationStateCreateInfo()
		.setDepthClampEnable(VK_FALSE)
		.setRasterizerDiscardEnable(VK_FALSE)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setLineWidth(1.0f)
		//.setCullMode(vk::CullModeFlagBits::eBack)
		//.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setDepthBiasEnable(VK_FALSE)
		.setDepthBiasConstantFactor(0.0f)
		.setDepthBiasClamp(0.0f)
		.setDepthBiasSlopeFactor(0.0f);

	auto multisampling = vk::PipelineMultisampleStateCreateInfo()
		.setSampleShadingEnable(VK_FALSE)
		.setRasterizationSamples(vk::SampleCountFlagBits::e1);

	auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState()
		.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
		.setBlendEnable(VK_FALSE);

	auto colorBlending = vk::PipelineColorBlendStateCreateInfo()
		.setLogicOpEnable(VK_FALSE)
		.setAttachmentCount(1)
		.setPAttachments(&colorBlendAttachment);

	auto depthStencil = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(VK_TRUE)
		.setDepthWriteEnable(VK_TRUE)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setDepthBoundsTestEnable(VK_FALSE)
		.setStencilTestEnable(VK_FALSE);

	auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
		.setSetLayoutCount(1)
		.setPSetLayouts(&mDescriptorSetLayout.get());

	mPipelineLayout = mDevice.get().createPipelineLayoutUnique(pipelineLayoutInfo);

	auto pipelineInfo = vk::GraphicsPipelineCreateInfo()
		.setStageCount(2)
		.setPStages(shaderStages)
		.setPVertexInputState(&vertexInputStateInfo)
		.setPInputAssemblyState(&inputAssemblyStateInfo)
		.setPViewportState(&viewportStateInfo)
		.setPRasterizationState(&rasterizerInfo)
		.setPMultisampleState(&multisampling)
		.setPDepthStencilState(&depthStencil)
		.setPColorBlendState(&colorBlending)

		.setLayout(mPipelineLayout.get())

		.setRenderPass(mRenderPass.get())
		.setSubpass(0);
	
	mPipeline = mDevice.get().createGraphicsPipelineUnique(vk::PipelineCache(), pipelineInfo);

	mDevice.get().destroyShaderModule(shaderFrag);
	mDevice.get().destroyShaderModule(shaderVert);
}

void renderer::createCommandPool()
{
	auto commandPoolInfo = vk::CommandPoolCreateInfo()
		.setQueueFamilyIndex(mQueueFamilyIndex);
	mCommandPool = mDevice.get().createCommandPoolUnique(commandPoolInfo);
}

void renderer::createCommandBuffers()
{
	// command buffers for each framebuffer
	mCommandBuffers.resize(mSwapchainFramebuffers.size());
	auto commandBuffersInfo = vk::CommandBufferAllocateInfo()
		.setCommandPool(mCommandPool.get())
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandBufferCount(static_cast<uint32_t>(mCommandBuffers.size()));
	mCommandBuffers = mDevice.get().allocateCommandBuffersUnique(commandBuffersInfo);

	for (size_t i = 0; i < mCommandBuffers.size(); ++i)
	{
		auto beginInfo = vk::CommandBufferBeginInfo();
		mCommandBuffers[i].get().begin(beginInfo);

		std::array<vk::ClearValue, 2> clearValues;
		clearValues[0] = vk::ClearValue()
			.setColor(vk::ClearColorValue(std::array<float, 4>({ 0.0f, 0.0f, 0.0f, 1.0f })));
		clearValues[1] = vk::ClearValue()
			.setDepthStencil(vk::ClearDepthStencilValue(1.0f));

		auto renderPassBeginInfo = vk::RenderPassBeginInfo()
			.setRenderPass(mRenderPass.get())
			.setFramebuffer(mSwapchainFramebuffers[i].get())
			.setRenderArea(
				vk::Rect2D()
				.setExtent(mSwapchainExtent)
				.setOffset({ 0,0 })
			)
			.setClearValueCount(static_cast<uint32_t>(clearValues.size()))
			.setPClearValues(clearValues.data());
		
		mCommandBuffers[i].get().beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		mCommandBuffers[i].get().bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline.get());
		std::array<vk::Buffer, 1>vBuffers = { buffers.getBuffer("vertexBuffer") }; std::array<vk::DeviceSize, 1> offsets = { 0 };
		mCommandBuffers[i].get().bindVertexBuffers(0, vBuffers, offsets);
		mCommandBuffers[i].get().bindIndexBuffer(buffers.getBuffer("indexBuffer"), 0, vk::IndexType::eUint16);
		std::vector<uint32_t> arr;
		mCommandBuffers[i].get().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mPipelineLayout.get(), 0, mDescriptorSets, arr);
		mCommandBuffers[i].get().drawIndexed(indices.size(), 1, 0, 0, 0);
		mCommandBuffers[i].get().endRenderPass();

		mCommandBuffers[i].get().end();
	}
}

void renderer::createSemaphores()
{
	auto semaphoreInfo = vk::SemaphoreCreateInfo();
	mSemaphore_NextImageAvailable = mDevice.get().createSemaphoreUnique(semaphoreInfo);
	mSemaphore_RenderFinished = mDevice.get().createSemaphoreUnique(semaphoreInfo);
}

void renderer::createDepthBuffer()
{
	vk::Format depthFormat = vk::Format::eD32Sfloat;// hardcoded format (change later with format lookup function)
	buffers.createDepthBuffer(mSwapchainExtent.width, mSwapchainExtent.height, depthFormat, "depthBuffer");
	createImageView(buffers.images["depthBuffer"].image, depthFormat, vk::ImageAspectFlagBits::eDepth, mDepthBufferImageView);
}

void renderer::createBuffers()
{
	buffers.createGPUBuffer(sizeof(verticesSquare[0]) * verticesSquare.size(), verticesSquare.data(), "vertexBuffer", VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	buffers.createGPUBuffer(sizeof(indices[0]) * indices.size(), indices.data(), "indexBuffer", VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void renderer::createImage()
{
	// create image data
	int32_t x, y, comp;
	stbi_uc * imageData = stbi_load("textures/texture.jpg", &x, &y, &comp, STBI_rgb_alpha);

	buffers.createGPUImage(x*y*4, imageData, (uint32_t)x, (uint32_t)y, "imageTexture");

	stbi_image_free(imageData);

	//create image view
	auto imageViewInfo = vk::ImageViewCreateInfo()
		.setImage(buffers.images["imageTexture"].image)
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(vk::Format::eR8G8B8A8Unorm)
		.setSubresourceRange(vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1)
		);

	mImageView = mDevice.get().createImageViewUnique(imageViewInfo);

	//create image sampler
	auto samplerInfo = vk::SamplerCreateInfo()
		.setMagFilter(vk::Filter::eLinear)
		.setMinFilter(vk::Filter::eLinear)
		.setAddressModeU(vk::SamplerAddressMode::eRepeat)
		.setAddressModeV(vk::SamplerAddressMode::eRepeat)
		.setAddressModeW(vk::SamplerAddressMode::eRepeat)
		.setAnisotropyEnable(VK_TRUE)
		.setMaxAnisotropy(1.0f)
		.setBorderColor(vk::BorderColor::eIntOpaqueBlack)
		.setUnnormalizedCoordinates(VK_FALSE)
		.setCompareEnable(VK_FALSE)
		.setCompareOp(vk::CompareOp::eAlways)
		.setMipmapMode(vk::SamplerMipmapMode::eLinear);
	mImageSampler = mDevice.get().createSamplerUnique(samplerInfo);
}

void renderer::drawFrame()
{
	uint32_t imageIndex;
	mDevice.get().acquireNextImageKHR(mSwapchain.get(), std::numeric_limits<uint64_t>::max(), mSemaphore_NextImageAvailable.get(), vk::Fence(), &imageIndex);

	vk::PipelineStageFlags PSflags = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	auto submitInfo = vk::SubmitInfo()
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&mSemaphore_NextImageAvailable.get())
		.setPWaitDstStageMask(&PSflags)
		.setCommandBufferCount(1)
		.setPCommandBuffers(&mCommandBuffers[imageIndex].get())
		.setSignalSemaphoreCount(1)
		.setPSignalSemaphores(&mSemaphore_RenderFinished.get());

	mQueue.submit(submitInfo, vk::Fence());

	auto presentInfo = vk::PresentInfoKHR()
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&mSemaphore_RenderFinished.get())
		.setSwapchainCount(1)
		.setPSwapchains(&mSwapchain.get())
		.setPImageIndices(&imageIndex);
	
	mQueue.presentKHR(presentInfo);
	
	//vkQueueWaitIdle(mQueue);
}

void renderer::updateMatrix(glm::mat4 view)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float>(currentTime - startTime).count();

	glm::mat4 model, projection;
	
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	model = glm::rotate(model, 0.8f * time, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 3.0f * time, glm::vec3(0.0f, 0.0f, 1.0f));

	//view = glm::lookAt(glm::vec3(0.0f, -3.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	projection = glm::perspective(glm::radians(45.0f), WIN_WIDTH / (float)WIN_HEIGHT, 0.1f, 500.0f);
	projection[1][1] *= -1;

	glm::mat4 mvp = projection * view * model;

	void * data;
	vmaMapMemory(buffers.allocator, buffers.buffers["MVP uniform buffer"].allocation, &data);
	memcpy(data, &mvp, sizeof(glm::mat4));
	vmaUnmapMemory(buffers.allocator, buffers.buffers["MVP uniform buffer"].allocation);
}

std::vector<const char*> renderer::getInstanceExtensions()
{
	std::vector<const char*> extensions;

	uint32_t extensionCount = 0;
	const char ** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

	extensions.resize(extensionCount);
	for (uint32_t i = 0; i < extensionCount; i++)
	{
		extensions[i] = (glfwExtensions[i]);
	}

#ifdef DEBUG
	extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif // DEBUG

	return extensions;
}

void renderer::createImageView(const vk::Image image, const vk::Format format, const vk::ImageAspectFlags aspectFlags, vk::UniqueImageView & imageView)
{
	auto imageViewInfo = vk::ImageViewCreateInfo()
		.setImage(image)
		.setFormat(format)
		.setViewType(vk::ImageViewType::e2D)
		.setSubresourceRange(
			vk::ImageSubresourceRange()
			.setAspectMask(aspectFlags)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1)
		);
	imageView = mDevice.get().createImageViewUnique(imageViewInfo);
}

std::vector<char> renderer::readShaderCode(const char * shaderPath)
{
	std::vector<char> buffer;
	std::ifstream shaderFile(shaderPath, std::ios::ate | std::ios::binary);
	if (!shaderFile.is_open()) throw vulkan_setup_error("failed to load shader file");

	size_t filesize = (size_t)shaderFile.tellg();
	buffer.resize(filesize);
	
	shaderFile.seekg(0);
	shaderFile.read(buffer.data(), filesize);

	shaderFile.close();

	return buffer;
}

vk::ShaderModule renderer::createShaderModule(std::vector<char>& shaderCode)
{
	auto shaderModuleInfo = vk::ShaderModuleCreateInfo()
		.setCodeSize(shaderCode.size())
		.setPCode(reinterpret_cast<uint32_t*>(shaderCode.data()));

	return mDevice.get().createShaderModule(shaderModuleInfo);
}

vk::CommandBuffer renderer::beginSingleTimeCommands() const
{
	auto allocInfo = vk::CommandBufferAllocateInfo()
		.setCommandBufferCount(1)
		.setCommandPool(mCommandPool.get())
		.setLevel(vk::CommandBufferLevel::ePrimary);

	std::vector<vk::CommandBuffer> commandBuffers;
	commandBuffers = mDevice.get().allocateCommandBuffers(allocInfo);
	vk::CommandBuffer commandBuffer = commandBuffers[0];

	auto beginInfo = vk::CommandBufferBeginInfo()
		.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	commandBuffer.begin(beginInfo);
	return commandBuffer;
}

void renderer::endSingleTimeCommands(vk::CommandBuffer commandBuffer) const
{
	commandBuffer.end();
	auto submitInfo = vk::SubmitInfo()
		.setCommandBufferCount(1)
		.setPCommandBuffers(&commandBuffer);
	mQueue.submit(submitInfo, vk::Fence());
	mQueue.waitIdle();

	mDevice.get().freeCommandBuffers(mCommandPool.get(), commandBuffer);
}

VKAPI_ATTR VkBool32 VKAPI_CALL renderer::debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{
	// bit old code
	std::cout << "--VALIDATION_LAYER--";

	switch (flags)
	{
	case(VK_DEBUG_REPORT_INFORMATION_BIT_EXT):
	{
		std::cout << "INFORMATION--";
		break;
	}
	case(VK_DEBUG_REPORT_WARNING_BIT_EXT):
	{
		std::cout << "WARNING--";
		break;
	}
	case(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT):
	{
		std::cout << "PERFORMANCE_WARNING--";
		break;
	}
	case(VK_DEBUG_REPORT_ERROR_BIT_EXT):
	{
		std::cout << "ERROR--";
		break;
	}
	case(VK_DEBUG_REPORT_DEBUG_BIT_EXT):
	{
		std::cout << "DEBUG--";
		break;
	}
	default:
		break;
	}
	std::cout << "\n";

	if (flags &  VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		std::cout << msg << std::endl;
		throw std::runtime_error(msg);
	}
	std::cout << msg << std::endl;

	return VK_FALSE;
}
void renderer::information(const char * info)
{
	std::cout << info << std::endl;
}
void renderer::random()
{
	// testing code below (
	

	uint32_t kfd[] = { 0 };
	auto bci = vk::BufferCreateInfo()
		.setSize(20)
		.setQueueFamilyIndexCount(1)
		.setPQueueFamilyIndices(kfd)
		.setUsage(vk::BufferUsageFlagBits::eVertexBuffer);

	VK_MAX_MEMORY_HEAPS;
	auto mProp = mPhysicalDevice.getMemoryProperties();
	mProp.memoryHeaps[0].flags; VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	mProp.memoryTypes[0].propertyFlags;
	vk::MemoryPropertyFlagBits::eDeviceLocal;
	vk::MemoryPropertyFlagBits::eHostVisible;
	vk::MemoryPropertyFlagBits::eHostCoherent;
	vk::MemoryPropertyFlagBits::eHostCached;
	vk::MemoryPropertyFlagBits::eLazilyAllocated;

	VmaStats stats;
	vmaCalculateStats(buffers.allocator, &stats);
}

VkResult renderer::CDdebugReportCallback::pfn_vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDebugReportCallbackEXT * pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else throw std::runtime_error("failed to get external function \"vkCreateDebugReportCallbackEXT\" ");
}

void renderer::CDdebugReportCallback::pfn_vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks * pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr)
	{
		func(instance, callback, pAllocator);
	}
	else throw std::runtime_error("failed to get external function \"vkDestroyDebugReportCallbackEXT\" ");
}

renderer::memoryBuffers::memoryBuffers(const renderer & p) : parent(p)
{
}

renderer::memoryBuffers::~memoryBuffers()
{
	for (const auto & it : buffers)
	{
		std::cout << "destroyed buffer with name: " << it.first << std::endl;
		vmaDestroyBuffer(allocator, it.second.buffer, it.second.allocation);
	}
	for (const auto & it : images)
	{
		std::cout << "destroyed image with name: " << it.first << std::endl;
		vmaDestroyImage(allocator, it.second.image, it.second.allocation);
	}
	vmaDestroyAllocator(allocator);
}

void renderer::memoryBuffers::createGPUBuffer(VkDeviceSize bufferSize, const void * bufferData, const std::string bufferName, VkBufferUsageFlagBits usage)
{
	VkBuffer stagingBuffer;
	VkBufferCreateInfo stagingBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	stagingBufferInfo.size = bufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VmaAllocation stagingBufferAllocation;
	VmaAllocationCreateInfo stagingBufferAllocationInfo = {};
	stagingBufferAllocationInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingBufferAllocationInfo, &stagingBuffer, &stagingBufferAllocation, nullptr);

	void * data;
	vmaMapMemory(allocator, stagingBufferAllocation, &data);
	memcpy(data, bufferData, bufferSize);
	vmaUnmapMemory(allocator, stagingBufferAllocation);

	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
	VmaAllocationCreateInfo bufferAllocationInfo = {};
	bufferAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	bufferAllocation buffer;
	vmaCreateBuffer(allocator, &bufferInfo, &bufferAllocationInfo, &buffer.buffer, &buffer.allocation, nullptr);
	buffers.emplace(bufferName, buffer);

	vk::CommandBuffer commandBuffer = parent.beginSingleTimeCommands();
	auto copyRegion = vk::BufferCopy()
		.setSize(bufferSize);
	commandBuffer.copyBuffer(stagingBuffer, buffers[bufferName].buffer, copyRegion);
	parent.endSingleTimeCommands(commandBuffer);
	
	vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);
}

void renderer::memoryBuffers::createGPUImage(VkDeviceSize imageSize, const void * imageData, uint32_t imageWidth, uint32_t imageHeight, const std::string imageName)
{
	// create staging buffer
	VkBuffer stagingBuffer;
	VkBufferCreateInfo stagingBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	stagingBufferInfo.size = imageSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VmaAllocation stagingBufferAllocation;
	VmaAllocationCreateInfo stagingBufferAllocationInfo = {};
	stagingBufferAllocationInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingBufferAllocationInfo, &stagingBuffer, &stagingBufferAllocation, nullptr);

	void * data;
	vmaMapMemory(allocator, stagingBufferAllocation, &data);
	memcpy(data, imageData, imageSize);
	vmaUnmapMemory(allocator, stagingBufferAllocation);
	
	auto imageInfo = vk::ImageCreateInfo()
		.setFormat(vk::Format::eR8G8B8A8Unorm)
		.setImageType(vk::ImageType::e2D)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setArrayLayers(1)
		.setMipLevels(1)
		.setTiling(vk::ImageTiling::eLinear)
		.setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setExtent(vk::Extent3D()
			.setHeight(imageHeight)
			.setWidth(imageWidth)
			.setDepth(1)
		);

	VmaAllocationCreateInfo imageAllocationInfo = {};
	imageAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	imageAllocation image;
	vmaCreateImage(allocator, &(VkImageCreateInfo)imageInfo, &imageAllocationInfo, &image.image, &image.allocation, nullptr);

	transitionImageLayout(image.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	vk::CommandBuffer commandBuffer = parent.beginSingleTimeCommands();
	auto region = vk::BufferImageCopy()
		.setImageSubresource(vk::ImageSubresourceLayers()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setLayerCount(1)
		)
		.setImageExtent({ imageWidth, imageHeight, 1 });
	commandBuffer.copyBufferToImage(stagingBuffer, image.image, vk::ImageLayout::eTransferDstOptimal, 1, &region);
	parent.endSingleTimeCommands(commandBuffer);

	transitionImageLayout(image.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	images.emplace(imageName, image);

	vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);
}

void renderer::memoryBuffers::createUniformBuffer(VkDeviceSize bufferSize, const std::string bufferName)
{
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo bufferAllocationInfo = {};
	bufferAllocationInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	bufferAllocation buffer;
	vmaCreateBuffer(allocator, &bufferInfo, &bufferAllocationInfo, &buffer.buffer, &buffer.allocation, nullptr);
	buffers.emplace(bufferName, buffer);
}

void renderer::memoryBuffers::createDepthBuffer(uint32_t imageWidth, uint32_t imageHeight, vk::Format depthFormat, const std::string imageName)
{
	auto imageInfo = vk::ImageCreateInfo()
		.setFormat(depthFormat)
		.setImageType(vk::ImageType::e2D)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setArrayLayers(1)
		.setMipLevels(1)
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setExtent(vk::Extent3D()
			.setHeight(imageHeight)
			.setWidth(imageWidth)
			.setDepth(1)
		);

	VmaAllocationCreateInfo imageAllocationInfo = {};
	imageAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	imageAllocation image;
	vmaCreateImage(allocator, &(VkImageCreateInfo)imageInfo, &imageAllocationInfo, &image.image, &image.allocation, nullptr);

	transitionImageLayout(image.image, (VkFormat)depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	images.emplace(imageName, image);
}

void renderer::memoryBuffers::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = parent.beginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	parent.endSingleTimeCommands(commandBuffer);
}

void renderer::memoryBuffers::init()
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = parent.mPhysicalDevice;
	allocatorInfo.device = parent.mDevice.get();

	vmaCreateAllocator(&allocatorInfo, &allocator);
}

VkBuffer & renderer::memoryBuffers::getBuffer(const std::string bufferName)
{
	return buffers[bufferName].buffer;
}
