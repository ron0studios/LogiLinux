#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <logilinux/device.h>
#include <logilinux/events.h>
#include <logilinux/logilinux.h>
#include <thread>

std::atomic<bool> running(true);

void signalHandler(int signal) { running = false; }

void printTimestamp(uint64_t timestamp) {
  std::cout << "[" << std::setw(10) << timestamp << "ms] ";
}

void onEvent(LogiLinux::EventPtr event) {
  printTimestamp(event->timestamp);

  if (auto button = std::dynamic_pointer_cast<LogiLinux::ButtonEvent>(event)) {
    auto mx_button = LogiLinux::getMXKeypadButton(button->button_code);
    const char *button_name = LogiLinux::getMXKeypadButtonName(mx_button);

    std::cout << (button->pressed ? "PRESS  " : "RELEASE")
              << " | Button: " << std::setw(12) << button_name << " | Code: 0x"
              << std::hex << std::setw(2) << std::setfill('0')
              << button->button_code << std::dec << std::setfill(' ')
              << std::endl;
  } else {
    std::cout << "Unknown event type" << std::endl;
  }
}

int main(int argc, char *argv[]) {
  auto version = LogiLinux::getVersion();
  std::cout << "=================================================="
            << std::endl;
  std::cout << "  LogiLinux MX Keypad Debug Tool" << std::endl;
  std::cout << "  Library Version: " << version.major << "." << version.minor
            << "." << version.patch << std::endl;
  std::cout << "=================================================="
            << std::endl;
  std::cout << std::endl;

  signal(SIGINT, signalHandler);

  LogiLinux::Library lib;

  std::cout << "Scanning for MX Keypad devices..." << std::endl;
  auto keypad = lib.findDevice(LogiLinux::DeviceType::MX_KEYPAD);

  if (!keypad) {
    std::cerr << "\nNo MX Keypad found!" << std::endl;
    std::cerr << "Make sure your device is connected and you have permissions."
              << std::endl;
    std::cerr << "Try: sudo usermod -a -G input $USER" << std::endl;
    return 1;
  }

  const auto &info = keypad->getInfo();
  std::cout << "\nFound MX Keypad:" << std::endl;
  std::cout << "  Name:        " << info.name << std::endl;
  std::cout << "  Vendor ID:   0x" << std::hex << std::setw(4)
            << std::setfill('0') << info.vendor_id << std::dec
            << std::setfill(' ') << std::endl;
  std::cout << "  Product ID:  0x" << std::hex << std::setw(4)
            << std::setfill('0') << info.product_id << std::dec
            << std::setfill(' ') << std::endl;
  std::cout << "  Device Path: " << info.device_path << std::endl;

  // Check for exclusive grab option
  bool exclusive = false;
  if (argc > 1 && std::string(argv[1]) == "--exclusive") {
    exclusive = true;
    std::cout << "\nExclusive mode enabled - device will be grabbed"
              << std::endl;
  }

  std::cout << "\nStarting event monitoring..." << std::endl;
  std::cout << "Press Ctrl+C to exit\n" << std::endl;

  std::cout << "Button Layout:" << std::endl;
  std::cout << "  3x3 Grid: GRID_0 to GRID_8 (codes 0-8)" << std::endl;
  std::cout << "  Navigation: P1_LEFT (0xa1), P2_RIGHT (0xa2)" << std::endl;
  std::cout << std::endl;

  std::cout << std::string(70, '-') << std::endl;
  std::cout << "Timestamp    | Event   | Button       | Code " << std::endl;
  std::cout << std::string(70, '-') << std::endl;

  keypad->setEventCallback(onEvent);

  if (exclusive && !keypad->grabExclusive(true)) {
    std::cerr << "Warning: Failed to grab device exclusively" << std::endl;
    std::cerr << "Try running with sudo for exclusive access" << std::endl;
  }

  keypad->startMonitoring();

  while (running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  keypad->stopMonitoring();

  std::cout << std::string(70, '-') << std::endl;
  std::cout << "\nExiting..." << std::endl;

  return 0;
}
