## Phase 1 Complete: ASM2 Symbol Table Translation

Implemented the ASM2 symbol table routines in asm.cpp with 1:1 label/comment preservation, including hashing, lookup, insertion, and reserved identifier handling. Corrected carry/borrow semantics and scan behavior to match the 6502 flow.

**Files created/changed:**

- src/lib/ei/asm.cpp

**Functions created/changed:**

- `ChrGet2`
- `FindSym`
- `HashFn`
- `RsvdId`
- `IsAXY`
- `AddNode`

**Tests created/changed:**

- None

**Review Status:** APPROVED

**Git Commit Message:**
feat: translate ASM2 symbol table logic

- implement FindSym/AddNode/HashFn/IsAXY logic
- align carry/borrow behavior with 6502 flow
- fix ChrGet2 scan behavior for symbol parsing
