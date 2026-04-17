# Listing Comparison Filters

This note lists only what `normalize_listing.py` currently filters out before EDASM vs EdAsmNg listing comparison.

## Filtered Out Entirely

- Empty lines (including lines that become empty after form-feed removal).
- EDASM diagnostic/error chatter lines:
  - `***** ... ERROR IN LINE N`
  - `ERROR SUMMARY`
  - `... ERROR IN LINE N OF FILE ...`
  - `** N ERRORS IN THIS ASSEMBLY`
- Listing-address lines (`XXXX:...`) that contain only carryover byte tokens and no source tokens.
- Standalone numeric source-text suffix lines (for example `123`).
- `END` pseudo-lines that appear as stale byte carryover artifacts.

## Normalized Values (Retained Lines)

- `** ASSEMBLER CREATED ON ...` lines are retained, but the date/time value is normalized to `** ASSEMBLER CREATED ON <DATE>`.
- `** FREE SPACE PAGE COUNT ...` lines are retained, but the numeric count is normalized to `** FREE SPACE PAGE COUNT <N>`.

## Retained As-Is

- Symbol-table dump records (including lines with an indicator character before a hex address, for example `?XXXX...`) are retained and compared.
