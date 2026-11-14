// vulkan.cpp
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026

#include "window/vulkan.h"

#include "picture/color.h"
#include "window/glfw.h"

// volk brings Vulkan's own header, without its functions: it finds them in the
// driver's library when the window opens. GLFW declares its Vulkan calls once
// that header is in.
#include <volk.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace xge
{
	namespace
	{
		// The two shaders, compiled to SPIR-V by
		//   glslangValidator -V --target-env vulkan1.0 quad.vert -o quad.vert.spv
		//   spirv-opt --strip-debug quad.vert.spv -o quad.vert.spv
		// from this (and the same for quad.frag below):
		//
		//   #version 450
		//   layout(push_constant) uniform Place { vec2 topLeft; vec2 bottomRight; vec2 gameSize; } place;
		//   layout(location = 0) out vec2 uv;
		//   void main()
		//   {
		//       vec2 corner = vec2(gl_VertexIndex & 1, gl_VertexIndex >> 1);
		//       uv = corner;
		//       gl_Position = vec4(mix(place.topLeft, place.bottomRight, corner) / place.gameSize * 2.0 - 1.0, 0.0, 1.0);
		//   }
		//
		// One rectangle from four corners and no vertex buffer: where it is in
		// the game's pixels, turned into Vulkan's -1 to 1 (top is -1).
		constexpr std::uint32_t kVertexShader[] = {
			0x07230203, 0x00010000, 0x0008000b, 0x00000039, 0x00000000, 0x00020011,
			0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
			0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0008000f, 0x00000000,
			0x00000004, 0x6e69616d, 0x00000000, 0x0000000c, 0x00000016, 0x0000001e,
			0x00040047, 0x0000000c, 0x0000000b, 0x0000002a, 0x00040047, 0x00000016,
			0x0000001e, 0x00000000, 0x00030047, 0x0000001c, 0x00000002, 0x00050048,
			0x0000001c, 0x00000000, 0x0000000b, 0x00000000, 0x00050048, 0x0000001c,
			0x00000001, 0x0000000b, 0x00000001, 0x00050048, 0x0000001c, 0x00000002,
			0x0000000b, 0x00000003, 0x00050048, 0x0000001c, 0x00000003, 0x0000000b,
			0x00000004, 0x00030047, 0x00000020, 0x00000002, 0x00050048, 0x00000020,
			0x00000000, 0x00000023, 0x00000000, 0x00050048, 0x00000020, 0x00000001,
			0x00000023, 0x00000008, 0x00050048, 0x00000020, 0x00000002, 0x00000023,
			0x00000010, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002,
			0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006,
			0x00000002, 0x00040020, 0x00000008, 0x00000007, 0x00000007, 0x00040015,
			0x0000000a, 0x00000020, 0x00000001, 0x00040020, 0x0000000b, 0x00000001,
			0x0000000a, 0x0004003b, 0x0000000b, 0x0000000c, 0x00000001, 0x0004002b,
			0x0000000a, 0x0000000e, 0x00000001, 0x00040020, 0x00000015, 0x00000003,
			0x00000007, 0x0004003b, 0x00000015, 0x00000016, 0x00000003, 0x00040017,
			0x00000018, 0x00000006, 0x00000004, 0x00040015, 0x00000019, 0x00000020,
			0x00000000, 0x0004002b, 0x00000019, 0x0000001a, 0x00000001, 0x0004001c,
			0x0000001b, 0x00000006, 0x0000001a, 0x0006001e, 0x0000001c, 0x00000018,
			0x00000006, 0x0000001b, 0x0000001b, 0x00040020, 0x0000001d, 0x00000003,
			0x0000001c, 0x0004003b, 0x0000001d, 0x0000001e, 0x00000003, 0x0004002b,
			0x0000000a, 0x0000001f, 0x00000000, 0x0005001e, 0x00000020, 0x00000007,
			0x00000007, 0x00000007, 0x00040020, 0x00000021, 0x00000009, 0x00000020,
			0x0004003b, 0x00000021, 0x00000022, 0x00000009, 0x00040020, 0x00000023,
			0x00000009, 0x00000007, 0x0004002b, 0x0000000a, 0x0000002a, 0x00000002,
			0x0004002b, 0x00000006, 0x0000002e, 0x40000000, 0x0004002b, 0x00000006,
			0x00000030, 0x3f800000, 0x0004002b, 0x00000006, 0x00000033, 0x00000000,
			0x00040020, 0x00000037, 0x00000003, 0x00000018, 0x00050036, 0x00000002,
			0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003b,
			0x00000008, 0x00000009, 0x00000007, 0x0004003d, 0x0000000a, 0x0000000d,
			0x0000000c, 0x000500c7, 0x0000000a, 0x0000000f, 0x0000000d, 0x0000000e,
			0x0004006f, 0x00000006, 0x00000010, 0x0000000f, 0x0004003d, 0x0000000a,
			0x00000011, 0x0000000c, 0x000500c3, 0x0000000a, 0x00000012, 0x00000011,
			0x0000000e, 0x0004006f, 0x00000006, 0x00000013, 0x00000012, 0x00050050,
			0x00000007, 0x00000014, 0x00000010, 0x00000013, 0x0003003e, 0x00000009,
			0x00000014, 0x0004003d, 0x00000007, 0x00000017, 0x00000009, 0x0003003e,
			0x00000016, 0x00000017, 0x00050041, 0x00000023, 0x00000024, 0x00000022,
			0x0000001f, 0x0004003d, 0x00000007, 0x00000025, 0x00000024, 0x00050041,
			0x00000023, 0x00000026, 0x00000022, 0x0000000e, 0x0004003d, 0x00000007,
			0x00000027, 0x00000026, 0x0004003d, 0x00000007, 0x00000028, 0x00000009,
			0x0008000c, 0x00000007, 0x00000029, 0x00000001, 0x0000002e, 0x00000025,
			0x00000027, 0x00000028, 0x00050041, 0x00000023, 0x0000002b, 0x00000022,
			0x0000002a, 0x0004003d, 0x00000007, 0x0000002c, 0x0000002b, 0x00050088,
			0x00000007, 0x0000002d, 0x00000029, 0x0000002c, 0x0005008e, 0x00000007,
			0x0000002f, 0x0000002d, 0x0000002e, 0x00050050, 0x00000007, 0x00000031,
			0x00000030, 0x00000030, 0x00050083, 0x00000007, 0x00000032, 0x0000002f,
			0x00000031, 0x00050051, 0x00000006, 0x00000034, 0x00000032, 0x00000000,
			0x00050051, 0x00000006, 0x00000035, 0x00000032, 0x00000001, 0x00070050,
			0x00000018, 0x00000036, 0x00000034, 0x00000035, 0x00000033, 0x00000030,
			0x00050041, 0x00000037, 0x00000038, 0x0000001e, 0x0000001f, 0x0003003e,
			0x00000038, 0x00000036, 0x000100fd, 0x00010038,
		};

		//   #version 450
		//   layout(set = 0, binding = 0) uniform sampler2D picture;
		//   layout(location = 0) in vec2 uv;
		//   layout(location = 0) out vec4 color;
		//   void main() { color = texture(picture, uv); }
		//
		// The picture as it is (straight alpha, blended over what is there).
		constexpr std::uint32_t kFragmentShader[] = {
			0x07230203, 0x00010000, 0x0008000b, 0x00000014, 0x00000000, 0x00020011,
			0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
			0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0007000f, 0x00000004,
			0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x00000011, 0x00030010,
			0x00000004, 0x00000007, 0x00040047, 0x00000009, 0x0000001e, 0x00000000,
			0x00040047, 0x0000000d, 0x00000021, 0x00000000, 0x00040047, 0x0000000d,
			0x00000022, 0x00000000, 0x00040047, 0x00000011, 0x0000001e, 0x00000000,
			0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016,
			0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004,
			0x00040020, 0x00000008, 0x00000003, 0x00000007, 0x0004003b, 0x00000008,
			0x00000009, 0x00000003, 0x00090019, 0x0000000a, 0x00000006, 0x00000001,
			0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x0003001b,
			0x0000000b, 0x0000000a, 0x00040020, 0x0000000c, 0x00000000, 0x0000000b,
			0x0004003b, 0x0000000c, 0x0000000d, 0x00000000, 0x00040017, 0x0000000f,
			0x00000006, 0x00000002, 0x00040020, 0x00000010, 0x00000001, 0x0000000f,
			0x0004003b, 0x00000010, 0x00000011, 0x00000001, 0x00050036, 0x00000002,
			0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d,
			0x0000000b, 0x0000000e, 0x0000000d, 0x0004003d, 0x0000000f, 0x00000012,
			0x00000011, 0x00050057, 0x00000007, 0x00000013, 0x0000000e, 0x00000012,
			0x0003003e, 0x00000009, 0x00000013, 0x000100fd, 0x00010038,
		};

		// What the vertex shader is pushed for each rectangle.
		struct Place
		{
			float left;
			float top;
			float right;
			float bottom;
			float gameWidth;
			float gameHeight;
		};

		// A Vulkan description with its type set and everything else zero.
		template <typename Description>
		Description blank(VkStructureType type)
		{
			Description description{};
			description.sType = type;
			return description;
		}

		void check(VkResult result, const char* what)
		{
			if (result != VK_SUCCESS) { throw std::runtime_error(std::string("Vulkan could not ") + what); }
		}
	}

	struct VulkanWindow::Parts
	{
		// A picture on the graphics card, and the set that hands it to the
		// fragment shader with its sampler.
		struct Picture
		{
			VkImage image{ VK_NULL_HANDLE };
			VkDeviceMemory memory{ VK_NULL_HANDLE };
			VkImageView view{ VK_NULL_HANDLE };
			VkDescriptorSet set{ VK_NULL_HANDLE };
			VkDescriptorPool pool{ VK_NULL_HANDLE };
		};

		struct Visual
		{
			Picture picture;
			float drawnAt{ 0.0f };   // the pictureScale it was drawn at
		};

		// Where a frame is: nothing yet, being recorded, or handed to the
		// graphics card (capture() does that early, to read it back).
		enum class Stage { Idle, Recording, Submitted };

		GLFWwindow* window{ nullptr };
		float gameWidth{ 1.0f };
		float gameHeight{ 1.0f };
		bool resized{ false };
		std::vector<std::pair<KeyCode, bool>> keys;

		// Where the game is drawn in the window: as large as fits with its
		// proportions, centred on whole pixels (the rest is bars).
		float fit{ 1.0f };
		float shownLeft{ 0.0f };
		float shownTop{ 0.0f };
		float shownWidth{ 1.0f };
		float shownHeight{ 1.0f };

		VkInstance instance{ VK_NULL_HANDLE };
		VkSurfaceKHR surface{ VK_NULL_HANDLE };
		VkPhysicalDevice gpu{ VK_NULL_HANDLE };
		VkDevice device{ VK_NULL_HANDLE };
		std::uint32_t family{ 0 };
		VkQueue queue{ VK_NULL_HANDLE };

		VkSwapchainKHR swapchain{ VK_NULL_HANDLE };
		VkFormat format{ VK_FORMAT_UNDEFINED };
		VkExtent2D extent{};
		bool canRead{ false };   // the swapchain's pictures can be copied out
		std::vector<VkImage> images;
		std::vector<VkImageView> views;
		std::vector<VkFramebuffer> framebuffers;
		std::vector<VkSemaphore> finished;   // one per swapchain picture

		VkRenderPass renderPass{ VK_NULL_HANDLE };
		VkDescriptorSetLayout setLayout{ VK_NULL_HANDLE };
		VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
		VkPipeline pipeline{ VK_NULL_HANDLE };
		VkSampler hardEdges{ VK_NULL_HANDLE };
		VkSampler smooth{ VK_NULL_HANDLE };
		// Pools of sets for the pictures, each with how many it has left:
		// Vulkan 1.0 does not allow asking a full pool.
		struct Pool
		{
			VkDescriptorPool pool{ VK_NULL_HANDLE };
			std::uint32_t left{ 0 };
		};
		std::vector<Pool> pools;

		VkCommandPool commandPool{ VK_NULL_HANDLE };
		VkCommandBuffer frame{ VK_NULL_HANDLE };
		VkCommandBuffer upload{ VK_NULL_HANDLE };
		VkSemaphore acquired{ VK_NULL_HANDLE };
		VkFence done{ VK_NULL_HANDLE };
		std::uint32_t shownImage{ 0 };
		Stage stage{ Stage::Idle };

		std::unordered_map<std::string, Visual> visuals;
		// Pictures replaced this frame: the last one may still be drawing
		// them, so they go once it is done.
		std::vector<Picture> retired;

		~Parts() { destroy(); }

		void destroy()
		{
			if (device)
			{
				vkDeviceWaitIdle(device);
				for (auto& [name, visual] : visuals) { drop(visual.picture); }
				visuals.clear();
				for (Picture& picture : retired) { drop(picture); }
				retired.clear();
				dropSwapchain();
				vkDestroySwapchainKHR(device, swapchain, nullptr);
				for (const Pool& each : pools) { vkDestroyDescriptorPool(device, each.pool, nullptr); }
				vkDestroySampler(device, hardEdges, nullptr);
				vkDestroySampler(device, smooth, nullptr);
				vkDestroyPipeline(device, pipeline, nullptr);
				vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
				vkDestroyDescriptorSetLayout(device, setLayout, nullptr);
				vkDestroyRenderPass(device, renderPass, nullptr);
				vkDestroySemaphore(device, acquired, nullptr);
				vkDestroyFence(device, done, nullptr);
				vkDestroyCommandPool(device, commandPool, nullptr);
				vkDestroyDevice(device, nullptr);
				device = VK_NULL_HANDLE;
			}
			if (instance)
			{
				vkDestroySurfaceKHR(instance, surface, nullptr);
				vkDestroyInstance(instance, nullptr);
				instance = VK_NULL_HANDLE;
			}
			if (window)
			{
				glfwDestroyWindow(window);
				window = nullptr;
				glfwTerminate();
			}
		}

		void drop(Picture& picture)
		{
			if (picture.set)
			{
				vkFreeDescriptorSets(device, picture.pool, 1, &picture.set);
				for (Pool& each : pools)
				{
					if (each.pool == picture.pool) { ++each.left; }
				}
			}
			vkDestroyImageView(device, picture.view, nullptr);
			vkDestroyImage(device, picture.image, nullptr);
			vkFreeMemory(device, picture.memory, nullptr);
			picture = {};
		}

		static void keyCallback(GLFWwindow* from, int key, int, int action, int)
		{
			if (action == GLFW_REPEAT) { return; }   // the engine only wants the first press
			auto* self = static_cast<Parts*>(glfwGetWindowUserPointer(from));
			const KeyCode code = keyFromGlfw(key);
			if (self && code != KeyCode::Unknown) { self->keys.emplace_back(code, action == GLFW_PRESS); }
		}

		static void sizeCallback(GLFWwindow* from, int, int)
		{
			auto* self = static_cast<Parts*>(glfwGetWindowUserPointer(from));
			if (self) { self->resized = true; }
		}

		std::uint32_t memoryType(std::uint32_t allowed, VkMemoryPropertyFlags wanted) const
		{
			VkPhysicalDeviceMemoryProperties memory{};
			vkGetPhysicalDeviceMemoryProperties(gpu, &memory);
			for (std::uint32_t type = 0; type < memory.memoryTypeCount; ++type)
			{
				if ((allowed & (1u << type)) != 0 && (memory.memoryTypes[type].propertyFlags & wanted) == wanted) { return type; }
			}
			throw std::runtime_error("Vulkan has no memory of the kind it needs");
		}

		// A buffer the processor writes or reads.
		std::pair<VkBuffer, VkDeviceMemory> hostBuffer(VkDeviceSize size, VkBufferUsageFlags usage) const
		{
			auto about = blank<VkBufferCreateInfo>(VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
			about.size = size;
			about.usage = usage;
			about.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VkBuffer buffer{ VK_NULL_HANDLE };
			check(vkCreateBuffer(device, &about, nullptr, &buffer), "make a buffer");
			VkMemoryRequirements needs{};
			vkGetBufferMemoryRequirements(device, buffer, &needs);
			auto allocate = blank<VkMemoryAllocateInfo>(VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
			allocate.allocationSize = needs.size;
			allocate.memoryTypeIndex = memoryType(needs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VkDeviceMemory memory{ VK_NULL_HANDLE };
			if (vkAllocateMemory(device, &allocate, nullptr, &memory) != VK_SUCCESS)
			{
				vkDestroyBuffer(device, buffer, nullptr);
				throw std::runtime_error("Vulkan could not find memory for a buffer");
			}
			vkBindBufferMemory(device, buffer, memory, 0);
			return { buffer, memory };
		}

		static void barrier(VkCommandBuffer commands, VkImage image, VkImageLayout from, VkImageLayout to,
			VkAccessFlags before, VkAccessFlags after, VkPipelineStageFlags beforeStage, VkPipelineStageFlags afterStage)
		{
			auto change = blank<VkImageMemoryBarrier>(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
			change.srcAccessMask = before;
			change.dstAccessMask = after;
			change.oldLayout = from;
			change.newLayout = to;
			change.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			change.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			change.image = image;
			change.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			vkCmdPipelineBarrier(commands, beforeStage, afterStage, 0, 0, nullptr, 0, nullptr, 1, &change);
		}

		void makeDevice()
		{
			std::uint32_t count = 0;
			vkEnumeratePhysicalDevices(instance, &count, nullptr);
			std::vector<VkPhysicalDevice> gpus(count);
			vkEnumeratePhysicalDevices(instance, &count, gpus.data());

			// The first card that can draw into the window, a card of its own
			// before one built into the processor.
			bool foundDiscrete = false;
			for (VkPhysicalDevice each : gpus)
			{
				std::uint32_t extensionCount = 0;
				vkEnumerateDeviceExtensionProperties(each, nullptr, &extensionCount, nullptr);
				std::vector<VkExtensionProperties> extensions(extensionCount);
				vkEnumerateDeviceExtensionProperties(each, nullptr, &extensionCount, extensions.data());
				const bool swaps = std::any_of(extensions.begin(), extensions.end(),
					[](const VkExtensionProperties& e) { return std::strcmp(e.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0; });
				if (!swaps) { continue; }

				std::uint32_t familyCount = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(each, &familyCount, nullptr);
				std::vector<VkQueueFamilyProperties> families(familyCount);
				vkGetPhysicalDeviceQueueFamilyProperties(each, &familyCount, families.data());
				for (std::uint32_t index = 0; index < familyCount; ++index)
				{
					VkBool32 shows = VK_FALSE;
					vkGetPhysicalDeviceSurfaceSupportKHR(each, index, surface, &shows);
					if ((families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 || !shows) { continue; }

					VkPhysicalDeviceProperties about{};
					vkGetPhysicalDeviceProperties(each, &about);
					const bool discrete = about.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
					if (!gpu || (discrete && !foundDiscrete))
					{
						gpu = each;
						family = index;
						foundDiscrete = discrete;
					}
					break;
				}
			}
			if (!gpu) { throw std::runtime_error("Vulkan found no graphics card that can draw in a window"); }

			const float priority = 1.0f;
			auto queueAbout = blank<VkDeviceQueueCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
			queueAbout.queueFamilyIndex = family;
			queueAbout.queueCount = 1;
			queueAbout.pQueuePriorities = &priority;
			const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
			auto about = blank<VkDeviceCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
			about.queueCreateInfoCount = 1;
			about.pQueueCreateInfos = &queueAbout;
			about.enabledExtensionCount = 1;
			about.ppEnabledExtensionNames = extensions;
			check(vkCreateDevice(gpu, &about, nullptr, &device), "start on this graphics card");
			volkLoadDevice(device);
			vkGetDeviceQueue(device, family, 0, &queue);
		}

		void fitGame()
		{
			const float wide = static_cast<float>(std::max(extent.width, 1u));
			const float high = static_cast<float>(std::max(extent.height, 1u));
			fit = std::min(wide / gameWidth, high / gameHeight);
			shownWidth = std::max(std::floor(gameWidth * fit), 1.0f);
			shownHeight = std::max(std::floor(gameHeight * fit), 1.0f);
			shownLeft = std::floor((wide - shownWidth) / 2.0f);
			shownTop = std::floor((high - shownHeight) / 2.0f);
		}

		void dropSwapchain()
		{
			for (VkFramebuffer each : framebuffers) { vkDestroyFramebuffer(device, each, nullptr); }
			for (VkImageView each : views) { vkDestroyImageView(device, each, nullptr); }
			for (VkSemaphore each : finished) { vkDestroySemaphore(device, each, nullptr); }
			framebuffers.clear();
			views.clear();
			finished.clear();
			images.clear();
		}

		// The pictures the window shows, at the window's size. False while it
		// has none (minimised): nothing is drawn then.
		bool makeSwapchain()
		{
			resized = false;
			VkSurfaceCapabilitiesKHR can{};
			check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &can), "ask about its window");
			VkExtent2D size = can.currentExtent;
			if (size.width == std::numeric_limits<std::uint32_t>::max())
			{
				int width = 0;
				int height = 0;
				glfwGetFramebufferSize(window, &width, &height);
				size.width = std::clamp(static_cast<std::uint32_t>(std::max(width, 0)), can.minImageExtent.width, can.maxImageExtent.width);
				size.height = std::clamp(static_cast<std::uint32_t>(std::max(height, 0)), can.minImageExtent.height, can.maxImageExtent.height);
			}
			if (size.width == 0 || size.height == 0) { return false; }

			vkDeviceWaitIdle(device);
			dropSwapchain();

			canRead = (can.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0;
			auto about = blank<VkSwapchainCreateInfoKHR>(VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
			about.surface = surface;
			about.minImageCount = can.maxImageCount == 0 ? can.minImageCount + 1 : std::min(can.minImageCount + 1, can.maxImageCount);
			about.imageFormat = format;
			about.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			about.imageExtent = size;
			about.imageArrayLayers = 1;
			about.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			if (canRead) { about.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; }
			about.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			about.preTransform = can.currentTransform;
			about.compositeAlpha = (can.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) != 0
				? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
			// Waits for the screen's refresh, so a picture is never torn; the
			// engine keeps the game's own pace (Engine::loop).
			about.presentMode = VK_PRESENT_MODE_FIFO_KHR;
			about.clipped = VK_TRUE;
			about.oldSwapchain = swapchain;
			VkSwapchainKHR made{ VK_NULL_HANDLE };
			check(vkCreateSwapchainKHR(device, &about, nullptr, &made), "draw into its window");
			vkDestroySwapchainKHR(device, swapchain, nullptr);
			swapchain = made;
			extent = size;

			std::uint32_t count = 0;
			vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr);
			images.resize(count);
			vkGetSwapchainImagesKHR(device, swapchain, &count, images.data());
			for (VkImage image : images)
			{
				auto viewAbout = blank<VkImageViewCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
				viewAbout.image = image;
				viewAbout.viewType = VK_IMAGE_VIEW_TYPE_2D;
				viewAbout.format = format;
				viewAbout.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
				VkImageView view{ VK_NULL_HANDLE };
				check(vkCreateImageView(device, &viewAbout, nullptr, &view), "see its window's pictures");
				views.push_back(view);

				auto frameAbout = blank<VkFramebufferCreateInfo>(VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
				frameAbout.renderPass = renderPass;
				frameAbout.attachmentCount = 1;
				frameAbout.pAttachments = &view;
				frameAbout.width = size.width;
				frameAbout.height = size.height;
				frameAbout.layers = 1;
				VkFramebuffer framebuffer{ VK_NULL_HANDLE };
				check(vkCreateFramebuffer(device, &frameAbout, nullptr, &framebuffer), "draw into its window's pictures");
				framebuffers.push_back(framebuffer);

				auto signalAbout = blank<VkSemaphoreCreateInfo>(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
				VkSemaphore signal{ VK_NULL_HANDLE };
				check(vkCreateSemaphore(device, &signalAbout, nullptr, &signal), "make a semaphore");
				finished.push_back(signal);
			}
			fitGame();
			return true;
		}

		// The window's colour format, chosen once: a plain 8 bits a channel,
		// as the pictures are.
		void chooseFormat()
		{
			std::uint32_t count = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &count, nullptr);
			std::vector<VkSurfaceFormatKHR> formats(count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &count, formats.data());
			if (formats.empty()) { throw std::runtime_error("Vulkan has no colour format for its window"); }
			format = formats.front().format;
			for (const VkSurfaceFormatKHR& each : formats)
			{
				if ((each.format == VK_FORMAT_B8G8R8A8_UNORM || each.format == VK_FORMAT_R8G8B8A8_UNORM)
					&& each.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					format = each.format;
					break;
				}
			}
		}

		VkShaderModule shader(const std::uint32_t* words, std::size_t bytes) const
		{
			auto about = blank<VkShaderModuleCreateInfo>(VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
			about.codeSize = bytes;
			about.pCode = words;
			VkShaderModule module{ VK_NULL_HANDLE };
			check(vkCreateShaderModule(device, &about, nullptr, &module), "load its shaders");
			return module;
		}

		void makeDrawing()
		{
			// The whole window cleared to the background, then left to be shown.
			VkAttachmentDescription colour{};
			colour.format = format;
			colour.samples = VK_SAMPLE_COUNT_1_BIT;
			colour.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colour.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colour.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colour.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colour.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colour.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			VkAttachmentReference reference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &reference;
			VkSubpassDependency after{};
			after.srcSubpass = VK_SUBPASS_EXTERNAL;
			after.dstSubpass = 0;
			after.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			after.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			after.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			auto passAbout = blank<VkRenderPassCreateInfo>(VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
			passAbout.attachmentCount = 1;
			passAbout.pAttachments = &colour;
			passAbout.subpassCount = 1;
			passAbout.pSubpasses = &subpass;
			passAbout.dependencyCount = 1;
			passAbout.pDependencies = &after;
			check(vkCreateRenderPass(device, &passAbout, nullptr, &renderPass), "set up its drawing");

			VkDescriptorSetLayoutBinding picture{};
			picture.binding = 0;
			picture.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			picture.descriptorCount = 1;
			picture.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			auto setAbout = blank<VkDescriptorSetLayoutCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
			setAbout.bindingCount = 1;
			setAbout.pBindings = &picture;
			check(vkCreateDescriptorSetLayout(device, &setAbout, nullptr, &setLayout), "set up its drawing");

			VkPushConstantRange pushed{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Place) };
			auto layoutAbout = blank<VkPipelineLayoutCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
			layoutAbout.setLayoutCount = 1;
			layoutAbout.pSetLayouts = &setLayout;
			layoutAbout.pushConstantRangeCount = 1;
			layoutAbout.pPushConstantRanges = &pushed;
			check(vkCreatePipelineLayout(device, &layoutAbout, nullptr, &pipelineLayout), "set up its drawing");

			const VkShaderModule vertex = shader(kVertexShader, sizeof(kVertexShader));
			const VkShaderModule fragment = shader(kFragmentShader, sizeof(kFragmentShader));
			VkPipelineShaderStageCreateInfo stages[2]{};
			stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			stages[0].module = vertex;
			stages[0].pName = "main";
			stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			stages[1].module = fragment;
			stages[1].pName = "main";

			auto noVertices = blank<VkPipelineVertexInputStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
			auto strip = blank<VkPipelineInputAssemblyStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
			strip.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			auto port = blank<VkPipelineViewportStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
			port.viewportCount = 1;
			port.scissorCount = 1;
			// Rectangles are drawn whichever way round their corners come.
			auto fill = blank<VkPipelineRasterizationStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
			fill.polygonMode = VK_POLYGON_MODE_FILL;
			fill.cullMode = VK_CULL_MODE_NONE;
			fill.frontFace = VK_FRONT_FACE_CLOCKWISE;
			fill.lineWidth = 1.0f;
			auto samples = blank<VkPipelineMultisampleStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
			samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			VkPipelineColorBlendAttachmentState over{};
			over.blendEnable = VK_TRUE;
			over.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			over.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			over.colorBlendOp = VK_BLEND_OP_ADD;
			over.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			over.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			over.alphaBlendOp = VK_BLEND_OP_ADD;
			over.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			auto blend = blank<VkPipelineColorBlendStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
			blend.attachmentCount = 1;
			blend.pAttachments = &over;
			const VkDynamicState changing[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
			auto dynamic = blank<VkPipelineDynamicStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
			dynamic.dynamicStateCount = 2;
			dynamic.pDynamicStates = changing;

			auto about = blank<VkGraphicsPipelineCreateInfo>(VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
			about.stageCount = 2;
			about.pStages = stages;
			about.pVertexInputState = &noVertices;
			about.pInputAssemblyState = &strip;
			about.pViewportState = &port;
			about.pRasterizationState = &fill;
			about.pMultisampleState = &samples;
			about.pColorBlendState = &blend;
			about.pDynamicState = &dynamic;
			about.layout = pipelineLayout;
			about.renderPass = renderPass;
			const VkResult made = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &about, nullptr, &pipeline);
			vkDestroyShaderModule(device, vertex, nullptr);
			vkDestroyShaderModule(device, fragment, nullptr);
			check(made, "set up its drawing");

			auto sampler = blank<VkSamplerCreateInfo>(VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
			sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler.magFilter = VK_FILTER_NEAREST;
			sampler.minFilter = VK_FILTER_NEAREST;
			sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			check(vkCreateSampler(device, &sampler, nullptr, &hardEdges), "set up its drawing");
			sampler.magFilter = VK_FILTER_LINEAR;
			sampler.minFilter = VK_FILTER_LINEAR;
			check(vkCreateSampler(device, &sampler, nullptr, &smooth), "set up its drawing");

			auto poolAbout = blank<VkCommandPoolCreateInfo>(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
			poolAbout.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			poolAbout.queueFamilyIndex = family;
			check(vkCreateCommandPool(device, &poolAbout, nullptr, &commandPool), "set up its drawing");
			auto buffers = blank<VkCommandBufferAllocateInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
			buffers.commandPool = commandPool;
			buffers.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			buffers.commandBufferCount = 1;
			check(vkAllocateCommandBuffers(device, &buffers, &frame), "set up its drawing");
			check(vkAllocateCommandBuffers(device, &buffers, &upload), "set up its drawing");

			auto signalAbout = blank<VkSemaphoreCreateInfo>(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
			check(vkCreateSemaphore(device, &signalAbout, nullptr, &acquired), "set up its drawing");
			auto fenceAbout = blank<VkFenceCreateInfo>(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
			fenceAbout.flags = VK_FENCE_CREATE_SIGNALED_BIT;   // no frame to wait for at first
			check(vkCreateFence(device, &fenceAbout, nullptr, &done), "set up its drawing");
		}

		// A set for one picture, from a pool with room; a new pool when none has.
		VkDescriptorSet newSet(VkDescriptorPool& from)
		{
			auto about = blank<VkDescriptorSetAllocateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
			about.descriptorSetCount = 1;
			about.pSetLayouts = &setLayout;
			VkDescriptorSet set{ VK_NULL_HANDLE };
			constexpr std::uint32_t kSets = 64;
			auto room = std::find_if(pools.begin(), pools.end(), [](const Pool& each) { return each.left > 0; });
			if (room == pools.end())
			{
				VkDescriptorPoolSize size{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kSets };
				auto poolAbout = blank<VkDescriptorPoolCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
				poolAbout.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
				poolAbout.maxSets = kSets;
				poolAbout.poolSizeCount = 1;
				poolAbout.pPoolSizes = &size;
				Pool made;
				made.left = kSets;
				check(vkCreateDescriptorPool(device, &poolAbout, nullptr, &made.pool), "make room for its pictures");
				pools.push_back(made);
				room = pools.end() - 1;
			}
			about.descriptorPool = room->pool;
			check(vkAllocateDescriptorSets(device, &about, &set), "make room for its pictures");
			--room->left;
			from = room->pool;
			return set;
		}

		// The engine's picture of the object, at `scale`, copied to the
		// graphics card through a buffer the processor can write.
		void build(Object& object, float scale)
		{
			object.visualDirty = false;
			Visual& visual = visuals[object.name];
			visual.drawnAt = scale;
			if (visual.picture.image) { retired.push_back(visual.picture); }
			visual.picture = {};

			const std::shared_ptr<const Bitmap> source = object.drawnPicture(scale);
			if (source->rgba.empty() || source->width <= 0 || source->height <= 0) { return; }
			const std::uint32_t width = static_cast<std::uint32_t>(source->width);
			const std::uint32_t height = static_cast<std::uint32_t>(source->height);

			Picture picture;
			auto about = blank<VkImageCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
			about.imageType = VK_IMAGE_TYPE_2D;
			about.format = VK_FORMAT_R8G8B8A8_UNORM;
			about.extent = { width, height, 1 };
			about.mipLevels = 1;
			about.arrayLayers = 1;
			about.samples = VK_SAMPLE_COUNT_1_BIT;
			about.tiling = VK_IMAGE_TILING_OPTIMAL;
			about.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			about.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			about.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			check(vkCreateImage(device, &about, nullptr, &picture.image), "make a picture");
			VkMemoryRequirements needs{};
			vkGetImageMemoryRequirements(device, picture.image, &needs);
			auto allocate = blank<VkMemoryAllocateInfo>(VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
			allocate.allocationSize = needs.size;
			allocate.memoryTypeIndex = memoryType(needs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			if (vkAllocateMemory(device, &allocate, nullptr, &picture.memory) != VK_SUCCESS)
			{
				drop(picture);
				throw std::runtime_error("Vulkan could not find memory for a picture");
			}
			vkBindImageMemory(device, picture.image, picture.memory, 0);

			const VkDeviceSize bytes = source->rgba.size();
			const auto [staging, stagingMemory] = hostBuffer(bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
			void* mapped = nullptr;
			vkMapMemory(device, stagingMemory, 0, bytes, 0, &mapped);
			std::memcpy(mapped, source->rgba.data(), source->rgba.size());
			vkUnmapMemory(device, stagingMemory);

			auto begin = blank<VkCommandBufferBeginInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
			begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkResetCommandBuffer(upload, 0);
			vkBeginCommandBuffer(upload, &begin);
			barrier(upload, picture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			VkBufferImageCopy copy{};
			copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copy.imageExtent = { width, height, 1 };
			vkCmdCopyBufferToImage(upload, staging, picture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
			barrier(upload, picture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			vkEndCommandBuffer(upload);
			auto submit = blank<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO);
			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &upload;
			const VkResult sent = vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE);
			vkQueueWaitIdle(queue);
			vkDestroyBuffer(device, staging, nullptr);
			vkFreeMemory(device, stagingMemory, nullptr);
			if (sent != VK_SUCCESS)
			{
				drop(picture);
				throw std::runtime_error("Vulkan could not copy a picture to the graphics card");
			}

			auto viewAbout = blank<VkImageViewCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
			viewAbout.image = picture.image;
			viewAbout.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewAbout.format = VK_FORMAT_R8G8B8A8_UNORM;
			viewAbout.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			check(vkCreateImageView(device, &viewAbout, nullptr, &picture.view), "make a picture");

			picture.set = newSet(picture.pool);
			VkDescriptorImageInfo shown{};
			shown.sampler = object.drawnAtScale() ? smooth : hardEdges;
			shown.imageView = picture.view;
			shown.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			auto write = blank<VkWriteDescriptorSet>(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
			write.dstSet = picture.set;
			write.dstBinding = 0;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write.pImageInfo = &shown;
			vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
			visual.picture = picture;
		}

		// Ends the frame's drawing and hands it to the graphics card; with
		// `readBack`, the picture is copied into it too, once drawn.
		void submit(VkBuffer readBack)
		{
			vkCmdEndRenderPass(frame);
			if (readBack)
			{
				const VkImage image = images[shownImage];
				barrier(frame, image, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
				VkBufferImageCopy copy{};
				copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
				copy.imageExtent = { extent.width, extent.height, 1 };
				vkCmdCopyImageToBuffer(frame, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readBack, 1, &copy);
				barrier(frame, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
					VK_ACCESS_TRANSFER_READ_BIT, 0, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
			}
			vkEndCommandBuffer(frame);

			const VkPipelineStageFlags waitAt = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			auto about = blank<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO);
			about.waitSemaphoreCount = 1;
			about.pWaitSemaphores = &acquired;
			about.pWaitDstStageMask = &waitAt;
			about.commandBufferCount = 1;
			about.pCommandBuffers = &frame;
			about.signalSemaphoreCount = 1;
			about.pSignalSemaphores = &finished[shownImage];
			check(vkQueueSubmit(queue, 1, &about, done), "draw a frame");
			stage = Stage::Submitted;
		}
	};

	VulkanWindow::VulkanWindow(const WindowDesc& windowDesc) :
		parts(std::make_unique<Parts>())
	{
		Parts& p = *parts;
		p.gameWidth = std::max(windowDesc.width, 1.0f);
		p.gameHeight = std::max(windowDesc.height, 1.0f);
		const int times = std::max(1, windowDesc.scale);

		if (volkInitialize() != VK_SUCCESS) { throw std::runtime_error("Vulkan is not on this computer (no Vulkan library found)"); }
		// GLFW asks Vulkan for the window's surface through the same library.
		glfwInitVulkanLoader(vkGetInstanceProcAddr);
		if (!glfwInit()) { throw std::runtime_error("GLFW could not start"); }
		if (!glfwVulkanSupported())
		{
			glfwTerminate();
			throw std::runtime_error("GLFW cannot use Vulkan on this computer");
		}

		glfwDefaultWindowHints();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		// The whole screen at its own size, or the game's size times its scale.
		GLFWmonitor* monitor = windowDesc.fullscreen == "true" ? glfwGetPrimaryMonitor() : nullptr;
		const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
		const int wide = mode ? mode->width : static_cast<int>(p.gameWidth) * times;
		const int high = mode ? mode->height : static_cast<int>(p.gameHeight) * times;
		p.window = glfwCreateWindow(wide, high, windowDesc.name.c_str(), mode ? monitor : nullptr, nullptr);
		if (!p.window)
		{
			glfwTerminate();
			throw std::runtime_error("GLFW could not make a Vulkan window");
		}
		glfwSetWindowUserPointer(p.window, &p);
		glfwSetKeyCallback(p.window, &Parts::keyCallback);
		glfwSetFramebufferSizeCallback(p.window, &Parts::sizeCallback);

		std::uint32_t extensionCount = 0;
		const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
		auto application = blank<VkApplicationInfo>(VK_STRUCTURE_TYPE_APPLICATION_INFO);
		application.pApplicationName = windowDesc.name.c_str();
		application.pEngineName = "XML Game Engine";
		application.apiVersion = VK_API_VERSION_1_0;
		auto about = blank<VkInstanceCreateInfo>(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
		about.pApplicationInfo = &application;
		about.enabledExtensionCount = extensionCount;
		about.ppEnabledExtensionNames = extensions;
		check(vkCreateInstance(&about, nullptr, &p.instance), "start (no driver for this graphics card)");
		volkLoadInstance(p.instance);
		check(glfwCreateWindowSurface(p.instance, p.window, nullptr, &p.surface), "draw in a window here");

		p.makeDevice();
		p.chooseFormat();
		p.makeDrawing();
		if (!p.makeSwapchain()) { p.fitGame(); }
		setPictureScale(p.fit);
	}

	VulkanWindow::~VulkanWindow() = default;

	bool VulkanWindow::isOpen() const
	{
		return parts->window && !glfwWindowShouldClose(parts->window);
	}

	void VulkanWindow::close()
	{
		// Not torn down here: the frame that is running still has to draw. The
		// destructor does that.
		if (parts->window) { glfwSetWindowShouldClose(parts->window, GLFW_TRUE); }
	}

	std::pair<int, int> VulkanWindow::position() const
	{
		int x = 0;
		int y = 0;
		glfwGetWindowPos(parts->window, &x, &y);
		return { x, y };
	}

	void VulkanWindow::setPosition(int x, int y)
	{
		glfwSetWindowPos(parts->window, x, y);
	}

	void VulkanWindow::setTitle(const std::string& title)
	{
		glfwSetWindowTitle(parts->window, title.c_str());
	}

	void VulkanWindow::init(std::vector<Object>& objects)
	{
		for (auto& object : objects)
		{
			parts->build(object, pictureScale());
		}
	}

	std::vector<std::pair<KeyCode, bool>> VulkanWindow::pollEvents()
	{
		glfwPollEvents();
		std::vector<std::pair<KeyCode, bool>> events;
		events.swap(parts->keys);
		return events;
	}

	Controls VulkanWindow::controls()
	{
		Controls controls;
		const Parts& p = *parts;
		if (!p.window || glfwGetWindowAttrib(p.window, GLFW_FOCUSED) == 0) { return controls; }

		// The cursor is in the window's units, the game in its pixels: the
		// two differ on a screen that scales windows up.
		double x = 0.0;
		double y = 0.0;
		glfwGetCursorPos(p.window, &x, &y);
		int windowWidth = 0;
		int windowHeight = 0;
		glfwGetWindowSize(p.window, &windowWidth, &windowHeight);
		int pixelsWide = 0;
		int pixelsHigh = 0;
		glfwGetFramebufferSize(p.window, &pixelsWide, &pixelsHigh);
		controls.pointerInside = x >= 0.0 && y >= 0.0 && x < windowWidth && y < windowHeight;
		const double acrossX = windowWidth > 0 ? static_cast<double>(pixelsWide) / windowWidth : 1.0;
		const double acrossY = windowHeight > 0 ? static_cast<double>(pixelsHigh) / windowHeight : 1.0;
		controls.pointerX = (static_cast<float>(x * acrossX) - p.shownLeft) / p.fit;
		controls.pointerY = (static_cast<float>(y * acrossY) - p.shownTop) / p.fit;
		controls.mouse = { glfwGetMouseButton(p.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS,
			glfwGetMouseButton(p.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS,
			glfwGetMouseButton(p.window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS };
		readGlfwPads(controls);
		return controls;
	}

	void VulkanWindow::clear(const std::string& colorName)
	{
		Parts& p = *parts;
		if (!p.device || p.stage != Parts::Stage::Idle) { return; }

		// One frame at a time: the last one is done before this one starts,
		// and so are the pictures it drew.
		vkWaitForFences(p.device, 1, &p.done, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
		for (Parts::Picture& picture : p.retired) { p.drop(picture); }
		p.retired.clear();

		if ((p.resized || !p.swapchain) && !p.makeSwapchain()) { return; }   // minimised: nothing to draw
		const VkResult got = vkAcquireNextImageKHR(p.device, p.swapchain, std::numeric_limits<std::uint64_t>::max(),
			p.acquired, VK_NULL_HANDLE, &p.shownImage);
		if (got == VK_ERROR_OUT_OF_DATE_KHR)
		{
			p.resized = true;   // made again next frame
			return;
		}
		if (got == VK_SUBOPTIMAL_KHR) { p.resized = true; }
		else { check(got, "show a frame"); }
		vkResetFences(p.device, 1, &p.done);

		auto begin = blank<VkCommandBufferBeginInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
		begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkResetCommandBuffer(p.frame, 0);
		vkBeginCommandBuffer(p.frame, &begin);

		// The whole window in the background's colour, bars included.
		const Color c = colorFromName(colorName);
		VkClearValue background{};
		background.color = { { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f } };
		auto pass = blank<VkRenderPassBeginInfo>(VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
		pass.renderPass = p.renderPass;
		pass.framebuffer = p.framebuffers[p.shownImage];
		pass.renderArea = { { 0, 0 }, p.extent };
		pass.clearValueCount = 1;
		pass.pClearValues = &background;
		vkCmdBeginRenderPass(p.frame, &pass, VK_SUBPASS_CONTENTS_INLINE);

		// The game where it is shown; nothing drawn over the bars.
		const VkViewport port{ p.shownLeft, p.shownTop, p.shownWidth, p.shownHeight, 0.0f, 1.0f };
		vkCmdSetViewport(p.frame, 0, 1, &port);
		const VkRect2D inside{ { static_cast<std::int32_t>(p.shownLeft), static_cast<std::int32_t>(p.shownTop) },
			{ static_cast<std::uint32_t>(p.shownWidth), static_cast<std::uint32_t>(p.shownHeight) } };
		vkCmdSetScissor(p.frame, 0, 1, &inside);
		vkCmdBindPipeline(p.frame, VK_PIPELINE_BIND_POINT_GRAPHICS, p.pipeline);
		p.stage = Parts::Stage::Recording;
	}

	void VulkanWindow::prepare(Object& object)
	{
		Parts& p = *parts;
		if (!p.device) { return; }
		const auto found = p.visuals.find(object.name);
		if (object.visualDirty || found == p.visuals.end() || (object.drawnAtScale() && found->second.drawnAt != pictureScale()))
		{
			p.build(object, pictureScale());
		}
	}

	void VulkanWindow::draw(Object& object)
	{
		Parts& p = *parts;
		if (p.stage != Parts::Stage::Recording) { return; }
		prepare(object);
		const Parts::Visual& visual = p.visuals[object.name];
		if (!visual.picture.set) { return; }

		// Whole game pixels, so a picture is never sampled between two of
		// its pixels; stretched over the object's size.
		const Vector2f size = object.pictureSize();
		const float left = std::floor(object.position.x);
		const float top = std::floor(object.position.y);
		const Place place{ left, top, left + size.x, top + size.y, p.gameWidth, p.gameHeight };
		vkCmdBindDescriptorSets(p.frame, VK_PIPELINE_BIND_POINT_GRAPHICS, p.pipelineLayout, 0, 1, &visual.picture.set, 0, nullptr);
		vkCmdPushConstants(p.frame, p.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(place), &place);
		vkCmdDraw(p.frame, 4, 1, 0, 0);
	}

	void VulkanWindow::display()
	{
		Parts& p = *parts;
		if (p.stage == Parts::Stage::Recording) { p.submit(VK_NULL_HANDLE); }
		if (p.stage != Parts::Stage::Submitted) { return; }
		p.stage = Parts::Stage::Idle;

		auto show = blank<VkPresentInfoKHR>(VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
		show.waitSemaphoreCount = 1;
		show.pWaitSemaphores = &p.finished[p.shownImage];
		show.swapchainCount = 1;
		show.pSwapchains = &p.swapchain;
		show.pImageIndices = &p.shownImage;
		const VkResult shown = vkQueuePresentKHR(p.queue, &show);
		if (shown == VK_ERROR_OUT_OF_DATE_KHR || shown == VK_SUBOPTIMAL_KHR) { p.resized = true; }
	}

	float VulkanWindow::fit() const
	{
		return parts->fit;
	}

	Bitmap VulkanWindow::capture()
	{
		Parts& p = *parts;
		if (p.stage != Parts::Stage::Recording || !p.canRead) { return {}; }

		// The frame is finished now and copied out; display() then shows it.
		const VkDeviceSize bytes = static_cast<VkDeviceSize>(p.extent.width) * p.extent.height * 4;
		const auto [buffer, memory] = p.hostBuffer(bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		p.submit(buffer);
		vkWaitForFences(p.device, 1, &p.done, VK_TRUE, std::numeric_limits<std::uint64_t>::max());

		Bitmap bitmap;
		bitmap.width = static_cast<int>(p.extent.width);
		bitmap.height = static_cast<int>(p.extent.height);
		bitmap.rgba.resize(static_cast<std::size_t>(bytes));
		void* mapped = nullptr;
		vkMapMemory(p.device, memory, 0, bytes, 0, &mapped);
		std::memcpy(bitmap.rgba.data(), mapped, bitmap.rgba.size());
		vkUnmapMemory(p.device, memory);
		vkDestroyBuffer(p.device, buffer, nullptr);
		vkFreeMemory(p.device, memory, nullptr);

		// Blue first in the window's own format; the picture is red first, and
		// the window has no see-through.
		const bool blueFirst = p.format == VK_FORMAT_B8G8R8A8_UNORM;
		for (std::size_t at = 0; at < bitmap.rgba.size(); at += 4)
		{
			if (blueFirst) { std::swap(bitmap.rgba[at], bitmap.rgba[at + 2]); }
			bitmap.rgba[at + 3] = 255;
		}
		return bitmap;
	}
}
