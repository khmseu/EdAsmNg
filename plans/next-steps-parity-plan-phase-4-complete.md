## Phase 4 Complete: Full DoPass2 Activation Strategy

Expanded confidence coverage for experimental Pass2 queue paths by adding focused instruction-level tests for immediate and branch emission behavior. This checkpoint keeps runtime behavior unchanged while increasing regression protection for queued code generation.

**Files created/changed:**

- tests/app_test.cpp
- plans/next-steps-parity-plan-phase-4-complete.md

**Functions created/changed:**

- Pass2Test.test_pass2_experimental_ldx_immediate_queue_emits_opcode_and_advances_objpc
- Pass2Test.test_pass2_experimental_bcc_queue_computes_displacement_and_advances_objpc

**Tests created/changed:**

- Pass2Test.test_pass2_experimental_ldx_immediate_queue_emits_opcode_and_advances_objpc
- Pass2Test.test_pass2_experimental_bcc_queue_computes_displacement_and_advances_objpc

**Review Status:** APPROVED

**Git Commit Message:**
test: add experimental ldx and bcc coverage

- add experimental LDX immediate queue path regression test
- add experimental BCC displacement queue path regression test
- keep behavior unchanged while increasing pass2 safety coverage
