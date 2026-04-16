#!/usr/bin/env python3
"""Normalize listing files for comparison.

Strips EDASM-specific volatile fields from listing files and normalizes
column spacing so EdAsmNg and EDASM listings can be semantically compared.
"""

from __future__ import annotations

import re
import sys


DIRECTIVE_TOKENS = {
    'ASC', 'ASCII', 'DCI', 'DFB', 'BYTE', 'DS', 'DW', 'WORD', 'EQU', 'LIST', 'LST',
    'NOLIST', 'ORG', 'PAGE', 'REL', 'TITLE', 'SBTL'
}
def _canonicalize_listing_line(line: str) -> str | None:
    line = line.replace('\f', '')
    if not line.strip():
        return None

    stripped = line.strip()

    # Drop EDASM diagnostic chatter and summary blocks; these are tool-specific
    # and can be emitted even when object bytes are equivalent.
    if re.match(r'^\*{5}\s+.+ERROR IN LINE\s+\d+', stripped):
        return None
    if re.match(r'^ERROR SUMMARY\s*$', stripped):
        return None
    if re.match(r'^[A-Z ]+ERROR IN LINE\s+\d+\s+OF FILE', stripped):
        return None
    if re.match(r'^\**\s*\d+\s+ERRORS IN THIS ASSEMBLY\s*$', stripped):
        return None

    if re.match(r'\*\*\s*ASSEMBLER CREATED ON', stripped):
        return None
    if re.match(r'\*\*\s*FREE SPACE PAGE COUNT', stripped):
        return None
    if re.match(r'\*\*\s*SUCCESSFUL ASSEMBLY', stripped):
        return None
    if re.match(r'\*\*\s*TOTAL LINES ASSEMBLED', stripped):
        return None
    if re.match(r'-{3,}\s*NEXT OBJECT FILE NAME', stripped):
        return None
    if re.match(r'SOURCE\s+FILE\s*#\d+\s*=>', stripped):
        return None
    if re.match(r'\?[0-9A-Fa-f]{4}\b', stripped):
        return None
    if re.match(r'^[0-9A-Fa-f]{4}\s+[A-Za-z_.$][A-Za-z0-9_.$]*\s*$', stripped):
        return None

    match = re.match(r'^([0-9A-Fa-f]{4}):(.*)$', line)
    if not match:
        return stripped

    rest = match.group(2)
    if not rest.strip():
        return None

    token_spans = list(re.finditer(r'\S+', rest))
    tokens = [span.group(0) for span in token_spans]
    byte_tokens = []
    idx = 0
    while idx < len(tokens) and re.fullmatch(r'[0-9A-Fa-f]{2}', tokens[idx]):
        byte_tokens.append(tokens[idx].upper())
        idx += 1

    source_tokens = tokens[idx:]
    if not source_tokens:
        # Raw EDASM object-record carryover bytes without source context are
        # volatile and do not represent semantic listing content.
        return None

    mnemonic_index = None
    for token_index, token in enumerate(source_tokens):
        token_upper = token.upper()
        if token_upper in DIRECTIVE_TOKENS or re.fullmatch(r'[A-Z]{3}', token_upper):
            mnemonic_index = token_index
            break

    source_start = 0
    if mnemonic_index is not None:
        source_start = mnemonic_index
        if mnemonic_index > 0 and not re.fullmatch(r'[0-9A-Fa-f]{3,4}', source_tokens[mnemonic_index - 1]):
            source_start = mnemonic_index - 1
        # Keep EDASM decimal line numbers when present before label/mnemonic tokens.
        if source_start > 0 and re.fullmatch(r'\d+', source_tokens[source_start - 1]):
            source_start -= 1

    # EDASM sometimes inserts a rendered expression/target address token
    # immediately before the decimal line number; ignore that volatile token
    # while preserving the line-number itself for parity checks.
    if (
        source_start + 1 < len(source_tokens)
        and re.fullmatch(r'[0-9A-Fa-f]{3,4}', source_tokens[source_start])
        and re.fullmatch(r'\d+', source_tokens[source_start + 1])
    ):
        source_start += 1

    abs_source_start = idx + source_start
    if abs_source_start >= len(token_spans):
        return None
    source_text = rest[token_spans[abs_source_start].start():]
    source_text_stripped = source_text.strip()
    if not source_text_stripped:
        return None

    # Drop standalone line-number suffixes and END pseudo-lines with stale bytes.
    if re.fullmatch(r'\d+', source_text_stripped):
        return None
    if source_text_stripped.upper() == 'END':
        return None

    first_token = source_text_stripped.split()[0].upper()
    if first_token in DIRECTIVE_TOKENS and first_token not in {'ASC', 'ASCII', 'DCI', 'DFB', 'BYTE', 'DS', 'DW', 'WORD'}:
        return source_text

    if byte_tokens:
        # Preserve listing whitespace exactly (excluding the leading display address).
        first_byte_span = token_spans[0]
        return rest[first_byte_span.start():]
    return source_text


def normalize_listing(text: str) -> str:
    """Return a normalized listing suitable for comparison.

    Removes volatile EDASM-only metadata and diagnostics while preserving
    whitespace in all retained listing content lines.
    """
    normalized = []

    for line in text.splitlines():
        canonical = _canonicalize_listing_line(line)
        if canonical is not None:
            normalized.append(canonical)

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
