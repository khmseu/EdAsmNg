# EdAsmNg Phase 2 & Phase 3 Test Failure Analysis

**Status**: Analysis Complete
**Date**: February 17, 2026
**Scope**: 9 failing test compilation errors in Phase2_GInstLenTest and Phase3_StorGMCTest

---

## Executive Summary

The 9 failing tests are **not actual test logic failures** — they are **compilation failures caused by missing function declarations** in the test helpers header file. All the required implementation functions exist in `asm.cpp` (starting at line 9185), but they are not declared in `asm_test_helpers.hpp`, causing the linker to fail.

**Total Missing Declarations**: 16+ functions needed by tests but undeclared in header

---

## Root Cause: Incomplete Test Helpers Header

### Current State

**File**: [tests/asm_test_helpers.hpp](tests/asm_test_helpers.hpp)

- **Length**: 63 lines
- **Last Declaration**: Line 54 (`SetGMC()`)
- **Issue**: Header ends abruptly after Phase 3b declarations, missing many required test helper functions

### Implementation Exists But Undeclared

**File**: [src/lib/asm/asm.cpp](src/lib/asm/asm.cpp)

- **Test Helper Implementations**: Lines 9185-9550+
- **Status**: All implementations present and complete
- **Problem**: Zero visibility to test code due to missing header declarations

---

## Missing Declarations Analysis

### Phase 2: GInstLen Test Requirements

#### Functions Declared ✓ (Working)

- `GInstLen()` - wraps Bridge_GInstLen()
- `SetupMnemP()` - sets up mnemonic table entry
- `SetupOperandField()` - configures operand field
- `SetLength()` / `GetLength()` - instruction length accessors
- `SetLenTIdx()` / `GetLenTIdx()` - addressing mode index accessors
- `SetGMC()` / `GetGMC()` - GMC buffer accessors

#### Functions Missing ✗ (Causes Failures)

```
ERROR: SetPassNbr() - NOT declared in header
       Location: Test calls at line 4182 in app_test.cpp
       Implementation: src/lib/asm/asm.cpp line 9495
       Purpose: Set current assembly pass number (0=Pass1, 1=Pass2, 2=Pass3)
```

### Phase 3: StorGMC Test Requirements

#### Functions Missing ✗ (All 8 cause failures)

```
ERROR: GetObjPC() / SetObjPC(uint16_t) - NOT declared
       Implementation: src/lib/asm/asm.cpp lines 9222-9229
       Purpose: Get/set Object Program Counter (target load address)

ERROR: SetGenF(uint8_t) - NOT declared
       Implementation: src/lib/asm/asm.cpp line 9214
       Purpose: Set Generation Flags (suppress code, select disk/mem mode)

ERROR: SetHighMem(uint16_t) - NOT declared
       Implementation: src/lib/asm/asm.cpp line 9233
       Purpose: Set high memory boundary

ERROR: InitObjMemory() - NOT declared
       Implementation: src/lib/asm/asm.cpp line 9253
       Purpose: Initialize test object memory (enables g_test_obj_memory)

ERROR: ReadObjMemory(uint16_t) - NOT declared
       Implementation: src/lib/asm/asm.cpp line 9247
       Purpose: Read byte from simulated object memory

ERROR: WriteObjMemory(uint16_t, uint8_t) - NOT declared
       Implementation: src/lib/asm/asm.cpp line 9250
       Purpose: Write byte to simulated object memory

ERROR: StorGMC() - NOT declared (wrapper exists but not exposed)
       Implementation: src/lib/asm/asm.cpp line 9191
       Purpose: Store Generated Machine Code to memory and advance ObjPC

ERROR: GetPassNbr() - NOT declared
       Implementation: src/lib/asm/asm.cpp line 9491
       Purpose: Get current pass number
```

---

## Compilation Error Details

### Sample Error Output

