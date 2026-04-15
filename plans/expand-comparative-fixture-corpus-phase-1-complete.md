# Phase 1 Complete: Document Parity-Safe Fixture Authoring

Added a fixture-authoring guide that documents how to create EDASM-parity comparative sources without depending on undefined or host-specific behavior. The guide is grounded in the current compare.py discovery rules and the existing 7 green fixtures, and it was reviewed to remove an overclaim about indirect JMP coverage.

**Files created/changed:**

- comparative-tests/FIXTURE_TEMPLATE.md
- plans/expand-listing-parity-phase-2-plan.md

**Functions created/changed:**

- No production functions changed

**Tests created/changed:**

- No automated tests added
- Validation completed through documentation review against compare.py discovery behavior and the current green fixture corpus

**Review Status:** APPROVED

**Git Commit Message:**
docs: add parity-safe fixture authoring guide

- document compare.py fixture discovery and ProDOS-safe naming rules
- capture EDASM parity authoring constraints for new comparative sources
- add a minimal template and checklist for object and listing verification
