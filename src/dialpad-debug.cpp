/*
 * Logitech MX Dialpad Debug Tool
 *
 * This is a simple debugging tool for the Logitech MX Dialpad that allows
 * you to test HID++ communication and receive events from the device.
 */

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>

#include <hid/DeviceMonitor.h>
#include <hid/RawDevice.h>
#include <hidpp/Device.h>
#include <hidpp/DispatcherThread.h>
#include <hidpp/SimpleDispatcher.h>
#include <hidpp10/Error.h>
#include <hidpp20/Device.h>
#include <hidpp20/Error.h>
#include <hidpp20/IFeatureSet.h>
#include <misc/Log.h>

extern "C" {
#include <signal.h>
#include <unistd.h>
}

std::atomic<bool> running(true);

void signalHandler(int signum) {
  std::cout << "\nInterrupt signal received. Shutting down..." << std::endl;
  running = false;
}

std::string findDialpad() {
  class DialpadFinder : public HID::DeviceMonitor {
  public:
    std::string dialpad_path;

  protected:
    void addDevice(const char *path) override {
      try {
        HIDPP::SimpleDispatcher dispatcher(path);
        try {
          HIDPP::Device dev(&dispatcher, HIDPP::DefaultDevice);
          auto version = dev.protocolVersion();

          if (dispatcher.hidraw().vendorID() == 0x046d &&
              dev.productID() == 0xbc00) {
            dialpad_path = path;
            printf("Found MX Dialpad at %s\n", path);
            printf("  Name: %s\n", dev.name().c_str());
            printf("  VID:PID: %04x:%04x\n", dispatcher.hidraw().vendorID(),
                   dev.productID());
            printf("  HID++ Version: %d.%d\n", std::get<0>(version),
                   std::get<1>(version));
          }
        } catch (HIDPP::Dispatcher::TimeoutError &e) {
        }
      } catch (HIDPP::Dispatcher::NoHIDPPReportException &e) {
      } catch (std::system_error &e) {
      }
    }

    void removeDevice(const char *path) override {}
  };

  DialpadFinder finder;
  finder.enumerate();
  return finder.dialpad_path;
}

void listenForEvents(const char *device_path) {
  try {
    HID::RawDevice device(device_path);

    printf("\nListening for RAW HID events from MX Dialpad...\n");
    printf("Device path: %s\n", device_path);
    printf("Press Ctrl+C to exit.\n");
    printf("Try rotating the dial or pressing buttons...\n");
    printf("Waiting for events (checking every 100ms)...\n\n");
    fflush(stdout);

    std::vector<uint8_t> report;
    int event_count = 0;
    int timeout_count = 0;

    while (running) {
      report.clear();
      int ret = device.readReport(report, 100);

      if (ret > 0) {
        event_count++;
        timeout_count = 0;
        printf("Event #%d - Raw HID Report (%d bytes):\n", event_count, ret);
        fflush(stdout);

        printf("  Hex: ");
        for (size_t i = 0; i < report.size(); ++i) {
          printf("%02x ", report[i]);
          if ((i + 1) % 16 == 0 && i + 1 < report.size()) {
            printf("\n       ");
          }
        }
        printf("\n");

        if (report.size() >= 4) {
          uint8_t report_id = report[0];
          if (report_id == 0x10 || report_id == 0x11 || report_id == 0x12) {
            printf("  HID++ Report Detected:\n");
            printf("    Report ID: 0x%02x ", report_id);
            if (report_id == 0x10)
              printf("(Short)\n");
            else if (report_id == 0x11)
              printf("(Long)\n");
            else if (report_id == 0x12)
              printf("(Very Long)\n");

            if (report.size() >= 4) {
              printf("    Device Index: 0x%02x\n", report[1]);
              printf("    Feature Index: 0x%02x\n", report[2]);
              printf("    Function/SW: 0x%02x\n", report[3]);

              if (report.size() > 4) {
                printf("    Params: ");
                for (size_t i = 4; i < report.size(); ++i) {
                  printf("%02x ", report[i]);
                }
                printf("\n");
              }
            }
          } else {
            printf("  Standard HID Report (ID: 0x%02x)\n", report_id);
          }
        }

        printf("\n");
        fflush(stdout);
      } else if (ret == 0) {
        timeout_count++;
        if (timeout_count % 50 == 0) {
          printf("Still listening... (%d timeouts)\n", timeout_count);
          fflush(stdout);
        }
        continue;
      } else {
        fprintf(stderr, "Error reading report: %d\n", ret);
        fflush(stderr);
        break;
      }
    }

    printf("\nTotal events received: %d\n", event_count);
    fflush(stdout);

  } catch (std::exception &e) {
    fprintf(stderr, "Error: %s\n", e.what());
  }
}

int main(int argc, char *argv[]) {
  struct sigaction sa, oldsa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = signalHandler;
  sigaction(SIGINT, &sa, &oldsa);

  std::string device_path;

  if (argc > 1) {
    device_path = argv[1];
    printf("Using device: %s\n", device_path.c_str());
  } else {
    printf("Searching for MX Dialpad...\n");
    device_path = findDialpad();

    if (device_path.empty()) {
      fprintf(stderr, "Error: MX Dialpad not found!\n");
      fprintf(stderr, "Make sure the device is connected and you have "
                      "permission to access it.\n");
      fprintf(stderr, "You may need to run with sudo or set up udev rules.\n");
      return EXIT_FAILURE;
    }
  }

  printf("\n");

  try {
    HIDPP::SimpleDispatcher dispatcher(device_path.c_str());
    HIDPP::Device device(&dispatcher, HIDPP::DefaultDevice);

    printf("Device Information:\n");
    printf("===================\n");
    printf("Name: %s\n", device.name().c_str());

    auto version = device.protocolVersion();
    printf("HID++ Version: %d.%d\n", std::get<0>(version),
           std::get<1>(version));

    printf("Product ID: 0x%04x\n", device.productID());
    printf("Vendor ID: 0x%04x\n", dispatcher.hidraw().vendorID());

    if (std::get<0>(version) >= 2) {
      try {
        HIDPP20::Device dev20(&dispatcher, HIDPP::DefaultDevice);
        HIDPP20::IFeatureSet ifs(&dev20);

        uint8_t feature_count = ifs.getCount();
        printf("Feature Count: %d\n", feature_count);

        printf("\nAvailable Features:\n");
        for (uint8_t i = 0; i < feature_count && i < 30; ++i) {
          try {
            uint16_t feature_id = ifs.getFeatureID(i);
            printf("  [%2d] Feature 0x%04x\n", i, feature_id);
          } catch (...) {
          }
        }
      } catch (std::exception &e) {
        Log::warning().printf("Could not enumerate features: %s\n", e.what());
      }
    }

    printf("\n");

  } catch (std::exception &e) {
    fprintf(stderr, "Error getting device info: %s\n", e.what());
    return EXIT_FAILURE;
  }

  listenForEvents(device_path.c_str());

  sigaction(SIGINT, &oldsa, nullptr);

  printf("\nGoodbye!\n");
  return EXIT_SUCCESS;
}
