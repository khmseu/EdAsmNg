# Comparison Testing Progress Summary

## Completed Tasks

### Task 1: Original EDASM Emulator Testing ✅

**Status:** WORKING

Successfully verified that the original EDASM assembler runs via ProDOS8Emu emulator with simple test cases:

- **Test File:** `input.src` (5 lines: LST ON, ORG $800, START NOP, NOP, RTS)
- **Execution:** Completed in 294,281 instructions (well under 1M limit)
- **Output:** Correctly generated 3 bytes: `EA EA 60` (NOP NOP RTS)
- **Listing:** Proper formatted listing with source and machine code

**Key Finding:** The timeout issue reported earlier was specific to `simple_test.asm`. The `input.src` test case works perfectly, confirming the emulator is functional.

**Reference Outputs Saved:**

- `comparative-tests/edasm-outputs/INPUT.LST` - Original EDASM listing
- `comparative-tests/edasm-outputs/INPUT.OBJ` - Original object code (3 bytes)

### Task 2: EdAsmNg CLI Development ⚠️

**Status:** PARTIALLY COMPLETE

Implemented basic command-line interface for EdAsmNg:

**Features Implemented:**

- ✅ Argument parsing (`--listing`, `--object`)
- ✅ File I/O (read source, write binary/text outputs)
- ✅ Unix LF → ProDOS CR conversion
- ✅ Three-pass assembly invocation (DoPass1/2/3)
- ✅ Result reporting (PC, ObjPC, CurAdr values)

**Build Status:**

- ✅ Compiles successfully
- ✅ Runs without crashing
- ✅ Fixed missing `#include <cstddef>` for `size_t`

**Current Limitation - Code Generation Issue:**

The CLI runs but doesn't generate machine code. Investigation revealed:

1. **Root Cause:** Architectural mismatch between test API and production assembly
   - `DoPass1/2/3()` functions expect file-based source via `OpenSrc1()`
   - Test framework uses `SetupMemorySource()` for unit testing
   - These two approaches are incompatible

2. **Evidence:**
   - PC advances correctly to $802 (indicates Pass 1 processed source)
   - Test object memory at $800-$802 contains all zeros (no code generated)
   - DoPass2() calls `OpenSrc1()` which expects file handles, not memory buffers

3. **Implication:**
   - Current `DoPass1/2/3()` were designed for file-based assembly
   - Memory-based testing API (`SetupMemorySource()`) is orthogonal
   - Need bridge between these two approaches

## Current Comparison Results

### Original EDASM Output (Reference)

```
Listing: comparative-tests/edasm-outputs/INPUT.LST
------
SOURCE   FILE #01 =>INPUT.SRC

0000:                1           LST   ON
----- NEXT OBJECT FILE NAME IS /OUT/INPUT.OBJ
0800:        0800    2           ORG   $800
0800:EA              3 START     NOP
0801:EA              4           NOP
0802:60              5           RTS

?0800 START
** SUCCESSFUL ASSEMBLY := NO ERRORS
** TOTAL LINES ASSEMBLED     5

Object: comparative-tests/edasm-outputs/INPUT.OBJ
------
00000000  ea ea 60                                          |..`|
```

### EdAsmNg Output (Current State)

```
Assembly complete:
  PC: $802        ← Correct (processed to end)
  ObjPC: $802     ← Indicates final address
  CurAdr: $802    ← Matches PC

Object: NO CODE GENERATED (all zeros)
Listing: Placeholder only
```

## Technical Analysis

### Assembly Process Flow

```
Original EDASM (6502):
  OpenSrc1() → Read from ProDOS file
  → DoPass1: Build symbol table
  → DoPass2: Generate machine code
  → DoPass3: Generate listing

EdAsmNg Test Framework:
  SetupMemorySource() → Load from memory buffer
  → Test individual components (HndlMnem, HndlDFB, etc.)
  → NOT designed for full three-pass assembly

EdAsmNg CLI (Current):
  SetupMemorySource() → Load file into memory
  → DoPass1() → calls OpenSrc1() → ❌ INCOMPATIBLE
  → DoPass2() → expects file I/O → ❌ NO CODE GENERATED
  → DoPass3() → incomplete
