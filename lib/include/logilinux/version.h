#ifndef LOGILINUX_VERSION_H
#define LOGILINUX_VERSION_H

#define LOGILINUX_VERSION_MAJOR 0
#define LOGILINUX_VERSION_MINOR 1
#define LOGILINUX_VERSION_PATCH 0

#define LOGILINUX_VERSION_STRING "0.1.0-dev"

namespace LogiLinux {

struct Version {
  int major;
  int minor;
  int patch;
  const char *string;
};

inline Version getVersion() {
  return {LOGILINUX_VERSION_MAJOR, LOGILINUX_VERSION_MINOR,
          LOGILINUX_VERSION_PATCH, LOGILINUX_VERSION_STRING};
}

} // namespace LogiLinux

#endif // LOGILINUX_VERSION_H
