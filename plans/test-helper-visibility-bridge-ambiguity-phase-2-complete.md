## Phase 2 Complete: Fix Bridge Ambiguity

Resolved ambiguous Bridge function overload resolution by maintaining internal linkage only and converting unimplemented handlers to stubs.

**Files created/changed:**

- [src/lib/asm/asm.cpp](src/lib/asm/asm.cpp)

**Functions created/changed:**

- `Bridge_HndlDCI`
- `Bridge_HndlLST`
- `Bridge_HndlNOLIST`
- `Bridge_DoPage`
- `Bridge_HndlSBTL`
- `Bridge_GOpAdr`
- `Bridge_ChkRng`
- `Bridge_ValidateRange`
- `Bridge_HndlOBJ`
- `Bridge_HndlREL`
- `Bridge_HndlDS`
- `Bridge_HndlDFB`
- `Bridge_HndlDW`
- `Bridge_HndlASC`
- `GAdrMod` (stub added)

**Tests created/changed:**

- None

**Review Status:** APPROVED (build passes cleanly with 0 errors/warnings)

**Git Commit Message:**

fix: resolve bridge function ambiguity

- convert unimplemented handlers to stubs
- maintain internal linkage only for bridge functions
- add GAdrMod stub for GInstLen dependency
