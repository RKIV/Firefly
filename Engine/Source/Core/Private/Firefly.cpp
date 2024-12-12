#define STB_IMAGE_IMPLEMENTATION

#include "Firefly.h"

#include <cassert>
#include <stdexcept>
#include <format>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>

// #include <vulkan/vk_enum_string_helper.h>

VkVertexInputBindingDescription Vertex::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

	return attributeDescriptions;
}

void Engine::run()
{
	initWindow();
	initGraphics();

	mainLoop();

	cleanupGraphics();
	cleanupWindow();
}

void Engine::mainLoop()
{
	// TODO: Delta time
	
	while (!windowCloseRequested)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();
		static auto previousTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - previousTime).count();

		SDL_Event event; 
		while(SDL_PollEvent(&event))
		{
			processEvent(event);
		}
		
		if(EKeyDown && !QKeyDown)
		{
			cameraPosition.z += deltaTime * 1.f;
		}
		else if(QKeyDown && !EKeyDown)
		{
			cameraPosition.z -= deltaTime * 1.f;
		}

		if(AKeyDown && !DKeyDown)
		{
			cameraPosition.y += deltaTime * 1.f;
		}
		else if(DKeyDown && !AKeyDown)
		{
			cameraPosition.y -= deltaTime * 1.f;
		}

		if(WKeyDown && !SKeyDown)
		{
			cameraPosition.x += deltaTime * 1.f;
		}
		else if(SKeyDown && !WKeyDown)
		{
			cameraPosition.x -= deltaTime * 1.f;
		}
		
		drawFrame(deltaTime);
		
		previousTime = currentTime;
	}

	vkDeviceWaitIdle(vulkanDevice);
}

void Engine::processEvent(const SDL_Event& event)
{
	switch(event.type)
	{
	case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
	{
		windowCloseRequested = true;
		break;
	}
	case SDL_EVENT_WINDOW_RESIZED:
		{
			frameBufferResized = true;
		}break;
	case SDL_EVENT_KEY_DOWN:
	case SDL_EVENT_KEY_UP:
		{
			bool* keyFlag = nullptr;
			switch(event.key.key)
			{
			case SDLK_W:
				{
					keyFlag = &WKeyDown;
				}break;
			case SDLK_S:
				{
					keyFlag = &SKeyDown;
				}break;
			case SDLK_A:
				{
					keyFlag = &AKeyDown;
				}break;
			case SDLK_D:
				{
					keyFlag = &DKeyDown;
				}break;
			case SDLK_Q:
				{
					keyFlag = &QKeyDown;
				}break;
			case SDLK_E:
				{
					keyFlag = &EKeyDown;

				}break;
			}
			
			if(keyFlag)
			{
				if(event.key.down)
				{
					*keyFlag = true;
				}
				else
				{
					*keyFlag = false;
				}
			}
		}break;
	default:
		{
		}break;
	}
}

void Engine::initWindow()
{
	SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);

	sdlWindow = SDL_CreateWindow("Firefly", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
}

void Engine::cleanupWindow()
{
	SDL_DestroyWindow(sdlWindow);

	SDL_Quit();
}