```

### Why DoPass1/2/3 Don't Work with Memory Sources

Looking at `DoPass2()` implementation:

```cpp
void DoPass2() {
    PassNbr++;
    // ...
    OpenSrc1();  // ← Expects file handle, not memory buffer
    // ...
    Pass2Lup:
        GSrcLin();  // ← Reads from file, not memory
        // ...
}
```

The `GSrcLin()` function is designed to read lines from ProDOS files, not from the memory buffer set up by `SetupMemorySource()`.

## Solutions & Next Steps

### Option 1: Refactor DoPass1/2/3 for Memory Sources (Recommended)

**Approach:**

- Modify `GSrcLin()` to check for memory source mode
- If `g_test_src_memory` active, read from memory instead of file
- Preserve file-based path for future file I/O implementation

**Pros:**

- Minimal changes to existing architecture
- Leverages existing memory source infrastructure
- Enables immediate testing

**Cons:**

- Adds conditional logic to core assembly loop
- Not the "real" file-based assembly

**Effort:** Medium (1-2 days)

### Option 2: Implement File-Based Source Management

**Approach:**

- Implement `OpenSrc1()` to read actual files
- Implement ProDOS-style file I/O layer
- Use real file handles throughout assembly

**Pros:**

- Closer to original EDASM architecture
- More "production-ready"
- Future-proof for multi-file assembly

**Cons:**

- Significant implementation effort
- Requires ProDOS file I/O emulation or native replacement
- More complex to test

**Effort:** High (3-5 days)

### Option 3: Continue with Test Framework Validation

**Approach:**

- Accept that unit tests are the primary validation
- Test individual mnemonics/directives exhaustively
- Skip full end-to-end assembly for now

**Pros:**

- 134 passing tests already validate correctness
- No architectural changes needed
- Can proceed with modularization work

**Cons:**

- No end-to-end comparison yet
- Can't generate object files for real programs
- Limited "real world" validation

**Effort:** Low (already working)

## Recommended Path Forward

**Short-term (Immediate):**

1. Document current state ✅ (this file)
2. Use test framework to continue validation of individual components
3. Focus on completing Phase 6 of extraction (enable asm_expr.cpp, asm_directives.cpp)

**Medium-term (Next Sprint):**

1. Implement Option 1 (Memory source support in DoPass1/2/3)
2. Generate working object code for simple test cases
3. Compare byte-for-byte with original EDASM

**Long-term (Future):**

1. Implement proper file I/O (Option 2)
2. Support multi-file assembly
3. Full feature parity with original EDASM

## Files & Outputs

**Test Inputs:**

- `comparative-tests/inputs/input.src` - Simple 5-line test
- `comparative-tests/inputs/simple_test.asm` - More complex test (not yet working)

**Reference Outputs (Original EDASM):**

- `comparative-tests/edasm-outputs/INPUT.LST` - Listing file
- `comparative-tests/edasm-outputs/INPUT.OBJ` - Object code (3 bytes: EA EA 60)

**EdAsmNg Outputs:**

- `comparative-tests/edasmng-outputs/INPUT.LST` - Placeholder listing
- `comparative-tests/edasmng-outputs/INPUT.OBJ` - Empty (all zeros)

**CLI Implementation:**

- `src/main.cpp` - Command-line interface
- `include/EdAsmNg/asm.hpp` - Public API (fixed size_t issue)

## Test Results Summary

| Component               | Status         | Evidence                                       |
| ----------------------- | -------------- | ---------------------------------------------- |
| Original EDASM Emulator | ✅ WORKING     | Generates correct output in <300K instructions |
| EdAsmNg Unit Tests      | ✅ PASSING     | 134/134 tests pass                             |
| EdAsmNg CLI Compilation | ✅ WORKING     | Builds and runs                                |
| EdAsmNg Code Generation | ❌ NOT WORKING | DoPass1/2/3 incompatible with memory sources   |
| End-to-End Comparison   | ⏸️ BLOCKED     | Waiting for code generation fix                |

## Conclusion

We've successfully completed Task 1 (emulator validation) and made significant progress on Task 2 (CLI development). The main blocker is architectural: the assembly passes expect file I/O, but our test framework uses memory sources. This is solvable with Option 1 (refactor for memory support) or Option 2 (implement file I/O).

For now, the 134 passing unit tests provide strong confidence in correctness. Once we implement memory source support in the passes, we'll be able to generate object code and perform byte-level comparisons with the original EDASM.
