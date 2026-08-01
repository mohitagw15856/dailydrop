# DailyDrop

**Daily offline digest for the Xteink X4/X3 e-reader.** A companion pipeline
compiles one compact digest a day (news, weather, calendar, quote, reading
goal); the device fetches it over Wi-Fi in seconds and then stays
distraction-free for the rest of the day.

Status: builds in CI, not yet verified on device.

Compatible with the CrossPoint ecosystem: DailyDrop targets the same Xteink
X4/X3 hardware, takes its whole device layer from
[inkkit](https://github.com/mohitagw15856/inkkit) (pinned in
`platformio.ini`), and keeps its files under its own `/dailydrop` SD root so
it lives happily alongside CrossPoint Reader and the other ecosystem apps.

## How it works

1. `dailydrop build` (companion, run daily by n8n or GitHub Actions)
   compiles `YYYY-MM-DD.drop` from your YAML config: RSS feeds, an
   OpenWeather source, ICS calendars and a quotes file.
2. The file is published anywhere plain HTTP can reach: a home server, or
   gh-pages (a GitHub raw URL works).
3. On the device, press SYNC: DailyDrop connects to Wi-Fi, downloads
   today's digest to `/dailydrop/`, disconnects, and renders it as
   button-navigable sections. The last 14 days stay browsable; older files
   are pruned automatically.

Failure states are explicit on screen: no config, no Wi-Fi, no new digest
yet, server error, interrupted download.

## Install and flash

Firmware (PlatformIO; run environments one at a time):

```sh
pio run -e xteink_x4              # or -e xteink_x3
pio run -e xteink_x4 -t upload    # flash over USB
```

Both environments build identical firmware (the device layer detects X4 vs
X3 at runtime); two envs exist so each target ships a named binary. See
[docs/HARDWARE_TESTING.md](docs/HARDWARE_TESTING.md) for the on-device
checklist.

On the SD card create `/dailydrop/config.txt`:

```
ssid YourNetwork
pass YourPassword
url  https://you.github.io/your-digests
```

Companion:

```sh
pip install -e "companion[test]"
dailydrop build --config my/config.yaml --out out/
```

Automation: a ready n8n workflow is in
[`companion/n8n/dailydrop-daily.json`](companion/n8n/dailydrop-daily.json)
and a GitHub Actions gh-pages publisher template is in
[`companion/actions/publish-digest.yml`](companion/actions/publish-digest.yml).

## Documentation

- [docs/FORMAT.md](docs/FORMAT.md) - the .drop format and layout contract
- [docs/HARDWARE_TESTING.md](docs/HARDWARE_TESTING.md) - on-device checklist
- [docs/INKKIT_GAPS.md](docs/INKKIT_GAPS.md) - device-layer gaps recorded for inkkit
- [ARCHITECTURE.md](ARCHITECTURE.md) - design and the standalone-app decision
- [CONTRIBUTING.md](CONTRIBUTING.md)

## Licence

MIT, see [LICENSE](LICENSE). The 5x7 text renderer is shared ecosystem code
originating in InkQuest (same author, MIT).
