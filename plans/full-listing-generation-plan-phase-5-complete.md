## Phase 5 Complete: Add Comparative Listing Parity Harness

Added comparative listing output to the parity harness. `BuildListingOutput` now returns actual assembled listing content, `normalize_listing.py` strips volatile EDASM metadata, and `compare.py --compare-listing` compares normalized listings with correct match/diff/skip accounting. All 179 tests pass, object parity is 6/6 across the current fixture set, and listing comparison is wired as report-only while formatting differences remain.

**Files created/changed:**

- `src/lib/asm/asm.cpp` — `BuildListingOutput` simplified to return `g_listing_sink`
- `tests/app_test.cpp` — new `CliListingTests.ListingOutputContainsActualCodeLines` test
- `comparative-tests/compare.py` — `--compare-listing` flag, `compare_listings()` function, match/diff/skip counters
- `comparative-tests/normalize_listing.py` (NEW) — strips EDASM volatile fields, normalizes code-line columns

**Functions created/changed:**

- `BuildListingOutput` (asm.cpp) — returns `g_listing_sink` instead of placeholder header
- `compare_listings()` (compare.py) — normalizes both listings, diffs them, returns `'match'`/`'diff'`/`'skip'`
- `normalize_listing()` (normalize_listing.py) — strips timestamp, free-space, source/object header lines; strips EDASM line-number column

**Tests created/changed:**

- `CliListingTests.ListingOutputContainsActualCodeLines` — asserts CLI listing file contains `0800:EA`

**Review Status:** APPROVED

**Git Commit Message:**
feat: add comparative listing parity harness

- fix BuildListingOutput to return actual assembled listing content
- add normalize_listing.py to strip volatile EDASM metadata fields
- add --compare-listing flag to compare.py with match/diff/skip reporting
- add CLI test asserting listing file contains real code lines
