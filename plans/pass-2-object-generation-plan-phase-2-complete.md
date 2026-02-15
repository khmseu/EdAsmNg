## Phase 2 Complete: Directive handling Pass-2

Pass 2 now supports ORG/DS/DFB/DW in absolute mode: ORG updates PC/ObjPC with bounds checks; DS fills zeros; DFB emits byte lists with overflow error 0x28; DW emits little-endian words. PC and ObjPC stay in sync for all directives, including large sizes.

**Files created/changed:**

- src/lib/ei/asm.cpp
- tests/app_test.cpp

**Functions created/changed:**

- HndlMnem (Pass-2 handling for ORG/DS/DFB/DW)

**Tests created/changed:**

- Pass2Test.test_pass2_org_relocates_objpc
- Pass2Test.test_pass2_ds_emits_zeros
- Pass2Test.test_pass2_dfb_emits_bytes
- Pass2Test.test_pass2_dw_emits_little_endian
- Pass2Test.test*pass2_pc_objpc_sync*\*
- Pass2Test.test_pass2_org_sets_both_pc_objpc
- Pass2Test.test_pass2_dfb_overflow_error
- Pass2Test.test_pass2_org_bounds_check
- Pass2Test.test_pass2_ds_large_size
- Pass2Test.test_pass2_dfb_many_bytes
- Pass2Test.test_pass2_dw_many_words
- Pass2Test.test_pass2_combined_pc_objpc_tracking

**Review Status:** APPROVED

**Git Commit Message:**
fix: add pass2 directive handling

- implement ORG/DS/DFB/DW in pass2 with PC/ObjPC sync
- enforce ORG bounds and DFB overflow errors; emit DS zeros and DW little-endian
- expand pass2 tests for directives, large sizes, and PC/ObjPC consistency
