# The .drop digest format

A digest is a UTF-8 text file named `YYYY-MM-DD.drop`, stored on the device
at `/dailydrop/YYYY-MM-DD.drop`. The companion builder does all layout work;
the device parses tags and paints lines verbatim.

## Structure

Line 1 must be the magic `DROP 1` (format version 1). Every following
non-empty line starts with a one-letter tag:

| Tag | Meaning | Payload |
|---|---|---|
| `M` | Header metadata | `key value` (keys: `date`, `title`) |
| `S` | Start a section | section name shown in the header bar |
| `H` | Heading line | emphasised text (rendered inverted) |
| `T` | Body line | pre-wrapped text; bare `T` is a blank line |
| `R` | Rule | horizontal separator, no payload |

Unknown tags are skipped, so older firmware tolerates newer builders. Empty
lines are ignored.

## Layout contract

- Lines are pre-wrapped by the builder to 64 columns (`wrap` config key).
  The device renders a fixed-width 5x7 font that fits 66 columns on both the
  X4 (800 px) and X3 (792 px) panels, so 64 is safe everywhere.
- Builder caps: 10 sections, 180 lines per section, 24 000 bytes per digest.
  The device additionally hard-caps parsing at 12 sections and 200 lines per
  section so a hand-edited file cannot exhaust RAM.

## Example

```
DROP 1
M date 2026-07-31
M title Thursday briefing
S News
H * Kernel 7.0 released
T The long-awaited release brings scheduler improvements.
R
S Quote
T "Fortune favours the brave."
T   - Virgil
```
