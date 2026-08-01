import json
from pathlib import Path

from dailydrop import weather

FIX = Path(__file__).parent / "fixtures"


def test_current_weather_format():
    payload = json.loads((FIX / "weather_current.json").read_text())
    lines = weather.format_weather(payload, "metric")
    assert lines == ["Seoul: 25C, Scattered clouds, humidity 61%"]


def test_onecall_format():
    payload = {
        "current": {"temp": 21.2, "weather": [{"description": "light rain"}]},
        "daily": [
            {"name": "Fri", "temp": {"min": 18, "max": 26}, "weather": [{"description": "sunny"}]},
        ],
    }
    lines = weather.format_weather(payload, "metric")
    assert lines[0] == "Now: 21C, Light rain"
    assert lines[1] == "Fri: 18-26C sunny"


def test_unknown_payload():
    assert weather.format_weather({}) == ["Weather data unavailable"]
