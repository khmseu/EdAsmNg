# Plan: Extract Expression Evaluation and Directive Handlers

Extract ~1972 lines of disabled code from the #if 0 block (lines 4661-6633) into two new modules: asm_expr.cpp and asm_directives.cpp. This enables full assembler functionality while maintaining modularity.

## Phases (6 phases)

### Phase 1: Extend State Bridge for Expression & Directives

**Objective:** Add all state variables and helper function declarations needed by both expression evaluation and directive handlers to asm_internal.hpp

**Files/Functions to Modify/Create:**

- src/lib/asm/asm_internal.hpp (add ~25 state variables, ~20 function declarations)

**State Variables to Add:**

```cpp
// Expression evaluation state
extern std::uint16_t& ValExpr_word;      // Expression value (16-bit)
extern std::uint8_t&  ValExpr_lobyte;    // Low byte of expression value
extern std::uint8_t&  ValExpr_hibyte;    // High byte of expression value
extern std::uint32_t& Accum;             // Expression accumulator
extern std::uint8_t&  RelExprF;          // Relocatable expression flag
extern std::uint8_t&  ExprAccF;          // Expression accumulated flags
extern std::uint8_t&  NxtToken;          // Next token indicator
extern std::uint8_t&  Ret816F;           // Return 8/16 bit flag
extern std::uint8_t&  GblAbsF;           // Global absolute flag
extern std::uint8_t&  SavSEF;            // Saved sub-expression flag
extern std::uint8_t&  RadixCh;           // Radix character
extern std::uint8_t&  BitsDig;           // Bits per digit
extern std::uint8_t&  msbF;              // MSB flag

// Directive state
extern std::uint16_t& ObjPC;             // Object program counter
extern std::uint16_t& Length;            // Length field
extern std::uint8_t&  ZAB;               // Zero page addressing byte
extern std::uint8_t&  EndianF;           // Endian flag
extern std::uint8_t&  ListingF;          // Listing flag
extern std::uint8_t*  GMC;               // Generated machine code buffer
extern std::uint8_t&  GMCIdx;            // GMC index
extern std::uint8_t*  SubTitle;          // Subtitle buffer
```

**Helper Functions to Declare:**

```cpp
// Expression evaluation
void EvalExpr();
void EvalTerm();
void EvalSExpr();
void ExprADD();
void ExprSUB();
void ExprMUL();
void ExprDIV();
void ExprEOR();
void ExprAND();
void ExprORA();
void GNToken();
void Mul2();
void AdvSrcP();

// Directive handlers
void DrtvDone();
void HndlEQU();
void HndlORG();
void HndlOBJ();
void HndlREL();
void HndlDS();
void HndlDFB();
void HndlDW();
void HndlDWCore();
void HndlASC();
void HndlASC_Core();
void HndlDCI();
void HndlLST();
void HndlNOLIST();
void DoPage();
void HndlSBTL();
void Is16K();
```

**Steps:**

1. Identify all state variables used by expression and directive code in #if 0 block
2. Add extern declarations to asm_internal.hpp
3. Add corresponding references in asm.cpp AsmInternal namespace block
4. Add function declarations to asm_internal.hpp
5. Build to verify no compilation errors (functions not yet implemented)

**Tests to Write:** None (build verification only)

---

### Phase 2: Create asm_expr.cpp

**Objective:** Extract expression evaluation functions (~800 lines) from #if 0 block to new file

**Files/Functions to Modify/Create:**

- src/lib/asm/asm_expr.cpp (create, ~800 lines)
- src/CMakeLists.txt (add asm_expr.cpp to build)

**Functions to Extract:**

- EvalExpr() (~250 lines) - Main expression parser
- EvalTerm() (~150 lines) - Term evaluator
- EvalSExpr() (~80 lines) - Sub-expression handler
- ExprADD, ExprSUB, ExprMUL, ExprDIV (~40 lines each, ~160 total)
- ExprEOR, ExprAND, ExprORA (~30 lines each, ~90 total)
- GNToken, Mul2, AdvSrcP (~70 lines total combined)

**Steps:**

