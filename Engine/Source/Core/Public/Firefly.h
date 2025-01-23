#pragma once

#include "Scene.h"

#include "SDL3/SDL_init.h"
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_vulkan.h"

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb_image.h>

#include <vector>
#include <array>
#include <iostream>
#include <queue>
#include <format>

inline const char *Vulkan_GetResultString(VkResult result)
{
    switch ((int)result) {
    case VK_SUCCESS:
        return "VK_SUCCESS";
    case VK_NOT_READY:
        return "VK_NOT_READY";
    case VK_TIMEOUT:
        return "VK_TIMEOUT";
    case VK_EVENT_SET:
        return "VK_EVENT_SET";
    case VK_EVENT_RESET:
        return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
        return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
        return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_UNKNOWN:
        return "VK_ERROR_UNKNOWN";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
        return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_FRAGMENTATION:
        return "VK_ERROR_FRAGMENTATION";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:
        return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:
        return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV:
        return "VK_ERROR_INVALID_SHADER_NV";
#if VK_HEADER_VERSION >= 135 && VK_HEADER_VERSION < 162
    case VK_ERROR_INCOMPATIBLE_VERSION_KHR:
        return "VK_ERROR_INCOMPATIBLE_VERSION_KHR";
#endif
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
        return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    case VK_ERROR_NOT_PERMITTED_EXT:
        return "VK_ERROR_NOT_PERMITTED_EXT";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
    case VK_THREAD_IDLE_KHR:
        return "VK_THREAD_IDLE_KHR";
    case VK_THREAD_DONE_KHR:
        return "VK_THREAD_DONE_KHR";
    case VK_OPERATION_DEFERRED_KHR:
        return "VK_OPERATION_DEFERRED_KHR";
    case VK_OPERATION_NOT_DEFERRED_KHR:
        return "VK_OPERATION_NOT_DEFERRED_KHR";
    case VK_PIPELINE_COMPILE_REQUIRED_EXT:
        return "VK_PIPELINE_COMPILE_REQUIRED_EXT";
    default:
        break;
    }
    if (result < 0) {
        return "VK_ERROR_<Unknown>";
    }
    return "VK_<Unknown>";
}

#define VK_CHECK(result, formattedMessage) if(result != VK_SUCCESS) { throw std::runtime_error(std::format(formattedMessage, Vulkan_GetResultString(result))); }

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

	void stepSimulation(float deltaTime);

	void processEvent(const SDL_Event& event);

	void initWindow();

	void cleanupWindow();

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
	SDL_Window* sdlWindow;

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
	Scene scene;
	
	bool windowCloseRequested = false;

	uint32_t inFlightFrameIdx = 0;

	bool frameBufferResized = false;

	enum class EBinaryInput
	{
		AKey = 0, BKey, CKey, DKey,
		EKey, FKey, GKey, HKey,
		IKey, JKey, KKey, LKey,
		MKey, NKey, OKey, PKey,
		QKey, RKey, SKey, TKey,
		UKey, VKey, WKey, XKey,
		YKey, ZKey
	};

	struct FInputState
	{
		struct FKeyboardState
		{
			uint8_t AKey : 1;
			uint8_t BKey : 1;
			uint8_t CKey : 1;
			uint8_t DKey : 1;
			uint8_t EKey : 1;
			uint8_t FKey : 1;
			uint8_t GKey : 1;
			uint8_t HKey : 1;
			uint8_t IKey : 1;
			uint8_t JKey : 1;
			uint8_t KKey : 1;
			uint8_t LKey : 1;
			uint8_t MKey : 1;
			uint8_t NKey : 1;
			uint8_t OKey : 1;
			uint8_t PKey : 1;
			uint8_t QKey : 1;
			uint8_t RKey : 1;
			uint8_t SKey : 1;
			uint8_t TKey : 1;
			uint8_t UKey : 1;
			uint8_t VKey : 1;
			uint8_t WKey : 1;
			uint8_t XKey : 1;
			uint8_t YKey : 1;
			uint8_t ZKey : 1;
		};
		union SKeyboardStateMaskUnion
		{
 			FKeyboardState KeyboardState;
			uint32_t Mask;
		};
		SKeyboardStateMaskUnion KeyboardStateMask{};
		// The keyboard keys that changed in the most recent update
		SKeyboardStateMaskUnion KeyboardStateChangedMask{};

		FKeyboardState GetKeyboardState() const { return KeyboardStateMask.KeyboardState; }
		FKeyboardState GetKeyboardStateChange() const { return KeyboardStateChangedMask.KeyboardState; }

	} InputState;
	
	float modelRotationRad = 0.f;

	glm::vec3 cameraPosition = {-2.f, 0.f, 2.f};

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderingFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
};
