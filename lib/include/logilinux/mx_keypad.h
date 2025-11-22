#ifndef LOGILINUX_MX_KEYPAD_H
#define LOGILINUX_MX_KEYPAD_H

#include "device.h"
#include <cstdint>
#include <vector>

namespace LogiLinux {

// Note: This is a declaration-only header for accessing MX Keypad API
// The actual implementation is in the library

/**
 * Cast a Device pointer to access MX Keypad specific features
 */
inline bool initializeCreativeConsole(Device *device) {
  // This will be linked from the library implementation
  extern bool mx_keypad_initialize(void *device);
  return mx_keypad_initialize(device);
}

inline bool setCreativeConsoleKeyImage(Device *device, int keyIndex,
                                       const std::vector<uint8_t> &jpegData) {
  extern bool mx_keypad_set_key_image(void *device, int keyIndex,
                                             const uint8_t *data, size_t size);
  return mx_keypad_set_key_image(device, keyIndex, jpegData.data(),
                                       jpegData.size());
}

} // namespace LogiLinux

#endif // LOGILINUX_MX_KEYPAD_H
