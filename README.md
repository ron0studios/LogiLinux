# LogiLinux - Logitech Device Library for Linux

A C++ library and tools for interfacing with Logitech Creator devices on Linux, starting with the MX Dialpad. This was made for LauzHack 2025.

## What is This?

LogiLinux provides a clean C++ library (liblogilinux) for working with Logitech input devices on Linux. It handles device discovery, event monitoring, and provides a type-safe API for building applications.

## Quick Start

### Building

```bash
./build.sh
```

Or manually:

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Running Examples

```bash
sudo ./build/examples/dialpad-example
sudo ./build/examples/volume-example
```

## Key Discovery

The MX Dialpad works as a standard Linux input device and sends events through the input subsystem. When you rotate the dial, it sends:

- REL_HWHEEL (axis 6): Low-resolution steps (1-6 units per tick)
- REL_MISC (axis 12): High-resolution angle (120 units per degree)
- Positive values = clockwise, negative = counter-clockwise

## Using the Library

### Simple Example

```cpp
#include <logilinux/logilinux.h>
#include <logilinux/events.h>

void onEvent(LogiLinux::EventPtr event) {
    if (auto rotation = std::dynamic_pointer_cast<LogiLinux::RotationEvent>(event)) {
        std::cout << "Rotated: " << rotation->delta << " steps" << std::endl;
    }
}

int main() {
    LogiLinux::Library lib;
    auto dialpad = lib.findDevice(LogiLinux::DeviceType::DIALPAD);

    dialpad->setEventCallback(onEvent);
    dialpad->startMonitoring();

    return 0;
}
```

Compile with:

```bash
g++ -std=c++17 myapp.cpp $(pkg-config --cflags --libs logilinux)
```

See `lib/README.md` for full API documentation.

## Prerequisites

```bash
sudo apt-get install build-essential cmake libudev-dev
sudo dnf install gcc-c++ cmake libudev-devel
sudo pacman -S base-devel cmake systemd
```

## Permissions

To avoid using sudo:

```bash
sudo bash -c 'cat > /etc/udev/rules.d/99-logitech.rules << EOF
SUBSYSTEM=="hidraw", KERNEL=="hidraw*", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="bc00", MODE="0666"
EOF'

sudo udevadm control --reload-rules
sudo udevadm trigger
```

## Device Information

- Model: Logitech MX Dialpad
- Vendor ID: `046d` (Logitech)
- Product ID: `bc00`
- Protocol: HID++ 4.5
