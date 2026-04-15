## Phase 3 Complete: Minimal Instruction Coverage Extensions for JSR and Abs-Indexed

Extended HndlMnem() with JSR absolute and LDA/STA abs-indexed addressing modes, added 4 unit tests (including a plain-absolute LDA guard test), and added 2 new comparative fixtures — all 186 unit tests pass and 12/12 comparative fixtures MATCH with LST MATCH.

**Files created/changed:**

- src/lib/asm/asm.cpp
- tests/app_test.cpp
- comparative-tests/inputs/jsrsubr.src
- comparative-tests/inputs/absidx.src
- plans/expand-comparative-fixture-corpus-phase-3-complete.md

**Functions created/changed:**

- HndlMnem() — added JSR abs handler; extended LDA to dispatch abs (0xAD) vs abs,X (0xBD) via `,X` detection; extended STA to dispatch abs (0x8D) vs abs,Y (0x99) via `,Y` detection

**Tests created/changed:**

- Pass2Test.test_pass2_experimental_jsr_absolute_emits_opcode_addr
- Pass2Test.test_pass2_experimental_lda_abs_x_emits_opcode_addr
- Pass2Test.test_pass2_experimental_lda_absolute_emits_opcode_addr (guard test)
- Pass2Test.test_pass2_experimental_sta_abs_y_emits_opcode_addr
- Phase84Pass1Test.Pass1_ForwardRefResolved (updated: LDA abs = 3 bytes)

**Review Status:** APPROVED after revision (added `,X` detection and plain-absolute guard test per reviewer)

**Git Commit Message:**
feat(asm): add JSR absolute and abs-indexed LDA/STA to experimental pass2

- add JSR abs handler emitting [0x20, lo, hi] (3 bytes)
- extend LDA to dispatch abs (0xAD) vs abs,X (0xBD) via ,X suffix check
- extend STA to dispatch abs (0x8D) vs abs,Y (0x99) via ,Y suffix check
- add 4 unit tests covering JSR, LDA abs,X, LDA abs, STA abs,Y
- add jsrsubr.src and absidx.src fixtures; all 12 comparative fixtures MATCH
