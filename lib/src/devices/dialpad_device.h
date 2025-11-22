
#ifndef LOGILINUX_DIALPAD_DEVICE_H
#define LOGILINUX_DIALPAD_DEVICE_H

#include "../core/input_monitor.h"
#include "logilinux/device.h"
#include <memory>

namespace LogiLinux {

class DialpadDevice : public Device {
public:
  explicit DialpadDevice(const DeviceInfo &info);
  ~DialpadDevice() override;

  const DeviceInfo &getInfo() const override { return info_; }
  DeviceType getType() const override { return info_.type; }
  bool hasCapability(DeviceCapability cap) const override;

  void setEventCallback(EventCallback callback) override;
  void startMonitoring() override;
  void stopMonitoring() override;
  bool isMonitoring() const override;

  bool grabExclusive(bool grab) override;

private:
  DeviceInfo info_;
  std::vector<DeviceCapability> capabilities_;
  EventCallback event_callback_;
  std::unique_ptr<InputMonitor> monitor_;
};

} // namespace LogiLinux

#endif // LOGILINUX_DIALPAD_DEVICE_H
