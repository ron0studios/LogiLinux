
#include "dialpad_device.h"
#include <algorithm>

namespace LogiLinux {

DialpadDevice::DialpadDevice(const DeviceInfo &info)
    : info_(info), monitor_(std::make_unique<InputMonitor>(info.device_path)) {

  capabilities_.push_back(DeviceCapability::ROTATION);
  capabilities_.push_back(DeviceCapability::BUTTONS);
  capabilities_.push_back(DeviceCapability::HIGH_RES_SCROLL);
}

DialpadDevice::~DialpadDevice() { stopMonitoring(); }

bool DialpadDevice::hasCapability(DeviceCapability cap) const {
  return std::find(capabilities_.begin(), capabilities_.end(), cap) !=
         capabilities_.end();
}

void DialpadDevice::setEventCallback(EventCallback callback) {
  event_callback_ = callback;
}

void DialpadDevice::startMonitoring() {
  if (event_callback_) {
    monitor_->start(event_callback_);
  }
}

void DialpadDevice::stopMonitoring() { monitor_->stop(); }

bool DialpadDevice::isMonitoring() const { return monitor_->isRunning(); }

bool DialpadDevice::grabExclusive(bool grab) {
  return monitor_->grabDevice(grab);
}

} // namespace LogiLinux
