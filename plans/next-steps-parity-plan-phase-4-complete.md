## Phase 4 Complete: Full DoPass2 Activation Strategy

Activated the real DoPass2 core as the default execution path and retained a legacy opt-out toggle for safe rollback. The final checkpoint validated full regression and parity thresholds after default-path activation.

**Files created/changed:**

- src/lib/asm/asm.cpp
- tests/app_test.cpp
- plans/next-steps-parity-plan-phase-4-complete.md

**Functions created/changed:**

- DoPass2
- ResetAsmState
- Pass2Test.test_pass2_default_uses_experimental_path_toggle_enabled
- Pass2Test.test_pass2_can_opt_out_to_legacy_path_with_toggle
- Pass2Test.test_pass2_experimental_brk_emits_opcode_and_advances_objpc

**Tests created/changed:**

- Pass2Test.test_pass2_default_uses_experimental_path_toggle_enabled
- Pass2Test.test_pass2_can_opt_out_to_legacy_path_with_toggle
- Pass2Test.test_pass2_experimental_ldx_immediate_queue_emits_opcode_and_advances_objpc
- Pass2Test.test_pass2_experimental_bcc_queue_computes_displacement_and_advances_objpc
- Pass2Test.test_pass2_experimental_bcc_queue_supports_negative_displacement
- Pass2Test.test_pass2_experimental_bcs_queue_computes_displacement_and_advances_objpc
- Pass2Test.test_pass2_experimental_bcs_queue_supports_negative_displacement
- Pass2Test.test_pass2_experimental_brk_emits_opcode_and_advances_objpc

**Review Status:** APPROVED

**Git Commit Message:**
feat: activate DoPass2 default path

- enable experimental DoPass2 core by default
- preserve legacy path via explicit opt-out toggle
- add pass2 activation and queue-path regression coverage
