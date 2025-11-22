#ifndef LOGILINUX_H
#define LOGILINUX_H

#include "device.h"
#include "events.h"
#include "version.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace LogiLinux {

// Forward declaration for Creative Console device-specific API
class CreativeConsoleDevice;

namespace detail {
bool hasLCD(CreativeConsoleDevice *device);
}


class Library {
public:
  Library();
  ~Library();

  Library(const Library &) = delete;
  Library &operator=(const Library &) = delete;

  std::vector<DevicePtr> discoverDevices();

  DevicePtr findDevice(DeviceType type);

  std::vector<DevicePtr> findDevices(DeviceType type);

  static Version getVersion();

private:
  class Impl;
  std::unique_ptr<Impl> pImpl;
};

} // namespace LogiLinux

#endif // LOGILINUX_H
