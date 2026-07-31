# Hardware testing checklist

DailyDrop's digest core is host-tested (`test/host/run.sh`) and the
companion is pytest-covered. What genuinely needs an Xteink X4/X3 is below;
each item is a `TODO(hardware-test)` in the source.

## Build and flash

```sh
pio run -e xteink_x4            # build (run envs one at a time)
pio run -e xteink_x4 -t upload  # flash over USB
pio device monitor              # serial log at 115200
```

## Items to verify on device

### Boot and bring-up (`src/main.cpp`)
- Boot order gpio -> Storage -> display -> clock -> power, from cold and
  from deep-sleep wake.
- Power-hold (2 s) enters deep sleep; the panel keeps the last screen.

### Sync (`src/device/Fetcher.cpp`)
- Wi-Fi association against WPA2 home networks, association timeout at 15 s.
- HTTP fetch from a plain HTTP server and from a GitHub raw/gh-pages URL
  (redirects and TLS: HTTPClient handles https on ESP32; confirm memory
  headroom during TLS handshake with the framebuffer allocated).
- Partial-download path: kill the AP mid-transfer; the temp file must be
  removed and "Download interrupted" shown.
- 404 shows "No new digest yet"; missing config.txt shows the config hint.

### Clock (`src/device/DailyDropApp.cpp`)
- With an RTC present, `time()` yields the right date for today's filename.
- With no clock, the reader falls back to the newest stored digest.

### Reader and archive
- Section navigation, scroll, archive open, and the 14-day prune (drop 20
  files on the card; exactly the oldest beyond 14 must be deleted).
- Fast vs full refresh choices feel right on the panel.
