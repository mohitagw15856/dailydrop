"""OpenWeather-compatible weather formatting.

Accepts either the One Call style payload ({"current": ..., "daily": [...]})
or the plain current-weather payload ({"main": ..., "weather": [...]}) and
formats a compact section. The source is a URL or a local JSON file, so tests
and offline builds never touch the network.
"""

from __future__ import annotations

import json

from .rss import fetch_bytes


def _units_suffix(units: str) -> str:
    return "C" if units == "metric" else "F"


def format_weather(payload: dict, units: str = "metric") -> list[str]:
    deg = _units_suffix(units)
    lines: list[str] = []

    if "current" in payload:  # One Call style
        cur = payload["current"]
        desc = ""
        if cur.get("weather"):
            desc = cur["weather"][0].get("description", "").capitalize()
        lines.append(f"Now: {round(cur.get('temp', 0))}{deg}, {desc}".rstrip(", "))
        for day in payload.get("daily", [])[:3]:
            name = day.get("name") or day.get("dt_txt") or ""
            temp = day.get("temp", {})
            hi = temp.get("max")
            lo = temp.get("min")
            desc = ""
            if day.get("weather"):
                desc = day["weather"][0].get("description", "")
            piece = f"{name}: " if name else ""
            lines.append(f"{piece}{round(lo)}-{round(hi)}{deg} {desc}".strip())
    elif "main" in payload:  # current weather style
        main = payload["main"]
        desc = ""
        if payload.get("weather"):
            desc = payload["weather"][0].get("description", "").capitalize()
        city = payload.get("name", "")
        head = f"{city}: " if city else ""
        lines.append(
            f"{head}{round(main.get('temp', 0))}{deg}, {desc}, "
            f"humidity {main.get('humidity', '?')}%"
        )
    else:
        lines.append("Weather data unavailable")
    return lines


def load_weather(source: str, units: str = "metric") -> list[str]:
    payload = json.loads(fetch_bytes(source).decode("utf-8"))
    return format_weather(payload, units)
