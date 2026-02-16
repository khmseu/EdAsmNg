## Plan: ASM2 Symbol Table Translation

This plan replaces the ASM2 symbol table stubs in asm.cpp with 1:1 translations of the original 6502 logic, preserving labels/comments and the existing control-flow style. It implements FindSym, AddNode, HashFn, and IsAXY so identifier resolution in EvalTerm and label parsing behaves like the original assembler. The focus is correctness vs. ASM2.S while keeping current project conventions and formatting.

**Steps**

1. Replace stubs in [src/lib/ei/asm.cpp](src/lib/ei/asm.cpp) for `FindSym`, `AddNode`, `RsvdId`, and add `HashFn` and `IsAXY` in the ASM2 helper section.
2. Preserve original labels and comments and ensure 6502 flag/carry behavior is reflected in pointer math and hashing.
3. Align new routines with existing globals/zero-page mappings and fix any missing dependencies needed by the symbol table logic.
4. Rebuild to confirm the changes compile with the rest of the translation.

**Verification**
Build the project using the existing build directory and confirm asm.cpp compiles without new errors.
