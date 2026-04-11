# EdAsmNg Comparative Testing Plan

## Overview

This document outlines the strategy for comparing EdAsmNg (C++ port) against the original EDASM (Apple II ProDOS assembler) to verify correctness and compatibility.

## Current Status

### Available Tools

1. **Original EDASM via ProDOS8Emu**
   - Location: `/bigdata/KAI/projects/ProDOS8Emu`
   - Automation: `tools/run_edasm_job.py`
   - Status: ⚠️ **TIMEOUT ISSUES** - Emulator hits instruction limit before assembly completes
   - Inputs: Bootstrapped (disk image, ROM, config)

2. **EdAsmNg (C++ Port)**
   - Location: `/bigdata/KAI/projects/EdAsmNg`
   - Build: `build/src/EdAsmNg_app`
   - Status: ✅ **LIBRARY FUNCTIONAL** - Core assembler functions work via unit tests
   - CLI: ❌ **NOT YET IMPLEMENTED** - No command-line interface for full assembly jobs

3. **Unit Test Suite**
   - Tests: 134 passing tests
   - Coverage: Mnemonics, directives, symbol table, error handling, Pass 1-3
   - Status: ✅ **WORKING** - Comprehensive coverage of core functionality

## Testing Strategy

### Phase 1: Unit-Level Comparison (Current Focus)

Test individual assembler components against documented original EDASM behavior:

**✅ Implemented:**

- Symbol table operations (FindSym, AddNode, HashFn)
- Mnemonic dispatch (LDA, STA, etc.)
- Directive dispatch (ORG, DFB, ASC, etc.)
- Error registration and tracking
- Pass 1-3 core logic
- Expression evaluation infrastructure

**Test Approach:**

```bash
cd /bigdata/KAI/projects/EdAsmNg/build
ctest --output-on-failure
```

### Phase 2: CLI Development (Next Step)

To enable full comparison testing, EdAsmNg needs a CLI interface:

**Required Features:**

1. Accept source file path as argument
2. Generate listing file output
3. Generate object code output
4. Match original EDASM command-line semantics

**Proposed Interface:**

```bash
./EdAsmNg_app <source.asm> --listing <output.lst> --object <output.obj>
```

**Implementation Tasks:**

- [ ] Add argument parsing to `src/main.cpp`
- [ ] Implement file I/O for source input
- [ ] Implement listing file generation
- [ ] Implement object file generation
- [ ] Hook up to existing assembler library functions

### Phase 3: Output Comparison (Future)

Once CLI is ready, use automated differentil testing:

**Test Cases:**

1. **Simple Instructions (`test_simple.asm`)**

   ```assembly
   * Basic 6502 instructions
           ORG   $0800
   START   LDA   #$00
           STA   $C000
           RTS
   ```

2. **Data Directives (`test_data.asm`)**

   ```assembly
   * DFB, ASC, DCI tests
           DFB   $01,$02,$03
   MSG     ASC   "HELLO"
           DCI   "WORLD"
   ```

3. **Symbols & Expressions (`test_symbols.asm`)**

   ```assembly
   * Symbol definitions and forward references
   BASE    =     $0800
           ORG   BASE
   LOOP    LDA   DATA
           JMP   LOOP
   DATA    DFB   $00
   ```

4. **Macros & Advanced (`test_advanced.asm`)**

   ```assembly
   * LST, REL, DO/ELSE/FIN
           LST   ON
           DO    1
           LDA   #$FF
           ELSE
           LDA   #$00
           FIN
   ```

**Comparison Script:**

```bash
#!/bin/bash
# comparative-tests/compare.sh

TEST=$1

# Run through original EDASM (if/when timeout fixed)
cd /bigdata/KAI/projects/ProDOS8Emu
./tools/run_edasm_job.py \
  --input ../EdAsmNg/comparative-tests/inputs/${TEST}.asm \
  --listing ${TEST}_orig.lst \
  --output ${TEST}_orig.obj

# Run through EdAsmNg
cd /bigdata/KAI/projects/EdAsmNg
./build/EdAsmNg_app \
  comparative-tests/inputs/${TEST}.asm \
  --listing comparative-tests/edasmng-outputs/${TEST}.lst \
  --object comparative-tests/edasmng-outputs/${TEST}.obj

# Compare outputs
diff -u \
  /bigdata/KAI/projects/ProDOS8Emu/work/volumes/OUT/${TEST}_orig.lst \
  comparative-tests/edasmng-outputs/${TEST}.lst

# Compare object files
cmp \
  /bigdata/KAI/projects/ProDOS8Emu/work/volumes/OUT/${TEST}_orig.obj \
  comparative-tests/edasmng-outputs/${TEST}.obj
```

