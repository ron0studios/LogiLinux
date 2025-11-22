# LogiLinux Library

A C++ library for interfacing with Logitech input devices on Linux, starting with the MX Dialpad.

## Features

- **Device Discovery**: Automatically finds connected Logitech devices
- **Event Monitoring**: Captures rotation and button events
- **High-Resolution Support**: Full support for high-resolution rotation events
- **Modern C++ API**: Clean, type-safe C++17 interface
- **Thread-Safe**: Event monitoring runs in background threads

## Requirements

- Linux with input subsystem support
- C++17 compatible compiler
- CMake 3.10 or later
- pthread

## Building

```bash
mkdir build && cd build
cmake ..
make
```

This builds:

- `liblogilinux.so` - The shared library
- `dialpad-example` - Basic event monitoring example
- `volume-example` - System volume control example

## Installation

```bash
sudo make install
```

This installs:

- Library: `/usr/local/lib/liblogilinux.so`
- Headers: `/usr/local/include/logilinux/`
- pkg-config: `/usr/local/lib/pkgconfig/logilinux.pc`

## Usage

### Basic Example

```cpp
#include <logilinux/logilinux.h>
#include <logilinux/events.h>
#include <iostream>

void onEvent(LogiLinux::EventPtr event) {
    if (auto rotation = std::dynamic_pointer_cast<LogiLinux::RotationEvent>(event)) {
        std::cout << "Rotated: " << rotation->delta << " steps" << std::endl;
    }
}

int main() {
    LogiLinux::Library lib;

    auto dialpad = lib.findDevice(LogiLinux::DeviceType::DIALPAD);
    if (!dialpad) {
        std::cerr << "No dialpad found!" << std::endl;
        return 1;
    }

    dialpad->setEventCallback(onEvent);
    dialpad->startMonitoring();

    // ... your application logic ...

    dialpad->stopMonitoring();
    return 0;
}
```

### Compiling with LogiLinux

```bash
# Using pkg-config
g++ -std=c++17 myapp.cpp $(pkg-config --cflags --libs logilinux)

# Or manually
g++ -std=c++17 myapp.cpp -llogilinux
```

## API Overview

### Main Classes

- **`Library`**: Main entry point for device discovery
- **`Device`**: Abstract base class for all devices
- **`DialpadDevice`**: MX Dialpad specific implementation

### Event Types

- **`RotationEvent`**: Dial rotation events
  - `delta`: Rotation in steps (low-resolution)
  - `delta_high_res`: High-resolution rotation (120 units = 1 degree)
- **`ButtonEvent`**: Button press/release events
  - `button_code`: Linux input button code
  - `pressed`: Current button state

### Device Discovery

```cpp
LogiLinux::Library lib;

// Find all devices
auto devices = lib.discoverDevices();

auto dialpad = lib.findDevice(LogiLinux::DeviceType::DIALPAD);

auto dialpads = lib.findDevices(LogiLinux::DeviceType::DIALPAD);
```

## Permissions

To access input devices without root, add your user to the `input` group:

```bash
sudo usermod -a -G input $USER
```

Or use the PoC udev rules (if available).

## Supported Devices

Currently supported:

- **Logitech MX Dialpad** (PID: 0xbc00)

Planned support:

- Logitech MX Creative Console

## Examples

See the `examples/` directory for complete working examples:

- **dialpad-example**: Basic event monitoring
- **volume-example**: System volume control using the dialpad

## License

See LICENSE file for details.

## Development

This is part of a larger project to support Logitech Creator devices on Linux.

Current version: **0.1.0-dev** (Phase 1: Foundation)

See `DEVELOPMENT_PLAN.md` for the complete roadmap.
