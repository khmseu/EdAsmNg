# Phase 1 Complete: Unify Listing Output Path

Phase 1 routed CLI listing generation through assembler-owned output instead of the hardcoded placeholder block and added a robust CLI test that validates equivalence with assembler-generated listing text. Review feedback was addressed with a follow-up fix for CurAdr reporting and safer executable-path quoting in the test command.

**Files created/changed:**

- include/EdAsmNg/asm.hpp
- src/lib/asm/asm.cpp
- src/main.cpp
- tests/CMakeLists.txt
- tests/app_test.cpp
- plans/full-listing-generation-plan.md
- plans/full-listing-generation-plan-phase-1-complete.md

**Functions created/changed:**

- BuildListingOutput
- GetCurAdr
- main
- CliListingTests.ListingOutputComesFromAssemblerPathNotPlaceholderBlock
- BuildExpectedAssemblerListing

**Tests created/changed:**

- CliListingTests.ListingOutputComesFromAssemblerPathNotPlaceholderBlock

**Review Status:** APPROVED

**Git Commit Message:**
feat: unify cli listing output path

- route --listing output through assembler listing API
- replace placeholder-only assertion with assembler-equivalence CLI test
- fix CurAdr listing field and harden test executable path quoting