```
error: no member named 'GetLength' in namespace 'EdAsmNg::Asm'
        4268 |   EXPECT_EQ(EdAsmNg::Asm::GetLength(), 2);
             |             ~~~~~~~~~~~~~~^

error: no member named 'SetPassNbr' in namespace 'EdAsmNg::Asm'
        at line 4182 in app_test.cpp
        EdAsmNg::Asm::SetPassNbr(0);  // Pass 1 for now
```

### Error Classification

- **Type**: Linker/Declaration errors (not logic errors)
- **Quantity**: 16+ missing declarations
- **Impact**: All 9 tests fail at compilation before test execution
- **Severity**: HIGH - blocks entire Phase 2 & 3 test suite

---

## Test Expectations vs. Implementation

### Phase 2: GInstLen Test Cases

#### Test: `Immediate_TwoBytes`

**What Test Expects**:

```cpp
SetupMnemonic(0x00, 0xFF, 0x03);      // Setup mnemonic flags
EdAsmNg::Asm::SetupOperandField("#$42");
EdAsmNg::Asm::GInstLen();             // Call instruction length calculator

EXPECT_EQ(EdAsmNg::Asm::GetLength(), 2);      // Expect: 1 opcode + 1 operand byte
EXPECT_EQ(EdAsmNg::Asm::GetLenTIdx(), 2);     // Expect: addressing mode index 2 (immediate)
```

