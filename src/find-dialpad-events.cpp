/*
 * Find which hidraw device sends events for MX Dialpad
 */

#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <hid/RawDevice.h>
#include <iostream>
#include <thread>
#include <vector>

extern "C" {
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
}

std::atomic<bool> running(true);

void signalHandler(int signum) {
  std::cout << "\nStopping..." << std::endl;
  running = false;
}

void monitorDevice(const std::string &path) {
  try {
    HID::RawDevice device(path);
    printf("[%s] Monitoring: %s (VID: 0x%04x, PID: 0x%04x)\n", path.c_str(),
           device.name().c_str(), device.vendorID(), device.productID());
    fflush(stdout);

    std::vector<uint8_t> report;

    while (running) {
      report.clear();
      int ret = device.readReport(report, 100);

      if (ret > 0) {
        printf("\n*** EVENT on %s *** (%d bytes)\n", path.c_str(), ret);
        printf("Hex: ");
        for (size_t i = 0; i < report.size(); ++i) {
          printf("%02x ", report[i]);
        }
        printf("\n\n");
        fflush(stdout);
      }
    }
  } catch (std::exception &e) {
  }
}

int main() {
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = signalHandler;
  sigaction(SIGINT, &sa, nullptr);

  std::vector<std::thread> threads;
  std::vector<std::string> devices;

  DIR *dir = opendir("/dev");
  if (dir) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string name = entry->d_name;
      if (name.find("hidraw") == 0) {
        devices.push_back("/dev/" + name);
      }
    }
    closedir(dir);
  }

  std::sort(devices.begin(), devices.end());

  printf("Monitoring %zu HID devices for events...\n", devices.size());
  printf("Press Ctrl+C to exit.\n");
  printf("Try interacting with your MX Dialpad now!\n\n");
  fflush(stdout);

  for (const auto &dev : devices) {
    threads.emplace_back(monitorDevice, dev);
  }

  for (auto &t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  printf("\nDone!\n");
  return 0;
}
