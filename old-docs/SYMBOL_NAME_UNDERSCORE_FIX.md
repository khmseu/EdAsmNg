# Symbol Name Underscore Support - Fix Summary

## Problem

The last failing test (`Phase84Pass1Test.test_pass1_label_in_dummy_section_behavior`) was checking for the DSECT relative bit in symbol flags for `DUMMY_LABEL`, but GetSymbolFlags was returning 0 (symbol not found).

## Root Cause Analysis

Through systematic debugging with targeted debug output, we discovered TWO distinct issues:

### Issue 1: Underscore Not Allowed in Symbol Names

- **Symptom**: Symbol `DUMMY_LABEL` was stored as `DUMMY` (truncated at underscore)
- **Root Cause**: `CharMap1[0x5F]` (underscore '\_') was set to `0x01`, marking it as non-alphanumeric
- **Impact**: AddNode's loop `while (C=0)` stopped storing chars when it hit the underscore
- **Fix**: Changed `CharMap1[0x5F]` from `0x01` to `0x00` to mark underscore as alphanumeric

### Issue 2: DCI Encoding Match Logic Error in GetSymbolFlags

- **Symptom**: Even after fixing underscore support, GetSymbolFlags still couldn't match symbols with exact DCI encoding
- **Root Cause**: The matching logic was stripping MSB from BOTH non-last and last characters, but DCI encoding stores:
  - Non-last chars WITH MSB set (e.g., 'D' = 0xC4)
  - Last char WITHOUT MSB (e.g., 'L' = 0x4C, not 0xCC)
- **Impact**: For `DUMMY_LABEL`, last char 'L' is stored as 0x4C, but we were comparing `ch != (stored & 0x7F)` which would match 0x4C or 0xCC
- **Fix**: Changed last-char comparison to `ch != stored` (direct comparison without masking)

## Solution Implementation

### CharMap1 Update (Lines 8072-8083)

```cpp
// 0x5B-0x60: '[' to '`' (6 bytes) - Updated: '_' (0x5F) is now alphanumeric (0x00)
0x01,  // 0x5B: '['
0x01,  // 0x5C: '\'
0x01,  // 0x5D: ']'
0x01,  // 0x5E: '^'
0x00,  // 0x5F: '_' - alphanumeric to allow underscores in symbol names
0x01,  // 0x60: '`'
```

### GetSymbolFlags DCI Match Fix (Lines 10098-10110)

```cpp
for (size_t i = 0; i < name_len; i++) {
  uint8_t ch     = name[i];
  uint8_t stored = name_ptr[idx];
  if (i == name_len - 1) {
    // Last char - stored WITHOUT MSB set
    if (ch != stored) {  // Direct comparison (was: ch != (stored & 0x7F))
      match = false;
      break;
    }
  } else {
    // Not last char - stored WITH MSB set, strip it for comparison
    if (ch != (stored & 0x7F)) {
      match = false;
      break;
    }
  }
  idx++;
}
```

## Test Results

**Before Fix**: 95/96 tests passing (1 failure)
**After Fix**: 96/96 tests passing (100%)

## Technical Notes

### DCI Encoding Format

DCI (Dextral Character Inverted) encoding is used for symbol names in the symbol table:

- All characters have MSB (bit 7) SET, except the last character
- Example: `DUMMY_LABEL` → `0xC4 0xD5 0xCD 0xCD 0xD9 0xDF 0xCC 0xC1 0xC2 0xC5 0x4C`
- Last 'L' = 0x4C (no MSB), all others have MSB set

### Symbol Name Character Rules (Updated)

After this fix, valid symbol name characters include:

- Letters: A-Z, a-z (case-insensitive, stored uppercase)
- Digits: 0-9 (not as first character)
- **Underscore: \_** (NOW SUPPORTED)

## Benefits

This enhancement brings EdAsmNg closer to modern assembler conventions where underscores in symbol names are standard practice, improving code readability and compatibility with contemporary development practices.
