## Phase 1 Complete: Investigate and Fix simple_test.asm Object Parity

simple_test.asm now matches EDASM object output on the active CLI and experimental pass-2 path, unblocking its future inclusion in listing comparison. The fix adds explicit serialized-object capture support, robust object-start tracking for CLI fallback behavior, and regression coverage for the 46-byte EDASM-parity stream.

**Files created/changed:**

- include/EdAsmNg/asm.hpp
- src/lib/asm/asm.cpp
- src/main.cpp
- tests/app_test.cpp

**Functions created/changed:**

- EnableSerializedObjectCapture
- ClearSerializedObjectBytes
- GetSerializedObjectBytes
- HasObjectWriteStartAddr
- GetObjectWriteStartAddr
- AppendSerializedByte
- AppendSerializedGMC
- NoteObjectWriteStart
- main

**Tests created/changed:**

- CliObjectTests.SimpleProgramWithBlankLinesMatchesEDASMParitySerializedObjectBytes
- CliObjectTests.ObjectWriteStartTracksFirstPass2EmissionWithoutSerializedCapture

**Review Status:** APPROVED

**Git Commit Message:**
fix: capture EDASM parity object stream

- track serialized object bytes during experimental pass 2
- make CLI object fallback use first emitted object address
- add regression coverage for simple_test object parity
