## Plan: Listing and Control Directives (Phase 7)

Implement listing and control directives (LST, NOLIST, PAGE, SBTL, OBJ, REL) to control assembler output modes and behavior. These directives manage listing generation, page formatting, object code generation mode (absolute vs relocatable), and pass-specific actions.

**Phases (6 phases)**

1. **Phase 7.1: Zero-Page Variables and Setup**
   - **Objective:** Define zero-page variables for listing/control flags and buffers
   - **Files/Functions to Modify/Create:** src/lib/ei/asm.cpp
   - **Tests to Write:** None (infrastructure setup)
   - **Steps:**
     1. Add zero-page variables: `ListingF` ($68), `SubTtlF` ($69), `GenF` ($BF), `RelCodeF` ($BD), `OnOffSW` ($CE)
     2. Add listing option flags: `LstFlags` base ($E0) for 8 option bytes (`LstCyc`, `LstUncond`, `LstExpnd`, `LstWarn`, `LstGlobl`, `LstASym`, `LstVSym`, `Lst6Cols`)
     3. Add page accounting variables: `LineCnt`, `PageNbr`, `LogPL`, `PhyPL` (ranges $82-$90)
     4. Add buffers: `SubTitle` buffer (35 chars + null terminator)
     5. Add object/relocatable mode variables: `MemTop` ($87), `RLDEnd` ($D0)
     6. Comment each variable with its purpose and original label

2. **Phase 7.2: LST Directive Handler**
   - **Objective:** Implement LST directive to enable/toggle listing and set listing options
   - **Files/Functions to Modify/Create:** src/lib/ei/asm.cpp
   - **Tests to Write:** TEST_AsmLST_ON, TEST_AsmLST_OFF, TEST_AsmLST_Options, TEST_AsmLST_InvalidOption
   - **Steps:**
     1. Implement `HndlLST` function (label L8ECA from ASM3.S:778-843)
     2. Parse `LST ON` / `LST OFF` by checking first two operand chars ('O','N' / 'O','F')
     3. Toggle `ListingF` MSB for ON/OFF: `SEC; ROR ListingF` for ON, `CLC; ROR ListingF` for OFF
     4. Parse comma-separated option letters using `LstOptns = "CUEWGAVS"` (first letter only)
     5. Map option letters to flag bytes: C→LstCyc, U→LstUncond, E→LstExpnd, W→LstWarn, G→LstGlobl, A→LstASym, V→LstVSym, S→Lst6Cols
     6. Toggle each option flag: rotate MSB with carry (SEC for +, CLC for -)
     7. Error on non-alphabetic or invalid option (JMP L9193 directive operand error)
     8. Write tests verifying flag states after LST ON, LST OFF, and various option combinations

3. **Phase 7.3: NOLIST and PAGE Directives**
   - **Objective:** Implement NOLIST (disable listing) and PAGE (page break) directives
   - **Files/Functions to Modify/Create:** src/lib/ei/asm.cpp
   - **Tests to Write:** TEST_AsmNOLIST, TEST_AsmPAGE_Pass1, TEST_AsmPAGE_Pass2
   - **Steps:**
     1. Implement `HndlNOLIST` function (label L8F3A from ASM3.S:846-856)
     2. Clear `ListingF` MSB: `CLC; ROR ListingF`
     3. No operand parsing; just toggle flag and return
     4. Implement `DoPage` function (label DoPage from ASM3.S:860-871)
     5. Check `PassNbr`: if Pass 1, return immediately (BEQ L8F5E)
     6. If Pass 2+, check if listing is enabled via `RVLsting` stub (to be implemented later)
     7. If listing enabled, output form-feed ($FF) and jump to `L9008` (NextRec advance)
     8. For now, stub output and flow control (since listing I/O is not yet implemented)
     9. Add both NOLIST and PAGE to mnemonic dispatch table
     10. Write tests verifying flag states and pass-specific behavior

