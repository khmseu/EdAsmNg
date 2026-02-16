## Plan: Pass-2 object generation

TL;DR — Implement Pass‑2 to emit opcodes and data, manage object code buffer, handle relocatable expressions, and output error summary; aligned with original EDASM architecture.

**Phases 4**

1. **Phase 8.5.1: Pass‑2 loop initialization & basic opcode emission**
   - **Objective:** Set up Pass‑2 variables, read source, parse mnemonics/directives, and emit basic opcodes (NOP, LDA, STA) with fixed-width operands.
   - **Files/Functions:** `DoPass2()` loop, `HndlMnem()` for Pass‑2, opcode table lookups, `StorByt()` buffer management.
   - **Tests to Write:**
     - `test_pass2_nop_emits_opcode`
     - `test_pass2_lda_emits_opcode_with_operand`
     - `test_pass2_output_buffer_tracking`
   - **Steps:**
     1. Write failing tests
     2. Implement `DoPass2()` loop, opcode emission stubs
     3. Run tests → pass
     4. Commit

2. **Phase 8.5.2: Directive handling in Pass‑2 (ORG, DS, DFB, DW)**
   - **Objective:** ORG changes output address, DS fills space, DFB/DW emit data bytes/words.
   - **Files/Functions:** `HndlORG()` for Pass‑2, `HndlDS()` output logic, `HndlDFB()` / `HndlDW()` data emission.
   - **Tests to Write:**
     - `test_pass2_org_relocates_output_address`
     - `test_pass2_dfb_emits_bytes`
     - `test_pass2_dw_emits_words_little_endian`
   - **Steps:** tests → implement → verify output → commit

3. **Phase 8.5.3: Relocatable expressions & RLD entries**
   - **Objective:** Track relocatable references, create RLD (Relocation Dictionary) entries, mark symbols as referenced.
   - **Files/Functions:** `AddRLDEnt()`, RLD buffer, symbol reference flags, `HndlMnem()` relocatable logic.
   - **Tests to Write:**
     - `test_pass2_relocatable_ref_creates_rld_entry`
     - `test_pass2_symbol_marked_referenced`
   - **Steps:** tests → implement RLD → verify entries → commit

4. **Phase 8.5.4: Error summary & output finalization**
   - **Objective:** Print error summary (warnings/errors count), close output files, final Pass‑2 cleanup.
   - **Files/Functions:** `PrSummry()`, error printing, file closing stubs.
   - **Tests to Write:**
     - `test_pass2_error_summary_printed`
     - `test_pass2_output_file_closed`
   - **Steps:** tests → implement → verify output → commit

**Open Questions**

1. Should RLD entries be created during Pass‑2 or tracked via flags set in Pass‑1? (Pass-2 creation recommended.)
2. Max object code per file → 64KB, or unlimited until MemTop collision? (128KB recommended.)
3. Error output goes to stdout or listing file? (Listing file in full mode; stdout in basic mode.)

**Acceptance Criteria**

- All new Pass‑2 tests pass.
- No regressions in Phase‑8.4 / Pass‑1 tests.
- Simple test program assembles: labels, NOP, LDA/STA, ORG, DFB.
- Opcode and data bytes stored in output buffer with correct sequence.
