## Phase 4 Complete: Integration and Edge Cases

Validated end-to-end Pass 3 runs in both alphabetic and value modes, ensuring compacted symbols, flags, and aux-array sizing match real assembler behavior under mixed symbol sets.

**Files created/changed:**

- [src/lib/asm/asm.cpp](src/lib/asm/asm.cpp)
- [tests/app_test.cpp](tests/app_test.cpp)

**Functions created/changed:**

- AddTestSymbol
- GetSortedSymbolName
- GetSortedSymbolValue
- GetCompactedSymbolFlags

**Tests created/changed:**

- DoPass3_EmptyTable_NoOutput
- DoPass3_SingleSymbol_Alphabetic
- DoPass3_MultipleSymbols_ValueOrder
- DoPass3_MixedDefinedUndefined
- DoPass3_ExternalSymbols
- DoPass3_BothListings_AlphaAndValue

**Review Status:** APPROVED

**Git Commit Message:**
test: add pass3 integration coverage

- Add Pass 3 integration and edge-case tests for alpha/value symbol listings
- Align aux-array helper sizing and compacted flag reads with listing mode
- Remove test-only flag overrides to use real compaction paths