void Engine::drawFrame(float deltaTime)
{
	vkWaitForFences(vulkanDevice, 1, &inFlightFences[inFlightFrameIdx], VK_TRUE, UINT64_MAX);

	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();


	uint32_t swapChainImageIndex;
	VkResult acquireImageResult = vkAcquireNextImageKHR(vulkanDevice, vulkanSwapchain, UINT64_MAX, imageAvailableSemaphores[inFlightFrameIdx], VK_NULL_HANDLE, &swapChainImageIndex);
	if (acquireImageResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// Handle minimization of window
		int width = 0, height = 0;
		SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);
		while (width == 0 || height == 0) {
			SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);
			SDL_Event event;
			SDL_WaitEvent(&event);
			processEvent(event);
		}

		vkDeviceWaitIdle(vulkanDevice);

		for (VkImageView imageView : vulkanSwapchainImageViews)
		{
			vkDestroyImageView(vulkanDevice, imageView, nullptr);
		}
		vulkanSwapchainImageViews.clear();

		vkDestroySwapchainKHR(vulkanDevice, vulkanSwapchain, nullptr);
		vulkanSwapchain = VK_NULL_HANDLE;
		vulkanSwapchainImages.clear();

		createSwapChain();
		createImageViews();
	}
	else if (acquireImageResult != VK_SUBOPTIMAL_KHR)
	{
		VK_CHECK(acquireImageResult, "Failed to acquire next swapchain image: {}");
	}

	vkResetFences(vulkanDevice, 1, &inFlightFences[inFlightFrameIdx]);

	VkResult resetCommandBufferResult = vkResetCommandBuffer(vulkanGraphicsCommandBuffers[inFlightFrameIdx], 0);
	VK_CHECK(resetCommandBufferResult, "Failed to reset command buffer: {}");

	{
		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.f), modelRotationRad += deltaTime * glm::radians(50.f), glm::vec3(0.f, 0.f, 1.f));
		ubo.view = glm::lookAt(cameraPosition, glm::vec3(0.f, cameraPosition.y, 0.f), glm::vec3(0.f, 0.f, 1.f));
		ubo.proj = glm::perspective(glm::radians(45.f), vulkanSwapchainSurfaceExtent.width / (float)vulkanSwapchainSurfaceExtent.height, 0.1f, 10.f);
		// GLM originally designed for OpenGL with inverted Y coordinate
		ubo.proj[1][1] *= -1;
		memcpy(vulkanUniformBufferMappedMemory[inFlightFrameIdx], &ubo, sizeof(ubo));
	}

	/// CONSTRUCT COMMAND BUFFER
	{
		VkCommandBuffer inFlightCommandBuffer = vulkanGraphicsCommandBuffers[inFlightFrameIdx];

		VkCommandBufferBeginInfo commandBufferBeginInfo{};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkResult commandBufferBeginResult = vkBeginCommandBuffer(inFlightCommandBuffer, &commandBufferBeginInfo);

		VK_CHECK(commandBufferBeginResult, "Failed to begin recording command buffer: {}");

		VkImageMemoryBarrier colorImageMemoryBarrier{};
		colorImageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		colorImageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		colorImageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorImageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorImageMemoryBarrier.image = vulkanSwapchainImages[swapChainImageIndex];
		colorImageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorImageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		colorImageMemoryBarrier.subresourceRange.levelCount = 1;
		colorImageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		colorImageMemoryBarrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(
			vulkanGraphicsCommandBuffers[inFlightFrameIdx],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // TOP_OF_PIPE means wait for nothing, just immediately perform the image layout transition
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Block any writes to our color out until this layout transition is done
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&colorImageMemoryBarrier
		);

		VkClearValue clearValue = { {{0.f, 0.f, 0.f, 1.f}} };

		VkRenderingAttachmentInfo renderingAttachmentInfo{};
		renderingAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		renderingAttachmentInfo.imageView = vulkanSwapchainImageViews[swapChainImageIndex];
		renderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		renderingAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		renderingAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderingAttachmentInfo.clearValue = clearValue;

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.offset = { 0, 0 };
		renderingInfo.renderArea.extent = vulkanSwapchainSurfaceExtent;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &renderingAttachmentInfo;

		vkCmdBeginRendering(inFlightCommandBuffer, &renderingInfo);

		vkCmdBindPipeline(inFlightCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanGraphicsPipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(vulkanSwapchainSurfaceExtent.width);
		viewport.height = static_cast<float>(vulkanSwapchainSurfaceExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(inFlightCommandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = vulkanSwapchainSurfaceExtent;
		vkCmdSetScissor(inFlightCommandBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(inFlightCommandBuffer, 0, 1, &vulkanVertexBuffer, offsets);

		vkCmdBindIndexBuffer(inFlightCommandBuffer, vulkanIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdBindDescriptorSets(inFlightCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelineLayout, 0, 1, &vulkanDescriptorSets[inFlightFrameIdx], 0, nullptr);

		vkCmdDrawIndexed(inFlightCommandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		vkCmdEndRendering(inFlightCommandBuffer);

		VkImageMemoryBarrier presentImageMemoryBarrier{};
		presentImageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		presentImageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		presentImageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		presentImageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		presentImageMemoryBarrier.image = vulkanSwapchainImages[swapChainImageIndex];
		presentImageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		presentImageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		presentImageMemoryBarrier.subresourceRange.levelCount = 1;
		presentImageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		presentImageMemoryBarrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(
			inFlightCommandBuffer,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&presentImageMemoryBarrier
		);

		VkResult endCommandBufferResult = vkEndCommandBuffer(inFlightCommandBuffer);

		VK_CHECK(endCommandBufferResult, "Failed to construct command buffer: {}");
	}

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[inFlightFrameIdx] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSemaphore signalSempahores[] = { renderingFinishedSemaphores[inFlightFrameIdx] };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vulkanGraphicsCommandBuffers[inFlightFrameIdx];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSempahores;

	VkResult submitQuueResult = vkQueueSubmit(vulkanGraphicsQueue, 1, &submitInfo, inFlightFences[inFlightFrameIdx]);
	VK_CHECK(submitQuueResult, "Failed to submit draw command buffer: {}");

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSempahores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &vulkanSwapchain;
	presentInfo.pImageIndices = &swapChainImageIndex;

	VkResult presentResult = vkQueuePresentKHR(vulkanPresentQueue, &presentInfo);

	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || frameBufferResized)
	{
		frameBufferResized = false;

		int width = 0, height = 0;
		SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);
		while (width == 0 || height == 0) {
			SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);
			SDL_Event event;
			SDL_WaitEvent(&event);
			processEvent(event);
		}

		vkDeviceWaitIdle(vulkanDevice);

		for (VkImageView imageView : vulkanSwapchainImageViews)
		{
			vkDestroyImageView(vulkanDevice, imageView, nullptr);
		}
		vulkanSwapchainImageViews.clear();

		vkDestroySwapchainKHR(vulkanDevice, vulkanSwapchain, nullptr);
		vulkanSwapchain = VK_NULL_HANDLE;
		vulkanSwapchainImages.clear();

		createSwapChain();
		createImageViews();
	}
	else
	{
		VK_CHECK(presentResult, "Failed to present image: {}");
	}

	inFlightFrameIdx = (inFlightFrameIdx + 1) & MAX_FRAMES_IN_FLIGHT;
}

void Engine::initGraphics()
{

#ifndef NDEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
	debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugMessengerCreateInfo.pfnUserCallback = debugCallback;
	debugMessengerCreateInfo.pUserData = nullptr;
#endif

	/// CREATE VULKAN INSTANCE
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Firefly";
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.pEngineName = nullptr;
		appInfo.engineVersion = 0;
		appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);

		VkInstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &appInfo;


		std::vector<const char*> extensions;

		{
			uint32_t sdlExtensionCount;
			const char *const * sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

			if (sdlExtensions == nullptr)
			{
				throw std::runtime_error("Failed to retrieve GLFW requested Vulkan extensions");
			}

			uint32_t supportedExtensionCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);

			std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, supportedExtensions.data());

			extensions.reserve(sdlExtensionCount);

			for (uint32_t glfwExtensionIdx = 0; glfwExtensionIdx < sdlExtensionCount; glfwExtensionIdx++)
			{
				const char* glfwExtensionName = sdlExtensions[glfwExtensionIdx];

				bool extensionSupported = false;
				for (const VkExtensionProperties& extensionProperties : supportedExtensions)
				{
					if (std::strcmp(glfwExtensionName, extensionProperties.extensionName) == 0)
					{
						extensionSupported = true;
						extensions.push_back(glfwExtensionName);
						break;
					}
				}
				if (!extensionSupported)
				{
					throw std::runtime_error(std::format("GLFW request extension {} is unsupported", glfwExtensionName));
				}
			}

#ifndef NDEBUG
			{
				const char* debugUtilsExtension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
				bool debugUtilsExtensionSupported = false;
				for (const VkExtensionProperties& extensionPropertes : supportedExtensions)
				{
					if (std::strcmp(debugUtilsExtension, extensionPropertes.extensionName) == 0)
					{
						debugUtilsExtensionSupported = true;
						extensions.push_back(debugUtilsExtension);
						break;
					}
				}

				if (!debugUtilsExtensionSupported)
				{
					throw std::runtime_error(std::format("Debug Utils Extension ({}) not supported", debugUtilsExtension));
				}
			}
#endif
		}

		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

