## Plan Complete: Debug Phase 2/3 Test Failures

All 9 failing tests in GInstLen and StorGMC have been successfully fixed. The addressing mode parsing logic now faithfully matches the original EDASM behavior, including proper table-driven lookups, zero-page/external symbol handling, SW16 instruction length calculation, and delimiter advancement. All code generation paths are now verified through comprehensive unit tests.

**Phases Completed:** 8 of 8

1. ✅ Phase 1: Diagnose Root Causes
2. ✅ Phase 2: Fix AdvObjPC Length Calculation
3. ✅ Phase 3: Fix GAdrMod Addressing Mode Detection
4. ✅ Phase 4: Fix Delimiter Advancement in GAdrMod
5. ✅ Phase 5: Expand InstLenT and Fix SW16 Length Lookup
6. ✅ Phase 6: Add Zero-Page/External Symbol Warnings
7. ✅ Phase 7: Remove Duplicate GAdrMod Definition
8. ✅ Phase 8: Fix Delimiter Advancement and Carry Flag Usage

**All Files Created/Modified:**

- src/lib/asm/asm.cpp
- tests/app_test.cpp
- plans/debug-phase2-phase3-failures-plan.md
- ANALYSIS_PHASE2_PHASE3_FAILURES.md

**Key Functions/Classes Fixed:**

- `GInstLen()` - Instruction length calculation for all addressing modes
- `GAdrMod()` - Addressing mode detection and parsing
- `AdvObjPC()` - Object PC advancement using correct length tables
- `IsZPMod()` - Zero-page mode detection with external symbol warnings
- `IsAccMod()` - Accumulator mode detection
- `Is65C02()` - 65C02 CPU detection
- `L8598()` - Instruction length table lookup
- `StorGMC()` - Object code generation and storage
- `InstLenT[]` - Expanded to 13 entries for all addressing modes
- `L851F[]` - SW16 instruction length table with correct mapping

**Test Coverage:**

- Total tests written: 13 (7 GInstLen + 6 StorGMC)
- All tests passing: ✅

**Recommendations for Next Steps:**

- Continue with Pass 3 (final assembly pass) implementation
- Add comprehensive error handling and edge case tests
- Implement remaining assembler directives and pseudo-ops
- Add symbol table dump and listing file generation
