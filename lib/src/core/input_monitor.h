/*
 * LogiLinux - Input Monitor
 * Monitors Linux input events from devices
 */

#ifndef LOGILINUX_INPUT_MONITOR_H
#define LOGILINUX_INPUT_MONITOR_H

#include "logilinux/events.h"
#include <atomic>
#include <functional>
#include <linux/input.h>
#include <string>
#include <thread>

namespace LogiLinux {

class InputMonitor {
public:
  InputMonitor(const std::string &device_path);
  ~InputMonitor();

  /**
   * Start monitoring events in a background thread
   */
  bool start(EventCallback callback);

  /**
   * Grab the device exclusively (prevents other apps from receiving events)
   */
  bool grabDevice(bool grab);

  /**
   * Stop monitoring
   */
  void stop();

  /**
   * Check if monitoring is active
   */
  bool isRunning() const { return running_; }

private:
  /**
   * Main monitoring loop (runs in separate thread)
   */
  void monitorLoop();

  /**
   * Process a raw input event
   */
  void processEvent(const struct input_event &ev);

  std::string device_path_;
  EventCallback callback_;

  std::thread monitor_thread_;
  std::atomic<bool> running_;
  std::atomic<bool> should_stop_;

  int device_fd_;
};

} // namespace LogiLinux

#endif // LOGILINUX_INPUT_MONITOR_H
