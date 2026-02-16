# Investigation Report: DW Directive Pass 2 Loop Issue

## Problem Statement

The DW (Define Word) directive handler's Pass 2 loop stops after entering without executing loop body code. Debug evidence shows:

- "Entering Pass 2 loop" message **IS** printed (line 4549)
- "Top of loop" message **NEVER** printed (line 4558)
- "In Pass 2 block" message **NEVER** printed (line 4568)
- No bytes emitted, no RLD entries created
- PC/ObjPC never advance beyond initial value
- RLD count remains 0

## Code Location

**File:** [src/lib/ei/asm.cpp](src/lib/ei/asm.cpp)
**Primary Handler:** [HndlMnem() - Phase 8.4](src/lib/ei/asm.cpp#L4062)
**DW Handler Block:** [lines 4507-4600](src/lib/ei/asm.cpp#L4507-L4600)
**DW Pass 2 Loop:** [lines 4551-4596](src/lib/ei/asm.cpp#L4551-L4596)

## Critical Code Flow Analysis

### DW Handler Entry Point (lines 4507-4520)

```cpp
if (mnemonic == "DW" || mnemonic == ".WORD" || mnemonic == ".DW") {
  ZAB = 0x80;  // Mark as directive
  // ... setup code ...

  fprintf(stderr, "DW handler: PassNbr=%d, RelCodeF=0x%02X, Y=%d, ch=0x%02X '%c'\n",
          PassNbr, RelCodeF, Y, SrcP_at(Y), SrcP_at(Y));

  if (PassNbr == 0) {
    // Pass 1: count operands and advance PC
    // ... lines 4521-4545 ...
    return;  // Line 4545
  }

  // REACH HERE ONLY IN PASS 2 (PassNbr == 1)
```

### Pass 2 Entry (lines 4548-4600)

**Line 4549-4551:** Debug printf executed successfully (user confirms this appears in output)

**Line 4551:** `while (true) {` - Loop starts
**Problem Location:** Loop body doesn't execute

## Investigation Findings

### 1. **While Loop Structure is Syntactically Correct**

- Confirmed: No rogue semicolon after `while (true)` at line 4551
- Properly formed brace-delimited block
- Loop would normally execute for `while(true)` unless exceptional conditions

### 2. **First Iteration Should Execute**

In the test case " DW START\r":

- Y=3 positioned at 'S' (start of START symbol)
- Space-skip loop (line 4554-4555) should NOT iterate (next char is 'S', not space)
- Character read at line 4557: ch = 'S' (0x53)
- **Debug printf at line 4558 MUST execute** - it's unconditional

### 3. **The "Entering Pass 2 loop" Printf Succeeds**

This proves:

- Code reaches line 4549-4551 ✓
- SrcP and Y are positioned correctly ✓
- Memory access via SrcP_at(Y) works ✓
- Fprintf to stderr functions correctly ✓

### 4. **Loop Body Never Executes - Four Possible Root Causes**

#### **ROOT CAUSE #1: Compiler Optimization or Control Flow Issue**

**Likelihood:** LOW (but possible)

If the while loop body is somehow being skipped by the compiler, this would be:

- A compiler bug
- Undefined behavior triggering unexpected optimization
- Invalid generated code

**Evidence Against:** The fprintf at 4549 executes successfully, showing normal execution flow up to that point.

#### **ROOT CAUSE #2: An Early Exit Before Loop Body (MOST LIKELY)**

**Likelihood:** MEDIUM

There could be code execution taking a different path between lines 4551 and 4558. Possibilities:

- **Infinite loop in space-skipping** (line 4554-4555): If Y overflows or loops indefinitely
- **Memory access fault** (line 4557): Reading ch could crash if Y points to invalid memory
- **Undefined behavior** causing abort or signal

**Critical Check Needed:** Does the inner while loop at line 4554 ever terminate?

```cpp
while (SrcP_at(Y) == ' ' || SrcP_at(Y) == '\t') Y++;
```

With Y initialized at position 3 ('S'), this loop should:

1. Evaluate: SrcP_at(3) == ' ' → false
2. Evaluate: SrcP_at(3) == '\t' → false
3. Exit loop immediately
4. Continue to line 4557

**BUT IF** Y wraps (Y is uint8_t, 0-255):

- If Y keeps incrementing past valid source line, we might have infinite loop
- Source line " DW START\r" is ~14 bytes, so Y won't naturally overflow to revisit spaces

#### **ROOT CAUSE #3: Incorrect Source Line Position Management**

**Likelihood:** MEDIUM-HIGH

The Y register position tracking may be incorrect:

**Before Pass 2 Loop (line 4509-4511):**

```cpp
while (SrcP_at(Y) == ' ' || SrcP_at(Y) == '\t') Y++;  // Line 4509-4510
uint16_t wordCount = 0;  // Line 4511
```

**Issue:** Between Pass 1 and Pass 2...

- Pass 1 processes the entire source line (lines 4521-4545)
- After Pass 1 completes and returns, source is **rewound** by test
- Pass 2 is called AGAIN with same source
- But Y register is **NOT reset** between passes!

**The Real Problem:** When Pass 2 handler is entered:

- The HndlMnem function extracts the mnemonic starting from THE BEGINNING of the line
- But Y register state from the PREVIOUS Pass 1 call might not be reset
- Initial Y position calculation (lines 4509-4510) might be operating on wrong Y

#### **ROOT CAUSE #4: Y Register State Corruption Between Pass 1 and Pass 2** (MOST LIKELY)

**Likelihood:** HIGH

**Suspected Issue:**

1. **Pass 1 execution** (line 4521-4545): Processes "DW START", Y advances through the line
2. **Pass 1 returns** at line 4545
3. **Test code rewinds source** calling RewindSource()
4. **Pass 2 handler called** - HndlMnem processes the SAME source line AGAIN
5. **Mnemonic extraction** at lines 4066-4079 starts from Y=0 (should be)
6. **Y positioning after mnemonic** at lines 4509-4510 should position Y at 'S'
7. **BUT** if Y is not properly reset...

**Critical Questions:**

- Is Y reset to 0 when source is rewound?
- Is Y reset at the start of HndlMnem processing?
- Does the mnemonic extraction properly initialize Y?

### 5. **EvalExpr() Never Called (Line 4562)**

**Evidence:** No "In Pass 2 block" output means line 4562+ is never reached

The fprintf at line 4568 would only be skipped if:

- EvalExpr() at line 4562 is never called
- Which means line 4558 (the loop body printf) is never reached
- Which means the while(true) loop body is skipped entirely

### 6. **RLD Entry Never Created**

**Evidence from AddRLDEnt (line 3446-3481):**

```cpp
fprintf(stderr, "AddRLDEnt: RelCodeF=0x%02X, (int8_t)RelCodeF=%d\n",
        RelCodeF, (int8_t)RelCodeF);

if ((int8_t)RelCodeF >= 0) {  // Line 3450
  fprintf(stderr, "AddRLDEnt: Early return - not in REL mode\n");
  return;
}
```

**Potential Issue:** If RelCodeF is not properly set:

- REL directive should set MSB: `RelCodeF = (RelCodeF >> 1) | 0x80;` (line 4297)
- But if this isn't executed or is overwritten, then AddRLDEnt returns early
- RLD count stays 0, PC doesn't advance

## Diagnosis Summary

### **PRIMARY ROOT CAUSE: Y Register/Source Position Management Failure**

The most likely scenario:

1. Y register state is incorrect when Pass 2 DW handler executes
2. Y position calculated at line 4509-4510 doesn't align with actual operand location
3. The space-skipping loop at 4554-4555 either:
   - Executes infinitely (if Y calculation is wrong and off-by-N)
   - Corrupts Y in unexpected way
   - Causes memory access fault

### **SECONDARY ISSUES TO VERIFY**

1. **Source Rewind Issue:**
   - RewindSource() called between Pass 1 and Pass 2 (line 4040 in test, approximately)
   - Y register **NOT examined** if it's reset properly after rewind

2. **Y Register Initialization:**
   - HndlMnem starts mnemonic parsing from Y=0 (line 4068-4079)
   - After mnemonic extraction, Y should be positioned after mnemonic
   - Space-skip at line 4509-4510 positions Y at operand start
   - **CRITICAL:** Is Y=0 guaranteed at start of HndlMnem call?

3. **RelCodeF State:**
   - Pass 1 REL directive execution at line 4294-4299 sets `RelCodeF |= 0x80`
   - Must verify RelCodeF persists correctly between passes
   - AddRLDEnt checks `(int8_t)RelCodeF >= 0` - this checks if MSB is SET
   - If check is inverted, early return triggers incorrectly

## Specific Lines That Require Debugging

| Line Range | Code                           | Issue to Verify                                    |
| ---------- | ------------------------------ | -------------------------------------------------- |
| 4509-4510  | Space-skip after mnemonic      | Does this position Y correctly? Does Y start at 0? |
| 4549-4551  | First debug printf             | ✓ Confirmed working                                |
| 4551-4555  | While loop + space-skip        | Does inner loop hang/overflow?                     |
| 4557       | `ch = SrcP_at(Y)`              | Does this execute? Invalid memory access?          |
| 4558       | Debug printf "Top of loop"     | ✗ Never prints                                     |
| 4562       | EvalExpr() call                | Never reached                                      |
| 4568       | Debug printf "In Pass 2 block" | Never printed - confirms line 4562 not reached     |
| 3450-3452  | AddRLDEnt early return check   | Is RelCodeF properly set from Pass 1?              |
| 4294-4299  | Pass 1 REL directive handling  | Is RelCodeF correctly set with MSB?                |

## CRITICAL DISCOVERY: Loop Body Should Execute

After detailed analysis of the exact code structure at lines 4551-4559, the space-skipping while loop contains no side effects other than incrementing Y. Under normal circumstances with Y=3 pointing to 'S', the loop should NOT iterate.

**Strong Hypothesis - Infinite Space-Skip Loop:**
The inner while loop at lines 4553-4555 is executing **infinitely**, preventing the fprintf at line 4558 from executing. This would occur if:

- SrcP pointer is pointing to wrong memory location in Pass 2
- SrcP_at(Y) continuously returns space character(s) instead of 'S'
- Loop increments Y indefinitely looking for non-space character

**Root Cause Theory - SrcP Pointer Corruption:**
The critical issue involves 16-bit pointer arithmetic in AdvSrcP() function. When RewindSource() resets SrcP in Pass 2, if subsequent NxtField() calls don't properly restore the source line position, SrcP could point to wrong location, causing space-skip loop to hang.

## Test Case Details

**File:** [tests/app_test.cpp](tests/app_test.cpp)
**Test:** [DW_RelocatableSymbol_CreatesRLDEntry (line 4028)](tests/app_test.cpp#L4028)
**Source:**

```
      REL          <- Sets RelCodeF with MSB at line 4297
START EQU $2000   <- Creates relocatable symbol
      DW START     <- Should emit 2 bytes + RLD entry in Pass 2
```

**Expected Behavior:**

- Pass 1: DW counts 1 operand, advances PC by 2 → PC = 0x1002 ✓
- Pass 2: DW emits 2 bytes, creates RLD entry → ObjPC = 0x1002, RLDCount > 0 ✗

**Actual Behavior:**

- Pass 1: ✓ Works (PC advances correctly)
- Pass 2: ✗ Loop doesn't execute, nothing emitted

## Comparison with DFB Handler

The DFB (Define Byte) handler at lines [4405-4503](src/lib/ei/asm.cpp#L4405-L4503) has similar structure but:

- **No debug output in Pass 2 loop** (unlike DW)
- Same potential Y register issues
- User confirms DFB also fails

## CRITICAL DISCOVERY: Loop Body Should Execute

After detailed analysis of the exact code structure (lines 4551-4559):

```cpp
while (true) {
  // Skip spaces
  while (SrcP_at(Y) == ' ' || SrcP_at(Y) == '\t') Y++;

  uint8_t ch = SrcP_at(Y);
  fprintf(stderr, "  Top of loop: Y=%d, ch=0x%02X\\n", Y, ch);  // Line 4557
```

**Key Finding:** The space-skipping while loop at lines 4553-4555 contains NO side effects other than incrementing Y. The fprintf at line 4557 is unconditional - it will always execute after the space loop, regardless of the condition.

**Strong Hypothesis:** The inner while loop (space-skipping) is executing **infinitely**:

- Y starts at value 3 (pointing to 'S')
- SrcP_at(3) should return 'S' (not space)
- Loop should NOT iterate
- **BUT** if SrcP is pointing to wrong memory location, SrcP_at(Y) might return space continuously
- This creates infinite loop, preventing fprintf execution

**Root Cause Theory:**

```
BEFORE Pass 2:
- RewindSource() sets SrcP = g_test_src_base (correct)

DURING Pass 2:
- NxtField() is called, which calls AdvSrcP()
- AdvSrcP() advances SrcP by Y bytes AND sets Y=0
- But AdvSrcP() uses ADDITION on SrcP:
  SrcP = (SrcP & 0xFF00) | ((SrcP & 0xFF) + Y)  // 16-bit pointer arithmetic

POTENTIAL BUG:
- If Y is 0 when AdvSrcP() is called, SrcP doesn't advance
- If the addition wraps (SrcP + Y > 0xFF), the logic might be wrong
- The high byte update: if (temp > 0xFF) SrcP += 0x100
- But this uses += 0x100, not proper 16-bit carry
```

**Test Case SrcP Expected Values:**

- g_test_src_base = start of source buffer
- After NxtField() in Pass 1: SrcP += 4 (to skip " " label area)
- After Pass 1 processing: SrcP has accumulated offsets
- RewindSource() should reset: SrcP = g_test_src_base
- After NxtField() in Pass 2: SrcP += 4 again (should be same as Pass 1)

**What if SrcP is incorrect in Pass 2?**

- If SrcP points to wrong memory (e.g., still pointing mid-source from Pass 1)
- Then SrcP_at(Y) reads from wrong location
- The inner while loop could read SPACE characters indefinitely
- Prevents loop body execution

## Recommendations for Fix Implementation

1. **Add Y register logging** at each critical point:
   - Start of HndlMnem
   - After mnemonic extraction
   - After space-skip at line 4510
   - Inside while loop body (already have this)

2. **Verify Y state persistence:**
   - Check if Y=0 at start of each Pass 2 handler call
   - Log Y value before and after RewindSource()

3. **Investigate source position:**
   - Add logging to show actual byte at position Y
   - Verify SrcP pointer hasn't changed between passes

4. **Check RelCodeF handling:**
   - Verify REL directive sets MSB correctly
   - Log RelCodeF value in AddRLDEnt
   - Verify symbol Flags include "relative" flag correctly

5. **Review mnemonic extraction logic:**
   - Ensure Y is properly initialized before extraction
   - Confirm space-skip positioning is correct

## Files Referenced

- [src/lib/ei/asm.cpp](src/lib/ei/asm.cpp) - Main handler code
- [tests/app_test.cpp](tests/app_test.cpp) - Test cases
- [src/lib/ei/app.hpp](src/lib/ei/app.hpp) - Header definitions (API)