## Known Limitations

### Original EDASM (ProDOS8Emu)

1. **Timeout Issues**
   - Emulator hits max instruction count (1,000,000)
   - Assembly jobs don't complete before timeout
   - Possible solutions:
     - Increase `--max-instructions` parameter
     - Optimize emulator performance
     - Use pre-generated reference outputs

2. **File Format Conversion**
   - ProDOS CR (`\r`) vs Linux LF (`\n`)
   - Handled by `tools/prodos_text_to_linux.py`

3. **Path Handling**
   - ProDOS 8.3 filename limitations
   - Uppercase-only filenames

### EdAsmNg (C++ Port)

1. **No CLI Interface**
   - Currently library-only
   - Requires development for end-to-end testing

2. **Extracted Modules Disabled**
   - `asm_expr.cpp` and `asm_directives.cpp` excluded from build
   - Phase 5 macro forwarding infrastructure complete
   - Ready for integration but pending full testing

3. **Feature Coverage**
   - Core functionality: ✅ Complete
   - Advanced features (macros, REL files): Status varies

## Current Test Results

### Unit Tests: **134/134 PASSING** ✅

Categories:

- Error registration: 11 tests
- Mnemonic dispatch: 15 tests
- Directive routing: 8 tests
- Symbol table: 25 tests
- Pass 1-3 logic: 35 tests
- Expression evaluation: 20 tests
- Helper functions: 20 tests

### Verification Commands

```bash
# Run all tests
cd /bigdata/KAI/projects/EdAsmNg/build
ctest

# Run with verbose output
ctest --output-on-failure --verbose

# Run specific test category
ctest -R "MnemonicDispatch"

# Build and test
ninja && ctest
```

## Next Steps

### Immediate (Phase 2 - CLI Development)

1. **Design CLI Interface**
   - Argument parsing library (e.g., CLI11, argparse)
   - Input/output file specification
   - Option flags (listing control, object format, etc.)

2. **Implement File I/O**
   - Source file reader
   - Listing file writer (with formatting)
   - Object file writer (binary or REL format)

3. **Integrate with Assembler Library**
   - Initialize assembler state
   - Load source into internal buffers
   - Execute three-pass assembly
   - Collect and write outputs

4. **Basic CLI Testing**
   - Assemble `simple_test.asm`
   - Verify listing generated
   - Verify object code generated
   - Manual inspection of outputs

### Medium-Term (Phase 3 - Comparison Testing)

1. **Resolve Emulator Timeout**
   - Investigate emulator configuration
   - Try increasing instruction limit
   - Consider alternative: Use pre-captured original EDASM outputs

2. **Create Test Suite**
   - Develop 10-20 test cases covering all features
   - Simple → Complex progression
   - Document expected outputs

3. **Automated Comparison**
   - Listing file diff (after format normalization)
   - Object code binary comparison
   - Symbol table verification
   - Error message matching

4. **Regression Testing**
   - CI/CD integration
   - Automated comparison on every commit
   - Performance benchmarking

### Long-Term (Phase 4 - Completeness)

1. **Feature Parity**
   - All mnemonics (6502 + 65C02)
   - All directives (ORG, DFB, ASC, DCI, LST, etc.)
   - Macro system (if applicable)
   - REL file format output

2. **Compatibility Mode**
   - Exact output matching (byte-for-byte)
   - Listing format matching
   - Error message matching

3. **Extended Testing**
   - Large programs (>1000 lines)
   - Real-world Apple II source code
   - Performance comparison

## Test Framework Structure

```
comparative-tests/
├── inputs/
│   ├── simple_test.asm          # Created ✅
│   ├── data_directives.asm      # TODO
│   ├── symbols_expressions.asm  # TODO
│   ├── advanced_features.asm    # TODO
│   └── real_world/              # Future: Actual Apple II programs
├── expected-outputs/            # Reference outputs from original EDASM
│   ├── simple_test.lst
│   ├── simple_test.obj
│   └── ...
├── edasmng-outputs/             # EdAsmNg generated outputs
│   ├── simple_test.lst
│   ├── simple_test.obj
│   └── ...
└── scripts/
    ├── run_comparison.sh        # Automated comparison script
    └── normalize_listing.py     # Format normalization for diff
```

## References

- Original EDASM: `/bigdata/KAI/projects/EdAsmNg/Original/EDASM.SRC/`
- ProDOS8Emu Skills: `/bigdata/KAI/projects/ProDOS8Emu/.github/skills/edasm-automation/`
- EdAsmNg Tests: `/bigdata/KAI/projects/EdAsmNg/tests/`
