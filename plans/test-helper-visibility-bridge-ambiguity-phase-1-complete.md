## Phase 1 Complete: Consolidate Test Helper Declarations

Created a shared test helper declaration header and removed duplicate helper prototypes in the tests so the test translation unit compiles cleanly.

**Files created/changed:**

- [tests/asm_test_helpers.hpp](tests/asm_test_helpers.hpp)
- [tests/app_test.cpp](tests/app_test.cpp)

**Functions created/changed:**

- `ResetErrorState`
- `GetErrorCount`
- `GetErrorMessages`
- `GetNonfatalError`
- `GetNonfatalAddr`
- `GetNonfatalLine`
- `ResetAsmState`
- `GetLength`
- `SetLength`
- `GetLenTIdx`
- `SetLenTIdx`
- `GetGMC`
- `SetGMC`
- `GetY`
- `SetY`
- `GetSubTIdx`
- `SetSubTIdx`
- `SetupMnemP`
- `SetupOperandField`
- `GInstLen`

**Tests created/changed:**

- None

**Review Status:** APPROVED

**Git Commit Message:**

test: centralize asm test helpers

- add shared helper declarations header
- remove redundant helper prototypes in tests
- keep helper API consistent for pass 2 tests
