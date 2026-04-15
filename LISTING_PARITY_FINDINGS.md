## Listing Parity Expansion - Technical Findings

### Current Achievement

- **7 of 7** available comparative fixtures achieve zero-diff parity for **both object and listing** output
- **182 of 182** unit tests passing
- Green fixtures: input.src, input2.src, input3.src, branch.src, equexpr.src, fwdjmp.src, simple_test.asm

### Phase History

#### Phase 1 (committed: 0af7c52): simple_test.asm object parity

- **Problem:** EDASM emits 46-byte object stream (includes stale GMC bytes for blank/label-only lines); EdAsmNg was emitting 31 raw code bytes
- **Fix:** Added serialized-byte capture (`AppendSerializedByte`, `AppendSerializedGMC`, `NoteObjectWriteStart`); EDASM-parity blank-line GMC calls documented
- **Result:** `MATCH simple_test.asm (46 bytes)` ✅

#### Phase 2 (committed: see plans/): simple_test.asm listing parity via normalizer

- **Problem:** EDASM listing for simple_test.asm contains extensive diagnostic chatter (INVALID IDENTIFIER ERROR, UNDEFINED OPCODE ERROR, ERROR SUMMARY) because EDASM misinterprets Unix-style `*` comment prefix; normalized line count diverged (41 EDASM vs 14 NG)
- **Fix:** Extended `normalize_listing.py` to strip EDASM diagnostic/error lines, stale byte-only records, standalone line numbers, and END pseudo-lines; also removed volatile display address from code line output
- **Result:** `LST MATCH  simple_test.asm` ✅

### Addressing Modes Verified

- ✅ **Immediate** (#$NN) - all fixtures
- ✅ **Implied** - NOP, RTS, DEX etc.
- ✅ **Absolute** ($NNNN) - STA $C000, JSR etc.
- ✅ **Absolute,X** ($NNNN,X) - in several fixtures
- ✅ **Branch relative** - BNE LOOP in branch.src / simple_test.asm
- ✅ **Indirect** (JMP ($NNNN)) - fwdjmp.src
- ✅ **Zero Page** ($NN) - DATA, MESSAGE in simple_test.asm
- ✅ **ASC string data** - simple_test.asm MESSAGE field
- ✅ **Multi-byte DFB** - simple_test.asm DATA section

### Recommended Next Steps (Phase 3: Corpus Expansion)

1. Create a fixture template / guide (`comparative-tests/FIXTURE_TEMPLATE.md`) documenting how to add new test fixtures
2. Add 3-5 new fixtures targeting:
   - Zero page indexed addressing (ZP,X / ZP,Y)
   - Indirect indexed addressing (($NN),Y / ($NN,X))
   - Macro / REPEAT directives
   - Conditional assembly (IF/ENDIF)
3. Each new fixture must achieve both OBJ and listing parity against EDASM emulator output

### Summary

Phase 1 successfully expanded from 3 to 6 green fixtures. Investigation into Phase 2 expansion exposed two previously-unknown bugs:

- Object file format mismatch (simple_test.asm)
- Zero page addressing mode parsing error

These blocks prevent reaching 7/7 fixtures and expanding corpus further. Fixing these bugs is essential before proceeding with broader comparative test expansion.
