/*
 * Monitor Linux input events from all devices
 */

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>
#include <vector>

std::atomic<bool> running(true);

void signalHandler(int) { running = false; }

void monitorInputDevice(const std::string &path) {
  int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    return;
  }

  char name[256] = "Unknown";
  ioctl(fd, EVIOCGNAME(sizeof(name)), name);

  struct input_id id;
  if (ioctl(fd, EVIOCGID, &id) >= 0) {
    if (id.vendor == 0x046d) {
      printf("[%s] Monitoring: %s (VID: 0x%04x, PID: 0x%04x)\n", path.c_str(),
             name, id.vendor, id.product);
      fflush(stdout);
    } else {
      close(fd);
      return;
    }
  } else {
    close(fd);
    return;
  }

  struct input_event ev;
  fd_set fds;
  struct timeval tv;

  while (running) {
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 100000;

    int ret = select(fd + 1, &fds, NULL, NULL, &tv);
    if (ret > 0) {
      ssize_t bytes = read(fd, &ev, sizeof(ev));
      if (bytes == sizeof(ev)) {
        printf("\n*** EVENT on %s ***\n", path.c_str());
        printf("Type: %d, Code: %d, Value: %d\n", ev.type, ev.code, ev.value);

        switch (ev.type) {
        case EV_KEY:
          printf("  [KEY] Button/Key %d %s\n", ev.code,
                 ev.value ? "PRESSED" : "RELEASED");
          break;
        case EV_REL:
          printf("  [REL] Relative axis %d, delta: %d\n", ev.code, ev.value);
          if (ev.code == REL_WHEEL)
            printf("    (Scroll wheel)\n");
          else if (ev.code == REL_HWHEEL)
            printf("    (Horizontal wheel)\n");
          else if (ev.code == REL_DIAL)
            printf("    (Dial)\n");
          break;
        case EV_ABS:
          printf("  [ABS] Absolute axis %d, value: %d\n", ev.code, ev.value);
          break;
        case EV_MSC:
          printf("  [MSC] Misc event %d, value: %d\n", ev.code, ev.value);
          break;
        case EV_SYN:
          printf("  [SYN] Sync event\n");
          break;
        default:
          printf("  [%d] Unknown type\n", ev.type);
        }
        printf("\n");
        fflush(stdout);
      }
    }
  }

  close(fd);
}

int main() {
  signal(SIGINT, signalHandler);

  std::vector<std::thread> threads;
  std::vector<std::string> devices;

  DIR *dir = opendir("/dev/input");
  if (dir) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string name = entry->d_name;
      if (name.find("event") == 0) {
        devices.push_back("/dev/input/" + name);
      }
    }
    closedir(dir);
  }

  printf("Monitoring Linux input devices for Logitech events...\n");
  printf("Press Ctrl+C to exit.\n");
  printf("Try interacting with your MX Dialpad now!\n\n");
  fflush(stdout);

  for (const auto &dev : devices) {
    threads.emplace_back(monitorInputDevice, dev);
  }

  for (auto &t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  printf("\nDone!\n");
  return 0;
}
