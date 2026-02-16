## Plan: Directive Handlers and Operand Parsing

Implement operand evaluation and directive dispatch, then translate core and data directives from ASM3.S with 1:1 labels/comments, followed by listing/control directives and pass-loop integration. This continues the faithful translation path while deferring macro/conditional complexity and file I/O where appropriate. The phases are scoped to keep each step testable and incremental, aligning with the existing opcode pipeline and translation conventions.

**Phases 5**

1. **Phase 4: Operand Evaluation and Directive Dispatch**
   - **Objective:** Implement EvalOprnd and connect directive dispatch in HndlMnem.
   - **Files/Functions to Modify/Create:** src/lib/ei/asm.cpp (EvalOprnd, HndlMnem directive branch, directive tables)
   - **Tests to Write:** EvalOprnd_Immediate, EvalOprnd_EmptyOperand, DirectiveDispatch_EQU, DirectiveDispatch_ORG
   - **Steps:**
     1. Translate EvalOprnd from ASM3.S (L9013 region) with labels/comments intact.
     2. Wire directive handler dispatch based on ZAB/directive tables.
     3. Add small test helpers to drive EvalOprnd and dispatch.

2. **Phase 5: Core Directives (EQU, ORG)**
   - **Objective:** Implement symbol assignment and origin control.
   - **Files/Functions to Modify/Create:** src/lib/ei/asm.cpp (EQU L8A31, ORG L8A82)
   - **Tests to Write:** EQU_DefinesSymbol, EQU_Redef_Error, ORG_UpdatesCurAdr, ORG_RejectsInvalid
   - **Steps:**
     1. Translate EQU and ORG handlers with 1:1 labels/comments.
     2. Ensure symbol table writes and CurAdr updates align with original semantics.
     3. Add error-path tests.

3. **Phase 6: Data Directives (DS, DFB, DW, ASC, DCI)**
   - **Objective:** Generate bytes/words/strings via StorByt with correct sizes.
   - **Files/Functions to Modify/Create:** src/lib/ei/asm.cpp (L8C0E, L8CC3, L8D67, string handlers)
   - **Tests to Write:** DFB_StoresBytes, DW_StoresWordsLE, ASC_StoresASCII, DS_ReservesSpace
   - **Steps:**
     1. Translate handlers from ASM3.S with preserved labels/comments.
     2. Use StorByt and ValidateRange where applicable.
     3. Add minimal tests per directive.

4. **Phase 7: Listing and Control Directives (LST/NOLIST/PAGE/SBTL/OBJ/REL/END)**
   - **Objective:** Translate listing and assembly control directives that affect output behavior.
   - **Files/Functions to Modify/Create:** src/lib/ei/asm.cpp (directive handlers and flags)
   - **Tests to Write:** LST_TogglesListing, PAGE_ResetsLineCount, END_TerminatesPass
   - **Steps:**
     1. Translate listing/control directives from ASM3.S.
     2. Wire to existing output/listing flags.
     3. Add sanity tests.

5. **Phase 8: Pass Loop Integration (DoPass1/DoPass2)**
   - **Objective:** Integrate directive flow into pass loops without completing macro/conditional logic.
   - **Files/Functions to Modify/Create:** src/lib/ei/asm.cpp (DoPass1, DoPass2)
   - **Tests to Write:** DoPass1_ProcessesEQU, DoPass2_ProcessesORG
   - **Steps:**
     1. Thread EvalOprnd and directive dispatch into pass loops.
     2. Keep macros/conditionals stubbed but consistent.
     3. Add minimal pass-level tests.

**Open Questions 3**

1. Prioritize only core/data directives before listing/control directives, or proceed in the phase order above?
2. For OBJ/REL directives, stub file I/O as before, or implement actual output now?
3. Integrate DoPass1/DoPass2 immediately after directives, or keep them stubbed until conditionals/macros are translated?
