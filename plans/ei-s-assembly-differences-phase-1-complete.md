## Phase 1 Complete: Unblock Pass-2 Traversal

Implemented the minimal pass-2 and parser fixes that removed the historical hard stop while assembling `EI.S`. Assembly now progresses through `RELOCATOR.S` into deeper include content (including `EDASMINT.S`) instead of terminating near the earlier boundary.

**Files created/changed:**

- src/lib/asm/asm.cpp

**Functions created/changed:**

- NxtField
- DoPass2_ExperimentalCore

**Tests created/changed:**

- None (validation done via EI assembly run)

**Review Status:** APPROVED with minor recommendations

**Git Commit Message:**
fix: unblock EI pass2 traversal

- clear carry before PollKbd in pass2 loop to prevent false aborts
- make NxtField skip tabs as well as spaces
- verify EI listing now traverses past prior RELOCATOR stop
