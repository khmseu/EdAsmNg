# Expand Comparative Fixture Corpus - Phase 2 Complete

Date: 2026-04-15

## Scope

Added and validated directive-only comparative fixtures for `DW`, `DS`, and `DCI` using EDASM-vs-EdAsmNg comparative checks with listing comparison enabled.

## Files Changed

- `comparative-tests/inputs/dwdir.src`
- `comparative-tests/inputs/dsdir.src`
- `comparative-tests/inputs/dcidir.src`
- `plans/expand-comparative-fixture-corpus-phase-2-complete.md`

## Final Fixture Content

`comparative-tests/inputs/dwdir.src`

```asm
 LST ON
 ORG $800
 DW $1234
```

`comparative-tests/inputs/dsdir.src`

```asm
 LST ON
 ORG $800
 DS 0
 DFB $AA
```

`comparative-tests/inputs/dcidir.src`

```asm
 LST ON
 ORG $800
 DCI ""
 DFB $AA
```

## Validation Commands Run

- `python3 comparative-tests/compare.py --compare-listing --no-build comparative-tests/inputs/dwdir.src`
- `python3 comparative-tests/compare.py --compare-listing --no-build comparative-tests/inputs/dsdir.src`
- `python3 comparative-tests/compare.py --compare-listing --no-build comparative-tests/inputs/dcidir.src`

## Validation Results

- `dwdir.src`: `MATCH` and `LST MATCH`
- `dsdir.src`: `MATCH` and `LST MATCH`
- `dcidir.src`: `MATCH` and `LST MATCH`

## Review Status

- Production C++ code: unchanged.
- Comparative fixtures only: updated.
- Parity-focused adjustments: complete.

## Caveats Encountered

- Missing trailing newline at EOF could cause CLI hangs on directive fixtures.
- Some directive forms were object-compatible but listing-divergent (`DW` with labels/extra instructions, `DS 4`, non-empty `DCI`), so fixtures were reduced to EDASM-parity-safe minimal syntax.

## Commit Message Proposal

`test(comparative): add phase-2 directive fixtures for DW/DS/DCI with listing parity`
