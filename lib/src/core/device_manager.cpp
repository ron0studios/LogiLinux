
#include "device_manager.h"
#include "../devices/dialpad_device.h"

#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace LogiLinux {

constexpr uint16_t LOGITECH_VENDOR_ID = 0x046d;

constexpr uint16_t MX_DIALPAD_PRODUCT_ID = 0xbc00;

DeviceManager::DeviceManager() {}

DeviceManager::~DeviceManager() {}

std::vector<DevicePtr> DeviceManager::scanDevices() {
  discovered_devices_.clear();

  DIR *dir = opendir("/dev/input");
  if (!dir) {
    return discovered_devices_;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    std::string name = entry->d_name;

    if (name.find("event") == 0) {
      std::string device_path = "/dev/input/" + name;

      auto info = probeDevice(device_path);
      if (info) {
        DevicePtr device;

        switch (info->type) {
        case DeviceType::DIALPAD:
          device = std::make_shared<DialpadDevice>(*info);
          break;

        case DeviceType::CREATIVE_CONSOLE:
          break;

        default:
          break;
        }

        if (device) {
          discovered_devices_.push_back(device);
        }
      }
    }
  }

  closedir(dir);
  return discovered_devices_;
}

std::vector<DevicePtr> DeviceManager::findDevicesByType(DeviceType type) {
  std::vector<DevicePtr> result;

  for (const auto &device : discovered_devices_) {
    if (device->getType() == type) {
      result.push_back(device);
    }
  }

  return result;
}

std::unique_ptr<DeviceInfo>
DeviceManager::probeDevice(const std::string &device_path) {
  int fd = open(device_path.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    return nullptr;
  }

  struct input_id id;
  if (ioctl(fd, EVIOCGID, &id) < 0) {
    close(fd);
    return nullptr;
  }

  if (id.vendor != LOGITECH_VENDOR_ID) {
    close(fd);
    return nullptr;
  }

  char name[256] = "Unknown";
  ioctl(fd, EVIOCGNAME(sizeof(name)), name);

  close(fd);

  DeviceType type = identifyDeviceType(id.vendor, id.product);

  if (type == DeviceType::UNKNOWN) {
    return nullptr;
  }

  auto info = std::make_unique<DeviceInfo>();
  info->name = name;
  info->device_path = device_path;
  info->vendor_id = id.vendor;
  info->product_id = id.product;
  info->type = type;

  return info;
}

DeviceType DeviceManager::identifyDeviceType(uint16_t vendor_id,
                                             uint16_t product_id) {
  if (vendor_id != LOGITECH_VENDOR_ID) {
    return DeviceType::UNKNOWN;
  }

  switch (product_id) {
  case MX_DIALPAD_PRODUCT_ID:
    return DeviceType::DIALPAD;

    // case MX_CREATIVE_CONSOLE_PRODUCT_ID:
    //     return DeviceType::CREATIVE_CONSOLE;

  default:
    return DeviceType::UNKNOWN;
  }
}

} // namespace LogiLinux
