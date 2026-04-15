## Phase 4 Complete: Wire Directive-Controlled Listing Behavior

LIST, LST, NOLIST, and PAGE directives now invoke real handler implementations during pass 2, modifying `ListingF` at runtime and emitting form feeds for PAGE.

**Files created/changed:**

- `src/lib/asm/asm.cpp` — dispatcher wired (LIST/LST/NOLIST/PAGE call real handlers behind `g_test_src_buffer == nullptr` guard), bridge functions implemented
- `include/EdAsmNg/asm.hpp` — `HndlLIST()`, `SetListingF()` added to public test API
- `tests/app_test.cpp` — 4 new Phase 4 tests added; NOLIST dispatch test strengthened

**Functions created/changed:**

- `Bridge_HndlLIST()` — new, delegates to `AsmInternal::HndlLIST()`
- `Bridge_HndlLST()` — stub replaced, delegates to `AsmInternal::HndlLST()`
- `Bridge_HndlNOLIST()` — stub replaced, delegates to `AsmInternal::HndlNOLIST()`
- `Bridge_DoPage()` — stub replaced, delegates to `AsmInternal::DoPage()`
- `DoPage()` (in anonymous namespace) — now emits `PrtFF()` form feed when PassNbr > 0 and ListingF MSB set
- Dispatcher (LIST/LST/NOLIST/PAGE branches) — calls real handlers in non-dispatch-test mode

**Tests created/changed:**

- `Pass2Test.test_pass2_list_directive_sets_listing_flag` — EXPECT_EQ ListingF == 0x80
- `Pass2Test.test_pass2_nolist_directive_clears_listing_flag` — EXPECT_EQ ListingF == 0x7F
- `Pass2Test.test_pass2_page_directive_emits_form_feed_to_listing_sink` — EXPECT_EQ sink == "\014"
- `Pass2Test.test_pass2_nolist_then_list_transitions_correctly` — exact 0x7F → 0xBF sequence
- `MnemonicDispatchTest.NOLIST_NonDotDirective_RoutesToHndlNOLIST` — added routing assertion

**Review Status:** APPROVED

**Git Commit Message:**

```
feat: wire LIST/LST/NOLIST/PAGE to real directive handlers

- connect dispatcher to HndlLIST/HndlLST/HndlNOLIST/DoPage behind
  g_test_src_buffer guard to preserve dispatch-only test path
- implement Bridge_HndlLIST and fix Bridge_HndlLST/NOLIST/DoPage stubs
- DoPage emits PrtFF form feed when pass2+ and listing enabled
- add 4 strict-equality Phase 4 tests for ListingF and sink state
- strengthen NOLIST dispatch test routing assertion
```
