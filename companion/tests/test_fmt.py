from dailydrop import fmt

import pytest


def test_wrap_width_and_paragraphs():
    text = "word " * 30 + "\n\nsecond paragraph"
    lines = fmt.wrap(text, width=20)
    assert all(len(line) <= 20 for line in lines)
    assert "" in lines  # paragraph break survives


def test_writer_serialises_expected_shape():
    w = fmt.DropWriter("2026-07-31", "Test digest")
    w.section("News")
    w.heading("A headline")
    w.text("Body text.")
    w.rule()
    doc = w.serialise()
    lines = doc.splitlines()
    assert lines[0] == "DROP 1"
    assert lines[1] == "M date 2026-07-31"
    assert lines[2] == "M title Test digest"
    assert "S News" in lines
    assert "H A headline" in lines
    assert "T Body text." in lines
    assert "R" in lines


def test_writer_rejects_oversize():
    w = fmt.DropWriter("2026-07-31", "Big")
    w.section("S")
    with pytest.raises(fmt.DropTooLargeError):
        for _ in range(10_000):
            w.text("x" * 60)


def test_writer_requires_section():
    w = fmt.DropWriter("2026-07-31", "T")
    with pytest.raises(ValueError):
        w.text("no section yet")
