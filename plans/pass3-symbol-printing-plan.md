## Plan: Test Pass 3 Symbol Table Printing

Pass 3 is already implemented (~300 LOC) but never tested. It handles printing the symbol table after assembly completes, with support for alphabetic or value-based sorting in 2/4/6 column formats. This plan will create comprehensive unit tests to verify all Pass 3 functionality and ensure correct symbol table management.

**Phases: 4**

1. **Phase 1: Symbol Table Compaction Tests**
   - **Objective:** Test the symbol table reorganization logic that removes link pointers and compacts nodes
   - **Files/Functions to Modify/Create:**
     - [tests/app_test.cpp](tests/app_test.cpp) - Add Phase3_CompactionTest suite
     - Test helpers for DoPass3 and internal state access
   - **Tests to Write:**
     - `EmptySymbolTable_ReturnsEarly` - Verify early return when all HeaderT entries are zero
     - `SingleSymbol_CompactsCorrectly` - Test compaction of one symbol node
     - `MultipleSymbols_PreservesOrder` - Verify multiple symbols compact without corruption
     - `SymbolFlags_PreservedAfterCompaction` - Ensure flag bytes survive reorganization
     - `EndSymT_UpdatedCorrectly` - Verify end pointer is adjusted after compaction
   - **Steps:**
     1. Add test helper declarations for Pass 3 internal functions
     2. Write test for empty symbol table early-exit path
     3. Write test for single symbol compaction, verify fields intact
     4. Add test for multiple symbols, check all preserved
     5. Test that symbol flags (defined/undefined, relative) are maintained
     6. Verify EndSymT and StrtSymT pointers are updated correctly
     7. Run tests to see them fail (no test helpers exposed yet)
     8. Expose necessary test helpers in asm.cpp
     9. Run tests again to confirm they pass

2. **Phase 2: Sorting and Array Building Tests**
   - **Objective:** Test the auxiliary array construction and sorting algorithm for both alphabetic and value-based modes
   - **Files/Functions to Modify/Create:**
     - [tests/app_test.cpp](tests/app_test.cpp) - Add Phase3_SortingTest suite
     - [src/lib/asm/asm.cpp](src/lib/asm/asm.cpp) - Expose DoSort, LD198 test helpers if needed
   - **Tests to Write:**
     - `BuildAuxArray_AlphabeticMode` - Verify 2-byte entries for symbol sort
     - `BuildAuxArray_ValueMode` - Verify 4-byte entries for address sort
     - `DoSort_AlphabeticOrder` - Test sorting symbols alphabetically
     - `DoSort_ValueOrder` - Test sorting symbols by address
     - `UndefinedSymbols_FlaggedInAlphaMode` - Verify undefined symbols get special flag treatment
     - `RecCnt_TracksEntryCount` - Ensure record counter is maintained correctly
   - **Steps:**
     1. Write test for building auxiliary array in alphabetic mode (2-byte entries)
     2. Test building aux array in value mode (4-byte entries with addresses)
     3. Run tests to see them fail (aux array not accessible)
     4. Add test helpers to expose UnsortedP, SortedP, RecCnt state
     5. Create test for DoSort in alphabetic mode with known symbol order
     6. Add test for value-based sorting with different addresses
     7. Test that undefined symbols get flag byte modified correctly in alpha mode
     8. Verify RecCnt is incremented for each symbol processed
     9. Run all tests to confirm sorting logic works correctly

3. **Phase 3: Symbol Table Output Formatting Tests**
   - **Objective:** Test column formatting (2/4/6 columns), subtitle handling, and symbol flag display
   - **Files/Functions to Modify/Create:**
     - [tests/app_test.cpp](tests/app_test.cpp) - Add Phase3_FormattingTest suite
     - [src/lib/asm/asm.cpp](src/lib/asm/asm.cpp) - Expose PrSymTbl test helper
   - **Tests to Write:**
     - `ColumnCount_2Columns_40ColVideo` - Verify 2-column mode for 40-col display
     - `ColumnCount_4Columns_DefaultMode` - Verify 4-column default
     - `ColumnCount_6Columns_PrinterMode` - Verify 6-column mode when Lst6Cols set
     - `Subtitle_ContainsSourcePath` - Test subtitle generation with source filename
     - `Subtitle_SymbolMode_ShowsSYMBOL` - Verify "SYMBOL" text in alphabetic mode
     - `Subtitle_AddressMode_ShowsADDRESS` - Verify "ADDRESS" text in value mode
     - `LstASym_EnablesAlphabeticListing` - Test LstASym flag controls alpha listing
     - `LstVSym_EnablesValueListing` - Test LstVSym flag controls value listing
   - **Steps:**
     1. Write test for 2-column mode (no printer, 40-col video)
     2. Test 4-column default mode
     3. Test 6-column mode when Lst6Cols is set and printer active
     4. Run tests to see them fail (formatting not testable yet)
     5. Add test helpers to capture/verify column count (NumCols)
     6. Write test for subtitle generation with source pathname
     7. Test "SYMBOL" vs "ADDRESS" subtitle text based on sort mode
     8. Add tests for LstASym and LstVSym flags enabling/disabling output
     9. Run all tests to confirm formatting logic is correct

4. **Phase 4: Integration and Edge Cases**
   - **Objective:** Test complete Pass 3 execution end-to-end with various symbol table configurations
   - **Files/Functions to Modify/Create:**
     - [tests/app_test.cpp](tests/app_test.cpp) - Add Phase3_IntegrationTest suite
     - Full DoPass3 execution with real symbol tables
   - **Tests to Write:**
     - `DoPass3_EmptyTable_NoOutput` - Verify graceful handling of empty table
     - `DoPass3_SingleSymbol_Alphabetic` - End-to-end test with one symbol, alpha mode
     - `DoPass3_MultipleSymbols_ValueOrder` - End-to-end test with multiple symbols, value mode
     - `DoPass3_MixedDefinedUndefined` - Test with mix of defined/undefined symbols
     - `DoPass3_ExternalSymbols` - Test external symbol flag handling
     - `DoPass3_BothListings_AlphaAndValue` - Test when both LstASym and LstVSym are enabled
   - **Steps:**
     1. Write test for empty symbol table (DoPass3 returns early)
     2. Test single symbol end-to-end in alphabetic mode
     3. Run tests to see them fail (Pass 3 not integrated in test harness)
     4. Build minimal symbol table setup in test fixture for Pass 3
     5. Write test for multiple symbols in value order mode
     6. Add test with mix of defined (msb=0 in flag) and undefined (msb=1) symbols
     7. Test external symbols (verify flag byte interpretation)
     8. Write test for enabling both LstASym and LstVSym (prints twice)
     9. Run all tests to confirm Pass 3 works correctly end-to-end
     10. Verify all formatting, sorting, and output logic integrates properly

**Open Questions:**

1. Should we stub PrSymTbl output or capture it for verification?
2. Do we need ncurses integration tests or mock output capture sufficient?
3. Should we test PollKbd abort handling in Pass 3, or defer that?
