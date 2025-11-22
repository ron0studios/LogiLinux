
#include "core/device_manager.h"
#include "logilinux/logilinux.h"
#include "logilinux/version.h"
#include <algorithm>

namespace LogiLinux {

class Library::Impl {
public:
  Impl() : device_manager_(std::make_unique<DeviceManager>()) {}

  std::unique_ptr<DeviceManager> device_manager_;
  std::vector<DevicePtr> devices_;
};

Library::Library() : pImpl(std::make_unique<Impl>()) {}

Library::~Library() = default;

Version Library::getVersion() { return LogiLinux::getVersion(); }

std::vector<DevicePtr> Library::discoverDevices() {
  pImpl->devices_ = pImpl->device_manager_->scanDevices();
  return pImpl->devices_;
}

DevicePtr Library::findDevice(DeviceType type) {
  if (pImpl->devices_.empty()) {
    discoverDevices();
  }

  for (const auto &device : pImpl->devices_) {
    if (device->getType() == type) {
      return device;
    }
  }

  return nullptr;
}

std::vector<DevicePtr> Library::findDevices(DeviceType type) {
  if (pImpl->devices_.empty()) {
    discoverDevices();
  }

  std::vector<DevicePtr> result;
  for (const auto &device : pImpl->devices_) {
    if (device->getType() == type) {
      result.push_back(device);
    }
  }

  return result;
}

} // namespace LogiLinux
