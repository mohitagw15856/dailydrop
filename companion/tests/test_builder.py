from pathlib import Path

from dailydrop import builder
from dailydrop.cli import main as cli_main

FIX = Path(__file__).parent / "fixtures"


def test_build_digest_all_sections(tmp_path):
    out = builder.build_to_file(FIX / "config.yaml", tmp_path, "2026-07-31")
    assert out.name == "2026-07-31.drop"
    doc = out.read_text()
    lines = doc.splitlines()
    assert lines[0] == "DROP 1"
    assert "S News" in lines
    assert "S Weather" in lines
    assert "S Calendar" in lines
    assert "S Quote" in lines
    assert "S Reading goal" in lines
    assert any(ln.startswith("H * Kernel 7.0 released") for ln in lines)
    assert "T Seoul: 25C, Scattered clouds, humidity 61%" in lines
    assert any("Pay rent" in ln for ln in lines)
    # Deterministic per-date quote: rebuilding yields the identical document.
    again = builder.build_to_file(FIX / "config.yaml", tmp_path, "2026-07-31")
    assert again.read_text() == doc


def test_dead_feed_is_reported_not_fatal(tmp_path):
    config = {
        "title": "t",
        "feeds": [{"url": "missing-file.xml", "name": "Dead"}],
    }
    doc = builder.build_digest(config, "2026-07-31", base_dir=tmp_path)
    assert "unavailable" in doc


def test_cli_build(tmp_path, capsys):
    rc = cli_main(
        ["build", "--config", str(FIX / "config.yaml"), "--out", str(tmp_path),
         "--date", "2026-07-30"]
    )
    assert rc == 0
    printed = capsys.readouterr().out.strip()
    assert printed.endswith("2026-07-30.drop")
    assert (tmp_path / "2026-07-30.drop").exists()
