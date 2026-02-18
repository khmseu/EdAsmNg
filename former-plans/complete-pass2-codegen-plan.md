## Plan: Complete Pass 2 Code Generation (Phase 8.5-8.7)

Implement Pass 2 object code generation and object code management by un-stubbing `GInstLen()`, `StorGMC()`, and fully enabling the Pass 2 loop with table-driven mnemonic/opcode dispatch. This completes the core assembler two-pass cycle, enabling the assembler to generate actual machine code output.

**Phases (5 phases)**

1. **Phase 1: Clean up code organization and resolve conflicts**
   - **Objective:** Unify duplicate function definitions, establish forward declarations, and prepare the codebase for enabling disabled Pass 2 logic
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp
   - **Tests to Write:** None (preparation phase)
   - **Steps:**
     1. Identify all duplicate function definitions in disabled blocks (e.g., multiple `HndlMnem()`, `GAdrMod()`, `AddRLDEnt()` versions)
     2. Consolidate scanner helpers (`ChrGot`, `ChrGet`, `WhiteSpc`, `SkipSpcs`) - keep only compiled versions, remove duplicates from disabled blocks
     3. Establish explicit `extern` declarations for tables used by `GInstLen()` (e.g., `AModTbl`, `InstLenT`)
     4. Verify ordering: ensure all dependencies are declared before use (especially `OpcodeT`, `AModCmds`, `CycTimes`)
     5. Create logical section boundaries in the file to group related functionality

2. **Phase 2: Un-stub and enable GInstLen() - Instruction length calculation**
   - **Objective:** Implement instruction length calculation by looking up addressing mode from operand syntax
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp
   - **Tests to Write:** TEST_GInstLen_Immediate, TEST_GInstLen_Absolute, TEST_GInstLen_Indirect, TEST_GInstLen_Addressing_Modes, TEST_GInstLen_AllOpcodes
   - **Steps:**
     1. Enable the disabled `GInstLen()` implementation (currently at ~line 2737)
     2. Resolve forward declaration issues by ensuring `AModTbl`, `OpcodeT`, and `InstLenT` are accessible
     3. Test `GInstLen()` with various addressing modes: immediate, absolute, indexed, indirect, etc.
     4. Verify it correctly sets `Length` field based on `AModTbl` (e.g., immediate = 2 bytes, absolute = 3 bytes)
     5. Validate against original EDASM behavior

3. **Phase 3: Un-stub and enable StorGMC() and object memory management**
   - **Objective:** Implement machine code storage to object memory/buffer
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp
   - **Tests to Write:** TEST_StorGMC_Memory, TEST_StorGMC_MultiByte, TEST_StorGMC_ObjPC_Advance, TEST_StorGMC_Memory_Bounds
   - **Steps:**
     1. Enable the disabled `StorGMC()` implementation (currently at ~line 3361)
     2. Implement memory-mode object code emission (V bit = 0): copy bytes from `GMC[]` to object memory at `ObjPC`
     3. Stub disk-mode for now (V bit = 1): skip without error (Phase 9+)
     4. Call `AdvObjPC()` to advance after each batch
     5. Test storage of various instruction lengths and multiple instructions
     6. Validate object memory integrity and PC advancement

4. **Phase 4: Implement table-driven opcode dispatch in HndlMnem()**
   - **Objective:** Replace hard-coded mnemonic handling with table-driven lookup using addressing mode tables
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp, extend existing `HndlMnem()`
   - **Tests to Write:** TEST_HndlMnem_TableDriven_LDA, TEST_HndlMnem_TableDriven_STA, TEST_HndlMnem_All_Addressing_Modes, TEST_Opcode_Lookup_Completeness
   - **Steps:**
     1. Augment the minimal `HndlMnem()` with table-driven lookup logic from the disabled block (line ~4830)
     2. Search `Tbl1stLet[]` by first character of mnemonic to find mnemonic table entry
     3. Compare DCI-encoded mnemonic strings to find the right entry
     4. Extract addressing mode requirements and valid combinations from `AModCmds` table
     5. Call `GAdrMod()` to parse and validate addressing mode from operand
     6. Look up opcode byte in `OpcodeT` indexed by `SubTIdx` and addressing mode
     7. Fill `GMC[]` with opcode and operand bytes
     8. For Pass 2, call `StorGMC()` to emit the machine code
     9. Test comprehensive instruction set coverage

5. **Phase 5: Enable full DoPass2() loop with code generation**
   - **Objective:** Un-stub Pass 2 main loop to perform complete two-pass assembly
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp
   - **Tests to Write:** TEST_DoPass2_Simple, TEST_DoPass2_With_Labels, TEST_DoPass2_With_Directives, TEST_Two_Pass_Complete, TEST_Generated_Code_Correctness
   - **Steps:**
     1. Enable the full `DoPass2()` implementation (currently at ~line 3067, wrapped in `#if 0`)
     2. Keep skeleton Pass 2 loop: open/rewind source, read lines, skip label field, call `HndlMnem()` with `PassNbr=1`
     3. For each mnemonic (non-directive): call `GInstLen()` to determine byte count, fill `GMC[]` with opcode/operand bytes, call `StorGMC()` to emit
     4. For directives: handle storage directives (ORG, DS, DFB, DW) - they should already work from Phase 1-4
     5. Call `PrtAsmLn()` if listing enabled (stub is fine for now)
     6. Call `AdvPC()` to advance program counter for next instruction
     7. Continue until EOF
     8. Write comprehensive tests that assemble small programs and verify generated object code byte-for-byte

**Open Questions**

1. Should we implement disk-mode object file output now (Wr1Byte/Flush) or keep it stubbed? **Option A: Keep stubbed for Phase 9+** / Option B: Implement now
2. Should we implement conditional assembly macros in Pass 2 or keep them disabled? **Option A: Keep disabled for now** / Option B: Implement basic support
3. How strict should EDASM compatibility be for GenF register encodings—preserve exact values or normalize to booleans? **Option A: Normalize to booleans** / Option B: Preserve original encoding
4. Should Phase 5 include Pass 3 (symbol listing) implementation or defer that? **Option A: Defer to Phase 10** / Option B: Implement basic version now
