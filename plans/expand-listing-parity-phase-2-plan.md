## Plan: Expand Comparative Fixture Corpus

Preserve the core project goal: EdAsmNg should accept the same sources as original EDASM and produce matching binary, object, and listing output aside from OS-specific differences such as charset, line endings, and file attributes. With 7 of 7 current fixtures now green, this plan expands the comparative corpus in small phases that either document parity-safe fixture authoring or add fixtures for behavior EdAsmNg already mirrors closely.

**Phases: 4 phases**

1. **Phase 1: Document Parity-Safe Fixture Authoring**
   - **Objective:** Create a fixture guide that documents how to add sources that both EDASM and EdAsmNg can assemble with matching semantic output, including naming rules, syntax constraints, and validation steps.
   - **Files/Functions to Modify/Create:**
     - comparative-tests/FIXTURE_TEMPLATE.md
   - **Tests to Write:**
     - No code tests; verification is review of guidance against compare.py discovery rules and current fixture behavior.
   - **Steps:**
     1. Document fixture naming and extension rules used by compare.py (`.src`/`.asm`, ProDOS-safe stems)
     2. Document source-authoring constraints needed for EDASM parity, including comment syntax and directives already covered in the current corpus
     3. Document the verification workflow using compare.py object and listing comparison modes
     4. Review the guide against current green fixtures to ensure it matches real repo behavior

2. **Phase 2: Add Directive-Only Fixtures That Already Mirror EDASM**
   - **Objective:** Add new comparative fixtures for directives already implemented and covered by unit tests, increasing corpus breadth without first expanding instruction dispatch.
   - **Files/Functions to Modify/Create:**
     - comparative-tests/inputs/dwdir.src
     - comparative-tests/inputs/dsdir.src
     - comparative-tests/inputs/dcidir.src
   - **Tests to Write:**
     - Comparative checks showing `MATCH` and `LST MATCH` for each new fixture
   - **Steps:**
     1. Add a `DW` fixture that exercises little-endian word emission
     2. Add a `DS` fixture that exercises zero-filled storage generation
     3. Add a `DCI` fixture that exercises EDASM-compatible string encoding
     4. Run compare.py for each fixture and fix any parity gaps exposed by real EDASM output

3. **Phase 3: Add Minimal Instruction Coverage Extensions Needed for New Fixtures**
   - **Objective:** Extend the experimental pass-2 mnemonic handling only where needed to support new parity-driven fixtures, starting with opcodes EDASM accepts and that fit the current structure closely.
   - **Files/Functions to Modify/Create:**
     - src/lib/asm/asm.cpp
     - tests/app_test.cpp
     - comparative-tests/inputs/jsrsubr.src
     - comparative-tests/inputs/absidx.src
   - **Tests to Write:**
     - Unit test for `JSR` absolute emission
     - Unit test for absolute-indexed `LDA` emission
     - Unit test for absolute-indexed `STA` emission
     - Comparative checks for the new fixtures
   - **Steps:**
     1. Add failing unit tests for `JSR`, `LDA abs,X`, and `STA abs,Y`
     2. Implement the minimal `HndlMnem()` support needed for those modes while preserving EDASM-style behavior
     3. Add fixture sources that exercise those opcodes in parity-safe ways
     4. Run unit and comparative tests to confirm object and listing parity

4. **Phase 4: Update Coverage Summary**
   - **Objective:** Refresh the parity findings to reflect the expanded corpus and note what remains intentionally deferred.
   - **Files/Functions to Modify/Create:**
     - LISTING_PARITY_FINDINGS.md
   - **Tests to Write:**
     - No new code tests; verification comes from the completed compare.py runs from earlier phases
   - **Steps:**
     1. Update fixture counts and covered directives/addressing modes
     2. Document remaining deferred parity work such as zero-page and indirect indexed instruction support

**Open Questions:**

1. None blocking for implementation; use short, parity-safe fixtures and prefer local labels over ROM-specific addresses.