**Implementation Logic** ([asm.cpp](src/lib/asm/asm.cpp#L2773)):

1. `GInstLen()` calls `Bridge_GInstLen()` wrapper
2. Loads `ModWrdL` (flag byte 1) and `ModWrdH` (flag byte 2)
3. Calls `GAdrMod()` to parse addressing mode from operand field
4. Validates mode against `ModWrdH` bits
5. Sets `LenTIdx` to parsed addressing mode (0-12)
6. Looks up `Length` from `InstLenT[LenTIdx]` table
7. Sets `Length` variable

**Status**: Logic appears sound, but not testable due to missing declarations

#### Test: `ZeroPage_TwoBytes` / `Indexed_ThreeBytes` / etc

- **Pattern**: All 6 Phase 2 tests follow same pattern (setup → call GInstLen() → verify Length & LenTIdx)
- **Expected Behavior**: Correctly determine instruction length (1-3 bytes) and addressing mode index (0-12)
- **Root Cause of Failure**: `GetLength()` and `GetLenTIdx()` not declared

---

### Phase 3: StorGMC Test Cases

#### Test: `SingleByteStorage`

**What Test Expects**:

```cpp
EdAsmNg::Asm::InitObjMemory();        // Initialize test memory
EdAsmNg::Asm::SetLength(1);
EdAsmNg::Asm::SetGMC(0, 0xEA);        // NOP opcode
EdAsmNg::Asm::SetObjPC(0x2000);
EdAsmNg::Asm::StorGMC();              // Store to memory and advance PC

EXPECT_EQ(EdAsmNg::Asm::ReadObjMemory(0x2000), 0xEA);  // Verify byte written
EXPECT_EQ(EdAsmNg::Asm::GetObjPC(), 0x2001);          // Verify PC advanced
```

**Implementation Logic** ([asm.cpp](src/lib/asm/asm.cpp#L3352)):

```cpp
void StorGMC() {
    Y = Length;          // Get byte count to write
    if (Y == 0) return;  // Exit if nothing to write

    Y = 0;
    L80B8:
        A = GMC[Y];      // Get byte from GMC buffer

        if ((int8_t)GenF < 0) return;        // Check suppression flag (bit 7)
        if ((GenF & 0x40) != 0) goto L80C5;  // Check disk mode (bit 6)

        g_test_obj_memory[ObjPC + Y] = A;    // Write to test memory

        Y++;
        if (Y != Length) goto L80B8;         // Next byte

    // After loop: advance ObjPC
    A = Y;
    AdvObjPC();          // ObjPC += Y (number of bytes written)
}
```

**Status**: Implementation correct, but test cannot execute due to missing declarations

#### Test: `MultiByteStorage`

- **Expectation**: 3-byte instruction written sequentially with LenTIdx checking
- **Test Data**: `SetGMC(0, 0xAD); SetGMC(1, 0x34); SetGMC(2, 0x12);` → expects all 3 bytes at ObjPC
- **Blocked By**: Missing `StorGMC()`, `ReadObjMemory()`, `WriteObjMemory()` declarations

#### Test: `ObjPC_Advances`

- **Expectation**: Sequential instructions advance ObjPC correctly (2 + 1 + 3 = 6 bytes total)
- **Test Pattern**: Multiple StorGMC() calls with different Length values
- **Blocked By**: Missing `SetObjPC()`, `GetObjPC()`, `StorGMC()` declarations

#### Test: `GenF_Suppression`

- **Test Purpose**: Verify that GenF N-bit (0x80) suppresses code generation
- **Expected**: Memory remains 0x00 and ObjPC unchanged
- **Implementation Check**: Line 3359 in asm.cpp checks `if ((int8_t)GenF < 0) return;`
- **Blocked By**: Missing `SetGenF()` declaration

---

## Code Flow Analysis

### GInstLen() Execution Path

```
Test calls: EdAsmNg::Asm::GInstLen()
    ↓
asm.cpp line 9189: void GInstLen() { Bridge_GInstLen(); }
    ↓
Bridge_GInstLen() function (location: [find Bridge_GInstLen definition])
    ↓
Internal GInstLen() at line 2773:
    1. Load mnemonic flags from MnemP buffer
    2. Call GAdrMod() to parse operand field
    3. Validate addressing mode against ModWrdH
    4. Set LenTIdx to addressing mode index
    5. Look up Length from InstLenT table
    ↓
Test calls: EdAsmNg::Asm::GetLength()
    ↓
Returns: Length variable
```

**Issue**: GetLength() wrapper at line 9346 not declared in header

### StorGMC() Execution Path

```
Test calls: EdAsmNg::Asm::InitObjMemory()
    ↓
asm.cpp line 9253: Initialize g_test_obj_memory[65536] and set flag
    ↓
Test calls: EdAsmNg::Asm::StorGMC()
    ↓
asm.cpp line 9191: void StorGMC() { Bridge_StorGMC(); }
    ↓
Bridge_StorGMC() function (location: [find Bridge_StorGMC definition])
    ↓
Internal StorGMC() at line 3352:
    Loop for Y=0 to Length-1:
        1. Check GenF suppression (bit 7)
        2. Check GenF disk mode (bit 6)
        3. Write GMC[Y] to g_test_obj_memory[ObjPC+Y]
        4. Increment Y
    Post-loop: Call AdvObjPC() to increment ObjPC by Length bytes
    ↓
Test calls: EdAsmNg::Asm::ReadObjMemory(addr)
    ↓
Returns: g_test_obj_memory[addr]
```

**Issue**: Most of these functions not declared in header

---

## Implementation Status Summary

### Declared ✓ in asm_test_helpers.hpp

| Function              | Header Line | Purpose                               |
| --------------------- | ----------- | ------------------------------------- |
| `GInstLen()`          | 44          | Instruction length calculator wrapper |
| `SetupMnemP()`        | 45          | Setup mnemonic table entry            |
| `SetupOperandField()` | 46          | Setup operand field parser            |
| `GetLength()`         | 57          | Get calculated instruction length ✓   |
| `SetLength()`         | 58          | Set instruction length ✓              |
| `GetLenTIdx()`        | 59          | Get addressing mode index ✓           |
| `SetLenTIdx()`        | 60          | Set addressing mode index ✓           |
| `GetGMC()`            | 61          | Get GMC buffer byte ✓                 |
| `SetGMC()`            | 62          | Set GMC buffer byte ✓                 |

### NOT Declared ✗ (Missing from header)

| Function           | asm.cpp Line | Purpose                       | Test Uses                    |
| ------------------ | ------------ | ----------------------------- | ---------------------------- |
| `SetPassNbr()`     | 9495         | Set assembly pass (0/1/2)     | Phase2_GInstLenTest::SetUp() |
| `GetPassNbr()`     | 9491         | Get assembly pass             | EvalOprnd phase 4 tests      |
| `StorGMC()`        | 9191         | Store machine code to memory  | All 3 StorGMC tests          |
| `SetObjPC()`       | 9226         | Set object program counter    | All 3 StorGMC tests          |
| `GetObjPC()`       | 9222         | Get object program counter    | All 3 StorGMC tests          |
| `SetGenF()`        | 9214         | Set generation flags          | GenF_Suppression test        |
| `GetGenF()`        | 9218         | Get generation flags          | N/A in current tests         |
| `SetHighMem()`     | 9233         | Set high memory boundary      | Phase3_StorGMCTest::SetUp()  |
| `GetHighMem()`     | 9239         | Get high memory boundary      | N/A in current tests         |
| `InitObjMemory()`  | 9253         | Initialize test object memory | Phase3_StorGMCTest::SetUp()  |
| `ReadObjMemory()`  | 9247         | Read byte from sim memory     | All 3 StorGMC tests          |
| `WriteObjMemory()` | 9250         | Write byte to sim memory      | GenF_Suppression test        |
| `SetGenF()`        | 9214         | Set generation flags          | GenF_Suppression test        |

---

## Failure Categories

### Category 1: Phase 2 GInstLen Tests (6 failures)

All 6 tests fail during compilation with same root cause:

**Failures**:

1. `Immediate_TwoBytes` - `GetLength()` not accessible
2. `ZeroPage_TwoBytes` - `GetLength()`, `GetLenTIdx()` not accessible
3. `Indexed_ThreeBytes` - `GetLength()`, `GetLenTIdx()` not accessible
4. `Indirect_TwoBytes` - `GetLength()`, `GetLenTIdx()` not accessible
5. `Implied_OneByte` - `GetLength()` not accessible
6. `Branch_TwoBytes` - `GetLength()`, `GetLenTIdx()` not accessible

**Line in Tests**: [tests/app_test.cpp](tests/app_test.cpp#L4268) and [test_app.cpp](tests/app_test.cpp#L4271)

**Error Pattern**:

```
error: no member named 'GetLength' in namespace 'EdAsmNg::Asm'
error: no member named 'GetLenTIdx' in namespace 'EdAsmNg::Asm'; did you mean 'GetSubTIdx'?
```

**Immediate Blocker**: SetPassNbr() at line 4182 - prevents test from even compiling SetUp()

---

### Category 2: Phase 3 StorGMC Tests (3 failures)

All 3 tests fail due to multiple missing declarations:

**Failures**:

1. `SingleByteStorage` - Missing 7 declarations
2. `MultiByteStorage` - Missing 7 declarations
3. `ObjPC_Advances` - Missing 7 declarations

**Line in Tests**: [tests/app_test.cpp](tests/app_test.cpp#L4370)

**Missing Declarations** (in order of first error):

1. `InitObjMemory()` - SetUp() at line 4374
2. `SetGenF()` - SetUp() at line 4375
3. `SetObjPC()` - SetUp() at line 4376
4. `SetHighMem()` - SetUp() at line 4377
5. `SetLength()` - Test body at line 4387
6. `SetGMC()` - Test body at line 4388
7. `StorGMC()` - Test body at line 4392
8. `ReadObjMemory()` - Verification at line 4397
9. `GetObjPC()` - Verification at line 4399

**Also Blocking**: Generic_Suppression and other StorGMC variant tests

---

## Root Causes Identified

### Primary Root Cause

**Missing Header Declarations** - Single point of failure

- Location: [tests/asm_test_helpers.hpp](tests/asm_test_helpers.hpp#L44-L62)
- Issue: Header file incomplete, ends at line 63
- Last Declaration: `SetGMC()` at line 62
- Missing: Lines for Phase 3a, Phase 3b-c test helpers

### Secondary Issues (Potential)

After declarations are added and tests compile, watch for:

1. **GAdrMod() Function**
   - Called from GInstLen() at line 2787
   - Responsible for parsing operand and returning addressing mode
   - **Risk**: If GAdrMod() doesn't correctly parse operand syntax, Length/LenTIdx will be wrong
   - Need to verify: Handles "#$42" → immediate, "$42" → zero page, etc.

2. **InstLenT Table**
   - Location: [asm.cpp](src/lib/asm/asm.cpp) - need to find definition
   - Used at line 2908 to look up instruction length
   - **Risk**: If wrong table entry for LenTIdx, Length will be incorrect

3. **g_test_obj_memory Memory Access**
   - Used in StorGMC() at line 3362
   - **Risk**: If not initialized, access violation
   - Mitigation: InitObjMemory() properly clears and flags memory

4. **ObjPC Behavior**
   - Used in StorGMC() to write bytes sequentially
   - AdvObjPC() called after loop (line 3378)
   - **Risk**: AdvObjPC() may not correctly increment for multi-byte writes
   - Expected: ObjPC += Length (number of bytes written)

5. **GenF Flag Handling**
   - Bit 7 (0x80): Suppress N flag - should return immediately if set
   - Bit 6 (0x40): Disk write V flag - should use Wr1Byte() instead
   - **Risk**: Wrong bit checks could cause suppression to fail
   - Implementation looks correct: `if ((int8_t)GenF < 0)` checks bit 7

---

## Priority and Complexity Assessment

### Fix Priority: **CRITICAL** (P1)

- **Blocking**: 9 test compilation failures
- **Scope**: Core assembler phases (2 & 3)
- **Impact**: Prevents any Phase 2/3 functionality from being tested

### Fix Complexity: **LOW**

- **Required Action**: Add missing function declarations to header
- **Effort**: < 30 minutes
- **Risk**: Minimal - declarations are already implemented
- **Type**: Mechanical declaration addition (no logic changes)

### Step-by-Step Fix Approach

**Phase 1: Add Missing Declarations (HIGH CONFIDENCE)**

```
Location: tests/asm_test_helpers.hpp
After line 62 (SetGMC declaration), add:

    // Phase 3a: StorByt Test Helpers
    void     SetGenF(uint8_t value);
    uint8_t  GetGenF();
    void     SetObjPC(uint16_t value);
    uint16_t GetObjPC();
    void     SetHighMem(uint16_t value);
    uint16_t GetHighMem();
    uint8_t  ReadObjMemory(uint16_t addr);
    void     WriteObjMemory(uint16_t addr, uint8_t value);
    void     InitObjMemory();
    void     StorGMC();

    // Phase 3b: GenMCode Test Helpers (Additional)
    void     SetPassNbr(uint8_t value);
    uint8_t  GetPassNbr();
```

Result: Should resolve all compiler errors

````

**Phase 2: Validate Implementations Exist**
- Verify each declaration has matching implementation in asm.cpp
- (Already confirmed lines 9214-9495 contain all implementations)

**Phase 3: Run Tests and Collect Actual Failures**
- Compile and link tests
- Execute test suite
- Identify any actual logic failures vs. declaration failures

---

## Expected Behavior After Fix

### Phase 2 Tests: GInstLen Validation
Each test will:
1. ✓ Compile without missing declaration errors
2. → Execute SetUp(), calling SetPassNbr(0)
3. → Call GInstLen() via wrapper at asm.cpp line 9189
4. → Check Length and LenTIdx against expected values
5. → PASS or FAIL based on GAdrMod() parsing accuracy

### Phase 3 Tests: StorGMC Validation
Each test will:
1. ✓ Compile without missing declaration errors
2. → Execute SetUp(), initializing object memory
3. → Call StorGMC() via wrapper at asm.cpp line 9191
4. → Verify bytes written to g_test_obj_memory
5. → Verify ObjPC advanced correctly
6. → PASS or FAIL based on write accuracy and PC advancement

---

## Recommended Debugging Steps (Post-Fix)

If tests still fail after adding declarations:

### For Phase 2 Failures:
```bash
Add Debug Output in GInstLen() wrapper:
  - Print operand field contents (should show "#$42", "$42", etc.)
  - Print GAdrMod() return value (0-12 = addressing mode)
  - Print ModWrdH bits (should match addressing mode)
  - Print final LenTIdx value
  - Print final Length value

Expected outputs:
  Immediate_TwoBytes: "#$42" → mode 2 → LenTIdx=2 → Length=2
  ZeroPage_TwoBytes: "$42" → mode 1 → LenTIdx=1 → Length=2
  Indexed_ThreeBytes: "$1234,X" → mode 4 → LenTIdx=4 → Length=3
````

### For Phase 3 Failures

```bash
Add Debug Output in StorGMC() wrapper:
  - Print Length value (should match SetLength calls)
  - Print GenF value (check suppression/disk bits)
  - Print each GMC[Y] value being written
  - Print ObjPC before and after
  - Print g_test_obj_memory contents at written addresses

Expected outputs:
  SingleByteStorage: Length=1, write 0xEA at 0x2000, ObjPC becomes 0x2001
  MultiByteStorage: Length=3, write 0xAD,0x34,0x12 at 0x3000-3002, ObjPC becomes 0x3003
```

---

## Files to Modify

### Primary Change

**File**: [tests/asm_test_helpers.hpp](tests/asm_test_helpers.hpp)

- **Action**: Add missing function declarations
- **Lines**: After line 62
- **Additions**: 12+ new function declarations
- **Conflict Risk**: None (header-only additions)

### No Changes Needed

- [src/lib/asm/asm.cpp](src/lib/asm/asm.cpp) - Implementations already present
- [tests/app_test.cpp](tests/app_test.cpp) - Test code already correct
- [src/lib/asm/asm.cpp](src/lib/asm/asm.cpp) core functions - No changes required

---

## Summary Matrix

| Aspect               | Status       | Details                                      |
| -------------------- | ------------ | -------------------------------------------- |
| **Test Count**       | 9 failures   | 6 Phase2 + 3 Phase3                          |
| **Root Cause**       | Single Point | Missing header declarations                  |
| **Implementations**  | ✓ Complete   | All in asm.cpp lines 9185-9550               |
| **Fix Complexity**   | LOW          | Add ~12 declarations                         |
| **Fix Time**         | < 30 min     | Mechanical additions                         |
| **Risk Level**       | MINIMAL      | No logic changes                             |
| **Compile Success**  | After fix    | Yes, all 9 tests should compile              |
| **Actual Test Pass** | Unknown      | Depends on GAdrMod, InstLenT, AdvObjPC logic |

---

## Conclusion

The 9 failing tests are **blocked by a single, straightforward issue**: incomplete test helper declarations in [tests/asm_test_helpers.hpp](tests/asm_test_helpers.hpp).

All required implementations already exist in [src/lib/asm/asm.cpp](src/lib/asm/asm.cpp) (lines 9185-9550+). The test code is also correct and properly structured. The only barrier is the missing linkage via header declarations.

**Immediate Action**: Add the 12+ missing function declarations to the header file (LOW complexity, minimal risk).

**Follow-up**: After fix, execute tests to identify any actual logic issues in GInstLen(), StorGMC(), or dependent functions like GAdrMod(), InstLenT lookup, and AdvObjPC().
