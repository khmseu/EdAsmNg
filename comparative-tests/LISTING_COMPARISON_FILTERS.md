# Listing Comparison Filters

This note summarizes what normalize_listing.py removes/transforms before EDASM vs EdAsmNg listing comparison.

## Filtered Out Entirely

- Empty lines after form-feed removal.
- EDASM diagnostic/error chatter lines:
  - **\*** ... ERROR IN LINE N
  - ERROR SUMMARY
  - ... ERROR IN LINE N OF FILE ...
  - \*\* N ERRORS IN THIS ASSEMBLY
- EDASM summary/footer metadata:
  - \*\* ASSEMBLER CREATED ON ...
  - \*\* FREE SPACE PAGE COUNT ...
  - \*\* SUCCESSFUL ASSEMBLY ...
  - \*\* TOTAL LINES ASSEMBLED ...
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

- Whitespace is collapsed to single spaces only for files marked with `[WS]` on the first source line (handled by compare.py).
- For lines with object bytes, comparison uses byte sequence + normalized source text (display address is ignored).
- For non-byte listing lines, comparison keeps everything from the first token after the display-address field so ER/expression values and line numbers are compared.

## Directive Behavior

- Non-byte directive lines (for example ORG/EQU/REL/LIST/TITLE/PAGE/SBTL) are not reduced to source-only text; ER/expression fields are part of comparison.
- Data directives (ASC/ASCII/DCI/DFB/BYTE/DS/DW/WORD) keep byte sequence + source text.
- ASC/DCI long-string output no longer emits duplicate trailing stale GMC bytes; comparison therefore expects a single canonical listing/object emission.
