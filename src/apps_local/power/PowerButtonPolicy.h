#pragma once
#include <cstdint>

#include "../../util/SideButtonActions.h"
namespace powermenu {
enum class ButtonAction : uint8_t { None, ShortPress, LongPress };
// Keep the first eight IDs compatible with saved Power settings.
enum class PressAction : uint8_t {
  Sleep, Menu, ToggleLight, Warmth, Refresh, Home, Ignore, ToggleTouchscreen,
  ReaderMenu, Bookmark, Sync, OpenShortcut, NextPage, Confirm, PreviousPage,
  PreviousChapter, NextChapter, RotateLeft, RotateRight, Count
};
inline constexpr SideButtonAction BINDINGS[] = {
    SideButtonAction::Sleep, SideButtonAction::PowerMenu, SideButtonAction::ToggleFrontlight,
    SideButtonAction::ToggleWarmth, SideButtonAction::Refresh, SideButtonAction::Home, SideButtonAction::Ignore,
    SideButtonAction::ToggleTouchscreen, SideButtonAction::ReaderMenu, SideButtonAction::Bookmark,
    SideButtonAction::Sync, SideButtonAction::OpenShortcut, SideButtonAction::NextPage, SideButtonAction::Confirm,
    SideButtonAction::PreviousPage, SideButtonAction::PreviousChapter, SideButtonAction::NextChapter,
    SideButtonAction::RotateLeft, SideButtonAction::RotateRight};
static_assert(sizeof(BINDINGS) / sizeof(BINDINGS[0]) == unsigned(PressAction::Count));
constexpr int bindingIndex(SideButtonAction action) {
  for (unsigned i = 0; i < unsigned(PressAction::Count); ++i)
    if (BINDINGS[i] == action) return int(i);
  return -1;
}
inline PressAction configuredAction(ButtonAction event, uint8_t shortAction, uint8_t longAction) {
  const unsigned value = event == ButtonAction::ShortPress ? shortAction : longAction;
  if (event == ButtonAction::None) return PressAction::Ignore;
  return value < uint8_t(PressAction::Count) ? PressAction(value)
         : event == ButtonAction::ShortPress ? PressAction::Sleep
                                             : PressAction::Menu;
}
inline PressAction configuredAction(PhysicalPress event, uint8_t single, uint8_t doublePress, uint8_t hold) {
  switch (event) {
    case PhysicalPress::Single:
      return configuredAction(ButtonAction::ShortPress, single, hold);
    case PhysicalPress::Double:
      return doublePress < uint8_t(PressAction::Count) ? PressAction(doublePress) : PressAction::Ignore;
    case PhysicalPress::Hold:
      return configuredAction(ButtonAction::LongPress, single, hold);
    case PhysicalPress::None:
      return PressAction::Ignore;
  }
  return PressAction::Ignore;
}

class ButtonPolicy {
  enum class State : uint8_t { WaitingForRelease, Ready, Pressed, MenuOpened };
  State state = State::WaitingForRelease;

 public:
  ButtonAction update(bool held, bool released, uint32_t heldMs, bool allowed) {
    if (!allowed) {
      state = State::WaitingForRelease;
      return ButtonAction::None;
    }
    if (!held) {
      const bool sleep = released && state == State::Pressed;
      state = State::Ready;
      return sleep ? ButtonAction::ShortPress : ButtonAction::None;
    }
    if (state == State::Ready) state = State::Pressed;
    if (state == State::Pressed && heldMs >= 700) {
      state = State::MenuOpened;
      return ButtonAction::LongPress;
    }
    return ButtonAction::None;
  }
};
}  // namespace powermenu
