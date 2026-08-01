# inkkit gaps

DailyDrop depends on [inkkit](https://github.com/mohitagw15856/inkkit)
v0.1.0-rc1 for the device layer and does not reimplement it. This file
records what DailyDrop needed that inkkit does not currently provide, so the
workarounds are visible and inkkit has a candidate list of additions.

## 1. No Wi-Fi / HTTP fetch helper

inkkit covers display, storage, buttons, clock and power, but has no
networking surface at all. DailyDrop's whole premise is "fetch one file a
day over HTTP", which needs: STA association with timeout, an HTTP GET
streamed to an SD file via a temp-file rename, and a clean failure taxonomy
(no config / no Wi-Fi / 404 / partial body).

**Workaround.** `src/device/Fetcher.{h,cpp}` implements exactly that against
the Arduino `WiFi` + `HTTPClient` APIs and inkkit's storage helpers.

**Suggested inkkit addition.** An `inkkit::net` module: `fetchToFile(url,
path)` with the temp-file dance and a result enum. Any networked ecosystem
app (sync, correspondence play, OTA content) needs this identical block.

## 2. No text or font engine (already recorded by InkQuest and PocketWiki)

DailyDrop carries the ecosystem's copy-pasted 5x7 `TextRenderer` +
`Font5x7.h` (from InkQuest, originally). This is the third app to duplicate
it; it belongs in inkkit as a small optional text module.

## 3. Byte-count access for HalFile writes

`inkkit::sd` offers whole-file and line helpers, but a streamed download
wants a "bytes written so far" check without tracking it manually. Minor;
worked around with a local counter.
