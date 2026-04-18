## Plan: Fix EI.S Assembly Differences

Unblock full EI traversal first, then reduce remaining opcode/directive mismatches in small validated increments so listing and object parity improves without destabilizing the assembler core.

**Phases 4**

1. **Phase 1: Unblock Pass-2 Traversal**
   - **Objective:** Remove false pass-2 abort conditions and normalize whitespace field parsing used by EI sources.
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp (`DoPass2_ExperimentalCore`, `NxtField`)
   - **Tests to Write:** `ei_pass2_does_not_abort_on_advpc_carry`, `nxtfield_skips_tabs`
   - **Steps:**
     1. Add regression tests that reproduce the carry-leak abort and tab field parsing mismatch.
     2. Run tests to observe expected failures.
     3. Implement minimal code changes in `DoPass2_ExperimentalCore` and `NxtField`.
     4. Re-run tests and assemble `EI.S` to verify traversal proceeds beyond the previous stop point.

2. **Phase 2: Implement Next Missing Opcode/Directive Cluster**
   - **Objective:** Fix the first exposed post-unblock mismatch cluster in EI listing/object output.
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp (`HndlMnem` active path), optionally src/lib/asm/asm_expr.cpp
   - **Tests to Write:** Targeted tests for first failing mnemonic/addressing forms found after Phase 1.
   - **Steps:**
     1. Identify the first mismatch frontier from EI listing after Phase 1.
     2. Write focused failing tests for those mnemonics/addressing modes.
     3. Implement minimal support in active path.
     4. Re-run targeted tests and EI assembly.

3. **Phase 3: Stabilize Symbol/Error Reporting for EI Scale**
   - **Objective:** Reduce misleading duplicate-identifier cascades and improve diagnostic fidelity.
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp, src/lib/asm/asm_symtab.cpp
   - **Tests to Write:** Symbol-table behavior tests for forward-defined labels and duplicate handling.
   - **Steps:**
     1. Add failing tests for duplicate vs undefined-forward-reference cases.
     2. Implement minimal fixes in pass/symbol interactions.
     3. Validate error output remains stable on smaller fixtures.
     4. Re-run EI and collect delta.

4. **Phase 4: Parity Validation and Delta Report**
   - **Objective:** Quantify EI improvements and document remaining differences.
   - **Files/Functions to Modify/Create:** plans artifacts only (if needed)
   - **Tests to Write:** none
   - **Steps:**
     1. Run EI assembly and comparative checks.
     2. Summarize listing/object parity improvements and residual blockers.
     3. Prepare next iteration targets from first remaining mismatch.
