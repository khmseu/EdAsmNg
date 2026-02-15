## Phase 1 Complete: Pass-2 loop initialization & basic opcode emission

Pass 2 now emits basic opcodes with consistent sizing: `LDA #imm` is treated as a 2-byte instruction (opcode + 8-bit immediate), aligning Pass1 PC tracking with Pass2 emission; Pass2 tests cover opcode bytes and object buffer advancement.

**Files created/changed:**

- src/lib/ei/asm.cpp
- tests/app_test.cpp

**Functions created/changed:**

- HndlMnem (LDA immediate handling)
- StorByt (used for emission)

**Tests created/changed:**

- Pass2Test.test_pass2_lda_operand_emits_opcode_byte
- Pass2Test.test_pass2_output_buffer_tracking
- Pass2Test.test_pass2_nop_emits_opcode

**Review Status:** APPROVED

**Git Commit Message:**
fix: align LDA immediate length

- make LDA immediate a 2-byte opcode+operand across passes
- update Pass2 tests and buffer expectations for 8-bit immediate
- keep opcode emission and PC tracking consistent
