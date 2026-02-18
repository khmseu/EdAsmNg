## Plan: Restore Test Helper Visibility and Fix Bridge Ambiguity

Make the test helper declarations consistent and visible to all tests, then remove ambiguous Bridge wrapper definitions so the test build compiles cleanly.

**Phases 3**

1. **Phase 1: Consolidate Test Helper Declarations**
   - **Objective:** Ensure `EdAsmNg::Asm` helpers like `GetLength`, `SetLength`, `GetLenTIdx`, and `SetGMC` are declared once and visible to tests.
   - **Files/Functions to Modify/Create:** [tests/app_test.cpp](tests/app_test.cpp), optional helper header (e.g., [tests/asm_test_helpers.hpp](tests/asm_test_helpers.hpp) or [include/EdAsmNg/asm_test_helpers.hpp](include/EdAsmNg/asm_test_helpers.hpp)).
   - **Tests to Write:** None; use existing test build failures as the red step.
   - **Steps:**
     1. Run the current test build to confirm missing helper member errors.
     2. Add a centralized declaration header or restore missing forward declarations.
     3. Rebuild tests to confirm those errors are resolved.

2. **Phase 2: Fix `Bridge_*` Ambiguity**
   - **Objective:** Remove ambiguous overload resolution between global `Bridge_*` declarations and anonymous namespace definitions.
   - **Files/Functions to Modify/Create:** [src/lib/asm/asm.cpp](src/lib/asm/asm.cpp).
   - **Tests to Write:** None; reuse the existing build as verification.
   - **Steps:**
     1. Rebuild tests to confirm the current ambiguity errors.
     2. Keep a single linkage strategy (one set of `Bridge_*` definitions).
     3. Rebuild to confirm ambiguity is resolved.

3. **Phase 3: Align Helper Signatures**
   - **Objective:** Normalize helper signatures (e.g., `GetGMC` index type) and remove redundant helpers.
   - **Files/Functions to Modify/Create:** [src/lib/asm/asm.cpp](src/lib/asm/asm.cpp), [tests/app_test.cpp](tests/app_test.cpp).
   - **Tests to Write:** None; rely on existing tests.
   - **Steps:**
     1. Identify signature mismatches and redundant helpers.
     2. Normalize signatures in definitions and declarations.
     3. Rebuild and run tests to confirm stability.

**Open Questions 0**
