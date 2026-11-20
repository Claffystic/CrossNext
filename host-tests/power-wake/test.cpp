#include <cstdio>
#include <initializer_list>

#include "apps_local/power/PowerButtonPolicy.h"
#include "apps_local/power/PowerWakePolicy.h"
#include "util/EdgeLightGesture.h"

namespace {
int checks = 0;
int failures = 0;
void check(const bool ok, const char* description) {
  ++checks;
  if (!ok) {
    ++failures;
    std::printf("FAIL: %s\n", description);
  }
}
}  // namespace

int main() {
  using namespace powermenu;
  check(configuredAction(ButtonAction::None, 2, 0) == PressAction::Ignore, "idle does not trigger a power assignment");
  check(configuredAction(ButtonAction::ShortPress, 2, 0) == PressAction::ToggleLight, "short press can toggle light");
  check(configuredAction(ButtonAction::LongPress, 2, 0) == PressAction::Sleep, "long press can suspend directly");
  check(configuredAction(ButtonAction::ShortPress, 255, 255) == PressAction::Sleep,
        "invalid short action stays recoverable");
  check(configuredAction(ButtonAction::LongPress, 255, 255) == PressAction::Menu,
        "invalid hold action opens power menu");
  static_assert(uint8_t(PressAction::Ignore) == 6);
  static_assert(uint8_t(PressAction::ToggleTouchscreen) == 7);
  bool seenHome[unsigned(HomeButtonAction::Count)]{};
  for (unsigned i = 0; i < unsigned(SideButtonAction::Count); ++i) {
    const auto shared = SideButtonAction(i);
    if (!selectableSideAction(shared) || shared == SideButtonAction::Navigation) continue;
    check(bindingIndex(shared) >= 0, "every shared action is available for Power");
    const auto home = homeBindingAction(shared);
    check(selectableHomeAction(home), "every shared action is available for Home");
    check(!seenHome[unsigned(home)], "each action has its own Home binding");
    seenHome[unsigned(home)] = true;
  }
  for (unsigned i = 0; i < unsigned(HomeButtonAction::Count); ++i)
    check(seenHome[i] == selectableHomeAction(HomeButtonAction(i)), "Home picker matches the shared list");
  check(bindingIndex(SideButtonAction::OpenShortcut) == int(PressAction::OpenShortcut), "Power can launch an app");

  PhysicalButtonInput power;
  const auto powerFrame = [&](uint32_t now, bool pressed, bool released, bool held) {
    return configuredAction(power.update(now, pressed, released, held, true, true), uint8_t(PressAction::ToggleLight),
                            uint8_t(PressAction::ToggleTouchscreen), uint8_t(PressAction::Sleep));
  };
  check(powerFrame(10, true, false, true) == PressAction::Ignore, "power press waits");
  check(powerFrame(60, false, true, false) == PressAction::Ignore, "first release does not toggle the light before double");
  check(powerFrame(130, true, false, true) == PressAction::Ignore, "second press waits for release");
  check(powerFrame(170, false, true, false) == PressAction::ToggleTouchscreen, "double press toggles touchscreen only");
  check(powerFrame(500, false, false, false) == PressAction::Ignore, "double press leaves no pending single action");
  check(powerFrame(600, true, false, true) == PressAction::Ignore, "hold starts without a short action");
  check(powerFrame(1300, false, false, true) == PressAction::Sleep, "power hold sleeps once");
  check(powerFrame(1400, false, true, false) == PressAction::Ignore, "release after hold does not toggle the light");
  check(powerFrame(1500, true, false, true) == PressAction::Ignore, "new short press starts independently");
  check(powerFrame(1540, false, true, false) == PressAction::Ignore, "short release waits for timeout");
  check(powerFrame(1891, false, false, false) == PressAction::ToggleLight, "isolated short press toggles light");

  for (const auto width : {480, 800}) {
    const int height = width == 480 ? 800 : 480;
    const auto leftUp = edgelight::classify(6, 300, 12, 100, width, height, 1, 2);
    const auto rightDown = edgelight::classify(width - 6, 100, width - 12, 300, width, height, 1, 2);
    check(leftUp.action == edgelight::Action::Brightness && leftUp.step == 10, "left upward gesture raises brightness");
    check(rightDown.action == edgelight::Action::Warmth && rightDown.step == -10,
          "right downward gesture lowers warmth");
    check(edgelight::classify(width / 2, 300, width / 2, 100, width, height, 1, 2).action == edgelight::Action::None,
          "middle swipe remains owned by the activity");
    check(edgelight::classify(6, 100, width / 2, 105, width, height, 1, 2).action == edgelight::Action::None,
          "horizontal edge-back swipe remains available");
    check(edgelight::classify(6, 300, 12, 100, width, height, 0, 0).action == edgelight::Action::None,
          "edge shortcuts default to disabled");
    check(edgelight::classify(6, 300, 12, 280, width, height, 1, 2).action == edgelight::Action::None,
          "tap slop does not adjust the light");
  }
  check(acceptsWake(WakeMode::ShortPress, false), "released short wake is accepted");
  check(acceptsWake(WakeMode::ShortPress, true), "holding Power also wakes in short mode");
  check(!acceptsWake(WakeMode::LongPress, false), "released short wake is rejected in hold mode");
  check(acceptsWake(WakeMode::LongPress, true), "verified hold wakes in hold mode");
  check(acceptsWake(static_cast<WakeMode>(255), false), "invalid mode cannot lock out a short wake");

  for (const auto mode : {WakeMode::ShortPress, WakeMode::LongPress}) {
    if (!acceptsWake(mode, true)) continue;
    ButtonPolicy policy;
    check(policy.update(true, false, 5000, false) == ButtonAction::None, "wake hold during boot is ignored");
    check(policy.update(true, false, 6000, true) == ButtonAction::None, "inherited wake hold cannot open menu");
    check(policy.update(false, true, 6000, true) == ButtonAction::None, "wake release cannot suspend");
    check(policy.update(true, false, 20, true) == ButtonAction::None, "new short press starts a gesture");
    check(policy.update(false, true, 80, true) == ButtonAction::ShortPress, "next short press still suspends");
    check(policy.update(true, false, 700, true) == ButtonAction::LongPress, "new hold still opens power menu");
    check(policy.update(false, true, 800, true) == ButtonAction::None, "menu hold release cannot suspend");
  }

  std::printf("power-wake: %d checks, %d failed\n", checks, failures);
  return failures ? 1 : 0;
}
