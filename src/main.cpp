// DailyDrop firmware entry point for the Xteink X4/X3 (ESP32-C3).
//
// Boot follows the ecosystem order: gpio (SPI + X4/X3 detect), storage,
// display, clock, power; then the screen state machine takes over.
//
// TODO(hardware-test): the whole boot path and every screen need a first run
// on a real device; see docs/HARDWARE_TESTING.md.
#ifdef ARDUINO

#include <Arduino.h>
#include <HalClock.h>
#include <HalDisplay.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <HalStorage.h>
#include <Logging.h>

#include <inkkit/inkkit.h>

#include "device/Config.h"
#include "device/DailyDropApp.h"
#include "device/Fetcher.h"
#include "device/TextRenderer.h"

namespace {

inkkit::Display g_display(display);
inkkit::Buttons g_buttons(gpio);
dailydrop::TextRenderer g_tr(g_display);
dailydrop::Fetcher g_fetcher;
dailydrop::DailyDropApp g_app(g_tr, g_buttons, g_fetcher);

}  // namespace

void setup() {
  Serial.begin(115200);
  LOG_INF("DD", "DailyDrop starting");

  gpio.begin();
  Storage.begin();
  g_display.begin();
  halClock.begin();
  powerManager.begin();

  g_app.begin();
}

void loop() {
  g_app.tick();

  // Hold power for two seconds to sleep; the panel keeps the last screen.
  if (g_buttons.powerHeldMs() >= 2000) {
    inkkit::Power(powerManager, display, gpio).deepSleep();
  }
  delay(20);
}

#endif  // ARDUINO
