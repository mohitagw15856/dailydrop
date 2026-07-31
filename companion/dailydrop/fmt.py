"""The .drop wire format: wrapping and emission.

The device renders pre-wrapped lines with a fixed-width 5x7 font at 66
columns on both the X4 (800 px) and X3 (792 px) panels, so the builder wraps
to a conservative 64 columns by default. See docs/FORMAT.md.
"""

from __future__ import annotations

import textwrap

MAGIC = "DROP 1"
DEFAULT_WRAP = 64
MAX_BYTES = 24_000  # device-side hard cap is higher; builder stays well under
MAX_SECTIONS = 10
MAX_LINES_PER_SECTION = 180


class DropTooLargeError(ValueError):
    """Raised when a digest exceeds the device-safe size caps."""


def wrap(text: str, width: int = DEFAULT_WRAP) -> list[str]:
    """Wrap paragraph text into device-width lines.

    Blank lines separate paragraphs and survive as empty text lines.
    """
    lines: list[str] = []
    for para in text.split("\n"):
        para = para.rstrip()
        if not para:
            if lines and lines[-1] != "":
                lines.append("")
            continue
        wrapped = textwrap.wrap(
            para, width=width, break_long_words=True, break_on_hyphens=False
        )
        lines.extend(wrapped if wrapped else [""])
    while lines and lines[-1] == "":
        lines.pop()
    return lines


class DropWriter:
    """Accumulates sections and serialises the digest document."""

    def __init__(self, date: str, title: str, width: int = DEFAULT_WRAP):
        self.date = date
        self.title = title
        self.width = width
        self._sections: list[tuple[str, list[str]]] = []

    def section(self, name: str) -> None:
        if len(self._sections) >= MAX_SECTIONS:
            raise DropTooLargeError(f"more than {MAX_SECTIONS} sections")
        self._sections.append((name, []))

    def _lines(self) -> list[str]:
        if not self._sections:
            raise ValueError("digest has no sections")
        return self._sections[-1][1]

    def heading(self, text: str) -> None:
        for i, line in enumerate(wrap(text, self.width)):
            self._push(("H " + line) if i == 0 else ("T " + line))

    def text(self, text: str) -> None:
        for line in wrap(text, self.width):
            self._push("T " + line if line else "T")

    def rule(self) -> None:
        self._push("R")

    def _push(self, raw: str) -> None:
        lines = self._lines()
        if len(lines) >= MAX_LINES_PER_SECTION:
            raise DropTooLargeError(
                f"section '{self._sections[-1][0]}' exceeds "
                f"{MAX_LINES_PER_SECTION} lines"
            )
        lines.append(raw)

    def serialise(self) -> str:
        out = [MAGIC, f"M date {self.date}", f"M title {self.title}"]
        for name, lines in self._sections:
            out.append(f"S {name}")
            out.extend(lines)
        doc = "\n".join(out) + "\n"
        size = len(doc.encode("utf-8"))
        if size > MAX_BYTES:
            raise DropTooLargeError(f"digest is {size} bytes (max {MAX_BYTES})")
        return doc