4. **Phase 7.4: SBTL (Subtitle) Directive**
   - **Objective:** Implement SBTL directive to set subtitle string for page headers
   - **Files/Functions to Modify/Create:** src/lib/ei/asm.cpp
   - **Tests to Write:** TEST_AsmSBTL_EmptyString, TEST_AsmSBTL_ValidString, TEST_AsmSBTL_MaxLength, TEST_AsmSBTL_InvalidDelimiter
   - **Steps:**
     1. Implement `HndlSBTL` function (label L8F61 from ASM3.S:874-914)
     2. Set `SubTtlF=$40` initially (marks SBTL encountered)
     3. Parse optional subtitle string in Pass 2+:
        - First non-space char becomes delimiter
        - Copy chars until delimiter repeats or CR
        - Max 35 chars; store in `SubTitle` buffer with null terminator
        - Error if non-space/CR follows closing delimiter
     4. If string is stored, set `SubTtlF=$FF`
     5. Always jump to `DoPage` at end (SBTL forces page break)
     6. Write tests for empty string, valid string, max length, and invalid delimiter

5. **Phase 7.5: OBJ (Absolute Object Mode) Directive**
   - **Objective:** Implement OBJ directive to control absolute object code generation
   - **Files/Functions to Modify/Create:** src/lib/ei/asm.cpp
   - **Tests to Write:** TEST_AsmOBJ_SuppressGeneration, TEST_AsmOBJ_SetAddress, TEST_AsmOBJ_ErrorRELModeActive, TEST_AsmOBJ_ErrorAddressBelowSymbolTable
   - **Steps:**
     1. Implement `HndlOBJ` function (label L8BAD from ASM3.S:287-340)
     2. Parse required expression operand; error if parsing fails or non-space tokens follow
     3. If `GenF` V-bit set (disk output mode), return immediately (OBJ ignored for disk output)
     4. If operand is $0000, set `GenF=$80` (N=1, suppress generation) and return
     5. Error if `RelCodeF` MSB set (REL mode already active) — "Can't use OBJ and REL directives together"
     6. Error if operand address < `EndSymT` (below end of symbol table)
     7. Set `ObjPC`, `MemTop`, and `RLDEnd` to operand value
     8. Clear `GenF` (N=0, V=0) to enable memory-based object generation
     9. Write tests for suppression, address setting, and error cases

6. **Phase 7.6: REL (Relocatable Mode) Directive**
   - **Objective:** Implement REL directive to enable relocatable code generation mode
   - **Files/Functions to Modify/Create:** src/lib/ei/asm.cpp
   - **Tests to Write:** TEST_AsmREL_EnableMode, TEST_AsmREL_SetFtype
   - **Steps:**
     1. Implement `HndlREL` function (label L9126 from ASM3.S:1168-1179)
     2. No operand parsing required
     3. Set `RelCodeF` MSB: `SEC; ROR RelCodeF`
     4. Set `ftypeT` to `RELtype` (file type storage for REL output)
     5. Note: REL mode affects ORG behavior and output flush (to be fully integrated in Phase 8)
     6. Write tests verifying `RelCodeF` and `ftypeT` are set correctly

**Open Questions**

1. Should we fully implement listing output (form-feed, subtitle printing) or stub it for now? **Option A: Stub listing I/O** / Option B: Full implementation
2. Should we implement `RVLsting` helper (checks listing enabled/pass) now or defer? **Option A: Stub for now** / Option B: Implement fully
3. Should we error on OBJ/REL in Pass 1, or silently ignore? **Option A: Process in all passes** / Option B: Pass 1 only
4. END directive: Not found in original tables. Should we add explicit END or rely on EOF? **Option A: EOF-driven only** / Option B: Add END directive
5. Should we integrate object/REL mode with existing ORG/storage in this phase? **Option A: Stub mode flags only** / Option B: Full integration
