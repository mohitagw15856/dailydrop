"""Digest assembly: YAML config in, .drop document out."""

from __future__ import annotations

import datetime as dt
import random
from pathlib import Path

import yaml

from . import fmt, ics, rss, weather


def _quote_for(quotes_path: str, iso_date: str) -> tuple[str, str]:
    """Deterministic quote of the day from a `quote|attribution` lines file."""
    lines = [
        ln.strip()
        for ln in Path(quotes_path).read_text(encoding="utf-8").splitlines()
        if ln.strip() and not ln.lstrip().startswith("#")
    ]
    if not lines:
        return ("", "")
    rng = random.Random(iso_date)  # same quote all day, varies by day
    chosen = rng.choice(lines)
    if "|" in chosen:
        quote, attribution = chosen.split("|", 1)
        return quote.strip(), attribution.strip()
    return chosen, ""


def build_digest(config: dict, iso_date: str, base_dir: Path | None = None) -> str:
    """Builds the .drop document text for `iso_date` from a parsed config.

    Relative fixture/config paths resolve against `base_dir` (the config
    file's directory) so builds work from any CWD.
    """
    base = base_dir or Path.cwd()

    def resolve(source: str) -> str:
        if source.startswith(("http://", "https://")):
            return source
        p = Path(source)
        return str(p if p.is_absolute() else base / p)

    title = config.get("title", "Daily digest")
    width = int(config.get("wrap", fmt.DEFAULT_WRAP))
    w = fmt.DropWriter(iso_date, title, width)

    feeds = config.get("feeds", [])
    if feeds:
        w.section("News")
        first = True
        for feed in feeds:
            limit = int(feed.get("limit", 3))
            try:
                articles = rss.parse_feed(rss.fetch_bytes(resolve(feed["url"])), limit)
            except Exception as exc:  # noqa: BLE001 - a dead feed must not kill the digest
                w.text(f"[{feed.get('name', feed['url'])}: unavailable ({type(exc).__name__})]")
                continue
            if not first:
                w.rule()
            first = False
            if feed.get("name"):
                w.heading(feed["name"])
            for art in articles:
                w.heading("* " + art.title)
                body = art.summary
                if feed.get("full_text") and art.link:
                    try:
                        html = rss.fetch_bytes(resolve(art.link)).decode(
                            "utf-8", "replace"
                        )
                        extracted = rss.extract_full_text(html)
                        if extracted:
                            body = extracted
                    except Exception:  # noqa: BLE001
                        pass  # fall back to the summary
                if body:
                    w.text(body)

    weather_cfg = config.get("weather")
    if weather_cfg:
        w.section("Weather")
        try:
            for line in weather.load_weather(
                resolve(weather_cfg["source"]), weather_cfg.get("units", "metric")
            ):
                w.text(line)
        except Exception as exc:  # noqa: BLE001
            w.text(f"[weather unavailable ({type(exc).__name__})]")

    calendars = config.get("calendar", [])
    if calendars:
        w.section("Calendar")
        any_event = False
        for cal in calendars:
            try:
                text = Path(resolve(cal)).read_text(encoding="utf-8")
            except OSError:
                w.text(f"[calendar {cal}: unavailable]")
                continue
            for event in ics.events_for(text, iso_date):
                stamp = event.time or "all day"
                w.text(f"{stamp}  {event.summary}")
                any_event = True
        if not any_event:
            w.text("No events today.")

    quotes = config.get("quotes")
    if quotes:
        quote, attribution = _quote_for(resolve(quotes), iso_date)
        if quote:
            w.section("Quote")
            w.text('"' + quote + '"')
            if attribution:
                w.text("  - " + attribution)

    goal = config.get("reading_goal")
    if goal:
        w.section("Reading goal")
        w.text(str(goal))

    return w.serialise()


def build_to_file(config_path: Path, out_dir: Path, iso_date: str | None = None) -> Path:
    config = yaml.safe_load(config_path.read_text(encoding="utf-8"))
    date = iso_date or dt.date.today().isoformat()
    doc = build_digest(config, date, base_dir=config_path.parent)
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / f"{date}.drop"
    out_path.write_text(doc, encoding="utf-8")
    return out_path
