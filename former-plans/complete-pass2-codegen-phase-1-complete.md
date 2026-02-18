## Phase 1 Complete: Code Organization and Cleanup

Prepared the codebase for enabling disabled Pass 2 logic by extracting helper functions and establishing proper code organization.

**Files created/changed:**

- [src/lib/asm/asm.cpp](../src/lib/asm/asm.cpp)

**Functions created/changed:**

- WhiteSpc() - extracted from disabled block, now compiled
- SkipSpcs() - extracted from disabled block, now compiled
- AdvObjPC() - new stub function for Phase 3

**Tests created/changed:**

- None (Phase 1 is preparation only)

**Review Status:** APPROVED

**Git Commit Message:**

```
refactor: Phase 1 - Extract scanner helpers for Pass 2

- Extract WhiteSpc() and SkipSpcs() from disabled block
- Add AdvObjPC() stub for future StorGMC() support
- Add clear section boundaries and documentation
- All tests pass, no regressions
```
