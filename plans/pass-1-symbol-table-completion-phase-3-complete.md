## Phase 3 Complete: Directive interactions affecting symbols/PC

TL;DR — Implemented Pass‑1 handling for `EQU`: the assembler now stores evaluated EQU values into the symbol table (low/high bytes), added unit coverage, fixed related byte-order bug, and removed temporary debug traces. All Pass‑1 tests in `Phase84Pass1Test` are passing.

**Files created/changed:**

- `src/lib/ei/asm.cpp` (fixed `HndlEQU`, inline `HndlMnem(EQU)`, removed temporary debug prints)
- `tests/app_test.cpp` (added `Pass1_EQU_DefinesSymbolValue`)

**Functions created/changed:**

- `HndlEQU` — write evaluated EQU value into existing symbol node (clears `undefined`, sets `unrefd/relative`, writes low/high bytes correctly)
- Inline `HndlMnem(EQU)` — mirror Pass‑1 EQU behavior used by tests
- `DoPass1` / `RegAsmEW` — removed temporary debug fprintf() traces

**Tests created/changed:**

- `Pass1_EQU_DefinesSymbolValue` — verifies symbol exists, flags updated, value stored, PC unchanged
- Ensured `Phase84Pass1Test` suite: all tests passing (14/14)

**Review Status:** APPROVED — Tests pass locally.

**Git Commit Message:**
fix: Pass-1 EQU/symbol-table write and remove debug traces

- Fix: store EQU evaluated value (low/high) into symbol node during Pass 1
- Fix: use correct high-byte (`ValExpr_hi`) instead of shifting 8-bit `ValExpr`
- Add: `Pass1_EQU_DefinesSymbolValue` unit test
- Chore: remove temporary debug fprintf() traces