#ifndef NDEBUG
		{
			uint32_t supportedLayersCount;
			vkEnumerateInstanceLayerProperties(&supportedLayersCount, nullptr);

			std::vector<VkLayerProperties> supportedLayers(supportedLayersCount);
			vkEnumerateInstanceLayerProperties(&supportedLayersCount, supportedLayers.data());

			for (const char* layerName : desiredValidationLayers)
			{
				bool layerFound = false;
				for (const VkLayerProperties& layerProperties : supportedLayers)
				{
					if (std::strcmp(layerName, layerProperties.layerName) == 0)
					{
						layerFound = true;
						break;
					}
				}

				if (layerFound == false)
				{
					throw std::runtime_error(std::format("Desired validation layer {} is unsupported", layerName));
				}
			}

			instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(desiredValidationLayers.size());
			instanceCreateInfo.ppEnabledLayerNames = desiredValidationLayers.data();

			instanceCreateInfo.pNext = &debugMessengerCreateInfo;
		}
#else
		instanceCreateInfo.enabledLayerCount = 0;
#endif

		VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &vulkanInstance);
		VK_CHECK(result, "Failed to create instance: {}");
	}

#ifndef NDEBUG
	/// SETUP DEBUG MESSENGER
	{
		PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(vulkanInstance, "vkCreateDebugUtilsMessengerEXT"));

		assert(pfnVkCreateDebugUtilsMessengerEXT != nullptr);

		pfnVkCreateDebugUtilsMessengerEXT(vulkanInstance, &debugMessengerCreateInfo, nullptr, &debugMessenger);
	}
