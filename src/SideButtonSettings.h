#pragma once

#include <I18n.h>

#include <array>

#include "CrossPointSettings.h"
#include "apps_local/power/PowerButtonPolicy.h"
#include "util/SideButtonActions.h"

namespace side_button {
inline constexpr StrId LABELS[] = {StrId::STR_SIDE_NAVIGATION,
                                   StrId::STR_SLEEP,
                                   StrId::STR_TOGGLE_FRONTLIGHT,
                                   StrId::STR_HOME_SHORTCUT,
                                   StrId::STR_READER_BOTTOM_SHEET,
                                   StrId::STR_BOOKMARK_OPTION,
                                   StrId::STR_DICTIONARY,
                                   StrId::STR_SAVE_CLIPPING,
                                   StrId::STR_KOSYNC,
                                   StrId::STR_FORCE_REFRESH,
                                   StrId::STR_TOGGLE_TOUCHSCREEN,
                                   StrId::STR_IGNORE,
                                   StrId::STR_SIDE_TOGGLE_WARMTH,
                                   StrId::STR_HOME_OPEN_SHORTCUT,
                                   StrId::STR_NEXT_PAGE,
                                   StrId::STR_FOOTNOTES,
                                   StrId::STR_CONFIRM,
                                   StrId::STR_PREV_PAGE,
                                   StrId::STR_BT_PREV_CHAPTER,
                                   StrId::STR_BT_NEXT_CHAPTER,
                                   StrId::STR_BUTTON_ROTATE_LEFT,
                                   StrId::STR_BUTTON_ROTATE_RIGHT,
                                   StrId::STR_POWER_MENU};
static_assert(sizeof(LABELS) / sizeof(LABELS[0]) == static_cast<unsigned>(SideButtonAction::Count));
inline constexpr uint8_t CrossPointSettings::* actionField(SideButtonKey key, PhysicalPress press) {
  if (press == PhysicalPress::Double)
    return key == SideButtonKey::Left ? &CrossPointSettings::sideLeftDoubleAction
                                      : &CrossPointSettings::sideRightDoubleAction;
  if (press == PhysicalPress::Hold)
    return key == SideButtonKey::Left ? &CrossPointSettings::sideLeftHoldAction
                                      : &CrossPointSettings::sideRightHoldAction;
  return key == SideButtonKey::Left ? &CrossPointSettings::sideLeftAction : &CrossPointSettings::sideRightAction;
}
inline char* shortcutTarget(SideButtonKey key, PhysicalPress press = PhysicalPress::Single) {
  if (press == PhysicalPress::Double)
    return key == SideButtonKey::Left ? SETTINGS.sideLeftDoubleShortcutTitle : SETTINGS.sideRightDoubleShortcutTitle;
  if (press == PhysicalPress::Hold)
    return key == SideButtonKey::Left ? SETTINGS.sideLeftHoldShortcutTitle : SETTINGS.sideRightHoldShortcutTitle;
  return key == SideButtonKey::Left ? SETTINGS.sideLeftShortcutTitle : SETTINGS.sideRightShortcutTitle;
}
}  // namespace side_button

namespace powermenu {
inline constexpr auto ACTION_LABELS = [] {
  std::array<StrId, unsigned(PressAction::Count)> labels{};
  for (unsigned i = 0; i < labels.size(); ++i) labels[i] = side_button::LABELS[unsigned(BINDINGS[i])];
  return labels;
}();
inline constexpr uint8_t CrossPointSettings::* actionField(PhysicalPress press) {
  if (press == PhysicalPress::Double) return &CrossPointSettings::touchPowerDoubleAction;
  if (press == PhysicalPress::Hold) return &CrossPointSettings::touchPowerLongAction;
  return &CrossPointSettings::touchPowerShortAction;
}
inline char* shortcutTarget(PhysicalPress press) {
  if (press == PhysicalPress::Double) return SETTINGS.powerDoubleShortcutTitle;
  if (press == PhysicalPress::Hold) return SETTINGS.powerHoldShortcutTitle;
  return SETTINGS.powerShortcutTitle;
}
}  // namespace powermenu
