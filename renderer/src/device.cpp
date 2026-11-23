#include "device.hpp"

#include <iostream>
#include <algorithm>

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data) {
  std::cerr << "validation layer: type " << to_string(type) << " msg: " << p_callback_data->pMessage << std::endl;
  return vk::False;
}

Device::Device(const std::vector<const char*>& extension_names, VulkanWindowProviders* providers, bool enable_validation_layers) : extensions_(extension_names), window_providers(providers), enable_validation_layers_(enable_validation_layers) { 

}

void Device::build_context(VulkanContext& context) {
  if (!create_instance(context)) {
    throw std::runtime_error("Unable to create vulkan instance");
  }
  if (!setup_debug_messenger(context) && enable_validation_layers_) {
    throw std::runtime_error("Unable to setup instance debug messenger");
  }
  if (!pick_physical_device(context)) {
    throw std::runtime_error("Unable to find suitable GPU");
  }
  if (!create_logical_device(context)) {
    throw std::runtime_error("Unable to create logical device");
  }
}

bool Device::create_instance(VulkanContext& context) {
  vk::ApplicationInfo appInfo {
    .pApplicationName = "Vulkan",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "Pluto Engine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = vk::ApiVersion13
  };

  // Add validation layers here
  if (!enable_validation_layers_)
  {
    validation_layers_.clear();
  }

  auto layer_properties = context.context.enumerateInstanceLayerProperties();
  auto any = [&layer_properties](auto const& required_layer) {
    auto none = [required_layer](auto const& layer_property) {
      return std::strcmp(layer_property.layerName, required_layer) == 0;
    };
    return std::ranges::none_of(layer_properties, none);
  };
  if (std::ranges::any_of(validation_layers_, any)) {
    std::cout << "One or more required validation layers not supported\n";
    return false;
  }
  std::cout << "All Vulkan Validation layers are supported\n";

  // SDL required instance extensions
  auto extension_properties = context.context.enumerateInstanceExtensionProperties();
  for (uint32_t i = 0; i < extensions_.size(); i++) {
    std::cout << "SDL Extension: " << extensions_[i] << std::endl;
    auto match = [extension = extensions_[i]](auto const& extension_property) {
      return std::strcmp(extension_property.extensionName, extension) == 0;
    };
    if (std::ranges::none_of(extension_properties, match)) {
      std::cout << "Required SDL extensions not supported\n";
      return false;
    }
  }
  if (enable_validation_layers_) {
    extensions_.push_back(vk::EXTDebugUtilsExtensionName);
  }
  std::cout << "All SDL extensions supported\n";

  vk::InstanceCreateInfo create_info {
    .pApplicationInfo = &appInfo,
    .enabledLayerCount = static_cast<uint32_t>(validation_layers_.size()),
    .ppEnabledLayerNames = validation_layers_.data(),
    .enabledExtensionCount = static_cast<uint32_t>(extensions_.size()),
    .ppEnabledExtensionNames = extensions_.data()
  };
  context.instance = vk::raii::Instance(context.context, create_info);
  if (context.instance == nullptr) {
    std::cout << "Unable to create vulkan instance\n";
    return false;
  }
  std::cout << "Created Vulkan Instance\n";
  return true;
}

bool Device::setup_debug_messenger(VulkanContext& context) {
  if (!enable_validation_layers_) {return false;}

  vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
  vk::DebugUtilsMessageTypeFlagsEXT type_flags(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral);
  vk::DebugUtilsMessengerCreateInfoEXT create_info {
    .messageSeverity = severity_flags,
    .messageType = type_flags,
    .pfnUserCallback = &debug_callback
  };

  context.debug_messenger_ = context.instance.createDebugUtilsMessengerEXT(create_info);
  std::cout << "Created Vulkan debug messenger\n";
  return true;
}

bool Device::pick_physical_device(VulkanContext& context) {
  auto devices = context.instance.enumeratePhysicalDevices();

  if (devices.empty()) {
    throw std::runtime_error("Failed to find capable GPU");
  }

  // Check device for Vulkan 1.3, queue family support, and extension support
  vk::raii::PhysicalDevice chosen_device = nullptr;
  for (const auto& device : devices) {
    // Vulkan 1.3 support
    bool is_suitable = device.getProperties().apiVersion >= VK_API_VERSION_1_3;

    // Queue support
    auto queue_families = device.getQueueFamilyProperties();
    // Find graphics support
    vk::QueueFlags zero = static_cast<vk::QueueFlags>(0);
    auto find_graphics = [zero](vk::QueueFamilyProperties const& qfp) {
      return ((qfp.queueFlags & vk::QueueFlagBits::eGraphics) != zero);
    };
    // Find dedicated transfer queue support (i.e. only transfer not graphics)
    auto find_transfer = [zero](vk::QueueFamilyProperties const& qfp) {
      return ((qfp.queueFlags & vk::QueueFlagBits::eTransfer) != zero) &&
             ((qfp.queueFlags & vk::QueueFlagBits::eGraphics) == zero);
    };
    // Find compute queue support
    auto find_compute = [zero](vk::QueueFamilyProperties const& qfp) {
      return ((qfp.queueFlags & vk::QueueFlagBits::eCompute) != zero);
    };
    // Check if suitable for each queue
    is_suitable &= (std::ranges::find_if(queue_families, find_graphics) != queue_families.end());
    is_suitable &= (std::ranges::find_if(queue_families, find_transfer) != queue_families.end());
    is_suitable &= (std::ranges::find_if(queue_families, find_compute) != queue_families.end());

    // Check extension support
    auto device_extensions = device.enumerateDeviceExtensionProperties();
    bool found = true;
    for (const auto& extension : required_extensions_) {
      auto find = [extension](auto const& ext) {
        return std::strcmp(ext.extensionName, extension) == 0;
      };
      found = found & (std::ranges::find_if(device_extensions, find) != device_extensions.end());
    }
    is_suitable &= found;

    if (is_suitable) {
      if (device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
        chosen_device = device;
        std::cout << "Device Chosen: " << device.getProperties().deviceName << std::endl;
        break;
      } else {
        chosen_device = device;
      }
    }
  }
  context.physical_device = std::move(chosen_device);
  if (context.physical_device == nullptr) {
    std::cout << "Unable to select suitable physical device\n";
    return false;
  }
  std::cout << "Found suitable physical device\n";
  return true;
}

