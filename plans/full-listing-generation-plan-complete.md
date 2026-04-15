# Plan Complete: Full Listing Generation Pipeline

Completed the five-phase listing pipeline plan end to end. The CLI now writes assembler-produced listing output, the low-level listing primitives and pass-2 rendering path are deterministic, LIST/LST/NOLIST/PAGE directives drive runtime listing behavior, and the comparative harness can report normalized listing differences alongside object-code parity. This leaves object parity green across the current fixture set and gives a concrete reporting path for the remaining listing-format fidelity work.

**Phases Completed:** 5 of 5

1. ✅ Phase 1: Unify Listing Output Path
2. ✅ Phase 2: Implement Core Listing Primitives
3. ✅ Phase 3: Activate Pass-2 Line Listing Formatting
4. ✅ Phase 4: Wire Directive-Controlled Listing Behavior
5. ✅ Phase 5: Add Comparative Listing Parity Harness

**All Files Created/Modified:**

- comparative-tests/compare.py
- comparative-tests/normalize_listing.py
- include/EdAsmNg/asm.hpp
- plans/full-listing-generation-plan.md
- plans/full-listing-generation-plan-phase-1-complete.md
- plans/full-listing-generation-plan-phase-2-complete.md
- plans/full-listing-generation-plan-phase-3-complete.md
- plans/full-listing-generation-plan-phase-4-complete.md
- plans/full-listing-generation-plan-phase-5-complete.md
- src/lib/asm/asm.cpp
- src/main.cpp
- tests/CMakeLists.txt
- tests/app_test.cpp

**Key Functions/Classes Added:**

- BuildListingOutput
- PutC
- PutCR
- PrtFF
- PrByte
- ListCode
- LstSrcLn
- PrtAsmLn
- Bridge_HndlLIST
- Bridge_HndlLST
- Bridge_HndlNOLIST
- Bridge_DoPage
- compare_listings
- normalize_listing

**Test Coverage:**

- Total tests written: 15
- All tests passing: ✅

**Recommendations for Next Steps:**

- Tighten normalized listing parity by aligning EdAsmNg line formatting with EDASM on the current three listing fixtures.
- Expand `--compare-listing` coverage from the current three fixtures to the broader comparative corpus once formatting deltas are intentionally addressed.
