## Plan: Expand Listing Parity to Simple Test & Beyond

Achieve zero-diff listing comparison across all available comparative test fixtures, and establish infrastructure for expanding the test corpus. Currently 6 of 7 fixtures pass listing comparison (input.src, input2.src, input3.src, branch.src, equexpr.src, fwdjmp.src). The simple_test.asm fixture has object code parity issues that prevent listing comparison inclusion. This plan addresses object parity remediation and establishes a path forward for creating additional comparable fixtures.

**Phases: 3 phases**

1. **Phase 1: Investigate and Fix simple_test.asm Object Parity**
   - **Objective:** Resolve the object code differences in simple_test.asm (currently 5 byte mismatches at offsets 1-2, 13, 15, and 30-34 in object file header/relocation sections) so it can be included in listing comparison.
   - **Files/Functions to Modify/Create:**
     - src/lib/asm/asm.cpp (Pass 2 object generation, relocation handling)
     - tests/app_test.cpp (if new regressions tests needed for object-file format)
   - **Tests to Write:**
     - Unit test for object file header format correctness
     - Test for relocation data structures in object output
     - Test for DFB relative addressing in object code
   - **Steps:**
     1. Write unit test for object file header format (expected 46 bytes from simple_test.asm)
     2. Run test to see current vs expected format
     3. Identify which phase-2 or phase-3 code generates object header bytes (offsets 0-2)
     4. Identify relocation data encoding differences (offsets 13, 15, 30-34)
     5. Fix object generation to match EDASM format
     6. Verify all tests pass and simple_test.asm comparision goes green

2. **Phase 2: Enable Listing Comparison for simple_test.asm**
   - **Objective:** Once object parity is fixed, add simple_test.asm to the listing comparison test set to achieve 7/7 green.
   - **Files/Functions to Modify/Create:**
     - comparative-tests/compare.py (update test list)
   - **Tests to Write:**
     - Comparative listing test for simple_test.asm
   - **Steps:**
     1. Verify object parity is green: `python3 compare.py --no-build simple_test.asm`
     2. Run listing comparison: `python3 compare.py --no-build --compare-listing simple_test.asm`
     3. If differences remain, debug normalized listing output
     4. Commit when 7 matched fixtures achieved

3. **Phase 3: Establish Corpus Expansion Infrastructure**
   - **Objective:** Create a documented process and set of fixture templates for adding new test cases to the comparative harness, enabling broader coverage without manual fixture creation.
   - **Files/Functions to Modify/Create:**
     - comparative-tests/FIXTURE_TEMPLATE.md (new)
     - comparative-tests/inputs/ (new fixture examples)
     - plans/expansion-strategy.md (new)
   - **Tests to Write:**
     - New fixture files covering: zero-page addressing, indirect addressing, macros/repeats, conditional assembly, expression evaluation
   - **Steps:**
     1. Document fixture creation template and guidelines
     2. Create 3-5 new fixture files targeting underrepresented addressing modes or features
     3. Verify each fixture achieves object AND listing parity
     4. Document lessons learned and next fixtures to prioritize

**Open Questions:**

1. Should simple_test.asm object differences be handled as a separate bug fix, or is this blockable on Phase 1 completion?
2. Are there preferred addressing modes or features for new fixtures in Phase 3 expansion?
3. Should Phase 3 defer to a later task, or pursue in this session if time permits?
