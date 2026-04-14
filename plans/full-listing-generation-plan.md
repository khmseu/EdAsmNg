## Plan: Full Listing Generation Pipeline

Build a single assembler-driven listing pipeline that replaces placeholder output, respects LST/LIST/NOLIST/PAGE semantics, and supports reliable parity checks using normalized listing comparisons first, then stricter matching as fidelity increases.

**Phases 5**

1. **Phase 1: Unify Listing Output Path**
   - **Objective:** Remove split behavior by routing --listing output through assembler listing machinery instead of placeholder text.
   - **Files/Functions to Modify/Create:** src/main.cpp, src/lib/asm/asm.cpp, include/EdAsmNg/asm.hpp only if API exposure is required.
   - **Tests to Write:** CLI-level tests asserting listing file creation and non-placeholder content markers.
   - **Steps:**
     1. Write failing tests that detect placeholder listing output text and require assembler-produced listing content.
     2. Introduce minimal plumbing for listing sink initialization/finalization from CLI.
     3. Re-run tests and ensure existing object-output behavior is unchanged.

2. **Phase 2: Implement Core Listing Primitives**
   - **Objective:** Make low-level listing emit functions operational and deterministic.
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp functions around PutC, PutCR, PrtFF, PrByte, and listing buffer/file state handling.
   - **Tests to Write:** unit tests for character/line/form-feed emission and buffered flush behavior.
   - **Steps:**
     1. Add failing tests for line termination, byte-print formatting, and form-feed behavior.
     2. Implement output primitives with deterministic line endings and explicit flush rules.
     3. Re-run targeted tests and full suite to verify no regressions.

3. **Phase 3: Activate Pass-2 Line Listing Formatting**
   - **Objective:** Replace stubs for line rendering so pass-2 code/source lines are emitted in EDASM-like structure.
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp around PrtAsmLn, ListCode, LstSrcLn, and pass-loop call sites.
   - **Tests to Write:** fixture-based tests for address/object-byte/source-line columns on representative lines (NOP, LDA, STA, branch/jump).
   - **Steps:**
     1. Add failing tests for pass-2 listing lines using existing parity fixtures.
     2. Implement minimal formatting for address, bytes, and source text alignment.
     3. Verify listing toggles (LST, NOLIST) still gate output correctly.

4. **Phase 4: Wire Directive-Controlled Listing Behavior**
   - **Objective:** Ensure active mnemonic path invokes real handlers for LIST/LST/NOLIST/PAGE and page controls affect output.
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp, src/lib/asm/asm_directives.cpp.
   - **Tests to Write:** integration tests for listing on/off transitions, PAGE form-feed behavior, and pass-aware directive effects.
   - **Steps:**
     1. Add failing tests for runtime directive effects during assembly (not only direct handler invocation).
     2. Connect active dispatcher path to directive implementations and PAGE behavior.
     3. Re-run directive suites plus pass2/pass3 suites to confirm compatibility.

5. **Phase 5: Add Comparative Listing Parity Harness**
   - **Objective:** Extend comparative tests to validate listing outputs with normalization for volatile fields.
   - **Files/Functions to Modify/Create:** comparative-tests/compare.py, normalization helper in comparative-tests, listing fixtures under comparative-tests/edasm-outputs and comparative-tests/edasmng-outputs as needed.
   - **Tests to Write:** normalized listing comparison tests for input.src, input2.src, input3.src, branch.src, fwdjmp.src, equexpr.src.
   - **Steps:**
     1. Add failing harness mode for listing comparison with normalization (timestamps/free-space/path noise).
     2. Implement normalization and semantic diff output (line/column context).
     3. Gate completion on green OBJ parity, green normalized LST parity, and full regression pass.

**Open Questions**

1. Should listing parity in CI start as non-blocking report-only for one phase before becoming required?
2. Should strict spacing parity be required immediately or only after semantic-column parity stabilizes?
3. Should timestamp and free-space footer metadata be reproduced or always normalized during parity checks?