uint32_t Device::find_graphics_queue(vk::raii::PhysicalDevice& physical_device, VkSurfaceKHR surface) {
  std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();

  uint32_t queue_index = queue_family_properties.size();
  for (uint32_t qfp = 0; qfp < queue_family_properties.size(); qfp++) {
    if ((queue_family_properties[qfp].queueFlags & vk::QueueFlagBits::eGraphics) && physical_device.getSurfaceSupportKHR(qfp, surface)) {
      queue_index = qfp;
      break;
    }
  }
  if (queue_index == queue_family_properties.size()) {
    throw std::runtime_error("Unable to find graphics queue family support");
  }
  return queue_index;
}

uint32_t Device::find_transfer_queue(vk::raii::PhysicalDevice& physical_device) {
  std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();

  uint32_t queue_index = queue_family_properties.size();
  for (uint32_t qfp = 0; qfp < queue_family_properties.size(); qfp++) {
    if ((queue_family_properties[qfp].queueFlags & vk::QueueFlagBits::eTransfer) && ((queue_family_properties[qfp].queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits{})) {
      queue_index = qfp;
      break;
    }
  }
  if (queue_index == queue_family_properties.size()) {
    throw std::runtime_error("Unable to dedicated transfer queue family support");
  }
  return queue_index;
}

// TODO find compute queue

bool Device::create_logical_device(VulkanContext& context) {
  
  if (!window_providers->surface_provider(context.instance, context.surface)) {
    throw std::runtime_error("Unable to create surface");
  }

  // get queue indices
  context.queue_indices.graphics = find_graphics_queue(context.physical_device, *context.surface);
  context.queue_indices.transfer = find_transfer_queue(context.physical_device);

  // all queues must have priority. Setting them all equal for now
  float queue_priority = 0.0f;

  // queue create info structs
  vk::DeviceQueueCreateInfo device_graphics_queue_create_info {
    .queueFamilyIndex = context.queue_indices.graphics,
    .queueCount = 1,
    .pQueuePriorities = &queue_priority
  };
  vk::DeviceQueueCreateInfo device_transfer_queue_create_info {
    .queueFamilyIndex = context.queue_indices.transfer,
    .queueCount = 1,
    .pQueuePriorities = &queue_priority
  };
  vk::DeviceQueueCreateInfo queue_create_infos[] = {device_graphics_queue_create_info, device_transfer_queue_create_info};

  // Enabling additional device features
  vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT, vk::PhysicalDeviceVulkan12Features> feature_chain = {
    {.features = {.samplerAnisotropy = true, .shaderInt64 = true}}, // vk::PhysicalDeviceFeatures2
    {.synchronization2 = true, .dynamicRendering = true}, // vk::PhysicalDeviceVulkan13Features
    {.extendedDynamicState = true}, // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    {.bufferDeviceAddress = true}
  };

  // create device
  vk::DeviceCreateInfo device_create_info {
    .pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>(),
    .queueCreateInfoCount = 2,
    .pQueueCreateInfos = queue_create_infos,
    .enabledExtensionCount = static_cast<uint32_t>(required_extensions_.size()),
    .ppEnabledExtensionNames = required_extensions_.data()
  };
  context.device = vk::raii::Device(context.physical_device, device_create_info);
  if (context.device == nullptr) {
    std::cout << "Unable to create logical device\n";
    return false;
  }
  std::cout << "Created logical device\n";
  context.queues.graphics = vk::raii::Queue(context.device, context.queue_indices.graphics, 0);
  if (context.queues.graphics == nullptr) {
    std::cout << "Unable to create graphics queue\n";
    return false;
  }
  std::cout << "Created graphics queue\n";
  context.queues.transfer = vk::raii::Queue(context.device, context.queue_indices.transfer, 0);
  if (context.queues.transfer == nullptr) {
    std::cout << "Unable to create transfer queue\n";
    return false;
  }
  std::cout << "Created transfer queue\n";
  return true;
}
