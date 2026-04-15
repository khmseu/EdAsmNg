# Listing Parity Expansion - Technical Findings

## Current Achievement

- **12 of 12** comparative fixtures achieve zero-diff parity for both object and listing output (`MATCH` + `LST MATCH`)
- **186 of 186** unit tests passing
- Current green fixtures:
  - `input.src`
  - `input2.src`
  - `input3.src`
  - `branch.src`
  - `equexpr.src`
  - `fwdjmp.src`
  - `simple_test.asm`
  - `dwdir.src`
  - `dsdir.src`
  - `dcidir.src`
  - `jsrsubr.src`
  - `absidx.src`

## Phase History

### Baseline Parity Stabilization

- `0af7c52`: fixed object-stream parity capture for blank/label-only serialization behavior
- `9bcf5c0`: normalized EDASM diagnostic/listing noise for stable listing comparison

### Corpus Expansion Plan Progress

- `6050fdf` (Phase 1): added parity-safe fixture authoring guide (`comparative-tests/FIXTURE_TEMPLATE.md`)
- `321916e` (Phase 2): added directive fixtures (`DW`, `DS`, `DCI`) with listing parity
- `e328369` (Phase 3): added minimal `HndlMnem()` support for `JSR abs`, `LDA abs/X`, `STA abs/Y`, plus unit tests and two fixtures (`jsrsubr.src`, `absidx.src`)

## Coverage Snapshot

### Directives covered by comparative fixtures

- `LST ON`, `ORG`, `END`
- `DFB`, `ASC`, `DW`, `DS`, `DCI`
- `EQU` expressions (via `equexpr.src`)

### Instruction/addressing coverage verified in comparative fixtures

- **Implied**: `NOP`, `RTS`, etc.
- **Immediate**: e.g. `LDA #$nn`
- **Absolute**: e.g. `JMP $nnnn`, `JSR $nnnn`, `STA $nnnn`
- **Absolute indexed**: `LDA $nnnn,X`, `STA $nnnn,Y`
- **Branch relative**: branch fixture coverage
- **Forward absolute jump**: `JMP label` coverage in `fwdjmp.src`

## Deferred / Known Gaps

- Zero-page indexed and indirect indexed addressing still need dedicated comparative fixtures
- `DS` non-zero-count forms and richer `DCI` string forms can be listing-sensitive and need targeted parity work before broadening fixture complexity
- Macro/repeat and conditional assembly parity remain out of scope for this expansion cycle

## Next Recommended Work

1. Add focused fixtures for zero-page indexed and indirect indexed addressing modes
2. Add parity-focused fixtures for non-trivial `DS`/`DCI` forms once listing formatting behavior is aligned
3. Extend corpus only after each addition is confirmed with both object and listing parity
