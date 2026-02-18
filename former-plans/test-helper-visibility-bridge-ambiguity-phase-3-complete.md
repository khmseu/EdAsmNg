## Phase 3 Complete: Align Helper Signatures

Normalized test helper signatures by removing signature mismatches and redundant helper functions.

**Files created/changed:**

- [src/lib/asm/asm.cpp](src/lib/asm/asm.cpp)

**Functions created/changed:**

- Removed duplicate `GetGMC(int index)` (kept `GetGMC(uint8_t index)`)
- Removed `SetAddressingMode()` (redundant with `SetLenTIdx()`)
- Removed `GetAddressingMode()` (redundant with `GetLenTIdx()`)

**Tests created/changed:**

- None

**Review Status:** APPROVED

**Git Commit Message:**

refactor: normalize test helper signatures

- remove duplicate GetGMC with incompatible signature
- consolidate addressing mode setters to SetLenTIdx
- use consistent uint8_t for buffer indices
