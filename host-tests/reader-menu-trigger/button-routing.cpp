#include <cassert>
#include <cstdio>
#include <initializer_list>

#include "src/apps_local/readertext/ReaderMenuTrigger.h"
#include "src/util/ButtonReleaseGate.h"
#include "src/util/SideButtonActions.h"
struct HalGPIO {
  static constexpr uint8_t BTN_UP = 0, BTN_DOWN = 1;
  bool down[2]{}, pressed[2]{}, released[2]{};
  bool hasTouch() const { return true; }
  bool isPressed(uint8_t n) const { return down[n]; }
  bool wasPressed(uint8_t n) const { return pressed[n]; }
  bool wasReleased(uint8_t n) const { return released[n]; }
};
struct CrossPointSettings {
  static constexpr uint8_t OFF = 0;
  uint8_t readerTextTrigger = uint8_t(readertext::MenuTrigger::LeftHold);
  uint8_t sideLeftAction = 0, sideRightAction = 0;
  uint8_t sideLeftDoubleAction = uint8_t(SideButtonAction::Ignore);
  uint8_t sideRightDoubleAction = uint8_t(SideButtonAction::Ignore);
  uint8_t sideLeftHoldAction = 0, sideRightHoldAction = 0;
  uint8_t longPressButtonBehavior = OFF;
} settings;

namespace BoardConfig {
constexpr bool isX4Pro() { return true; }
}  // namespace BoardConfig
#define SETTINGS settings
class MappedInputManager {
 public:
  enum class RemoteButton { None, SideLeft, SideRight };
  struct RemoteInput {
    RemoteButton button = RemoteButton::None;
  } remoteInput;
  HalGPIO gpio;
  bool readerTextContext = false;
  uint8_t suppressedNavigationReleases = 0;
  mutable uint8_t managedSideButtons = 0, sideNavigationEdges = 0;
  ButtonReleaseGate releaseGate;
  mutable SideButtonEvent sideEvent;
  mutable HomeButtonAction homeAction = HomeButtonAction::Ignore;
  mutable PhysicalButtonInput sidePresses[2];
  mutable unsigned long sideNavigationHeldMs = 0;
  void updateSideButtonBindings(uint8_t customHolds, uint32_t now) const;
  bool readButton(uint8_t, bool (HalGPIO::*)(uint8_t) const) const;
};
#include "button-routing-production.inc"
static void testIndependentBindings() {
  using Side = SideButtonAction;
  using Press = PhysicalPress;
  settings.sideLeftAction = uint8_t(Side::PreviousPage);
  settings.sideLeftHoldAction = uint8_t(Side::PreviousChapter);
  settings.sideLeftDoubleAction = uint8_t(Side::ToggleFrontlight);
  MappedInputManager input;
  input.readerTextContext = true;

  const auto frame = [&](uint32_t now, bool leftPress, bool leftRelease, bool leftHeld) {
    input.gpio.pressed[0] = leftPress;
    input.gpio.released[0] = leftRelease;
    input.gpio.down[0] = leftHeld;
    input.sideEvent = {};
    input.homeAction = HomeButtonAction::Ignore;
    input.managedSideButtons = input.sideNavigationEdges = 0;
    input.sideNavigationHeldMs = 0;
    input.updateSideButtonBindings(1, now);
    assert(!input.readButton(0, &HalGPIO::wasPressed));
    assert(!input.readButton(0, &HalGPIO::wasReleased));
    assert(!input.readButton(0, &HalGPIO::isPressed));
    return input.sideEvent.action;
  };

  assert(frame(10, true, false, true) == Side::Navigation);
  assert(frame(60, false, true, false) == Side::Navigation);
  assert(input.sidePresses[0].nextDelay(60, 1000) == PhysicalButtonInput::DoubleMs + 1);
  assert(frame(410, false, false, false) == Side::Navigation);
  assert(frame(411, false, false, false) == Side::PreviousPage);
  assert(input.sideEvent.press == Press::Single);
  assert(frame(412, false, false, false) == Side::Navigation);

  assert(frame(500, true, false, true) == Side::Navigation);
  assert(frame(550, false, true, false) == Side::Navigation);
  input.gpio.pressed[1] = true;
  assert(frame(600, true, false, true) == Side::Navigation);
  assert(input.readButton(1, &HalGPIO::wasPressed));
  input.gpio.pressed[1] = false;
  assert(frame(650, false, true, false) == Side::ToggleFrontlight);
  assert(input.sideEvent.press == Press::Double);
  assert(input.homeAction == HomeButtonAction::ToggleFrontlight);
  assert(frame(1001, false, false, false) == Side::Navigation);

  assert(frame(1100, true, false, true) == Side::Navigation);
  assert(frame(1799, false, false, true) == Side::Navigation);
  assert(frame(1800, false, false, true) == Side::PreviousChapter);
  assert(input.sideEvent.press == Press::Hold);
  assert(frame(1801, false, false, true) == Side::Navigation);
  assert(frame(1900, false, true, false) == Side::Navigation);
  assert(frame(2300, false, false, false) == Side::Navigation);

  assert(frame(2400, true, false, true) == Side::Navigation);
  assert(frame(2450, false, true, false) == Side::Navigation);
  input.sidePresses[0].reset();
  assert(frame(3000, false, false, false) == Side::Navigation);
  std::puts("Independent Left bindings: previous page, previous chapter and frontlight; Right stays independent");
}

int main() {
  MappedInputManager input;
  input.gpio.down[0] = input.gpio.pressed[0] = true;
  assert(input.readButton(0, &HalGPIO::wasPressed));
  input.readerTextContext = true;
  assert(!input.readButton(0, &HalGPIO::wasPressed));
  assert(!input.readButton(0, &HalGPIO::isPressed));
  input.gpio.pressed[1] = true;
  assert(input.readButton(1, &HalGPIO::wasPressed));
  input.gpio.down[0] = input.gpio.pressed[0] = false;
  input.gpio.released[0] = true;
  assert(input.readButton(0, &HalGPIO::wasPressed));
  assert(input.readButton(0, &HalGPIO::wasReleased));
  input.suppressedNavigationReleases = 1;
  assert(!input.readButton(0, &HalGPIO::wasPressed));
  input.suppressedNavigationReleases = 0;
  input.releaseGate.arm(1);
  assert(!input.readButton(0, &HalGPIO::wasPressed));
  assert(!input.readButton(0, &HalGPIO::wasReleased));
  std::puts(
      "Production button routing: deferred short navigation, no page/chapter on menu hold, unselected key, "
      "held-release and activity gate passed");
  testIndependentBindings();
}
