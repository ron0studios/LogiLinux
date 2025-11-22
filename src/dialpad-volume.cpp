/*
 * Logitech MX Dialpad Volume Controller
 *
 * Maps the dialpad rotation to system volume control
 * Uses pactl for PipeWire/PulseAudio compatibility
 */

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/input.h>
#include <regex>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>

std::atomic<bool> running(true);

void signalHandler(int) {
  printf("\nShutting down...\n");
  running = false;
}

class VolumeController {
private:
  int current_volume;
  bool is_muted;

  std::string exec(const char *cmd) {
    char buffer[128];
    std::string result = "";

    const char *sudo_user = getenv("SUDO_USER");
    const char *sudo_uid = getenv("SUDO_UID");

    std::string full_cmd;
    if (sudo_user && sudo_uid) {
      full_cmd = std::string("sudo -u ") + sudo_user +
                 " XDG_RUNTIME_DIR=/run/user/" + sudo_uid + " " + cmd;
    } else {
      full_cmd = cmd;
    }

    FILE *pipe = popen(full_cmd.c_str(), "r");
    if (!pipe)
      return "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
      result += buffer;
    }
    pclose(pipe);
    return result;
  }

  void updateStatus() {
    std::string output = exec("pactl get-sink-volume @DEFAULT_SINK@");

    std::regex volume_regex(R"((\d+)%)");
    std::smatch match;
    if (std::regex_search(output, match, volume_regex)) {
      current_volume = std::stoi(match[1]);
    }

    output = exec("pactl get-sink-mute @DEFAULT_SINK@");
    is_muted = (output.find("yes") != std::string::npos);
  }

public:
  VolumeController() {
    current_volume = 0;
    is_muted = false;
    updateStatus();
    printf("Volume controller initialized (PipeWire/PulseAudio)\n");
    printf("Current volume: %d%%\n", current_volume);
  }

  int getCurrentVolumePercent() { return current_volume; }

  void adjustVolume(int delta_percent) {
    char cmd[512];
    if (delta_percent > 0) {
      snprintf(cmd, sizeof(cmd), "pactl set-sink-volume @DEFAULT_SINK@ +%d%%",
               delta_percent);
    } else {
      snprintf(cmd, sizeof(cmd), "pactl set-sink-volume @DEFAULT_SINK@ %d%%",
               delta_percent);
    }
    exec(cmd);

    updateStatus();

    printf("Volume: %d%% ", current_volume);

    printf("[");
    for (int i = 0; i < 20; i++) {
      if (i < current_volume / 5) {
        printf("=");
      } else {
        printf(" ");
      }
    }
    printf("]");

    if (is_muted) {
      printf(" (MUTED)");
    }
    printf("\n");
    fflush(stdout);
  }

  void setMute(bool mute) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pactl set-sink-mute @DEFAULT_SINK@ %d",
             mute ? 1 : 0);
    exec(cmd);

    updateStatus();
    printf("Mute: %s\n", is_muted ? "ON" : "OFF");
    fflush(stdout);
  }

  void toggleMute() {
    exec("pactl set-sink-mute @DEFAULT_SINK@ toggle");
    updateStatus();
    printf("Mute: %s\n", is_muted ? "ON" : "OFF");
    fflush(stdout);
  }
};

std::string findDialpadDevice() {
  for (int i = 0; i < 300; i++) {
    char path[64];
    snprintf(path, sizeof(path), "/dev/input/event%d", i);

    int fd = open(path, O_RDONLY);
    if (fd < 0)
      continue;

    struct input_id id;
    if (ioctl(fd, EVIOCGID, &id) >= 0) {
      if (id.vendor == 0x046d && id.product == 0xbc00) {
        char name[256] = "Unknown";
        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        close(fd);
        printf("Found MX Dialpad: %s\n", path);
        printf("  Name: %s\n", name);
        return path;
      }
    }
    close(fd);
  }
  return "";
}

int main(int argc, char *argv[]) {
  signal(SIGINT, signalHandler);

  printf("Logitech MX Dialpad Volume Controller\n");
  printf("======================================\n\n");

  std::string device_path;
  if (argc > 1) {
    device_path = argv[1];
  } else {
    device_path = findDialpadDevice();
    if (device_path.empty()) {
      fprintf(stderr, "Error: MX Dialpad not found!\n");
      fprintf(stderr, "Make sure the device is connected.\n");
      return 1;
    }
  }

  printf("\n");

  VolumeController volume;
  printf("Current volume: %d%%\n\n", volume.getCurrentVolumePercent());

  int fd = open(device_path.c_str(), O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Error: Cannot open %s\n", device_path.c_str());
    fprintf(stderr, "Try running with sudo\n");
    return 1;
  }

  printf("Listening for dial events...\n");
  printf("  Rotate dial: adjust volume\n");
  printf("  Press dial: toggle mute\n");
  printf("  Press Ctrl+C to exit\n\n");
  fflush(stdout);

  struct input_event ev;
  fd_set fds;
  struct timeval tv;

  int accumulated_steps = 0;
  bool dial_pressed = false;

  while (running) {
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 100000;

    int ret = select(fd + 1, &fds, NULL, NULL, &tv);
    if (ret > 0) {
      ssize_t bytes = read(fd, &ev, sizeof(ev));
      if (bytes == sizeof(ev)) {

        if (ev.type == EV_REL && ev.code == 6) {
          accumulated_steps += ev.value;

          int volume_delta = ev.value * 2;
          volume.adjustVolume(volume_delta);
        }

        if (ev.type == EV_KEY) {
          if (ev.value == 1) {
            dial_pressed = true;
          } else if (ev.value == 0 && dial_pressed) {
            volume.toggleMute();
            dial_pressed = false;
          }
        }
      }
    }
  }

  close(fd);
  printf("\nGoodbye!\n");
  return 0;
}
