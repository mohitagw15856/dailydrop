"""RSS/Atom parsing and best-effort article text extraction, stdlib only."""

from __future__ import annotations

import re
import urllib.request
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from html.parser import HTMLParser
from pathlib import Path

_ATOM = "{http://www.w3.org/2005/Atom}"


@dataclass
class Article:
    title: str
    summary: str = ""
    link: str = ""
    full_text: str = ""


class _TextExtractor(HTMLParser):
    """Collects paragraph text from an HTML page, skipping script/style."""

    SKIP = {"script", "style", "nav", "header", "footer", "aside"}

    def __init__(self) -> None:
        super().__init__()
        self._skip_depth = 0
        self._in_p = False
        self._current: list[str] = []
        self.paragraphs: list[str] = field(default_factory=list) if False else []

    def handle_starttag(self, tag, attrs):
        if tag in self.SKIP:
            self._skip_depth += 1
        elif tag == "p" and self._skip_depth == 0:
            self._in_p = True
            self._current = []

    def handle_endtag(self, tag):
        if tag in self.SKIP and self._skip_depth > 0:
            self._skip_depth -= 1
        elif tag == "p" and self._in_p:
            text = re.sub(r"\s+", " ", "".join(self._current)).strip()
            if len(text) > 40:  # ignore boilerplate crumbs
                self.paragraphs.append(text)
            self._in_p = False

    def handle_data(self, data):
        if self._in_p and self._skip_depth == 0:
            self._current.append(data)


def strip_html(text: str) -> str:
    """Flatten an HTML fragment (e.g. an RSS description) to plain text."""

    class _Flat(HTMLParser):
        def __init__(self):
            super().__init__()
            self.parts: list[str] = []

        def handle_data(self, data):
            self.parts.append(data)

    p = _Flat()
    p.feed(text)
    return re.sub(r"\s+", " ", "".join(p.parts)).strip()


def fetch_bytes(source: str, timeout: int = 20) -> bytes:
    """Reads a URL or a local file path (used by tests and offline builds)."""
    if source.startswith(("http://", "https://")):
        req = urllib.request.Request(source, headers={"User-Agent": "dailydrop/0.1"})
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return resp.read()
    return Path(source).read_bytes()


def parse_feed(data: bytes, limit: int = 5) -> list[Article]:
    """Parses RSS 2.0 or Atom into at most `limit` articles."""
    root = ET.fromstring(data)
    articles: list[Article] = []

    if root.tag == f"{_ATOM}feed":
        for entry in root.findall(f"{_ATOM}entry"):
            title = (entry.findtext(f"{_ATOM}title") or "").strip()
            summary = (
                entry.findtext(f"{_ATOM}summary")
                or entry.findtext(f"{_ATOM}content")
                or ""
            )
            link = ""
            for link_el in entry.findall(f"{_ATOM}link"):
                if link_el.get("rel", "alternate") == "alternate":
                    link = link_el.get("href", "")
                    break
            articles.append(Article(title, strip_html(summary), link))
            if len(articles) >= limit:
                break
    else:
        channel = root.find("channel")
        items = channel.findall("item") if channel is not None else root.findall("item")
        for item in items:
            title = (item.findtext("title") or "").strip()
            summary = item.findtext("description") or ""
            link = (item.findtext("link") or "").strip()
            articles.append(Article(title, strip_html(summary), link))
            if len(articles) >= limit:
                break
    return articles


def extract_full_text(html: str, max_paragraphs: int = 6) -> str:
    """Best-effort readable text from an article page: its longest <p> run."""
    p = _TextExtractor()
    p.feed(html)
    return "\n".join(p.paragraphs[:max_paragraphs])
