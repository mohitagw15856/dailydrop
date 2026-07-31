"""Command line interface: `dailydrop build`."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from . import builder


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog="dailydrop", description="Build daily .drop digests"
    )
    sub = parser.add_subparsers(dest="command", required=True)

    build = sub.add_parser("build", help="compile today's digest from a YAML config")
    build.add_argument("--config", required=True, type=Path, help="config YAML path")
    build.add_argument("--out", required=True, type=Path, help="output directory")
    build.add_argument("--date", help="ISO date to build for (default: today)")

    args = parser.parse_args(argv)

    if args.command == "build":
        try:
            out = builder.build_to_file(args.config, args.out, args.date)
        except Exception as exc:  # noqa: BLE001
            print(f"error: {exc}", file=sys.stderr)
            return 1
        print(out)
        return 0
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
