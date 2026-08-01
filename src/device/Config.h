// Device-side configuration: SD-card layout, sync settings and button map.
//
// The SD paths follow the ecosystem convention of a single app root; here that
// root is /dailydrop, so a card can carry CrossPoint books and DailyDrop
// digests side by side without collision.
#pragma once

#include <cstdint>

namespace dailydrop {

constexpr const char* kAppRoot = "/dailydrop";
constexpr const char* kConfigPath = "/dailydrop/config.txt";
constexpr const char* kTag = "DD";

// Logical buttons. The mapping onto raw inkkit button indices is hardware
// specific and must be checked on device.
enum class Button : uint8_t { Up, Down, Select, Back, Sync };

// TODO(hardware-test): confirm these raw inkkit button indices against the
// Xteink X4/X3 button harness. They follow the CrossPoint convention
// (BTN_BACK=0, BTN_CONFIRM=1, BTN_LEFT=2, BTN_RIGHT=3, BTN_UP=4, BTN_DOWN=5).
constexpr uint8_t kBtnUp = 4;
constexpr uint8_t kBtnDown = 5;
constexpr uint8_t kBtnSelect = 1;
constexpr uint8_t kBtnBack = 0;
constexpr uint8_t kBtnSync = 3;

// Text renderer metrics (shared with the ecosystem 5x7 font renderer).
constexpr int kMarginX = 6;
constexpr int kMarginTop = 6;
constexpr int kLineGap = 2;   // extra pixels between text lines
constexpr int kGlyphGap = 1;  // extra pixels between glyphs

}  // namespace dailydrop
