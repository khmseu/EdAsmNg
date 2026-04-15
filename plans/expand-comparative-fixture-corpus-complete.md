# Plan Complete: Expand Comparative Fixture Corpus

Expanded the comparative parity corpus from 7 to 12 fixtures while preserving the EDASM-mirroring goal for object and listing output. The work added parity-safe fixture guidance, directive coverage fixtures, minimal Phase 3 opcode/addressing support needed for new fixtures, and a refreshed coverage/deferred-work summary. The resulting state is stable and verifiable: all unit tests pass and comparative object/listing parity remains green across the full corpus.

**Phases Completed:** 4 of 4

1. ✅ Phase 1: Document parity-safe fixture authoring
2. ✅ Phase 2: Add directive-only fixtures (`DW`, `DS`, `DCI`)
3. ✅ Phase 3: Add minimal instruction coverage for `JSR`, `LDA abs/X`, `STA abs/Y`
4. ✅ Phase 4: Refresh parity findings and deferred scope

**All Files Created/Modified:**

- comparative-tests/FIXTURE_TEMPLATE.md
- comparative-tests/inputs/dwdir.src
- comparative-tests/inputs/dsdir.src
- comparative-tests/inputs/dcidir.src
- comparative-tests/inputs/jsrsubr.src
- comparative-tests/inputs/absidx.src
- src/lib/asm/asm.cpp
- tests/app_test.cpp
- LISTING_PARITY_FINDINGS.md
- plans/expand-comparative-fixture-corpus-phase-1-complete.md
- plans/expand-comparative-fixture-corpus-phase-2-complete.md
- plans/expand-comparative-fixture-corpus-phase-3-complete.md
- plans/expand-comparative-fixture-corpus-phase-4-complete.md
- plans/expand-comparative-fixture-corpus-complete.md

**Key Functions/Classes Added or Changed:**

- `HndlMnem()` in `src/lib/asm/asm.cpp`
  - Added `JSR` absolute emission (`0x20`)
  - Extended `LDA` dispatch for absolute (`0xAD`) and absolute,X (`0xBD`)
  - Extended `STA` dispatch for absolute (`0x8D`) and absolute,Y (`0x99`)

**Test Coverage:**

- Total unit tests passing: 186
- Comparative fixtures passing: 12 of 12
- Listing parity: 12 matched, 0 differed
- All tests passing: ✅

**Recommendations for Next Steps:**

- Add comparative fixtures for zero-page indexed and indirect indexed addressing modes
- Add parity-focused fixtures for non-trivial `DS`/`DCI` listing forms once formatting alignment is improved
