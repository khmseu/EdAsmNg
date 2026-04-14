# Plan: Close Next-Step Gaps for EDASM Parity

Deliver practical parity progress by expanding comparative coverage first, then unblocking architecture in small TDD-safe increments. This sequence keeps failure signals clear and avoids broad destabilizing rewrites.

## Phases: 4

1. **Phase 1: Strengthen Comparative Coverage**
   - **Objective:** Add focused fixtures for forward references and expressions so remaining gaps are reproducible and measurable.
   - **Files/Functions to Modify/Create:** comparative-tests/inputs, comparative-tests/edasm-outputs, comparative-tests/compare.py (only if fixture orchestration needs minor updates).
   - **Tests to Write:** fixture comparisons for forward label resolution, expression arithmetic, and mixed data directives.
   - **Steps:**
     1. Create minimal fixture files, one behavior per fixture.
     2. Run original EDASM with elevated instruction budget and capture reference artifacts.
     3. Run EdAsmNg and record exact byte-level deltas as phase baseline.

2. **Phase 2: Incremental Instruction/Directive Parity**
   - **Objective:** Implement only the missing assembler behavior required by failing Phase 1 fixtures.
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp and any small helper surfaces needed for operand parsing/code emission.
   - **Tests to Write:** failing-first tests per missing behavior plus fixture parity checks.
   - **Steps:**
     1. Add failing tests for each unresolved behavior detected in Phase 1.
     2. Implement minimal code changes to pass each test.
     3. Re-run fixture comparisons and confirm byte parity per case.

3. **Phase 3: Re-enable Regression Suite Reliability**
   - **Objective:** Restore build/discovery/execution of the existing regression test target.
   - **Files/Functions to Modify/Create:** test target wiring and related build configuration surfaces.
   - **Tests to Write:** test-target buildability checks and core regression smoke coverage.
   - **Steps:**
     1. Make the regression test target buildable and discoverable.
     2. Run regression suite to categorize failures.
     3. Resolve blockers that prevent routine green/smoke runs.

4. **Phase 4: Full DoPass2 Activation Strategy**
   - **Objective:** Replace stub dependency blockers and safely activate the real DoPass2 path.
   - **Files/Functions to Modify/Create:** src/lib/asm/asm.cpp dependency surfaces around real DoPass2 and related compile guards.
   - **Tests to Write:** pass-level integration tests that validate activation safety and output correctness.
   - **Steps:**
     1. Resolve missing dependencies in small compilable batches.
     2. Validate each batch with focused tests and comparative fixtures.
     3. Switch the default path once parity and regression thresholds are met.

## Open Questions

1. No strict ordering constraints were requested for implementation options.
2. No strict rollout preference was requested.
3. No strict scope preference was requested beyond following this phased plan.
