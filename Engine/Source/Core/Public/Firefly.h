#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb_image.h>

#include <chrono>
#include <vector>
#include <array>
#include <iostream>

#define VK_CHECK(result, formattedMessage) if(result != VK_SUCCESS) { throw std::runtime_error(std::format(formattedMessage, string_VkResult(result))); }

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription();

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

class Engine
{
public:
	void run();

private:
	void mainLoop();

	void initWindow();

	void cleanupWindow();

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:

	void drawFrame(float deltaTime);

	void initGraphics();

	void cleanupGraphics();

private:
	void createSwapChain();

	void createImageViews();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);


private:
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
	{
		std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

private:
	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;

	const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

	const std::vector<Vertex> vertices =
	{
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.f, 0.f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.f, 0.f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.f, 1.f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.f, 1.f}}
	};

	const std::vector<uint16_t> indices =
	{
		3, 0, 2, 1
	};

#ifndef NDEBUG
	const std::vector<const char*> desiredValidationLayers =
	{
		// Useful standard validations are bundled into this layer
		"VK_LAYER_KHRONOS_validation"
	};

	VkDebugUtilsMessengerEXT debugMessenger;
#endif

	const std::vector<const char*> desiredDeviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

private:
	GLFWwindow* glfwWindow;

private:
	VkInstance vulkanInstance = VK_NULL_HANDLE;

	VkPhysicalDevice vulkanPhysicalDevice = VK_NULL_HANDLE;

	VkDevice vulkanDevice = VK_NULL_HANDLE;

	VkSurfaceKHR vulkanSurface = VK_NULL_HANDLE;

	VkSwapchainKHR vulkanSwapchain = VK_NULL_HANDLE;
	std::vector<VkImage> vulkanSwapchainImages;
	std::vector<VkImageView> vulkanSwapchainImageViews;
	VkSurfaceFormatKHR vulkanSwapchainSurfaceFormat;
	VkExtent2D vulkanSwapchainSurfaceExtent;

	VkDescriptorSetLayout vulkanDescriptorSetLayout;
	VkPipelineLayout vulkanPipelineLayout;

	VkPipeline vulkanGraphicsPipeline;

	uint32_t graphicsQueueFamilyIndex = 0;
	VkQueue vulkanGraphicsQueue;

	uint32_t presentQueueFamilyIndex = 0;
	VkQueue vulkanPresentQueue;

	uint32_t transferQueueFamilyIdx = 0;
	VkQueue vulkanTransferQueue;

	VkCommandPool vulkanGraphicsCommandPool;
	VkCommandPool vulkanTransferCommandPool;

	std::vector<VkCommandBuffer> vulkanGraphicsCommandBuffers;

private:
	VkBuffer vulkanVertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vulkanVertexBufferMemory = VK_NULL_HANDLE;

	VkBuffer vulkanIndexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vulkanIndexBufferMemory = VK_NULL_HANDLE;

	std::vector<VkBuffer> vulkanUniformBuffers;
	std::vector<VkDeviceMemory> vulkanUniformBufferMemory;
	std::vector<void*> vulkanUniformBufferMappedMemory;

	VkDescriptorPool vulkanDescriptorPool;
	std::vector<VkDescriptorSet> vulkanDescriptorSets;

private:
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;

	VkImageView textureImageView;
	VkSampler textureSampler;

private:
	uint32_t inFlightFrameIdx = 0;

	bool frameBufferResized = false;

	bool WKeyDown = false;
	bool SKeyDown = false;
	bool AKeyDown = false;
	bool DKeyDown = false;
	bool QKeyDown = false;
	bool EKeyDown = false;

	float modelRotationRad = 0.f;

	glm::vec3 cameraPosition = {-2.f, 0.f, 2.f};

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderingFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
};
