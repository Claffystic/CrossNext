#pragma once

#include <cstdint>

enum class PhysicalPress : uint8_t { None, Single, Double, Hold };

class PhysicalButtonInput {
 public:
  static constexpr uint32_t DoubleMs = 350;
  static constexpr uint32_t HoldMs = 700;

  PhysicalPress update(uint32_t now, bool pressed, bool released, bool held, bool doubleEnabled, bool holdEnabled) {
    holdArmed = holdEnabled;
    if (pressed) {
      second = pending && now - releasedAt <= DoubleMs;
      down = true;
      pressedAt = now;
    }

    if (down && holdEnabled && (held || released) && now - pressedAt >= HoldMs) {
      reset();
      return PhysicalPress::Hold;
    }

    if (released && down) {
      down = false;
      if (second) {
        reset();
        return PhysicalPress::Double;
      }
      if (!doubleEnabled) return PhysicalPress::Single;
      if (!pending) {
        pending = true;
        releasedAt = now;
        return PhysicalPress::None;
      }
      // A second click began outside the window before the first was emitted.
      releasedAt = now;
      return PhysicalPress::Single;
    }

    if (pending && !second && now - releasedAt > DoubleMs) {
      pending = false;
      return PhysicalPress::Single;
    }
    return PhysicalPress::None;
  }

  uint32_t nextDelay(uint32_t now, uint32_t maximum) const {
    if (down && holdArmed) {
      const uint32_t elapsed = now - pressedAt;
      const uint32_t remaining = elapsed >= HoldMs ? 0 : HoldMs - elapsed;
      if (remaining < maximum) maximum = remaining;
    }
    if (!pending || second) return maximum;
    const uint32_t elapsed = now - releasedAt;
    const uint32_t remaining = elapsed > DoubleMs ? 0 : DoubleMs + 1 - elapsed;
    return remaining < maximum ? remaining : maximum;
  }

  void reset() { down = pending = second = false; }

 private:
  uint32_t pressedAt = 0;
  uint32_t releasedAt = 0;
  bool down = false;
  bool pending = false;
  bool second = false;
  bool holdArmed = false;
};