#endif

	// CREATE SURFACE
	{
		
		if(!SDL_Vulkan_CreateSurface(sdlWindow, vulkanInstance, nullptr, &vulkanSurface))
		{
			throw std::runtime_error(std::format("Failed to create window surface: {}", SDL_GetError()));
		}
	}

	// PICK PHYSICAL DEVICE
	{
		uint32_t availablePhysicalDeviceCount = 0;
		vkEnumeratePhysicalDevices(vulkanInstance, &availablePhysicalDeviceCount, nullptr);

		if (availablePhysicalDeviceCount == 0)
		{
			throw std::runtime_error("Failed to find valid Vulkan device");
		}

		std::vector<VkPhysicalDevice> availablePhysicalDevices(availablePhysicalDeviceCount);
		vkEnumeratePhysicalDevices(vulkanInstance, &availablePhysicalDeviceCount, availablePhysicalDevices.data());

		uint32_t bestPhysicalDeviceScore = 0;
		VkPhysicalDevice bestPhysicalDevice = VK_NULL_HANDLE;
		for (const VkPhysicalDevice& physicalDevice : availablePhysicalDevices)
		{
			uint32_t physicalDeviceScore = 1;

			// Check we have the necessary queue families
			{
				uint32_t queueFamilyCount = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

				std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
				vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

				bool hasPresentQueue = false;
				bool hasGraphicsQueue = false;
				bool hasDedicatedTransferQueue = false;
				for (uint32_t queueFamilyIdx = 0; queueFamilyIdx < queueFamilyCount; queueFamilyIdx++)
				{
					const VkQueueFamilyProperties& queueFamily = queueFamilies[queueFamilyIdx];

					if (!hasGraphicsQueue && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
					{
						hasGraphicsQueue = true;
					}
					else if (!hasDedicatedTransferQueue && ((queueFamily.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT)) == VK_QUEUE_TRANSFER_BIT))
					{
						hasDedicatedTransferQueue = true;
						physicalDeviceScore += 50;
					}
					if (!hasPresentQueue)
					{
						VkBool32 presentSupport = false;
						vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIdx, vulkanSurface, &presentSupport);
						if (presentSupport)
						{
							hasPresentQueue = true;
						}
					}


					if (hasGraphicsQueue && hasPresentQueue)
					{
						break;
					}
				}

				if (!hasGraphicsQueue || !hasPresentQueue)
				{
					continue;
				}
			}

			// Check to make sure we have the necessary device extensions
			{
				uint32_t deviceExtensionCount = 0;
				vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);

				std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
				vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data());

				for (const char* desiredExtension : desiredDeviceExtensions)
				{
					bool bExtensionsSupported = false;
					for (const VkExtensionProperties& extension : deviceExtensions)
					{
						if (std::strcmp(desiredExtension, extension.extensionName) == 0)
						{
							bExtensionsSupported = true;
							break;
						}
					}

					if (!bExtensionsSupported)
					{
						continue;
					}
				}
			}

			// Check to make sure swap chain is supported
			{
				uint32_t formatCount = 0;
				vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vulkanSurface, &formatCount, nullptr);

				uint32_t presentModeCount = 0;
				vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vulkanSurface, &presentModeCount, nullptr);

				if (formatCount == 0 || presentModeCount == 0)
				{
					continue;
				}
			}

			// Check if the device supports dynamic rendering
			{
				VkPhysicalDeviceDynamicRenderingFeatures physicalDeviceDynamicRenderingFeatures{};
				physicalDeviceDynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;

				VkPhysicalDeviceFeatures2 physicalDeviceFeatures{};
				physicalDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
				physicalDeviceFeatures.pNext = &physicalDeviceDynamicRenderingFeatures;
				vkGetPhysicalDeviceFeatures2(physicalDevice, &physicalDeviceFeatures);

				if (physicalDeviceDynamicRenderingFeatures.dynamicRendering == VK_FALSE)
				{
					continue;
				}

				if(physicalDeviceFeatures.features.samplerAnisotropy == VK_FALSE)
				{
					continue;
				}
			}


			VkPhysicalDeviceProperties physicalDeviceProperties;
			vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

			if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				physicalDeviceScore += 100;
			}

			if (physicalDeviceScore > bestPhysicalDeviceScore)
			{
				bestPhysicalDevice = physicalDevice;
			}
		}

		if (bestPhysicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Failed to find a suitable Vulkan device");
		}

		vulkanPhysicalDevice = bestPhysicalDevice;
	}

	/// CREATE LOGICAL DEVICE
	{
		std::optional<uint32_t> graphicsQueueIdx;
		std::optional<uint32_t> presentQueueIdx;
		std::optional<uint32_t> transferQueueIdx;
		bool dedicatedTransferQueue = false;

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		{
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(vulkanPhysicalDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(vulkanPhysicalDevice, &queueFamilyCount, queueFamilies.data());

			for (uint32_t queueFamilyIdx = 0; queueFamilyIdx < queueFamilyCount; queueFamilyIdx++)
			{
				const VkQueueFamilyProperties& queueFamily = queueFamilies[queueFamilyIdx];

				if (!graphicsQueueIdx.has_value() && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
				{
					graphicsQueueIdx = queueFamilyIdx;
				}
				if (transferQueueIdx.has_value() == false || dedicatedTransferQueue == false)
				{
					if (dedicatedTransferQueue == false && ((queueFamily.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT)) == VK_QUEUE_TRANSFER_BIT))
					{
						transferQueueIdx = queueFamilyIdx;
						dedicatedTransferQueue = true;
					}
					else if (transferQueueIdx.has_value() == false && (queueFamily.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT)) != 0)
					{
						transferQueueIdx = queueFamilyIdx;
					}
				}
				if (!presentQueueIdx.has_value())
				{
					VkBool32 presentSupport = false;
					vkGetPhysicalDeviceSurfaceSupportKHR(vulkanPhysicalDevice, queueFamilyIdx, vulkanSurface, &presentSupport);
					if (presentSupport)
					{
						presentQueueIdx = queueFamilyIdx;
					}
				}

				if (graphicsQueueIdx.has_value() && presentQueueIdx.has_value())
				{
					break;
				}
			}

			std::set<uint32_t> uniqueQueueFamilies = { graphicsQueueIdx.value(), presentQueueIdx.value(), transferQueueIdx.value() };

			const float queuePriority = 1.f;
			for (uint32_t queueFamilyIdx : uniqueQueueFamilies)
			{
				VkDeviceQueueCreateInfo queueCreateInfo{};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamilyIdx;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
			}
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeautres{};
		dynamicRenderingFeautres.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
		dynamicRenderingFeautres.dynamicRendering = VK_TRUE;

		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(desiredDeviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = desiredDeviceExtensions.data();

		deviceCreateInfo.pNext = &dynamicRenderingFeautres;

		VkResult result = vkCreateDevice(vulkanPhysicalDevice, &deviceCreateInfo, nullptr, &vulkanDevice);
		VK_CHECK(result, "Failed to create logical device: {}");

		vkGetDeviceQueue(vulkanDevice, graphicsQueueIdx.value(), 0, &vulkanGraphicsQueue);
		vkGetDeviceQueue(vulkanDevice, presentQueueIdx.value(), 0, &vulkanPresentQueue);
		vkGetDeviceQueue(vulkanDevice, transferQueueIdx.value(), 0, &vulkanTransferQueue);

		graphicsQueueFamilyIndex = graphicsQueueIdx.value();
		presentQueueFamilyIndex = presentQueueIdx.value();
		transferQueueFamilyIdx = transferQueueIdx.value();
	}

	/// CREATE SWAPCHAIN
	createSwapChain();

	/// CREATE IMAGE VIEWS
	createImageViews();

	/// CREATE GRAPHICS PIPELINE
	{
		auto readFile = [](const std::string& filename) -> std::vector<char>
			{
				std::ifstream file(filename, std::ios::ate | std::ios::binary);

				if (!file.is_open())
				{
					throw std::runtime_error(std::format("Failed to open file {}", filename));
				}

				size_t fileSize = (size_t)file.tellg();

				std::vector<char> buffer(fileSize);

				file.seekg(0);
				file.read(buffer.data(), fileSize);
				file.close();

				return buffer;
			};

		auto createShaderModule = [device = vulkanDevice](const std::vector<char>& code) -> VkShaderModule
			{
				VkShaderModuleCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				createInfo.codeSize = code.size();
				// Apparently vector will automatically 4 byte align our vector
				createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

				VkShaderModule shaderModule;

				VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
				VK_CHECK(result, "Failed to create shader module: {}")

				return shaderModule;
			};

		VkShaderModule vertShaderModule = createShaderModule(readFile("Shaders/vert.spv"));
		VkShaderModule fragShaderModule = createShaderModule(readFile("Shaders/frag.spv"));

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		// Entry point of shader
		vertShaderStageInfo.pName = "main";
		// pSpecializationInfo allows you to specify values for shader constants
		// You can use a single shader module where its behavior can be configured at pipeline creation by specifying different values for the constants used in it.
		// This is more efficient than configuring the shader using variables at render time, because the compiler can do optimizations like eliminating if statements that depend on these values.
		vertShaderStageInfo.pSpecializationInfo = nullptr;

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";
		fragShaderStageInfo.pSpecializationInfo = nullptr;

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		// Dynamic states allow you to change certain very specific parts of the piepline at runtime
		// Most of the pipeline is fixed after creation
		std::vector<VkDynamicState> dynamicStates =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicStateInfo.pDynamicStates = dynamicStates.data();

		VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = Vertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssemplyInfo{};
		inputAssemplyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemplyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		inputAssemplyInfo.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportStateInfo{};
		viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateInfo.viewportCount = 1;
		viewportStateInfo.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
		rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		// If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are clamped to them as opposed to discarding them.
		rasterizationInfo.depthClampEnable = VK_FALSE;
		// If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage.
		rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationInfo.lineWidth = 1.0f;
		rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		// The rasterizer can alter the depth values by adding a constant value or biasing them based on a fragment's slope
		rasterizationInfo.depthBiasEnable = VK_FALSE;

		// Combining the fragment shader results of multiple polygons that rasterize to the same pixel
		// This mainly occurs along edges, which is also where the most noticeable aliasing artifacts occur
		VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
		multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplingInfo.sampleShadingEnable = VK_FALSE;
		multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisamplingInfo.minSampleShading = 1.0f;
		multisamplingInfo.pSampleMask = nullptr;
		multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
		multisamplingInfo.alphaToOneEnable = VK_FALSE;

		// Defines how colors are blended as they're added to the frame buffer
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		// Which color channels to write to
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		// If color blending is enabled then the equation for the blending is:
		//	if (blendEnable) {
		//		finalColor.rgb = (srcColorBlendFactor * newColor.rgb) < colorBlendOp > (dstColorBlendFactor * oldColor.rgb);
		//		finalColor.a = (srcAlphaBlendFactor * newColor.a) < alphaBlendOp > (dstAlphaBlendFactor * oldColor.a);
		//	}
		//	else {
		//			finalColor = newColor;
		//	}
		//	finalColor = finalColor & colorWriteMask;

		VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
		colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		// If true, allows blending using bitwise operations. Disables blending methods defined in each attachment structure
		colorBlendInfo.logicOpEnable = VK_FALSE;
		// There should be one attachment structure for each frame buffer
		colorBlendInfo.attachmentCount = 1;
		colorBlendInfo.pAttachments = &colorBlendAttachment;

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings = {uboLayoutBinding, samplerLayoutBinding};

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
		layoutInfo.pBindings = layoutBindings.data();

		VkResult descriptorSetResult = vkCreateDescriptorSetLayout(vulkanDevice, &layoutInfo, nullptr, &vulkanDescriptorSetLayout);
		VK_CHECK(descriptorSetResult, "Failed to create descriptor set layout: {}");

		// Used to specify shader uniform variables and push constants that can be changed at runtime
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &vulkanDescriptorSetLayout;

		VkResult pipelineLayoutResult = vkCreatePipelineLayout(vulkanDevice, &pipelineLayoutInfo, nullptr, &vulkanPipelineLayout);

		VK_CHECK(pipelineLayoutResult, "Failed to create pipeline layout: {}");

		VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
		pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		pipelineRenderingInfo.colorAttachmentCount = 1;
		pipelineRenderingInfo.pColorAttachmentFormats = &vulkanSwapchainSurfaceFormat.format;

		VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
		graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineInfo.stageCount = 2;
		graphicsPipelineInfo.pStages = shaderStages;

		graphicsPipelineInfo.pDynamicState = &dynamicStateInfo;
		graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
		graphicsPipelineInfo.pInputAssemblyState = &inputAssemplyInfo;
		graphicsPipelineInfo.pViewportState = &viewportStateInfo;
		graphicsPipelineInfo.pRasterizationState = &rasterizationInfo;
		graphicsPipelineInfo.pMultisampleState = &multisamplingInfo;
		graphicsPipelineInfo.pDepthStencilState = nullptr;
		graphicsPipelineInfo.pColorBlendState = &colorBlendInfo;

		graphicsPipelineInfo.layout = vulkanPipelineLayout;

		graphicsPipelineInfo.pNext = &pipelineRenderingInfo;

		VkResult graphicsPipelineResult = vkCreateGraphicsPipelines(vulkanDevice, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &vulkanGraphicsPipeline);

		vkDestroyShaderModule(vulkanDevice, vertShaderModule, nullptr);
		vkDestroyShaderModule(vulkanDevice, fragShaderModule, nullptr);

		VK_CHECK(graphicsPipelineResult, "Failed to create graphics pipeline: {}");
	}

	/// CREATE COMMAND POOL
	{
		VkCommandPoolCreateInfo graphicsPoolInfo{};
		graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		graphicsPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		graphicsPoolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

		VkResult vertexPoolResult = vkCreateCommandPool(vulkanDevice, &graphicsPoolInfo, nullptr, &vulkanGraphicsCommandPool);
		VK_CHECK(vertexPoolResult, "Failed to create graphics command pool : {}");

		VkCommandPoolCreateInfo transferPoolInfo{};
		transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		transferPoolInfo.queueFamilyIndex = transferQueueFamilyIdx;

		VkResult transferPoolResult = vkCreateCommandPool(vulkanDevice, &transferPoolInfo, nullptr, &vulkanTransferCommandPool);
		VK_CHECK(transferPoolResult, "Failed to create transfer command pool: {}");
	}

	/// CREATE COMMAND BUFFERS
	{
		vulkanGraphicsCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo commandBufferInfo{};
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferInfo.commandPool = vulkanGraphicsCommandPool;
		commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

		VkResult result = vkAllocateCommandBuffers(vulkanDevice, &commandBufferInfo, vulkanGraphicsCommandBuffers.data());
		VK_CHECK(result, "Failed to create command buffer: {}");
	}

	/// CREATE SYNC OBJECTS
	{
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderingFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VK_CHECK(vkCreateSemaphore(vulkanDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]), "Failed to create image available semaphores: {}");
			VK_CHECK(vkCreateSemaphore(vulkanDevice, &semaphoreInfo, nullptr, &renderingFinishedSemaphores[i]), "Failed to create rendering finished semaphores: {}");
			VK_CHECK(vkCreateFence(vulkanDevice, &fenceInfo, nullptr, &inFlightFences[i]), "Failed to create in flight fences: {}");
		}
	}

	/// CREATE VERTEX BUFFER AND INDEX BUFFER
	{
		VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer vertexStagingBuffer = VK_NULL_HANDLE;
		VkDeviceMemory vertexStagingBufferMemory = VK_NULL_HANDLE;
		createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexStagingBuffer, vertexStagingBufferMemory);

		void* vertexData;
		vkMapMemory(vulkanDevice, vertexStagingBufferMemory, 0, vertexBufferSize, 0, &vertexData);
		memcpy(vertexData, vertices.data(), static_cast<size_t>(vertexBufferSize));
		vkUnmapMemory(vulkanDevice, vertexStagingBufferMemory);

		createBuffer(sizeof(Vertex)* vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkanVertexBuffer, vulkanVertexBufferMemory);

		VkDeviceSize indexBufferSize = sizeof(uint16_t) * indices.size();

		VkBuffer indexStagingBuffer = VK_NULL_HANDLE;
		VkDeviceMemory indexStagingBufferMemory = VK_NULL_HANDLE;
		createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indexStagingBuffer, indexStagingBufferMemory);

		void* indexdata;
		vkMapMemory(vulkanDevice, indexStagingBufferMemory, 0, indexBufferSize, 0, &indexdata);
		memcpy(indexdata, indices.data(), static_cast<size_t>(indexBufferSize));
		vkUnmapMemory(vulkanDevice, indexStagingBufferMemory);

		createBuffer(sizeof(Vertex)* vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkanIndexBuffer, vulkanIndexBufferMemory);

		VkCommandBuffer transferCommandBuffer;

		VkCommandBufferAllocateInfo transferCommandBufferInfo{};
		transferCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		transferCommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		transferCommandBufferInfo.commandPool = vulkanTransferCommandPool;
		transferCommandBufferInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(vulkanDevice, &transferCommandBufferInfo, &transferCommandBuffer);

		VkCommandBufferBeginInfo commandBufferBeginInfo{};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkResult beginCommandBufferResult = vkBeginCommandBuffer(transferCommandBuffer, &commandBufferBeginInfo);
		VK_CHECK(beginCommandBufferResult, "Failed to begin vertex copy command buffer: {}");

		VkBufferCopy vertexBufferCopy{};	
		vertexBufferCopy.srcOffset = 0;
		vertexBufferCopy.dstOffset = 0;
		vertexBufferCopy.size = vertexBufferSize;

		vkCmdCopyBuffer(transferCommandBuffer, vertexStagingBuffer, vulkanVertexBuffer, 1, &vertexBufferCopy);

		VkBufferCopy indexBufferCopy{};
		indexBufferCopy.srcOffset = 0;
		indexBufferCopy.dstOffset = 0;
		indexBufferCopy.size = indexBufferSize;

		vkCmdCopyBuffer(transferCommandBuffer, indexStagingBuffer, vulkanIndexBuffer, 1, &indexBufferCopy);

		VkResult endCommandBufferResult = vkEndCommandBuffer(transferCommandBuffer);
		VK_CHECK(endCommandBufferResult, "Failed to end vertex copy command buffer: {}");

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &transferCommandBuffer;

		VkFence transferFence;
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		VK_CHECK(vkCreateFence(vulkanDevice, &fenceInfo, nullptr, &transferFence), "Failed to create transfer fence: {}");

		VK_CHECK(vkQueueSubmit(vulkanTransferQueue, 1, &submitInfo, transferFence), "Failed to submit transfer queue: {}");

		vkWaitForFences(vulkanDevice, 1, &transferFence, VK_TRUE, UINT64_MAX);

		vkDestroyFence(vulkanDevice, transferFence, nullptr);
		vkFreeCommandBuffers(vulkanDevice, vulkanTransferCommandPool, 1, &transferCommandBuffer);
		vkDestroyBuffer(vulkanDevice, vertexStagingBuffer, nullptr);
		vkFreeMemory(vulkanDevice, vertexStagingBufferMemory, nullptr);

		vkDestroyBuffer(vulkanDevice, indexStagingBuffer, nullptr);
		vkFreeMemory(vulkanDevice, indexStagingBufferMemory, nullptr);
	}

	/// CREATE UNIFORM BUFFERS
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		vulkanUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		vulkanUniformBufferMemory.resize(MAX_FRAMES_IN_FLIGHT);
		vulkanUniformBufferMappedMemory.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vulkanUniformBuffers[i], vulkanUniformBufferMemory[i]);

			vkMapMemory(vulkanDevice, vulkanUniformBufferMemory[i], 0, bufferSize, 0, &vulkanUniformBufferMappedMemory[i]);
		}
	}

	/// LOAD TEXTURE INTO IMAGE BUFFER
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load("Textures/OuterWildsSquare.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("Failed to load texture image!");
		}

		VkBuffer stagingBuffer{0};
		VkDeviceMemory stagingBufferMemory{0};

		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(vulkanDevice, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(vulkanDevice, stagingBufferMemory);

		stbi_image_free(pixels);

		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent.width = texWidth;
		imageCreateInfo.extent.height = texHeight;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		if (transferQueueFamilyIdx != graphicsQueueFamilyIndex)
		{
			uint32_t queueIndices[2] = { graphicsQueueFamilyIndex, transferQueueFamilyIdx };
			imageCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
			imageCreateInfo.queueFamilyIndexCount = 2;
			imageCreateInfo.pQueueFamilyIndices = queueIndices;
		}
		else
		{
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.flags = 0;

		VkResult imageResult = vkCreateImage(vulkanDevice, &imageCreateInfo, nullptr, &textureImage);
		VK_CHECK(imageResult, "Failed to create image: {}");

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(vulkanDevice, textureImage, &memoryRequirements);

		VkPhysicalDeviceMemoryProperties memoryProperties{};
		vkGetPhysicalDeviceMemoryProperties(vulkanPhysicalDevice, &memoryProperties);

		std::optional<uint32_t> memoryTypeIndex;
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if ((memoryRequirements.memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			{
				memoryTypeIndex = i;
			}
		}

		if (memoryTypeIndex.has_value() == false)
		{
			throw std::runtime_error("Failed to find valid memory type");
		}

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memoryRequirements.size;
		allocInfo.memoryTypeIndex = memoryTypeIndex.value();

		VkResult allocateResult = vkAllocateMemory(vulkanDevice, &allocInfo, nullptr, &textureImageMemory);
		VK_CHECK(allocateResult, "Failed to allocate texture image memory: {}");

		vkBindImageMemory(vulkanDevice, textureImage, textureImageMemory, 0);

		VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = 1;
		commandBufferAllocateInfo.commandPool = vulkanTransferCommandPool;

		VkCommandBuffer commandBuffer;
		
		vkAllocateCommandBuffers(vulkanDevice, &commandBufferAllocateInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkImageMemoryBarrier transferBarrier{};
		transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		transferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		transferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		transferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		transferBarrier.image = textureImage;
		transferBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		transferBarrier.subresourceRange.baseMipLevel = 0;
		transferBarrier.subresourceRange.levelCount = 1;
		transferBarrier.subresourceRange.baseArrayLayer = 0;
		transferBarrier.subresourceRange.layerCount = 1;

		VkImageMemoryBarrier shaderReadBarrier = transferBarrier;
		
		transferBarrier.srcAccessMask = VK_ACCESS_NONE;
		transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		shaderReadBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		shaderReadBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		shaderReadBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		shaderReadBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &transferBarrier);

		VkBufferImageCopy copyRegion{};
		copyRegion.bufferOffset = 0;
		// Specifying 0 here indicates that the pixel values are simply tightly packed in the source buffer with no padding
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;

		copyRegion.imageOffset = {0, 0, 0};
		copyRegion.imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1};
		
		vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &shaderReadBarrier);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VkFence loadTextureFence;

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = 0;
		
		vkCreateFence(vulkanDevice, &fenceCreateInfo, nullptr, &loadTextureFence);
		
		vkQueueSubmit(vulkanTransferQueue, 1, &submitInfo, loadTextureFence);
		vkWaitForFences(vulkanDevice, 1, &loadTextureFence, VK_TRUE, UINT64_MAX);
		vkDestroyFence(vulkanDevice, loadTextureFence, nullptr);

		vkFreeCommandBuffers(vulkanDevice, vulkanTransferCommandPool, 1, &commandBuffer);

		vkDestroyBuffer(vulkanDevice, stagingBuffer, nullptr);
		vkFreeMemory(vulkanDevice, stagingBufferMemory, nullptr);
	}

	/// CREATE TEXTURE IMAGE VIEW
	{
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = textureImage;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;

		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;

		VkResult imageViewResult = vkCreateImageView(vulkanDevice, &imageViewCreateInfo, nullptr, &textureImageView);
		VK_CHECK(imageViewResult, "Failed to create image texture view {}");
	}

	/// CREATE TEXTURE SAMPLER
	{
		VkSamplerCreateInfo samplerCreateInfo{};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR; // How we handle oversampling (more fragments than texels i.e. screen is higher res than texture)
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR; // How we handle undersampling (more texeles than fragments i.e. texture is higher res than screen)
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

		VkPhysicalDeviceProperties deviceProperties{};
		vkGetPhysicalDeviceProperties(vulkanPhysicalDevice, &deviceProperties);

		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;

		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE; // Address texture with [0, 1) range

		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.mipLodBias = 0.f;
		samplerCreateInfo.minLod = 0.f;
		samplerCreateInfo.maxLod = 0.f;
		
		VkResult createSamplerResult = vkCreateSampler(vulkanDevice, &samplerCreateInfo, nullptr, &textureSampler);
		VK_CHECK(createSamplerResult, "Failed to create texture sampler {}");
	}

	/// ALLOCATE AND BIND DESCRIPTOR SETS
	{
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VkResult descriptorPoolResult = vkCreateDescriptorPool(vulkanDevice, &poolInfo, nullptr, &vulkanDescriptorPool);
		VK_CHECK(descriptorPoolResult, "Failed to create descriptor pool: {}");

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT, vulkanDescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = vulkanDescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = descriptorSetLayouts.data();

		vulkanDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		VkResult descriptorSetResult = vkAllocateDescriptorSets(vulkanDevice, &allocInfo, vulkanDescriptorSets.data());
		VK_CHECK(descriptorSetResult, "Failed to create descriptor sets: {}");

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = vulkanUniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageView = textureImageView;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.sampler = textureSampler;

			std::array<VkWriteDescriptorSet, 2> descriptorSetWrites{};
			
			descriptorSetWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorSetWrites[0].dstSet = vulkanDescriptorSets[i];
			descriptorSetWrites[0].dstBinding = 0;
			descriptorSetWrites[0].dstArrayElement = 0;
			descriptorSetWrites[0].descriptorCount = 1;
			descriptorSetWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorSetWrites[0].pBufferInfo = &bufferInfo;
			descriptorSetWrites[0].pImageInfo = nullptr;
			descriptorSetWrites[0].pTexelBufferView = nullptr;
			
			descriptorSetWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorSetWrites[1].dstSet = vulkanDescriptorSets[i];
			descriptorSetWrites[1].dstBinding = 1;
			descriptorSetWrites[1].dstArrayElement = 0;
			descriptorSetWrites[1].descriptorCount = 1;
			descriptorSetWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorSetWrites[1].pBufferInfo = nullptr;
			descriptorSetWrites[1].pImageInfo = &imageInfo;
			descriptorSetWrites[1].pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(vulkanDevice, 2, descriptorSetWrites.data(), 0, nullptr);
		}
	}
}

