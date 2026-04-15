#!/usr/bin/env python3
"""Normalize listing files for comparison.

Strips EDASM-specific volatile fields from listing files and normalizes
column spacing so EdAsmNg and EDASM listings can be semantically compared.
"""

from __future__ import annotations

import re
import sys


def normalize_listing(text: str) -> str:
    """Return a normalized listing suitable for comparison.

    Removes:
    - Leading blank lines and header noise (SOURCE FILE #01 =>...)
    - Object file name lines (----- NEXT OBJECT FILE NAME...)
    - Timestamp lines (** ASSEMBLER CREATED ON...)
    - Free space lines (** FREE SPACE PAGE COUNT...)
    - EDASM line-number column from code lines

    Normalizes:
    - Trailing whitespace on each line
    - Trailing blank lines (removed)
    """
    lines = text.splitlines()
    normalized = []

    for line in lines:
        line = line.rstrip()

        # Strip EDASM volatile metadata lines
        if re.match(r'\*\*\s*ASSEMBLER CREATED ON', line):
            continue
        if re.match(r'\*\*\s*FREE SPACE PAGE COUNT', line):
            continue
        if re.match(r'-{3,}\s*NEXT OBJECT FILE NAME', line):
            continue
        if re.match(r'SOURCE\s+FILE\s*#\d+\s*=>', line):
            continue

        # Normalize EDASM code lines: strip the line-number column.
        # EDASM format: "AAAA:BB BB               N LABEL   MNEMONIC"
        # EdAsmNg format: "AAAA:BB BB               LABEL   MNEMONIC"
        # Strip 1-5 digit decimal line number after the bytes columns.
        line = re.sub(r'^([0-9A-Fa-f]{4}:[0-9A-Fa-f ]{12,20})\s+\d+\s+', r'\1 ', line)

        normalized.append(line)

    # Remove leading blank lines
    while normalized and not normalized[0]:
        normalized.pop(0)

    # Remove trailing blank lines
    while normalized and not normalized[-1]:
        normalized.pop()

    return '\n'.join(normalized) + '\n' if normalized else ''


def main() -> int:
    if len(sys.argv) < 2:
        print("Usage: normalize_listing.py <listing_file>", file=sys.stderr)
        return 1

    with open(sys.argv[1], 'r', errors='replace') as f:
        text = f.read()

    print(normalize_listing(text), end='')
    return 0


if __name__ == '__main__':
    sys.exit(main())
