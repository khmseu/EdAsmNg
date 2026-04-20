#!/usr/bin/env python3
"""Scan EDASM/EdAsmNg listings for Lxxxx labels that disagree with the address column."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

LISTING_SUFFIXES = {".lst", ".LST"}
LABEL_DEF_LINE_RE = re.compile(r"^([0-9A-Fa-f]{4}):.*\b(\d+)\s+(L[0-9A-Fa-f]{4})\b")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__,
    )
    parser.add_argument(
        "paths",
        nargs="+",
        help="Listing file(s) or directory/directories to scan.",
    )
    return parser.parse_args(argv)


def iter_listing_files(paths: list[str]) -> list[Path]:
    listing_files: list[Path] = []

    for raw_path in paths:
        path = Path(raw_path)
        if not path.exists():
            print(f"warning: path does not exist: {path}", file=sys.stderr)
            continue

        if path.is_dir():
            listing_files.extend(
                candidate
                for candidate in sorted(path.rglob("*"))
                if candidate.is_file() and candidate.suffix in LISTING_SUFFIXES
            )
            continue

        listing_files.append(path)

    seen: set[Path] = set()
    unique_files: list[Path] = []
    for path in listing_files:
        resolved = path.resolve()
        if resolved in seen:
            continue
        seen.add(resolved)
        unique_files.append(path)
    return unique_files


def find_mismatches(listing_path: Path) -> list[tuple[int, str, str, str]]:
    mismatches: list[tuple[int, str, str, str]] = []

    with listing_path.open("r", encoding="latin-1", errors="replace") as handle:
        for line_number, raw_line in enumerate(handle, 1):
            line = raw_line.rstrip("\n")
            match = LABEL_DEF_LINE_RE.match(line)
            if not match:
                continue

            address = match.group(1).upper()
            label = match.group(3).upper()
            label_address = label[1:]
            if label_address == address:
                continue

            mismatches.append((line_number, address, label, line))

    return mismatches


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    listing_files = iter_listing_files(args.paths)
    if not listing_files:
        print("No listing files found.", file=sys.stderr)
        return 1

    total_mismatches = 0
    scanned_files = 0
    mismatch_counts: list[tuple[Path, int]] = []

    for listing_path in listing_files:
        mismatches = find_mismatches(listing_path)
        scanned_files += 1
        if not mismatches:
            continue

        mismatch_counts.append((listing_path, len(mismatches)))

        print(f"{listing_path}:")
        for line_number, address, label, line in mismatches:
            print(f"  line {line_number}: address {address} vs label {label}")
            print(f"    {line}")
        total_mismatches += len(mismatches)

    if total_mismatches == 0:
        print(f"No out-of-sync Lxxxx labels found in {scanned_files} listing file(s).")
        return 0

    print(
        f"Found {total_mismatches} out-of-sync Lxxxx label(s) "
        f"across {scanned_files} listing file(s)."
    )
    print("Files with out-of-sync labels:")
    for listing_path, count in sorted(
        mismatch_counts,
        key=lambda item: (-item[1], str(item[0])),
    ):
        print(f"  {count:4d}  {listing_path}")
    return 2


if __name__ == "__main__":
    sys.exit(main())
