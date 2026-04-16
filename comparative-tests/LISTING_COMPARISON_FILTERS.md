# Listing Comparison Filters

This note summarizes what normalize_listing.py removes before EDASM vs EdAsmNg listing comparison.

## Filtered Out Entirely

- Empty lines after form-feed removal.
- EDASM diagnostic/error chatter lines:
  - ***** ... ERROR IN LINE N
  - ERROR SUMMARY
  - ... ERROR IN LINE N OF FILE ...
  - ** N ERRORS IN THIS ASSEMBLY
- EDASM summary/footer metadata:
  - ** ASSEMBLER CREATED ON ...
  - ** FREE SPACE PAGE COUNT ...
  - ** SUCCESSFUL ASSEMBLY ...
  - ** TOTAL LINES ASSEMBLED ...
- Object file banner/header noise:
  - ----- NEXT OBJECT FILE NAME ...
  - SOURCE FILE #N => ...
- Symbol-table style records:
  - ?XXXX lines (hex address records)
  - standalone symbol lines shaped like: XXXX SYMBOL
- Code/listing lines that normalize to no source tokens (raw carryover bytes only).
- Standalone numeric line markers (for example: 123).
- END pseudo-lines with stale bytes.

## Normalized (Not Fully Removed)

- Whitespace is collapsed to single spaces for semantic comparison.
- For code lines with object bytes, comparison uses byte sequence + normalized source text (display address is ignored).
- Decimal line-number tokens are retained as part of normalized source text.

## Directive Behavior

- Non-data directives (for example ORG, REL, LIST, TITLE, PAGE, SBTL) are compared as source text only.
- Data directives (ASC/ASCII/DCI/DFB/BYTE/DS/DW/WORD) keep byte sequence + source text.
