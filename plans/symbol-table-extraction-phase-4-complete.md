## Phase 4 Complete: Build, Test and Verify Symbol Table Extraction

Successfully completed the build, test, and verification phase after resolving a critical macro expansion issue.

**Files created/changed:**

- src/lib/asm/asm.cpp (added out-of-line AsmInternal::SrcP_at implementation with macro #undef/#define guards)

**Functions created/changed:**

- AsmInternal::SrcP_at() (added proper implementation outside namespace block to prevent inlining and ensure symbol generation)

**Critical Bug Fixed:**

- **Macro Expansion Issue**: The `#define SrcP_at(idx) SrcP_byte(idx)` macro was expanding in the function definition name itself, causing the compiler to try to define `AsmInternal::SrcP_byte` instead of `AsmInternal::SrcP_at`. This resulted in undefined reference linker errors.
- **Solution**: Wrapped the function implementation with `#undef SrcP_at` before and `#define SrcP_at(idx) SrcP_byte(idx)` after to prevent macro expansion during function definition.

**Tests created/changed:**

- No test changes required - all 134 existing tests pass

**Review Status:** APPROVED - All tests passing, no regressions detected

**Test Results:**

```
[==========] 134 tests from 14 test suites ran. (1 ms total)
[  PASSED  ] 134 tests.
```

**Git Commit Message:**

```
fix: resolve SrcP_at linker error in symbol table extraction

- Add out-of-line implementation of AsmInternal::SrcP_at with macro guards
- Work around macro expansion issue where #define SrcP_at expanded in function name
- All 134 tests passing, no regressions detected
```
