## Plan: ASM2 Addressing-Mode Helpers

We will complete the ASM2 addressing-mode helper routines in asm.cpp so GAdrMod can resolve accumulator, zero-page, 65C02, and SW16 register cases exactly like the original assembler. The work is split into three small phases to keep each change self-contained and easy to verify. No tests will be added because there is no existing harness for asm.cpp helpers.

**Phases 3**

1. **Phase 1: Implement IsAccMod and IsZPMod**
   - **Objective:** Translate IsAccMod and IsZPMod from ASM2.S with 1:1 labels/comments and carry/flag behavior.
   - **Files/Functions to Modify/Create:** src/lib/ei/asm.cpp (IsAccMod, IsZPMod and any local labels).
   - **Tests to Write:** None (no existing harness).
   - **Steps:**
     1. Locate IsAccMod and IsZPMod in Original/EDASM.SRC/ASM/ASM2.S.
     2. Replace the missing/stubbed routines in asm.cpp with 1:1 logic.
     3. Verify L8598 jump-table flow still matches ASM2.S.

2. **Phase 2: Implement Is65C02**
   - **Objective:** Replace the Is65C02 stub with ASM2.S logic so 65C02-only addressing modes are parsed correctly.
   - **Files/Functions to Modify/Create:** src/lib/ei/asm.cpp (Is65C02).
   - **Tests to Write:** None (no existing harness).
   - **Steps:**
     1. Translate Is65C02 from ASM2.S.
     2. Preserve label/comment mapping and flag behavior.
     3. Ensure GAdrMod and L8598 behavior remains consistent.

3. **Phase 3: Implement IsSW16Reg**
   - **Objective:** Replace the IsSW16Reg stub with ASM2.S logic for SW16 register operands.
   - **Files/Functions to Modify/Create:** src/lib/ei/asm.cpp (IsSW16Reg).
   - **Tests to Write:** None (no existing harness).
   - **Steps:**
     1. Translate IsSW16Reg from ASM2.S.
     2. Preserve label/comment mapping and flag behavior.
     3. Validate the SW16 operand path is intact.

**Open Questions 0**

(No open questions; proceeding without tests due to lack of harness.)
