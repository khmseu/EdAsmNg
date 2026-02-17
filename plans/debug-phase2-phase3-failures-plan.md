## Plan: Debug and Fix Phase 2/3 Test Failures (9 Failing Tests)

Fix the 9 failing tests in GInstLen (Phase 2) and StorGMC (Phase 3) by debugging root causes and correcting the instruction length calculation and object code storage logic. This restores complete Pass 2 functionality with full test coverage.

**Phases (3 phases)**

1. **Phase 1: Debug GInstLen() - Instruction Length Calculation (6 failing tests)**
   - **Objective:** Fix instruction length calculation and addressing mode index lookup in GInstLen() to correctly determine bytes needed for each addressing mode
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp (GInstLen function and supporting logic)
   - **Tests to Fix:**
     - Immediate_TwoBytes (expects Length=2, LenTIdx=2)
     - ZeroPage_TwoBytes (expects Length=2, LenTIdx=1)
     - Indexed_ThreeBytes (expects Length=3, LenTIdx=4)
     - Indirect_TwoBytes (expects Length=2, LenTIdx=6)
     - Implied_OneByte (expects Length=1, LenTIdx=0)
     - Branch_TwoBytes (expects Length=2, LenTIdx=0 for relative)
   - **Steps:**
     1. Analyze the failing test outputs: currently getting Length=3 for all addressing modes (not 1, 2, or 3 as expected)
     2. Trace GInstLen() logic to find where it sets Length - check the addressing mode table lookup
     3. Verify that AModTbl correctly maps addressing modes to byte counts (imm=2, zp=2, abs=3, etc.)
     4. Check if GAdrMod() correctly identifies the addressing mode from operand syntax
     5. Check if LenTIdx is being set from AModTbl or if it's hardcoded/uninitialized
     6. Identify whether the issue is in addressing mode detection or length table lookup
     7. Fix the logic step by step, running tests after each fix
     8. Verify all 6 GInstLen tests pass

2. **Phase 2: Debug StorGMC() - Object Code Storage (3 failing tests)**
   - **Objective:** Fix object code emission to properly store GMC bytes to object memory and advance ObjPC
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp (StorGMC, AdvObjPC, L828A, and related functions)
   - **Tests to Fix:**
     - SingleByteStorage (expects ObjPC to advance 0x2000→0x2001, memory[0x2000]=0xEA)
     - MultiByteStorage (expects ObjPC to advance 0x2000→0x2003, memory[0x2000:2]=0xAD, memory[0x2001:2]=0x34, memory[0x2002:2]=0x12)
     - ObjPC_Advances (expects ObjPC to advance correctly for 1-byte and 3-byte instructions)
   - **Steps:**
     1. Analyze failing test outputs: ObjPC advancing but memory not being written, or values being wrong
     2. Verify InitObjMemory() is initializing the test memory buffer correctly
     3. Check StorGMC() logic: verify it reads Length, copies GMC[0..Length-1] to object memory at ObjPC
     4. Verify AdvObjPC() is incrementing ObjPC correctly
     5. Check if there's a write mode issue (GenF flag or disk vs. memory mode)
     6. Verify the test memory buffer is actually being written to (check g_test_obj_memory implementation)
     7. Fix each issue, running tests after each fix
     8. Verify all 3 StorGMC tests pass

3. **Phase 3: Integration & Full Pass 2 Validation**
   - **Objective:** Verify all Phase 2/3 logic works end-to-end with comprehensive multi-instruction tests
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp (ensure coordinated function behavior)
   - **Tests to Write/Verify:**
     - Enhance existing Pass2Test suite: add multi-instruction sequences
     - Create comprehensive integration tests: 5+ instruction programs, verify all generated opcodes
     - Edge case tests: zero-length instructions, boundary conditions, mode switches
   - **Steps:**
     1. Run full test suite (all 109 tests) to verify no regressions
     2. Write integration tests combining multiple addressing modes in sequence
     3. Verify object code output matches expected 6502 machine code byte-for-byte
     4. Document any remaining issues or design considerations
     5. Confirm clean build and all tests passing

**Open Questions**

1. Is GInstLen() correctly detecting addressing mode from operand syntax, or is there confusion in the operand parser?
2. Is the AModTbl indexed correctly by addressing mode, or is there an offset/indexing bug?
3. Is StorGMC() writing to the correct memory addresses, or is there a pointer/offset issue?
4. Are Length and LenTIdx both being set correctly in GInstLen(), or is one being left uninitialized?
5. Should we debug one test at a time with detailed output, or fix all issues in parallel based on code review?
