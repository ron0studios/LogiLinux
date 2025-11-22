
#include "device_manager.h"
#include "../devices/dialpad_device.h"
#include "../devices/mx_keypad_device.h"

#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace LogiLinux {

constexpr uint16_t LOGITECH_VENDOR_ID = 0x046d;

constexpr uint16_t MX_DIALPAD_PRODUCT_ID = 0xbc00;
constexpr uint16_t MX_KEYPAD_PRODUCT_ID = 0xc354;

DeviceManager::DeviceManager() {}

DeviceManager::~DeviceManager() {}

std::vector<DevicePtr> DeviceManager::scanDevices() {
  discovered_devices_.clear();

  // Scan /dev/input/event* devices
  DIR *dir = opendir("/dev/input");
  if (dir) {
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

          case DeviceType::MX_KEYPAD:
            device = std::make_shared<MXKeypadDevice>(*info);
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
  }

  // Also scan /dev/hidraw* devices for devices that don't have event interfaces
  for (int i = 0; i < 20; i++) {
    std::string hidraw_path = "/dev/hidraw" + std::to_string(i);
    auto info = probeHidrawDevice(hidraw_path);
    if (info) {
      DevicePtr device;

      switch (info->type) {
      case DeviceType::MX_KEYPAD:
        device = std::make_shared<MXKeypadDevice>(*info);
        break;

      default:
        break;
      }

      if (device) {
        discovered_devices_.push_back(device);
      }
    }
  }

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

std::unique_ptr<DeviceInfo>
DeviceManager::probeHidrawDevice(const std::string &device_path) {
  int fd = open(device_path.c_str(), O_RDWR | O_NONBLOCK);
  if (fd < 0) {
    return nullptr;
  }

  struct hidraw_devinfo info;
  if (ioctl(fd, HIDIOCGRAWINFO, &info) < 0) {
    close(fd);
    return nullptr;
  }

  if (info.vendor != LOGITECH_VENDOR_ID) {
    close(fd);
    return nullptr;
  }

  char name[256] = "Unknown";
  ioctl(fd, HIDIOCGRAWNAME(sizeof(name)), name);

  close(fd);

  DeviceType type = identifyDeviceType(info.vendor, info.product);

  if (type == DeviceType::UNKNOWN) {
    return nullptr;
  }

  auto device_info = std::make_unique<DeviceInfo>();
  device_info->name = name;
  device_info->device_path = device_path;
  device_info->vendor_id = info.vendor;
  device_info->product_id = info.product;
  device_info->type = type;

  return device_info;
}

DeviceType DeviceManager::identifyDeviceType(uint16_t vendor_id,
                                             uint16_t product_id) {
  if (vendor_id != LOGITECH_VENDOR_ID) {
    return DeviceType::UNKNOWN;
  }

  switch (product_id) {
  case MX_DIALPAD_PRODUCT_ID:
    return DeviceType::DIALPAD;

  case MX_KEYPAD_PRODUCT_ID:
    return DeviceType::MX_KEYPAD;

  default:
    return DeviceType::UNKNOWN;
  }
}

} // namespace LogiLinux
