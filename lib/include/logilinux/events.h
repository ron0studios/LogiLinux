#ifndef LOGILINUX_EVENTS_H
#define LOGILINUX_EVENTS_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace LogiLinux {

enum class RotationType { DIAL, WHEEL };

enum class DialpadButton {
  TOP_LEFT = 275,
  TOP_RIGHT = 276,
  BOTTOM_LEFT = 277,
  BOTTOM_RIGHT = 278,
  UNKNOWN = 0
};

enum class EventType {
  ROTATION,
  BUTTON_PRESS,
  BUTTON_RELEASE,
  DEVICE_CONNECTED,
  DEVICE_DISCONNECTED
};

struct Event {
  EventType type;
  uint64_t timestamp;

  Event() : type(EventType::ROTATION), timestamp(0) {}
  explicit Event(EventType t) : type(t), timestamp(0) {}
  virtual ~Event() = default;
};

struct RotationEvent : public Event {
  RotationType rotation_type;
  int32_t delta;
  int32_t delta_high_res;
  uint16_t raw_event_code;

  RotationEvent()
      : Event(EventType::ROTATION), rotation_type(RotationType::DIAL), delta(0),
        delta_high_res(0), raw_event_code(0) {}
};

struct ButtonEvent : public Event {
  uint32_t button_code;
  bool pressed;

  ButtonEvent()
      : Event(EventType::BUTTON_PRESS), button_code(0), pressed(false) {}
};

struct DeviceEvent : public Event {
  std::string device_path;

  DeviceEvent() : Event(EventType::DEVICE_CONNECTED) {}
};

using EventPtr = std::shared_ptr<Event>;
using RotationEventPtr = std::shared_ptr<RotationEvent>;
using ButtonEventPtr = std::shared_ptr<ButtonEvent>;
using DeviceEventPtr = std::shared_ptr<DeviceEvent>;

using EventCallback = std::function<void(EventPtr)>;

inline DialpadButton getDialpadButton(uint32_t button_code) {
  switch (button_code) {
  case 275:
    return DialpadButton::TOP_LEFT;
  case 276:
    return DialpadButton::TOP_RIGHT;
  case 277:
    return DialpadButton::BOTTOM_LEFT;
  case 278:
    return DialpadButton::BOTTOM_RIGHT;
  default:
    return DialpadButton::UNKNOWN;
  }
}

inline const char *getDialpadButtonName(DialpadButton button) {
  switch (button) {
  case DialpadButton::TOP_LEFT:
    return "TOP_LEFT";
  case DialpadButton::TOP_RIGHT:
    return "TOP_RIGHT";
  case DialpadButton::BOTTOM_LEFT:
    return "BOTTOM_LEFT";
  case DialpadButton::BOTTOM_RIGHT:
    return "BOTTOM_RIGHT";
  default:
    return "UNKNOWN";
  }
}

} // namespace LogiLinux

#endif // LOGILINUX_EVENTS_H
