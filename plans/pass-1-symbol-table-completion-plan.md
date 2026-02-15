## Plan: Pass-1 symbol-table completion

TL;DR — Finish Pass‑1 symbol handling: validate label identifiers (reserved/invalid), detect duplicate/forward definitions, and ensure directives that affect symbol values (EQU/ORG/DS/DFB/DW) update the table and PC correctly.

**Phases 5**

1. **Phase 8.4.1: Reserved IDs & basic validation**
   - **Objective:** Reject reserved single‑letter identifiers (A, X, Y) and invalid first‑character labels; add unit tests and register appropriate errors.
   - **Files/Functions to Modify/Create:** `src/lib/ei/asm.cpp` — `DoPass1()` (label checks), add `RsvdId()`‑style logic; `tests/app_test.cpp` — new Pass‑1 validation tests.
   - **Tests to Write:**
     - `test_pass1_reserved_label_A_error`
     - `test_pass1_invalid_label_first_char_error`
   - **Steps:**
     1. Write tests (fail)
     2. Implement checks in `DoPass1()` (use existing `RegAsmEW()` for errors)
     3. Run tests → fix → commit

2. **Phase 8.4.2: Duplicate & forward/backward definitions**
   - **Objective:** Correctly detect duplicate labels and update forward-referenced symbols when later defined.
   - **Files/Functions:** `FindSym()`, `AddNode()`, flag-byte handling in symbol records; `DoPass1()` label handling.
   - **Tests to Write:**
     - `test_pass1_duplicate_label_error`
     - `test_pass1_forward_ref_resolved_on_definition`
   - **Steps:** tests → implement → verify symbol flags and stored PC → commit

3. **Phase 8.4.3: Directive interactions affecting symbols/PC**
   - **Objective:** Ensure `EQU`, `ORG`, `DS`, `DFB`, `DW` affect symbol values/PC during Pass‑1.
   - **Files/Functions:** directive handlers in `HndlMnem()`/`HndlEQU()`/`HndlORG()`, `DoPass1()` integration.
   - **Tests to Write:**
     - `test_pass1_equ_defines_symbol_value`
     - `test_pass1_ds_advances_pc`
     - `test_pass1_dfb_dw_affect_pc_and_symbols`
   - **Steps:** tests → implement → validate PC/symbols → commit

4. **Phase 8.4.4: Edge cases & error reporting**
   - **Objective:** Support colon‑terminated labels, label==mnemonic conflicts, behavior inside DSECT/Dummy sections; improve error tokens and messages.
   - **Files/Functions:** `DoPass1()`, `RegAsmEW()`, `DummyF` handling.
   - **Tests to Write:**
     - `test_pass1_label_with_colon`
     - `test_pass1_label_conflicts_with_mnemonic`
     - `test_pass1_label_in_dummy_section_behavior`
   - **Steps:** tests → implement → verify error codes and table entries → commit

5. **Phase 8.4.5: Cleanup, docs & commit**
   - **Objective:** Tidy code, add comments, ensure all Pass‑1 tests pass and no regressions.
   - **Files:** `src/lib/ei/asm.cpp`, `tests/app_test.cpp`, `plans/*`
   - **Steps:** refactor, run full test suite, commit and update plan status

**Open Questions**

1. Should single‑character labels that are not exactly A/X/Y (e.g., "B") be allowed? (Yes — allow.)
2. Max label length — follow EDASM original (preserve up to 15 chars). (recommended)

**Acceptance criteria**

- All new Pass‑1 tests (validation/duplicates/directive interactions) pass.
- No regressions in existing tests.
- Symbol table flags and stored addresses match Pass‑1 expectations.