void Engine::cleanupGraphics()
{
	vkDestroySampler(vulkanDevice, textureSampler, nullptr);
	
	vkDestroyImageView(vulkanDevice, textureImageView, nullptr);
	
	vkDestroyImage(vulkanDevice, textureImage, nullptr);
	vkFreeMemory(vulkanDevice, textureImageMemory, nullptr);
	
	vkFreeDescriptorSets(vulkanDevice, vulkanDescriptorPool, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT), vulkanDescriptorSets.data());
	vkDestroyDescriptorPool(vulkanDevice, vulkanDescriptorPool, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyBuffer(vulkanDevice, vulkanUniformBuffers[i], nullptr);
		vkFreeMemory(vulkanDevice, vulkanUniformBufferMemory[i], nullptr);
	}

	vkDestroyBuffer(vulkanDevice, vulkanVertexBuffer, nullptr);
	vkFreeMemory(vulkanDevice, vulkanVertexBufferMemory, nullptr);

	vkDestroyBuffer(vulkanDevice, vulkanIndexBuffer, nullptr);
	vkFreeMemory(vulkanDevice, vulkanIndexBufferMemory, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(vulkanDevice, imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(vulkanDevice, renderingFinishedSemaphores[i], nullptr);
		vkDestroyFence(vulkanDevice, inFlightFences[i], nullptr);
	}
	imageAvailableSemaphores.clear();
	renderingFinishedSemaphores.clear();
	inFlightFences.clear();

	assert(vulkanGraphicsCommandBuffers.size() == MAX_FRAMES_IN_FLIGHT);
	vkFreeCommandBuffers(vulkanDevice, vulkanGraphicsCommandPool, MAX_FRAMES_IN_FLIGHT, vulkanGraphicsCommandBuffers.data());

	vkDestroyCommandPool(vulkanDevice, vulkanGraphicsCommandPool, nullptr);
	vkDestroyCommandPool(vulkanDevice, vulkanTransferCommandPool, nullptr);

	vkDestroyPipeline(vulkanDevice, vulkanGraphicsPipeline, nullptr);
	
	vkDestroyPipelineLayout(vulkanDevice, vulkanPipelineLayout, nullptr);

	vkDestroyDescriptorSetLayout(vulkanDevice, vulkanDescriptorSetLayout, nullptr);

	for (VkImageView imageView : vulkanSwapchainImageViews)
	{
		vkDestroyImageView(vulkanDevice, imageView, nullptr);
	}
	vulkanSwapchainImageViews.clear();

	vkDestroySwapchainKHR(vulkanDevice, vulkanSwapchain, nullptr);
	vulkanSwapchainImages.clear();

	vkDestroyDevice(vulkanDevice, nullptr);

	vkDestroySurfaceKHR(vulkanInstance, vulkanSurface, nullptr);

#ifndef NDEBUG
	PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(vulkanInstance, "vkDestroyDebugUtilsMessengerEXT"));

	assert(pfnVkDestroyDebugUtilsMessengerEXT != nullptr);

	pfnVkDestroyDebugUtilsMessengerEXT(vulkanInstance, debugMessenger, nullptr);
