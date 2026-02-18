## Plan: Finish Pass3 Symbol Printing

Resume the Pass3 symbol printing work to complete formatting validation and end-to-end integration, keeping prior phases intact and using strict TDD.

**Phases 4**

1. **Phase 1: Symbol Table Compaction Tests**
   - **Objective:** Validate compaction and zero-page backup/restore.
   - **Files/Functions to Modify/Create:** tests/app_test.cpp; src/lib/asm/asm.cpp around DoPass3.
   - **Tests to Write:** Compaction/zero-page regression coverage (already written).
   - **Steps:**
     1. Add compaction tests; run to see failure.
     2. Implement minimal compaction fixes; rerun tests.
     3. Confirm full suite green.

2. **Phase 2: Sorting and Array Building Tests**
   - **Objective:** Verify alphabetic/value sorting and auxiliary array population.
   - **Files/Functions to Modify/Create:** tests/app_test.cpp; src/lib/asm/asm.cpp around DoSort and LD198.
   - **Tests to Write:** Alphabetic/value sorting and aux-array tests (already written).
   - **Steps:**
     1. Add sorting tests; run to see failure.
     2. Implement minimal sorting fixes; rerun tests.
     3. Confirm full suite green.

3. **Phase 3: Symbol Table Output Formatting Tests**
   - **Objective:** Confirm 2/4/6 column selection, subtitle text, and LstASym/LstVSym gating.
   - **Files/Functions to Modify/Create:** tests/app_test.cpp (Phase3_FormattingTest); src/lib/asm/asm.cpp formatting helpers (NumCols, Lst6Cols, SubTtlF, subtitle buffer, printer slot).
   - **Tests to Write:** ColumnCount_2Columns_40ColVideo; ColumnCount_4Columns_DefaultMode; ColumnCount_6Columns_PrinterMode; Subtitle_ContainsSourcePath; Subtitle_SymbolMode_ShowsSYMBOL; Subtitle_AddressMode_ShowsADDRESS; LstASym_EnablesAlphabeticListing; LstVSym_EnablesValueListing.
   - **Steps:**
     1. Add formatting tests; run to confirm red.
     2. Expose/adjust formatting helpers and logic minimally; rerun formatting and full suite to green.
     3. Record phase completion file and commit message.

4. **Phase 4: Integration and Edge Cases**
   - **Objective:** Execute DoPass3 end-to-end on realistic symbol tables (empty/single/multiple, defined/undefined, external, dual listings).
   - **Files/Functions to Modify/Create:** tests/app_test.cpp (Phase3_IntegrationTest); src/lib/asm/asm.cpp DoPass3/PrSymTbl integration.
   - **Tests to Write:** DoPass3_EmptyTable_NoOutput; DoPass3_SingleSymbol_Alphabetic; DoPass3_MultipleSymbols_ValueOrder; DoPass3_MixedDefinedUndefined; DoPass3_ExternalSymbols; DoPass3_BothListings_AlphaAndValue.
   - **Steps:**
     1. Add end-to-end fixtures/helpers and the above tests; run to see red.
     2. Implement minimal DoPass3/printing fixes to satisfy tests; rerun full suite.
     3. Record phase completion file and commit message.

**Open Questions 2**

1. For Phase 4 outputs, assert exact listing text snapshots or structured state to avoid whitespace brittleness?
2. For undefined/external flag bytes, confirm expected bit patterns (is MSB=1 sufficient, or match legacy flag values per symbol type?).
