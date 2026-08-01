"""Minimal ICS (RFC 5545) VEVENT parsing for the calendar section.

Handles folded lines, DTSTART with and without time, and recurrence-free
events. Recurring events are included only on their DTSTART date; this is a
documented v1 limitation.
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass
class Event:
    date: str  # YYYY-MM-DD
    time: str  # HH:MM or "" for all-day
    summary: str


def _unfold(text: str) -> list[str]:
    lines: list[str] = []
    for raw in text.splitlines():
        if raw.startswith((" ", "\t")) and lines:
            lines[-1] += raw[1:]
        else:
            lines.append(raw)
    return lines


def parse_ics(text: str) -> list[Event]:
    events: list[Event] = []
    date = time = summary = None
    in_event = False
    for line in _unfold(text):
        if line.startswith("BEGIN:VEVENT"):
            in_event, date, time, summary = True, None, "", None
        elif line.startswith("END:VEVENT"):
            if in_event and date and summary:
                events.append(Event(date, time or "", summary))
            in_event = False
        elif in_event:
            if line.startswith("DTSTART"):
                value = line.split(":", 1)[-1].strip()
                if len(value) >= 8 and value[:8].isdigit():
                    date = f"{value[0:4]}-{value[4:6]}-{value[6:8]}"
                    if "T" in value and len(value) >= 13:
                        time = f"{value[9:11]}:{value[11:13]}"
            elif line.startswith("SUMMARY"):
                summary = line.split(":", 1)[-1].strip()
    return events


def events_for(text: str, iso_date: str) -> list[Event]:
    todays = [e for e in parse_ics(text) if e.date == iso_date]
    todays.sort(key=lambda e: e.time)
    return todays