#endif

	vkDestroyInstance(vulkanInstance, nullptr);
}

void Engine::createSwapChain()
{
	assert(vulkanPhysicalDevice != VK_NULL_HANDLE);
	assert(vulkanSurface != VK_NULL_HANDLE);
	assert(vulkanDevice != VK_NULL_HANDLE);

	assert(vulkanSwapchain == VK_NULL_HANDLE);
	assert(vulkanSwapchainImages.empty());

	VkSurfaceCapabilitiesKHR surfaceCapabilities;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanPhysicalDevice, vulkanSurface, &surfaceCapabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanPhysicalDevice, vulkanSurface, &formatCount, nullptr);

	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanPhysicalDevice, vulkanSurface, &formatCount, surfaceFormats.data());

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanPhysicalDevice, vulkanSurface, &presentModeCount, nullptr);

	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanPhysicalDevice, vulkanSurface, &presentModeCount, presentModes.data());

	VkSurfaceFormatKHR selectedSurfaceFormat = surfaceFormats[0];
	for (const VkSurfaceFormatKHR& surfaceFormat : surfaceFormats)
	{
		if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			selectedSurfaceFormat = surfaceFormat;
			break;
		}
	}

	// TODO: Rank other formats if we can't find the one we want

	VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const VkPresentModeKHR& presentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			selectedPresentMode = presentMode;
			break;
		}
	}

	VkExtent2D surfaceExtent{};

	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		surfaceExtent = surfaceCapabilities.currentExtent;
	}
	else
	{
		int width, height;
		SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);

		surfaceExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		surfaceExtent.width = std::clamp(surfaceExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
		surfaceExtent.height = std::clamp(surfaceExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
	}

	// Simply sticking to this minimum means that we may sometimes have to wait on the driver to complete
	// internal operations before we can acquire another image to render to.
	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;

	if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
	{
		imageCount = surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = vulkanSurface;
	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageFormat = selectedSurfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = selectedSurfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent = surfaceExtent;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = selectedPresentMode;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
	{
		// Present and graphics queues are different so we need to share the swap chain images
		uint32_t queueFamilyIndices[] = { presentQueueFamilyIndex, graphicsQueueFamilyIndex };
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	VkResult result = vkCreateSwapchainKHR(vulkanDevice, &swapChainCreateInfo, nullptr, &vulkanSwapchain);
	VK_CHECK(result, "Failed to create swap chain: {}");

	vkGetSwapchainImagesKHR(vulkanDevice, vulkanSwapchain, &imageCount, nullptr);
	vulkanSwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(vulkanDevice, vulkanSwapchain, &imageCount, vulkanSwapchainImages.data());

	vulkanSwapchainSurfaceFormat = selectedSurfaceFormat;
	vulkanSwapchainSurfaceExtent = surfaceExtent;
}

void Engine::createImageViews()
{
	assert(vulkanDevice != VK_NULL_HANDLE);
	assert(vulkanSwapchainImages.empty() == false);
	assert(vulkanSwapchainImageViews.empty());
	assert(vulkanSwapchainSurfaceFormat.format != VK_FORMAT_UNDEFINED);

	vulkanSwapchainImageViews.resize(vulkanSwapchainImages.size());

	for (size_t i = 0; i < vulkanSwapchainImageViews.size(); i++)
	{
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = vulkanSwapchainImages[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = vulkanSwapchainSurfaceFormat.format;

		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView(vulkanDevice, &imageViewCreateInfo, nullptr, &vulkanSwapchainImageViews[i]);
		VK_CHECK(result, "Failed to create image view: {}");
	}
}

void Engine::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	assert(buffer == VK_NULL_HANDLE);
	assert(bufferMemory == VK_NULL_HANDLE);

	assert(vulkanDevice != VK_NULL_HANDLE);
	assert(vulkanPhysicalDevice != VK_NULL_HANDLE);

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;

	if (transferQueueFamilyIdx != graphicsQueueFamilyIndex)
	{
		uint32_t queueIndices[2] = { graphicsQueueFamilyIndex, transferQueueFamilyIdx };
		bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		bufferInfo.pQueueFamilyIndices = queueIndices;
	}
	else
	{
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}


	VkResult bufferResult = vkCreateBuffer(vulkanDevice, &bufferInfo, nullptr, &buffer);
	VK_CHECK(bufferResult, "Failed to create buffer: {}");

	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(vulkanDevice, buffer, &memoryRequirements);

	VkPhysicalDeviceMemoryProperties memoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(vulkanPhysicalDevice, &memoryProperties);

	std::optional<uint32_t> memoryTypeIndex;
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((memoryRequirements.memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			memoryTypeIndex = i;
		}
	}

	if (memoryTypeIndex.has_value() == false)
	{
		throw std::runtime_error("Failed to find valid memory type");
	}

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryTypeIndex.value();

	VkResult allocateResult = vkAllocateMemory(vulkanDevice, &allocateInfo, nullptr, &bufferMemory);
	VK_CHECK(allocateResult, "Failed to allocate buffer memory: {}");

	VkResult bindResult = vkBindBufferMemory(vulkanDevice, buffer, bufferMemory, 0);
	VK_CHECK(bindResult, "Failed to bind buffer memory: {}");
}
