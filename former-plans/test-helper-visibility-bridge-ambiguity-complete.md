## Plan Complete: Restore Test Helper Visibility and Fix Bridge Ambiguity

Successfully consolidated test helper declarations, resolved Bridge function ambiguity, and normalized helper signatures. The project now builds cleanly with no compilation or linker errors, and test helpers have consistent, unambiguous APIs.

**Phases Completed:** 3 of 3

1. ✅ Phase 1: Consolidate Test Helper Declarations
2. ✅ Phase 2: Fix Bridge Ambiguity
3. ✅ Phase 3: Align Helper Signatures

**All Files Created/Modified:**

- [tests/asm_test_helpers.hpp](tests/asm_test_helpers.hpp) (new)
- [tests/app_test.cpp](tests/app_test.cpp)
- [src/lib/asm/asm.cpp](src/lib/asm/asm.cpp)
- [plans/test-helper-visibility-bridge-ambiguity-plan.md](plans/test-helper-visibility-bridge-ambiguity-plan.md)
- [plans/test-helper-visibility-bridge-ambiguity-phase-1-complete.md](plans/test-helper-visibility-bridge-ambiguity-phase-1-complete.md)
- [plans/test-helper-visibility-bridge-ambiguity-phase-2-complete.md](plans/test-helper-visibility-bridge-ambiguity-phase-2-complete.md)
- [plans/test-helper-visibility-bridge-ambiguity-phase-3-complete.md](plans/test-helper-visibility-bridge-ambiguity-phase-3-complete.md)

**Key Functions/Classes Added/Modified:**

- Helper declarations centralized: `ResetErrorState`, `GetErrorCount`, `GetErrorMessages`, `GetNonfatalError`, `ResetAsmState`, `GetLength`, `SetLength`, `GetLenTIdx`, `SetLenTIdx`, `GetGMC`, `SetGMC`, `GetY`, `SetY`, `GetSubTIdx`, `SetSubTIdx`, `SetupMnemP`, `SetupOperandField`, `GInstLen`
- Bridge functions converted to stubs for unimplemented handlers
- Duplicate helpers removed: `GetGMC(int)`, `SetAddressingMode()`, `GetAddressingMode()`
- All signatures normalized to use `uint8_t` for buffer indices

**Test Coverage:**

- Test compilation: ✅ Successful (0 errors, 0 warnings)
- Library build: ✅ Successful
- Test executable link: ✅ Successful (1.7 MB)
- Tests run: ✅ Phase 2 GInstLen tests execute (6 failures are pre-existing functionality issues, not declaration/linkage issues)

**Commits Made:**

1. `1c7e545` - test: centralize asm test helpers
2. `6b5e4aa` - fix: resolve bridge function ambiguity
3. `72ee17c` - refactor: normalize test helper signatures

**Recommendations for Next Steps:**

- Once handler implementations (#if 0 blocks) are enabled, Bridge\_\* stubs should be updated to call the actual implementations
- Consider moving `ErrorInfo` struct to a shared "test API" header to prevent future drift between declaration and definition
- The test suite itself is ready for Pass 2 verification once underlying address mode and directive handlers are implemented
