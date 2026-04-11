## Plan Complete: Symbol Table Management Extraction

Successfully extracted symbol table management functions (FindSym, HashFn, AddNode) from monolithic asm.cpp into dedicated asm_symtab.cpp module.

**Phases Completed:** 4 of 4

1. ✅ Phase 1: Extend State Bridge
2. ✅ Phase 2: Extract Symbol Table Functions
3. ✅ Phase 3: Remove from asm.cpp and Update Call Sites
4. ✅ Phase 4: Build, Test and Verify

**All Files Created/Modified:**

- src/lib/asm/asm_internal.hpp (extended with 17 state variables, 4 constants, 6 helper functions)
- src/lib/asm/asm_symtab.cpp (created, ~350 lines, 3 extracted functions)
- src/lib/asm/asm.cpp (reduced by ~326 lines, added AsmInternal namespace bridge, added SrcP_at implementation)
- src/CMakeLists.txt (added asm_symtab.cpp to build sources)
- plans/symbol-table-extraction-phase-4-complete.md (phase completion document)

**Key Functions/Classes Added:**

- AsmInternal::FindSym() - Hash-based symbol lookup with linked list traversal (76 lines, 10 goto labels)
- AsmInternal::HashFn() - 3-character hash function with bit manipulation (90 lines, 7 goto labels)
- AsmInternal::AddNode() - Create and link new symbol node (118 lines, 0 goto labels)
- AsmInternal::SrcP_at() - Source pointer byte access bridge (resolves macro expansion issue)

**Test Coverage:**

- Total tests written: 0 new (all existing tests reused)
- All tests passing: ✅ 134/134
- Test execution time: 1 ms

**Code Metrics:**

- Lines extracted: ~367 (FindSym 76, HashFn 90, AddNode 118, supporting code 83)
- Lines reduced in asm.cpp: ~326 (from ~10,200 to ~9,874)
- Comment fidelity: 60+ original 6502 assembly comments preserved
- Goto labels preserved: 17 of 17 (100%)

**Critical Issues Resolved:**

1. **Type Mismatches** (Phase 1): Corrected HashIdx from uint16_t to uint8_t, ZPSaveY to uint16_t
2. **Macro Expansion Bug** (Phase 4): Fixed SrcP_at linker error by preventing `#define SrcP_at(idx) SrcP_byte(idx)` from expanding in function definition using #undef/#define guards

**Lessons Learned:**

1. **Macro Hygiene**: When extracting functions that use macros, check if those macros can interfere with function declarations/definitions. Use #undef/#define guards when creating out-of-line implementations.
2. **Inline Optimization**: Functions defined in namespace blocks may be implicitly inlined; create out-of-line implementations for cross-TU visibility.
3. **Reference Bridges**: State variable bridges work reliably when types match exactly; automated type checking would help catch mismatches earlier.
4. **Subagent Delegation**: Complex multi-step transformations (like removing large function blocks and updating call sites) are well-suited for subagent delegation.

**Recommendations for Next Steps:**

1. **Continue Modularization**: Target Pass 2 code generation functions (~1,500 lines) or directive handlers (~800 lines)
2. **Improve State Management**: Consider creating a single AsmState struct to replace individual reference variables
3. **Macro Refactoring**: Convert SrcP_at macro to constexpr function template to avoid future expansion issues
4. **Automated Validation**: Create pre-commit hooks to verify all tests pass after extractions

**Project Status:**

- Original asm.cpp size: ~10,200 lines
- After Pass 3 extraction: ~10,200 lines (Pass 3 moved to separate file)
- After symbol table extraction: ~9,874 lines (3.2% reduction)
- Cumulative extraction: ~850 lines (Pass 3: 483 + Symbol Table: 367)
- Remaining monolithic code: ~9,024 lines (~88% of original)
