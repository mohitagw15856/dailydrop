from pathlib import Path

from dailydrop import rss

FIX = Path(__file__).parent / "fixtures"


def test_parse_rss_with_limit():
    articles = rss.parse_feed((FIX / "feed_rss.xml").read_bytes(), limit=2)
    assert len(articles) == 2
    assert articles[0].title == "Kernel 7.0 released"
    assert "scheduler improvements" in articles[0].summary
    assert "<p>" not in articles[0].summary  # html stripped


def test_parse_atom():
    articles = rss.parse_feed((FIX / "feed_atom.xml").read_bytes(), limit=5)
    assert [a.title for a in articles] == ["Atom entry one", "Atom entry two"]
    assert articles[0].link == "https://example.com/a1"


def test_extract_full_text_skips_nav_and_short():
    html = (FIX / "article.html").read_text()
    text = rss.extract_full_text(html)
    assert "scheduler improvements" in text
    assert "Menu item" not in text
    assert "ok" not in text.split("\n")
