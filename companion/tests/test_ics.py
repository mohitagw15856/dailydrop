from pathlib import Path

from dailydrop import ics

FIX = Path(__file__).parent / "fixtures"


def test_events_for_date_sorted_and_folded():
    text = (FIX / "cal.ics").read_text()
    events = ics.events_for(text, "2026-07-31")
    assert len(events) == 2
    # All-day event ("" time) sorts first, then the timed standup.
    assert events[0].summary == "Pay rent"
    assert events[0].time == ""
    assert events[1].summary == "Standup with the firmwareteam"
    assert events[1].time == "09:15"


def test_other_dates_excluded():
    text = (FIX / "cal.ics").read_text()
    assert [e.summary for e in ics.events_for(text, "2026-08-01")] == ["Tomorrow only"]
    assert ics.events_for(text, "2026-08-02") == []
