# Phase 3 Complete: Activate Pass-2 Line Listing Formatting

Phase 3 replaced pass-2 listing stubs with deterministic line rendering for address, object bytes, and source text columns. Tests now assert exact rendered output lines, including spacing and newline boundaries, and include a source-copy boundary case at index 0xFF.

**Files created/changed:**

- src/lib/asm/asm.cpp
- tests/app_test.cpp
- plans/full-listing-generation-plan-phase-3-complete.md

**Functions created/changed:**

- ListCode
- LstSrcLn
- PrtAsmLn

**Tests created/changed:**

- Pass2Test.test_pass2_listing_line_nop_includes_address_bytes_and_source
- Pass2Test.test_pass2_listing_line_multibyte_groups_bytes_and_source
- Pass2Test.test_pass2_listing_line_jump_fixture_shape_includes_expected_address_and_bytes
- Pass2Test.test_pass2_listing_line_source_copy_includes_char_at_0xFF_boundary

**Review Status:** APPROVED

**Git Commit Message:**
feat: render deterministic pass2 listing lines

- replace pass2 listing stubs with deterministic address/bytes/source rendering
- strengthen listing tests to strict full-line equality assertions
- fix source copy boundary handling at index 0xFF
