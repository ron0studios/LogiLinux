
#ifndef LOGILINUX_DEVICE_MANAGER_H
#define LOGILINUX_DEVICE_MANAGER_H

#include "logilinux/device.h"
#include <memory>
#include <vector>

namespace LogiLinux {

class DeviceManager {
public:
  DeviceManager();
  ~DeviceManager();

  /**
   * Scan for all Logitech devices
   */
  std::vector<DevicePtr> scanDevices();

  /**
   * Find devices by type
   */
  std::vector<DevicePtr> findDevicesByType(DeviceType type);

private:
  /**
   * Check if a device path is a Logitech device
   * Returns DeviceInfo if valid, nullptr otherwise
   */
  std::unique_ptr<DeviceInfo> probeDevice(const std::string &device_path);

  /**
   * Check if a hidraw device is a Logitech device
   * Returns DeviceInfo if valid, nullptr otherwise
   */
  std::unique_ptr<DeviceInfo> probeHidrawDevice(const std::string &device_path);

  /**
   * Identify device type from VID/PID
   */
  DeviceType identifyDeviceType(uint16_t vendor_id, uint16_t product_id);

  std::vector<DevicePtr> discovered_devices_;
};

} // namespace LogiLinux

#endif // LOGILINUX_DEVICE_MANAGER_H
