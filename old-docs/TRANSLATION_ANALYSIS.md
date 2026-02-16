# EdAsm Assembler Translation Analysis

**Date:** 2026-02-14  
**Scope:** Comparison of original 6502 assembly (ASM1.S, ASM2.S, ASM3.S) vs. C++ translation  
**Status:** Phase 2 (Mnemonic Dispatch) Complete; ~54% structural translation, ~35% functional implementation

---

## Summary Statistics

| Metric                  | Value                                |
| ----------------------- | ------------------------------------ |
| Original 6502 code      | 9,697 lines (ASM1.S, ASM2.S, ASM3.S) |
| Current C++ translation | 5,221 lines                          |
| Translation coverage    | ~54% structural, ~35% functional     |
| Completed phases        | 2 of 4                               |
| Stubbed functions       | 40+                                  |
| Test coverage           | 20+ unit tests for Phase 2           |

---

## <completed_sections>

### 1. **Initialization & Infrastructure (100% Complete)**

**Location:** [src/lib/ei/asm.cpp](src/lib/ei/asm.cpp#L30-L350)

**What's Translated:**

- Zero page memory locations ($60-$F1) mapped as C++ variables
- Symbol table flag bits (undefined, external, entry, macro, etc.)
- ASCII character constants and control codes
- File control table (FCT) indices for I/O
- Global 6502 CPU register emulation (A, X, Y, C, Z, N, V flags)
- Forward declarations for function pointers
- Global test buffer infrastructure for unit testing
- Character mapping tables (CharMap1, CharMap2)

**Test Coverage:** All infrastructure tested via initialization functions

---

### 2. **Pass 3 - Symbol Table Printing (95% Complete)**

**Location:** [src/lib/ei/asm.cpp](src/lib/ei/asm.cpp#L406-L1080)

**Functions Implemented:**

- `DoPass3()` - Main symbol table printing control
- `LD198()` - Symbol table existence check
- `DoSort()` - Shell sort algorithm for symbol ordering (793+ lines)
- `PrSymTbl()` - Print symbol table in 2/4/6 column format (1042+)
- `AdvRecP()` - Advance record pointer for pagination
- Helper I/O: `PutC()`, `PrByte()`, `PutCR()`, `PrtFF()`, `NextRec()`

**What's Missing:** Conditional logic for subtitle generation (deferred to Phase 3)

---

### 3. **Character & Token Processing (100% Complete)**

**Location:** [src/lib/ei/asm.cpp](src/lib/ei/asm.cpp#L3130-L3260)

**Functions Implemented:**

- `ChrGet()` / `ChrGot()` - Character fetching with ASCII-to-uppercase conversion
- `L81F0()` - Skip non-blank characters
- `AdvSrcP()` - Advance source pointer
- `WhiteSpc()` - Check for whitespace (space/CR)
- `GNToken()` - Get next token (comma, paren, space/CR)
- Character flag byte interpretation (alphabetic, numeric, hex-digit detection)

**Testing:** ChrGot/ChrGet semantics fully verified; flags (C, Z, V, N) properly set

---

### 4. **Phase 2 - Mnemonic Dispatch (100% Complete)**

**Location:** [src/lib/ei/asm.cpp](src/lib/ei/asm.cpp#L3245-L3310)

**HndlMnem() Function:**

- First-letter indexing into mnemonic table
- Character-by-character mnemonic matching with MSB masking
- Opcode flag byte extraction
- Directive detection and identification
- Macro invocation detection (stub for nesting checks)
- Full label/comment preservation matching ASM2.S line 2054

**Dispatch State Variables Set:**

- `MnemP` - Pointer to mnemonic table entry
- `ZAB` - Flag byte (addressing mode bits for opcodes, directive flag)
- `SubTIdx` - Subtable index
- `C` flag - Error status (0=success, 1=not found/error)

**Test Coverage:** 20+ unit tests in [tests/app_test.cpp](tests/app_test.cpp#L169-L434)

- Valid 6502 mnemonics (LDA, STA, ADC, AND, ASL, BCC, BCS, etc.)
- Valid directives (EQU, ORG, DFB, DW, etc.)
- Invalid mnemonic rejection
- Case sensitivity verification
- Whitespace handling

---

### 5. **Expression Evaluation (95% Complete)**

**Location:** [src/lib/ei/asm.cpp](src/lib/ei/asm.cpp#L3700-L4100+)

**Functions Implemented:**

- `EvalExpr()` - Master expression evaluator with byte operators (<, >)
- `EvalTerm()` - Terminal expression (constants, identifiers, special cases)
- `EvalSExpr()` - Sub-expression evaluation with operator dispatch
- **Operator Functions:**
  - `ExprADD()` - Addition (+)
  - `ExprSUB()` - Subtraction (-)
  - `ExprMUL()` - Multiplication (\*) via bit shifts
  - `ExprDIV()` - Division (/)
  - `ExprAND()` - Bitwise AND (^)
  - `ExprORA()` - Bitwise OR (|)
  - `ExprEOR()` - Bitwise XOR (!)

**Features:**

- Unary operators (leading +/-)
- Byte extraction operators (<low, >high)
- Symbol lookup via FindSym()
- Relocatable expression tracking (RelExprF)
- Forward reference detection
- Numeric constant parsing (decimal, binary %, hex $, octal @)
- ASCII character constants ('X')
- Program counter references (\*)
- Error accumulation in ExprAccF
- 16-bit arithmetic with carry propagation

**Limitations:**

- No overflow detection (per original design)
- Bit shift multiplication/division lack overflow checks
- RelExprF checks incomplete for conditional assembly integration

---

### 6. **Addressing Mode Selection (90% Complete)**

**Location:** [src/lib/ei/asm.cpp](src/lib/ei/asm.cpp#L3570-L3650)

**Functions Implemented:**

- `GAdrMod()` - Parse operand addressing mode
- `IsZPMod()` - Zero page addressing detection
- `IsAccMod()` - Accumulator addressing (single 'A')
- `Is65C02()` - 65C02 processor feature validation
- `IsC02Op()` - Rockwell 65C02 opcode validation
- `WhiteSpc()` - Whitespace validation

**Features:**

- 13 addressing modes supported (indexed, indirect, etc.)
- Token-based parsing via AModTkns/AModCmds tables
- Error detection with line/statement reporting
- Helper function dispatch via function pointers

**Testing:** Addressing mode selection tested via HndlMnem tests

---

### 7. **Symbol Table and Lookup (85% Complete)**

**Location:** [src/lib/ei/asm.cpp](src/lib/ei/asm.cpp#L1682-L1920)

**Functions Implemented:**

- `FindSym()` - Hash-based symbol lookup with collision handling
- `HashFn()` - Hash function for symbol names
- `RsvdId()` - Reserved identifier checking (A, X, Y)
- `AddNode()` - Add new symbol to hash table

**Features:**

- Chained hash table with 256 buckets
- Symbol flag byte management
- Forward reference tracking
- External symbol accumulation
- New symbol insertion with proper linking

---

### 8. **Error Handling Infrastructure (70% Complete)**

**Location:** [src/lib/ei/asm.cpp](src/lib/ei/asm.cpp#L1600-L1650)

**Functions Implemented:**

- `RegAsmEW()` - Register error with token and flag context
- `SaveErrInfo()` - Save error info for line reporting
- `DoAlert()` (stub) - Alert display
- `PrtErrMsg()` (stub) - Error message printing

**Error Message Table:** 48 error strings defined (LD50F-LD781)

- Undefined identifier
- Duplicate identifier
- Overflow, syntax errors
- Addressing mode errors
- Macro nesting errors
- Symbol table full
- etc.

**Missing:** File number and line number encoding in error records

---

### 9. **Data Tables & Constants (100% Complete)**

**Location:** [src/lib/ei/asm.cpp](src/lib/ei/asm.cpp#L3800-End)

**Tables Provided:**

- Mnemonic/Directive table (5 sections: LtrA, LtrB-Z, DotDrtv)
- 6502/65C02 opcode translation table (OpcodeT, 213 bytes)
- CPU cycle times (CycTimes, 213 bytes)
- Character classification maps (CharMap1, CharMap2)
- Operand parser tokens (AModTkns, AModCmds)
- Helper function dispatch tables (L888E, L8895)
- Error message pointers (ErrMsgT)

**Mnemonic Coverage:** 70+ mnemonics/directives including:

- CPU mnemonics: ADC, AND, ASL, BIT, CMP, CPX, CPY, DEC, EOR, INC, JMP, JSR, LDA, LDX, LDY, LSR, NOP, ORA, PHA, PHP, PHX, PHY, PLA, PLP, PLX, PLY, ROL, ROR, RTS, RTI, RTN, SBC, STA, STX, STY, STZ, TAX, TAY, TSX, TXA, TXS, TYA
- Branch instructions: BCC, BCS, BEQ, BMI, BNE, BPL, BVC, BVS
- Directives: EQU, ORG, OBJ, DS, DB, DFB, DW, DCI, ASC, END, ENTRY, EXTRN, INCLUDE, MAC, PAUSE, etc.
- Sweet16 pseudo-opcodes (20+)
- Dot directives (.EQU, .ORG, etc.)

---

## <remaining_sections>

### 1. **Pass 1 - Symbol Building (0% Complete)**

**Location:** Original ASM2.S lines 80-220 (DoPass1 entry point)

**Scope:** ~1,800 lines of 6502 code

**What Needs Implementing:**

- [ ] Loop through source lines
- [ ] Call `HndlMnem()` for each line's mnemonic field (DONE - called but result ignored)
- [ ] Signal success/completion (C=0, Y=0 at line end)
- [ ] Detect FIN/ELSE/END directives to control conditional compilation
- [ ] Directive dispatch for:
  - **EQU** - Store symbol: value, flag byte (ASM3.S L8A31)
  - **ORG** - Set program counter (ASM3.S L8A82)
  - **OBJ** - Set object memory location (ASM3.S L8BAD)
  - **REL** - Enable relocatable code (ASM2.S ~line 640)
  - **MAC** - Begin macro definition (ASM3.S ~line 1380)
  - **ENTRY** - Export symbol (ASM3.S ~line 1600)
  - **EXTRN** - Import symbol (ASM3.S ~line 1620)
  - **DS** - Reserve space (ASM3.S L8C0E)
  - **DFB** - Define byte(s) - stub space calculation only (ASM3.S L8CC3)
  - **Conditional directives:** IF, IFNOT, ELSE, FIN, etc.
- [ ] GetSrcLin() - Fetch next source line
- [ ] BCDNbr increment for line counter
- [ ] DskSrcF detection (disk vs. memory source)
- [ ] FileNbr tracking for INCLUDEs

**Dependencies:**

- HndlMnem() DONE
- Symbol table operations DONE
- Expression evaluation DONE
- Directive handler stubs needed

**Estimated Lines:** ~400-500 for control flow + ~100-150 per directive handler

---

### 2. **Pass 2 - Code Generation (0% Complete)**

**Location:** Original ASM2.S lines 220-350 (DoPass2 entry point)

**Scope:** ~1,200 lines of 6502 code

**What Needs Implementing:**

- [ ] Loop through source lines again (similar structure to Pass 1)
- [ ] HndlMnem() call to get mnemonic dispatch data
- [ ] Direction handler invocation for directives (already partially stubbed)
- [ ] Opcode processing **NEW MAJOR PHASE 3:**
  - Generate machine code from opcode + addressing mode
  - Select correct opcode byte from OpcodeT table
  - Validate addressing mode combinations
  - Generate 1-, 2-, or 3-byte instructions
  - Handle relative addressing for branch instructions
  - Relocatable code (RLD) entry generation
  - Object file buffering (GMC buffer)
  - Listing control (LstCodeF setup)
- [ ] GAdrMod() evaluation - already DONE but not integrated
- [ ] ValueError handling for out-of-range operands
- [ ] Branch range validation (±128 bytes)
- [ ] Opcode suffix handling (.W, .I, .D, .P for Sweet16)

**Critical Subfunctions Needed:**

- StorByt() - Write generated byte to object file
- ListCode() (stub) - Output assembled code to listing
- GenMCode() - Generate machine code from opcode + mode

**Dependencies:**

- HndlMnem() DONE
- GAdrMod() mostly DONE (needs integration)
- Expression evaluation DONE
- OpCode translation table DONE
- ObjPC (object file pointer) management needed
- ListingF and LstCodeF control needed

**Estimated Lines:** ~600-800 for control + ~80-120 per opcode handler

---

### 3. **Directive Handlers (15% Complete)**

**Location:** Original ASM3.S lines 1-600, ASM2.S various

**Status:** Only framework visible. Handler entry points referenced but not implemented.

**What Needs Implementing:**

| Directive                           | ASM3.S Line  | Status    | Complexity | Lines |
| ----------------------------------- | ------------ | --------- | ---------- | ----- |
| EQU                                 | L8A31        | STUB      | Low        | 30    |
| ORG                                 | L8A82        | STUB      | Medium     | 80    |
| OBJ                                 | L8BAD        | STUB      | Medium     | 60    |
| REL                                 | ~640/2.S     | STUB      | Medium     | 40    |
| DS/.BLOCK                           | L8C0E        | STUB      | Medium     | 70    |
| DB/DFB                              | L8CC3        | STUB      | High       | 120   |
| DW/.WORD                            | L8D67        | STUB      | Medium     | 60    |
| DCI                                 | L8E54        | STUB      | Medium     | 40    |
| ASC                                 | L8DD2        | STUB      | Low        | 35    |
| MAC                                 | ~1380        | STUB      | Very High  | 250+  |
| END                                 | L9215        | STUB      | Low        | 20    |
| ENTRY                               | L9144        | STUB      | Low        | 25    |
| EXTRN                               | L91A8        | STUB      | Low        | 25    |
| IF/IFNOT/ELSE/FIN                   | L90DE+       | STUB      | High       | 150+  |
| INCLUDE                             | L9360        | STUB      | Very High  | 200+  |
| Dot directives (.ASC, .BLOCK, etc.) | LtrD[], etc. | Framework | Medium     | 60    |

**Key Missing Infrastructure:**

- RTS-trampoline dispatch for directives (asm2/asm3 use JMP via stack)
- Macro parameter expansion
- Conditional assembly state machine
- Include file nesting and stack management
- Macro file searching

**Estimated Total:** 1,400-1,800 lines

---

### 4. **File I/O Operations (25% Complete)**

**Location:** Original across ASM2.S and ASM3.S

**What Needs Implementing:**

- [ ] **Open4RW()** - Open file for reading/writing (ProDOS 8)
- [ ] **ClsFile()** - Close file by FCT index
- [ ] **Wr1Byte()** - Write one byte to object file
- [ ] **GetFPos()/SetFPos()** - Get/set file position
- [ ] **ReadBlk()/WriteBlk()** - Bulk I/O operations
- [ ] **PRODOS8()** - ProDOS 8 system call interface
- [ ] **DOSErrs()** - DOS error handler (no return)
- [ ] **L99DF()** - Flush all object code buffers
- [ ] Buffer management (obj code, listing output)

**Dependencies:** Direct ProDOS 8 integration (Apple II specific)

**Note:** These are Apple II/ProDOS specific. In a modern C++ port, these would be replaced with standard C++ file I/O.

**Estimated Lines:** 200-300

---

### 5. **Listing File Output (0% Complete)**

**Location:** Original ASM1.S (mostly), ASM2.S, ASM3.S scattered

**What Needs Implementing:**

- [ ] **PrtAsmLn()** - Print assembled line with code bytes
- [ ] **ListCode()** - Output generated machine code to listing
- [ ] **LstSrcLn()** - Print source line in listing format
- [ ] **PrtSymTbl()** - Already DONE but integration with listing needed
- [ ] Page breaks and headers
- [ ] Column formatting (2, 4, 6 columns for symbol table)
- [ ] Cycle time calculation (LstCyc flag)
- [ ] CPU cycle times display

**Features to Support:**

- Address field (4 hex digits)
- Generated code field (up to 6 bytes: XX XX XX | XX XX XX format)
- Source line field
- Error indicators
- Symbol definitions
- Pass 3 symbol table output

**Estimated Lines:** 300-400

---

### 6. **Macro Support (0% Complete)**

**Location:** Original ASM3.S lines 1380-1600+

**Scope:** Very large, complex subsystem

**What Needs Implementing:**

- [ ] Macro definition parsing (MAC ... MEND)
- [ ] Parameter counting and substitution
- [ ] Argument parsing from invocation
- [ ] Macro expansion (text substitution)
- [ ] Nesting depth enforcement
- [ ] Macro library file searching
- [ ] MACLIB directive

**Critical Variables:**

- MacroF - Macro status flag
- MacArg - Macro argument number
- MParmCnt - Parameter count
- MacFile - Macro file FCT index

**Estimated Lines:** 400-600

---

### 7. **Error Detection & Recovery (30% Complete)**

**Location:** Scattered, foundation laid

**What Needs Implementing:**

- [ ] **SkipErrs()** - Skip to end of line after error
- [ ] Error context capture (file, line number)
- [ ] Error count tracking (NbrErrs increment)
- [ ] Suggestion mechanism for common errors
- [ ] Continue assembly despite errors
- [ ] Syntax error diagnosis

**What's Done:**

- Error token system (0x00-0x48)
- Error message table (48 messages)
- SaveErrInfo() infrastructure
- RegAsmEW() error registration

**Estimated Lines:** 150-200

---

### 8. **Initialization & Cleanup (40% Complete)**

**Location:** ASM2.S ExecAsm, InitASM, cleanup code

**What's Done:**

- SaveZP() and restoration of zero page state
- Vector setup (SetupVec)
- Assembly process control flow skeleton

**What Needs Implementing:**

- [ ] **InitASM()** - Initialize assembler state (FCT, variables, flags)
- [ ] **DoPass1()** - Main Pass 1 control loop
- [ ] **DoPass2()** - Main Pass 2 control loop
- [ ] File table initialization
- [ ] Memory allocation for symbol table
- [ ] Buffer initialization
- [ ] Flag state setup

**Estimated Lines:** 200-300

---

## <next_phases>

### Recommended Translation Sequence

**Phase 3: Opcode Processing & Code Generation (2,000-2,500 lines)**

- **Priority:** CRITICAL - blocks Pass 2 execution
- **Dependencies:** HndlMnem (DONE), EvalExpr (DONE), GAdrMod (DONE)
- **Tasks:**
  1. Implement `StorByt()` - Write byte to object buffer
  2. Implement `GenMCode()` - Generate machine code from opcode + addressing mode
  3. Create opcode handler dispatch table
  4. Handle 6502 opcode variants per addressing mode
  5. Branch instruction relative address calculation
  6. Implement `ListCode()` for code listing output
  7. Create integration tests for code generation

- **Deliverable:** Pass 2 can generate valid 6502 object code

---

**Phase 4: Directive Handlers (1,400-1,800 lines)**

- **Priority:** HIGH - required for usable assembler
- **Dependencies:** Phase 3 (StorByt, machine code gen)
- **Sequence:**
  1. **Simple Directives (week 1):** ASC, DCI, DFB, DW, DS, OBJ, ORG, EQU
  2. **Symbol Directives (week 2):** ENTRY, EXTRN, symbol table link
  3. **Control Directives (week 3):** IF, IFNOT, ELSE, FIN (conditional asm)
  4. **Complex Directives (week 4):** MAC/MEND (macro definition), INCLUDE

- **Deliverable:** Basic 6502 assembly functional (no macros yet)

---

**Phase 5: File I/O & Listing Output (500-700 lines)**

- **Priority:** HIGH - required for assembly output
- **Dependencies:** Phase 3-4 (code generation, directives)
- **Tasks:**
  1. Adapt ProDOS 8 I/O calls to modern C++ file operations
  2. Implement object file writing
  3. Implement listing file output
  4. Page break and column formatting
  5. Error summary reporting

- **Deliverable:** Complete assembler output (OBJ, LST files)

---

**Phase 6: Macro Expansion (400-600 lines)**

- **Priority:** MEDIUM - advanced feature
- **Dependencies:** Phase 4-5
- **Tasks:**
  1. Macro definition parsing (MAC ... MEND)
  2. Parameter substitution
  3. Macro invocation from HndlMnem
  4. Nesting validation
  5. MACLIB library support

- **Deliverable:** Full macro support matching EDASM capabilities

---

**Phase 7: Pass 1 Main Loop (300-400 lines)**

- **Priority:** CRITICAL - required for assembly process
- **Dependencies:** Phases 3-6 (most directive handlers)
- **Tasks:**
  1. Source line reading loop
  2. Label field parsing and symbol creation
  3. Directive dispatch
  4. Line number tracking (BCD)
  5. File nesting management (INCLUDE)

- **Deliverable:** Pass 1 fully functional

---

**Phase 8: Pass 2 Main Loop (300-500 lines)**

- **Priority:** CRITICAL - depends on Phase 1
- **Dependencies:** Phases 3-7
- **Tasks:**
  1. Source line reading loop
  2. Mnemonic/Directive handling
  3. Operand evaluation with forward references
  4. Machine code generation
  5. Listing output
  6. Error reporting with context

- **Deliverable:** Pass 2 fully functional - runnable assembler

---

## <dependencies>

### Critical Prerequisite Functions

#### Already Satisfied (✓ DONE):

- [x] `HndlMnem()` - Mnemonic dispatch (Phase 2)
- [x] `EvalExpr()` / `EvalTerm()` - Expression evaluation
- [x] `GAdrMod()` - Addressing mode selection
- [x] `FindSym()` / `AddNode()` - Symbol table operations
- [x] Character processing (ChrGot, WhiteSpc, etc.)
- [x] Data tables (Mnemonics, Opcodes, Messages, etc.)

#### Still Required (⚠ NOT DONE):

1. **StorByt()** - Prerequisite for all code generation
2. **GenMCode()** - Prerequisite for Pass 2
3. **GSrcLin()** - Prerequisite for Pass 1 loops
4. **EvalOprnd()** - Expression evaluation in Pass 1/2 context (force Pass 2)
5. **L8CAB()** - Fill with random data (DS directive)
6. **Wr1Byte()** - Write to disk (object file I/O)
7. **Open4RW()** - File opening (I/O)
8. **L99DF()** - Flush buffers before closing

### Variable State Dependencies

**Before Pass 1 Execution:**

- [ ] Symbol table memory allocated (StrtSymT, EndSymT)
- [ ] File control table (FCT) initialized
- [ ] Source file opened (ChnFile)
- [ ] Object file created if needed
- [ ] PassNbr = 0 (Pass 1 flag)
- [ ] PC, ObjPC initialized per ORG directive

**Before Pass 2 Execution:**

- [ ] Symbol table populated from Pass 1
- [ ] PC, ObjPC reset (or ORG'd)
- [ ] PassNbr = 1 (Pass 2 flag)
- [ ] Object buffer cleared
- [ ] ListingF state set

**Before Pass 3 Execution:**

- [ ] Assembly completed or aborted
- [ ] Symbol table complete (EndSymT valid)
- [ ] Error count finalized (NbrErrs)

### Integration Points

**HndlMnem() → Directive Dispatch:**

```
HndlMnem() returns:
  - C = 0 (success)
  - ZAB = flag byte (addressing modes for opcodes, directive flag for directives)
  - MnemP = pointer to table entry

For directives (ZAB & 0x80):
  → Need RTS-trampoline dispatch OR
  → Need function pointer table for directive handlers
```

**Operand → Machine Code Pipeline:**

```
1. HndlMnem() identifies opcode
2. GAdrMod() parses operand → addressing mode index
3. EvalExpr() calculates operand value
4. GenMCode() looks up opcode byte from table
5. StorByt() writes to object buffer
```

**Error Handling Context:**

```
RegAsmEW(errorToken) needs:
  - FileNbr (current file)
  - BCDNbr[0,1] (BCD line number)
  - ErrorToken (0x00-0x48)
→ SaveErrInfo() records these
→ PrtErrMsg() retrieves and displays
```

---

## Summary: What Must Happen First

**Blocker #1:** `StorByt()` - Without this, no object code can be generated (blocks Phase 3)

**Blocker #2:** `GSrcLin()` - Without this, cannot loop through source (blocks Pass 1 & 2)

**Blocker #3:** Directive handlers - Without these, only raw opcodes assemble (blocks practical use)

**Recommended First Steps:**

1. Implement `StorByt()` and object code buffer management
2. Implement `GenMCode()` and validate against test cases
3. Implement `GSrcLin()` for source file reading
4. Implement `EvalOprnd()` wrapper
5. Add basic directive handlers (EQU, ORG, OBJ)
6. Get Pass 1 → Pass 2 working with simple test files
7. Add remaining directive handlers incrementally
8. Integrate macro support

</completed_sections>

</remaining_sections>

</next_phases>

</dependencies>