1. Copy expression functions from #if 0 block (lines ~4661-5324) to new file
2. Wrap in AsmInternal namespace
3. Add #include "asm_internal.hpp"
4. Remove #if 0 wrapper
5. Add to CMakeLists.txt: lib/asm/asm_expr.cpp
6. Verify compilation (may have errors to fix in Phase 3)

**Tests to Write:** Build verification

---

### Phase 3: Create asm_directives.cpp

**Objective:** Extract directive handlers (~1200 lines) from #if 0 block to new file

**Files/Functions to Modify/Create:**

- src/lib/asm/asm_directives.cpp (create, ~1200 lines)
- src/CMakeLists.txt (add asm_directives.cpp to build)

**Functions to Extract:**

- DrtvDone() (~20 lines)
- HndlEQU() (~80 lines)
- HndlORG() (~60 lines)
- HndlOBJ() (~40 lines)
- HndlREL() (~40 lines)
- HndlDS() (~50 lines)
- HndlDFB() (~100 lines)
- HndlDW() (~120 lines)
- HndlDWCore() (~80 lines)
- HndlASC() (~150 lines)
- HndlASC_Core() (~100 lines)
- HndlDCI() (~80 lines)
- HndlLST(), HndlNOLIST(), DoPage(), HndlSBTL() (~300 lines combined)
- Is16K() (~30 lines)

**Steps:**

1. Copy directive functions from #if 0 block (lines ~5325-6633) to new file
2. Wrap in AsmInternal namespace
3. Add #include "asm_internal.hpp"
4. Remove #if 0 wrapper
5. Add to CMakeLists.txt: lib/asm/asm_directives.cpp
6. Verify compilation

**Tests to Write:** Build verification

---

### Phase 4: Remove #if 0 Block from asm.cpp

**Objective:** Delete the extracted code from asm.cpp (remove lines 4661-6633)

**Files/Functions to Modify/Create:**

- src/lib/asm/asm.cpp (delete ~1972 lines)

**Steps:**

1. Delete entire #if 0 block (lines 4661-6633 inclusive)
2. Verify file compiles
3. Check that asm.cpp size is now ~9,874 - 1,972 = ~7,902 lines

**Tests to Write:** Build verification

---

### Phase 5: Fix Compilation Errors

**Objective:** Resolve any compilation errors from extraction (type mismatches, missing dependencies)

**Files/Functions to Modify/Create:**

- src/lib/asm/asm.cpp (add missing bridge implementations if needed)
- src/lib/asm/asm_internal.hpp (correct type declarations)
- src/lib/asm/asm_expr.cpp (fix undefined references)
- src/lib/asm/asm_directives.cpp (fix undefined references)

**Steps:**

1. Build and collect all compilation errors
2. Fix type mismatches in state variable declarations
3. Add missing helper function implementations or stubs
4. Resolve SrcP_at-style macro issues if any
5. Iterate until clean build

**Tests to Write:** Build must succeed with no errors

---

### Phase 6: Build, Test & Verify

**Objective:** Ensure all 134 tests pass with no regressions

**Files/Functions to Modify/Create:** None (verification only)

**Steps:**

1. Run full test suite: `./build/tests/EdAsmNg_app_test`
2. Verify all 134 tests pass
3. Check that expression evaluation and directives are now functional
4. Document any remaining issues for future phases

**Tests to Write:** All existing 134 tests must pass

---

## Open Questions

1. **Should expression and directives be one or two commits?**
   - Option A: Single commit with both extractions (cleaner history)
   - Option B: Two commits (expression first, then directives)
   - **Recommendation: Option A** - They're in the same #if 0 block and interdependent

2. **Are there compilation errors in the #if 0 code?**
   - The comment says "EvalExpr has compilation errors"
   - Option A: Fix during Phase 5
   - Option B: Extract as #if 0 and fix in future phase
   - **Recommendation: Option A** - Fix now since we're enabling the code

3. **Do tests exist for these functions?**
   - Need to check if any of the 134 tests exercise directives/expressions
   - May need to add tests after extraction
   - **Recommendation:** Run existing tests first, add new tests only if gaps found
