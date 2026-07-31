# DailyDrop architecture

## Fork vs standalone app

Considered: (a) forking CrossPoint Reader and adding a digest screen, (b) a
CrossPoint plugin, (c) a standalone firmware on the shared device layer.

**(c) Standalone firmware. Chosen.** A digest reader shares almost no
application logic with an EPUB reader; forking CrossPoint would drag the
whole book pipeline along for one screen. DailyDrop instead consumes
[inkkit](https://github.com/mohitagw15856/inkkit) (pinned in
`platformio.ini`), which vendors the same HAL and SDK hardware libraries
CrossPoint uses, so hardware behaviour stays ecosystem-identical while the
app remains ~1 500 lines.

## Layering

```
companion/dailydrop/      Python builder: RSS + weather + ICS + quotes -> .drop
src/core/                 portable, Arduino-free: .drop parser, archive policy
src/device/               inkkit-facing: renderer, Wi-Fi fetcher, screens
src/main.cpp              boot wiring (ecosystem boot order)
```

The heavy lifting (feed fetching, HTML extraction, wrapping, size caps)
happens in the companion at build time, per the ecosystem memory
discipline: the device parses a pre-wrapped, size-capped text file line by
line and never allocates beyond small fixed caps.

## Key decisions

- **Pre-wrapped text.** The builder wraps to 64 columns; the device does no
  wrapping. One code path, one place to test typography limits (companion
  pytest), no device-side surprises.
- **Temp-file downloads.** Fetches write `<name>.drop.tmp` and rename on
  completion, so a dropped connection can never leave a torn digest where
  the reader would find it.
- **Failure taxonomy over retries.** The device reports one of six explicit
  failure states rather than retrying silently; the user always knows why a
  sync produced nothing.
- **14-day window as pure logic.** Prune/list decisions are computed in
  `src/core/Archive.cpp` (host-tested); the device only executes deletions.
