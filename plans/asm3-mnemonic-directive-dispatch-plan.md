# Plan: Mnemonic/Directive Dispatch & Core Directives

Implement the core instruction/directive recognition and handling pipeline (HndlMnem, RegAsmEW, EvalOprnd) along with foundational directives (EQU, ORG, SetPC). This unblocks Pass 1/2 to process real assembly statements and is the next logical step after addressing-mode helpers.

**Why This Phase:**

- `HndlMnem()` is the "front door" — without it, Pass 1/2 can't recognize any mnemonics or directives
- `EvalExpr()` is already mostly translated; this phase wires it into directive handlers
- Tables (`OpcodeT`, `DotDrtv`, letter tables) are already in `asm.cpp`; this phase ports control logic
- Error path (`RegAsmEW`, `SaveErrInfo`) unblocks proper error reporting for all downstream features

## Phases (4 total)

### 1. Phase 1: Error Registration & Warning System (RegAsmEW, SaveErrInfo, minimal error reporting)

**Objective:** Implement central error/warning registration so all downstream code can report issues.

**Files/Functions to Modify/Create:**

- `src/lib/ei/asm.cpp`: `RegAsmEW()`, `SaveErrInfo()`, error buffer management

**Tests to Write:**

- `tests/app_test.cpp`: Test error registration, overflow behavior, error deduplication

**Steps:**

1. Write tests for `RegAsmEW()` error/warning capture (match ASM2 line ~354 semantics)
2. Implement `RegAsmEW()` with error accumulation
3. Implement `SaveErrInfo()` to store first N error locations
4. Run tests to verify error collection works
5. Verify no other code breaks

### 2. Phase 2: Mnemonic & Directive Dispatch (HndlMnem)

**Objective:** Map input text (instruction/directive) to opcode/directive type; populate dispatch state (MnemP, ZAB, SubTIdx, etc.).

**Files/Functions to Modify/Create:**

- `src/lib/ei/asm.cpp`: `HndlMnem()` with full letter-by-letter table lookup (ASM2 line ~2054 semantics)

**Tests to Write:**

- `tests/app_test.cpp`: Test mnemonic lookup (valid opcodes, directives, invalid mnemonics), populate dispatch state

**Steps:**

1. Write tests for `HndlMnem()` with known mnemonics (LDA, STA, EQU, ORG) and invalid text
2. Implement `HndlMnem()` with full letter-table traversal and mnemonic classification
3. Verify dispatch state (MnemP, ZAB, SubTIdx) matches original semantics
4. Test error cases (undefined mnemonic, routed through `RegAsmEW()`)
5. Run tests; verify integration with Pass 1/2 loops

### 3. Phase 3: Directive Operand Evaluation (EvalOprnd) & Core Directives (EQU, ORG)

**Objective:** Implement directive operand wrapper that forces pass-2 symbol resolution, and core directives for symbol definition (EQU) and PC control (ORG).

**Files/Functions to Modify/Create:**

- `src/lib/ei/asm.cpp`: `EvalOprnd()`, `L8A31()` (EQU), `L8A82()` (ORG), `SetPC()`

**Tests to Write:**

- `tests/app_test.cpp`: Test `EQU` symbol definition, PC updates, ORG origin setting, operand evaluation in directive context

**Steps:**

1. Write tests for `EQU` directive (define symbol, verify entry in symbol table)
2. Implement `EvalOprnd()` wrapper to force pass-2 semantics (matching ASM3 line ~347)
3. Implement `L8A31()` (EQU): symbol capture, expression evaluation, symbol table insertion
4. Write tests for `ORG` directive (set origin, verify PC updates)
5. Implement `L8A82()` (ORG) + `SetPC()`: origin setting logic
6. Run tests; verify symbol table and PC state after directives

### 4. Phase 4: Extended Directives (DS/DFB/DW) & Integration with Pass 1/2

**Objective:** Add space allocation (DS) and byte/word definition directives; ensure Pass 1/2 loops call `HndlMnem()` → directive dispatch and can progress through a real assembly listing.

**Files/Functions to Modify/Create:**

- `src/lib/ei/asm.cpp`: `L8C0E()` (DS/.BLOCK), `L8CC3()` (DFB/.BYTE), `L8D67()` (DW/.WORD), refine `DoPass1()`/`DoPass2()` stub integration

**Tests to Write:**

- `tests/app_test.cpp`: Test DS/DFB/DW directives (space allocation, byte/word values, PC updates)
- `tests/app_test.cpp`: Integration test: full assembly with EQU/ORG/DS/DFB (parse, symbol table, PC progression)

**Steps:**

1. Write tests for `DS`/`DFB`/`DW` directives
2. Implement `L8C0E()` (DS), `L8CC3()` (DFB), `L8D67()` (DW)
3. Ensure PC updates correctly after each directive
4. Test Pass 1/2 integration: ensure they call `HndlMnem()`, dispatch to directive handlers, advance to next line
5. Integration test: full mini-assembly (EQU, ORG, DS, DFB, simple mnemonics) processes without errors
6. Run full test suite; verify no regressions

## Notes

- All implementations strictly preserve label/comment semantics from ASM2.S (error system) and ASM3.S (directives)
- Listing output and object emission deferred until later phases
- Forward references handled conservatively (backward-only symbol resolution in Phase 2-3; forward refs addressed later)
- TDD strictly applied: failing tests → minimal code → passing tests
