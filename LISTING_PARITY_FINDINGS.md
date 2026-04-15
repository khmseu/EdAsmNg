## Listing Parity Expansion - Technical Findings

### Current Achievement

- **6 of 7** available comparative fixtures achieve zero-diff listing compliance
- **180 of 180** unit tests passing
- Green fixtures: input.src, input2.src, input3.src, branch.src, equexpr.src, fwdjmp.src

### Discovered Issues

#### 1. simple_test.asm Object Code Parity (Blocker)

**Status:** Unresolved - prevents listing comparison
**Symptoms:**

- Object file format differences at offsets 1-2, 13, 15, 30-34
- Byte 13 (branch displacement): `FD` expected vs `FA` generated
- Likely object file header/relocation metadata differences

**Impact:** Prevents 7th fixture from being included in listing comparisons

#### 2. Zero Page Addressing Mode Bug (New Discovery)

**Status:** Unresolved - exposed during expansion attempt
**Symptoms:**

- `LDA $80` (zero page) generates `A9 00` (LDA #$00) instead of `A5 80`
- Indicates operand parsing issue in directive/mnemonic handling
- Affects any fixture relying on zero page direct addressing

**Test Case:**

```asm
      ORG $0800
      LDA $80
      RTS
```

Expected: `A5 80 60` (LDA $80, RTS)
Actual: `A9 00 60` (LDA #$00, RTS)

**Impact:** Cannot create additional test fixtures using zero page addressing until this is fixed

### Addressing Modes Under Test

- ✅ **Immediate** (#$NN) - Working (all 6 green fixtures use this)
- ✅ **Implied** - Working (NOP, RTS, etc. in green fixtures)
- ✅ **Absolute** ($NNNN) - Working (STA $C000 in branch.src)
- ✅ **Absolute,X** ($NNNN,X) - Working (in green fixtures)
- ✅ **Branch relative** - Working (BNE LOOP in branch.src)
- ✅ **Indirect** (JMP ($NNNN)) - Partially working (fwdjmp.src passes)
- ❌ **Zero Page** ($NN) - **BROKEN** (generates immediate instead)
- ❌ **Zero Page,X** ($NN,X) - **POTENTIALLY BROKEN** (not tested)
- ❌ **Indirect Y** (($NN),Y) - **POTENTIALLY BROKEN** (not tested)

### Root Cause Analysis

The zero page addressing bug suggests an issue in operand parsing where `$80` (zero page address) is being misinterpreted. Possible causes:

1. Operand parser confusing `$` zero page prefix with expression evaluation
2. Addressing mode detection not correctly identifying zero page vs immediate
3. ASC/DCI directive handler changes may have affected operand parsing flow

### Recommended Next Steps

1. **Priority 1:** Fix zero page addressing mode bug
   - Add unit tests for zero page operands
   - Debug operand parser in HndlMnem/GOpAdr
   - Verify addressing mode detection logic

2. **Priority 2:** Investigate simple_test.asm object code format
   - Analyze EDASM object file header structure
   - Compare with other object formats (OBJ0 vs REL)
   - May require understanding of relocatable code handling

3. **Priority 3:** Expand fixture coverage after fixes
   - Create fixtures for each addressing mode
   - Test edge cases (boundary addresses, expression evaluation)
   - Systematically cover instruction families

### Summary

Phase 1 successfully expanded from 3 to 6 green fixtures. Investigation into Phase 2 expansion exposed two previously-unknown bugs:

- Object file format mismatch (simple_test.asm)
- Zero page addressing mode parsing error

These blocks prevent reaching 7/7 fixtures and expanding corpus further. Fixing these bugs is essential before proceeding with broader comparative test expansion.
