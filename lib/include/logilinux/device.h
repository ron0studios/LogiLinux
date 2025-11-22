#ifndef LOGILINUX_DEVICE_H
#define LOGILINUX_DEVICE_H

#include "events.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace LogiLinux {

enum class DeviceType {
  UNKNOWN,
  DIALPAD,
  MX_KEYPAD,
};

enum class DeviceCapability {
  ROTATION,
  BUTTONS,
  HIGH_RES_SCROLL,
  LCD_DISPLAY,
  IMAGE_UPLOAD,
};

struct DeviceInfo {
  std::string name;
  std::string device_path;
  uint16_t vendor_id;
  uint16_t product_id;
  DeviceType type;
};

class Device {
public:
  virtual ~Device() = default;

  virtual const DeviceInfo &getInfo() const = 0;
  virtual DeviceType getType() const = 0;

  virtual bool hasCapability(DeviceCapability cap) const = 0;

  virtual void setEventCallback(EventCallback callback) = 0;
  virtual void startMonitoring() = 0;
  virtual void stopMonitoring() = 0;
  virtual bool isMonitoring() const = 0;

  virtual bool grabExclusive(bool grab) = 0;
};

using DevicePtr = std::shared_ptr<Device>;

} // namespace LogiLinux

#endif // LOGILINUX_DEVICE_H
