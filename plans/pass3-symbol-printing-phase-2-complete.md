## Phase 2 Complete: Sorting and Array Building

Aux array construction and sorting now mirror the original Pass 3 flows for alphabetic and value modes, NxtToken is preserved across zero-page save/restore, and compacted flag reads are accurate without synthesis.

**Files created/changed:**

- [src/lib/asm/asm.cpp](src/lib/asm/asm.cpp)
- [tests/app_test.cpp](tests/app_test.cpp)
- [tests/asm_test_helpers.hpp](tests/asm_test_helpers.hpp)

**Functions created/changed:**

- SaveZP / RestoreZP (added NxtToken round-trip)
- GetNxtToken / SetNxtToken test helpers
- GetCompactedSymbolFlags (pure read of compacted flag byte)

**Tests created/changed:**

- Phase3*SortingTest BuildAuxArray*\_and DoSort\_\_ suites in [tests/app_test.cpp](tests/app_test.cpp)
- Phase3_SortingTest UndefinedSymbols_FlaggedInAlphaMode strengthened for name-based lookup and flag expectations
- Phase3_SortingTest SaveRestoreZP_NxtToken added to verify NxtToken persistence

**Review Status:** APPROVED with minor recommendations

**Git Commit Message:**

```
feat: add pass3 sorting helpers and tests

- Add Phase3 sorting and flag coverage for alphabetic/value modes
- Preserve NxtToken through SaveZP/RestoreZP and expose helpers
- Simplify compacted flag helper to pure read of stored byte
```
