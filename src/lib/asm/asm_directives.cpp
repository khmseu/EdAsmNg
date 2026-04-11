//=================================================
// asm_directives.cpp - Directive Handler Functions
// Extracted from asm.cpp #if 0 block (Phase 3)
//=================================================

#include "asm_internal.hpp"

namespace AsmInternal {

  //=================================================
  // Directive Handler Stubs (Phase 4)
  // These are placeholder handlers for directive dispatch
  // Full implementation will come in later phases
  //=================================================

  // DrtvDone - Common return point for directive handlers
  // (ASM3.S line ~542, label DrtvDone)
  void DrtvDone() {
    Y = 0;      // LDY #0
    A = ZAB;    // LDA ZAB
    C = false;  // CLC
    // RTS
  }

  // Handler stubs for specific directives
  // These will be implemented in later phases

  //=================================================
  // L8A31 - EQU directive handler (ASM3.S lines 47-108)
  // Assigns a value to a symbol (equate).
  // Format: label EQU expression
  // Pass 1: Add symbol to table with evaluated value
  // Pass 2: Skip (no code generation)
  //=================================================
  void HndlEQU() {
    g_LastDirectiveCalled = "HndlEQU";

  L8A31:

    A = PassNbr;             // LDA PassNbr - Check current assembly pass
    if (A != 0) goto L8A41;  // BNE L8A41 - If not Pass 1, skip symbol marking

    // Pass 1 processing: Mark symbol as defined
    A = LabelF;              // LDA LabelF - Does source line have a label field?
    if (A == 0) goto L8A53;  // BEQ L8A53 - No label, error condition

    // TODO: Symbol table integration disabled (SymFBP pointer safety)
    // To be completed with proper symbol storage architecture
    // Original code (stubbed):
    //   X = 0x00;  // LDX #$00
    //   std::uint8_t* SymFBP_ptr =
    //   reinterpret_cast<std::uint8_t*>(static_cast<uintptr_t>(SymFBP)); A = undefined;  // LDA
    //   #undefined - Get undefined flag bit A |= SymFBP_ptr[X];  // ORA (SymFBP,X) - Set
    //   undefined bit in symbol's flag byte SymFBP_ptr[X] = A;  // STA (SymFBP,X) - (SymFBP set
    //   only during Pass 1)

    // Process the operand expression to get the value
  L8A41:
    EvalOprnd();               // JSR EvalOprnd - Evaluate operand expression
    if (C) goto L8A50;         // BCS L8A50 - If error, branch
    A = NxtToken;              // LDA NxtToken - Check next token type
    if (A != 0) goto L8A50;    // BNE L8A50 - If not space/CR, error (junk after operand)
    A = PassNbr;               // LDA PassNbr - Check current pass
    if (A != 0) goto DrtvFin;  // BNE DrtvFin - If Pass 2, done (SymFBP not set in Pass 2)
    if (A == 0) goto L8A5E;    // BEQ L8A5E - Pass 1

  L8A50:
    X = 0x24;  // LDX #$24 - directive operand err
    goto L8A53_skip_ldx;

  L8A53:
    X = 0x0C;  // LDX #$0C - equate err
  L8A53_skip_ldx:
    RegAsmEW(X);  // JSR RegAsmEW

  DrtvFin:
    A = ZAB;    // LDA ZAB - Same as DrtvDone!
    Y = 0;      // LDY #0
    C = false;  // CLC
    return;     // RTS - Ret to HndlMnem

    // External idfers can not be used to define the label
  L8A5E:
    A = ExprAccF;  // LDA ExprAccF - Expr's accumulated flag bits
    // BIT Bit10 - EXTeRNal
    if ((A & Bit10) != 0) goto L8A53;  // BNE L8A53 - Yes

    // Pass 1: store evaluated EQU value into the symbol table entry
    // Find the symbol (SymP may have been set when label was parsed in DoPass1)
    if (LabelF != 0) {
      // Ensure SymP points to the symbol entry; if not, attempt to locate it
      if (SymP == 0) {
        uint16_t saved_SrcP = SrcP;
        uint8_t  saved_Y    = Y;
        Y                   = 0;  // point at label
        AsmInternal::FindSym();   // locate symbol; sets SymP and Y when found
        Y    = saved_Y;
        SrcP = saved_SrcP;
      }

      if (SymP != 0) {
        uint8_t* symptr = SimPtrToMemPtr(SymP);
        // locate flag byte: scan past symbol chars (msb set on all but last char)
        int idx = 0;
        while ((symptr[idx] & 0x80) != 0) idx++;
        idx++;  // now points at flag byte

        // Clear 'undefined' and set unrefd/relative as appropriate
        uint8_t flags = symptr[idx];
        flags &= static_cast<uint8_t>(~undefined);  // clear undefined bit
        flags |= (RelExprF & relative);             // preserve relative bit if any
        if ((int8_t)DummyF < 0) flags |= relative;  // DSECT -> symbol is relative
        flags |= unrefd;                            // mark as unreferenced by default
        symptr[idx] = flags;

        // store evaluated value (ValExpr / ValExpr_hi)
        symptr[idx + 1] = ValExpr;
        symptr[idx + 2] = ValExpr_hi;
      }
    }

    goto DrtvFin;  // JMP DrtvFin
  }

  //=================================================
  // L8A82 - ORG directive handler (ASM3.S lines 109-180)
  // At least one ORG must be declared or no object code will be generated.
  // If REL code is to be generated, then the REL directive must precede
  // the ORG directive
  // Ref pg 93 on rel ORG
  //=================================================
  void HndlORG() {
    g_LastDirectiveCalled = "HndlORG";

  L8A82:
    // Process ORG in both Pass 1 and Pass 2
    EvalOprnd();                 // JSR EvalOprnd
    if (C) goto L8A50_ORG;       // BCS L8A50
    A = NxtToken;                // LDA NxtToken - Is sp/cr?
    if (A != 0) goto L8A50_ORG;  // BNE L8A50 - No

    // Bounds validation: Check if ORG address is within valid range
    // ValExpr must be < HighMem
    A = ValExpr_hi;  // LDA ValExpr+1
    // CMP HighMem+1
    if (A > HighMem_hi) goto L8A50_ORG;  // Out of range
    if (A < HighMem_hi) goto L8A9E;      // In range
    // High bytes equal, check low byte
    A = ValExpr;  // LDA ValExpr
    // CMP HighMem
    if (A >= static_cast<uint8_t>(HighMem)) goto L8A50_ORG;  // BCS - Out of range

    // Original ASM3.S logic for disk mode, RelCodeF, CurrORG, and file flushing
    // is deferred for Phase 5. This is a simplified stub that only handles
    // absolute ORG in Pass 1. Full implementation requires:
    // - Relative code mode (REL directive) support
    // - Disk file I/O for object code output
    // - Buffer flushing when ORG changes location
    // - CurrORG tracking for relative addressing
    // See ASM3.S lines 109-180 for complete original logic
    A = PassNbr;             // LDA PassNbr - Pass 1?
    if (A == 0) goto L8A9E;  // BEQ L8A9E - Yes
    // BIT DummyF - R we in a dummy section?
    if ((int8_t)DummyF < 0) goto L8A9E;  // BMI L8A9E - Yes
    A = RelExprF;                        // LDA RelExprF
    if (A == 0) goto L8A9A;              // BEQ L8A9A - expr's val is abs
    // JMP L8B2F - Handle relative ORG (stub for now)
    // For Phase 5, we'll just do simple absolute ORG
    goto L8A9E;

  L8A9A:
    // BIT GenF
    if ((GenF & 0x40) != 0) goto L8AA1;  // BVS L8AA1 - Output MC to disk
  L8A9E:
    goto SetPC_ORG;  // JMP SetPC - NB. (ObjPC) is not changed

  L8AA1:
    // File I/O operations - stubbed for Phase 5 (see comment above)
    // This section handles disk file management for ORG
    // For now, we just update PC
    goto SetPC_ORG;

  L8A50_ORG:
    X = 0x24;     // LDX #$24 - directive operand err
    RegAsmEW(X);  // JSR RegAsmEW
    goto DrtvFin_ORG;

  SetPC_ORG:
    // SetPC - Set both PC and ObjPC from ValExpr (both passes)
    A     = ValExpr;                      // LDA ValExpr
    PC    = (PC & 0xFF00) | A;            // STA PC - PC=new ORG Addr
    ObjPC = (ObjPC & 0xFF00) | A;         // Also set ObjPC
    A     = ValExpr_hi;                   // LDA ValExpr+1
    PC    = (PC & 0x00FF) | (A << 8);     // STA PC+1
    ObjPC = (ObjPC & 0x00FF) | (A << 8);  // Also set ObjPC+1
    goto DrtvFin_ORG;                     // JMP DrtvFin

  DrtvDone_ORG:
  DrtvFin_ORG:
    A = ZAB;    // LDA ZAB
    Y = 0;      // LDY #0
    C = false;  // CLC
    return;     // RTS
  }

  //=================================================
  // L8BAD - OBJ directive handler (ASM3.S lines 287-340)
  // Control absolute object code generation mode.
  // Format: OBJ address
  //   OBJ 0     - Suppress code generation
  //   OBJ $6000 - Generate code starting at $6000
  // Cannot be used with REL directive (mutually exclusive).
  // Address must be >= EndSymT (end of symbol table).
  //=================================================
  void HndlOBJ() {
    g_LastDirectiveCalled = "HndlOBJ";

  L8BAD:
    // Initialize ValExpr to $0000
    ValExpr    = 0;  // LDA #0 / STA ValExpr
    ValExpr_hi = 0;  // STA ValExpr+1

    EvalOprnd();        // JSR EvalOprnd - Parse operand expression
    if (C) goto L8BB8;  // BCS L8BB8 - Error, jump to error handler
    // Success, continue

  L8BBB:
    A = NxtToken;            // LDA NxtToken - Check next token
    if (A != 0) goto L8BB8;  // BNE L8BB8 - Must be space/CR

    // BIT GenF - Check disk output mode (V flag)
    if ((GenF & 0x40) != 0) goto L8BFC;  // BVS L8BFC - Disk mode, ignore OBJ

    // Check if operand is $0000 (suppress generation)
    A = ValExpr;             // LDA ValExpr
    A |= ValExpr_hi;         // ORA ValExpr+1
    if (A != 0) goto L8BCF;  // BNE L8BCF - Not zero, set address

    // Operand is $0000: suppress generation
    A    = 0x80;  // LDA #$80 - N=1 suppress flag
    GenF = A;     // STA GenF
    goto L8BFC;   // BNE L8BFC (always taken after LDA #$80)

  L8BCF:
    // BIT RelCodeF - Check if REL mode active
    if ((int8_t)RelCodeF < 0) goto L8BD3_REL;  // BMI L8BD3 - REL active, error

  L8BDB:
    // Check if address < EndSymT
    A = EndSymT;  // LDA EndSymT
    // CMP ValExpr - Compare EndSymT with ValExpr
    // If EndSymT < ValExpr, carry clear
    // If EndSymT >= ValExpr, carry set
    A = EndSymT_hi;  // LDA EndSymT+1
    // SBC ValExpr+1 (with borrow from previous compare)

    // Perform 16-bit comparison: EndSymT vs ValExpr
    // We want to error if ValExpr < EndSymT
    // CMP/SBC sets carry if minuend >= subtrahend
    // BCS means EndSymT >= ValExpr, which is the error condition
    uint16_t endSymT_val = static_cast<uint16_t>(EndSymT);
    uint16_t valExpr_val = ValExpr_word;

    if (valExpr_val < endSymT_val) goto L8BD3_ADDR;  // Address < EndSymT, error

    // Success: Set object generation parameters
    A      = ValExpr;  // LDA ValExpr
    ObjPC  = A;        // STA ObjPC
    MemTop = A;        // STA MemTop
    RLDEnd = A;        // STA RLDEnd

    A         = ValExpr_hi;  // LDA ValExpr+1
    ObjPC_hi  = A;           // STA ObjPC+1
    RLDEnd_hi = A;           // STA RLDEnd+1
    MemTop_hi = A;           // STA MemTop+1

    // JSR L828A - Ensure val < mem limit (call Is16K check)
    // Check if operand exceeds 16K memory limit
    Is16K();             // JSR Is16K
    if (!C) goto L8BFA;  // BCC L8BFA - Within limit, continue
    // Memory limit exceeded
    X = 0x06;          // LDX #$06 - overflow error
    RegAsmEW(X);       // JSR RegAsmEW
    goto DrtvFin_OBJ;  // Return

  L8BFA:
    A    = 0x00;  // LDA #$00 - N=0,V=0 enable memory generation
    GenF = A;     // STA GenF

  L8BFC:
    goto DrtvFin_OBJ;  // JMP DrtvFin

  L8BB8:
    X = 0x24;     // LDX #$24 - directive operand error
    RegAsmEW(X);  // JSR RegAsmEW
    goto DrtvFin_OBJ;

  L8BD3_REL:
    X = 0x76;          // LDX #$76 - Can't use OBJ and REL directives together
    RegAsmEW(X);       // JSR RegAsmEW
    goto DrtvFin_OBJ;  // JMP DrtvFin

  L8BD3_ADDR:
    X = 0x75;          // LDX #$75 - Address is below end of symbol table
    RegAsmEW(X);       // JSR RegAsmEW
    goto DrtvFin_OBJ;  // JMP DrtvFin

  DrtvFin_OBJ:
    A = ZAB;    // LDA ZAB
    Y = 0;      // LDY #0
    C = false;  // CLC
    return;     // RTS
  }

  //=================================================
  // L9126 - REL directive handler
  // ASM3.S lines 1168-1179 (label L9126)
  //
  // Enables relocatable code generation mode.
  // Sets RelCodeF MSB to in(RelCodeF >> 1) | 0x80
  //   The carry flag is set to 1, then ROR shifts right and puts carry into MSB
  //=================================================
  void HndlREL() {
    g_LastDirectiveCalled = "HndlREL";

  L9126:
    // SEC; ROR RelCodeF - Set MSB to indicate REL mode
    // SEC sets carry to 1, ROR shifts right and puts carry into bit 7
    C        = true;                    // SEC
    RelCodeF = (RelCodeF >> 1) | 0x80;  // ROR RelCodeF (shift right + set MSB)

    // For now, we just note that file type would be set to REL
    // JMP DrtvDone
    DrtvDone();
    return;
  }

  //=================================================
  // Greater than 16384 (16K)
  // C=0 - No
  // C=1 - Yes
  //=================================================
  void Is16K() {
  Is16K:
    A = ValExpr_hi;                   // BIT ValExpr+1
    if ((A & 0x80) != 0) goto L8BAB;  // BMI L8BAB
    if ((A & 0x40) == 0) goto L8BA9;  // BVC L8BA9
    A = ValExpr_hi & 0x3F;            // AND #%00111111
    A |= ValExpr;                     // ORA ValExpr
    if (A != 0) goto L8BAB;           // BNE L8BAB
  L8BA9:
    C = false;  // CLC
    return;     // RTS
  L8BAB:
    C = true;  // SEC
    return;    // RTS
  }

  //=================================================
  // L8C0E - DS/.BLOCK directive handler (ASM3.S lines 360-461)
  // Define Storage / Reserve space
  // Format: DS size[,filler]
  // Pass 1: Advance PC by size bytes
  // Pass 2: Optionally fill with filler byte or random data
  //=================================================
  void HndlDS() {
    g_LastDirectiveCalled = "HndlDS";
    uint16_t size         = 0;
    uint16_t sum          = 0;

  L8C0E:
    EvalOprnd();         // JSR EvalOprnd
    if (C) goto L8C1D;   // BCS L8C1D - Error
    Is16K();             // JSR Is16K - Is expr's val > 16384?
    if (!C) goto L8C23;  // BCC L8C23
    X = 0x06;            // LDX #$06 - overflow
    goto L8C1A;

  L8C1A:
    // DB $2C (skip next LDX)
    goto L8C1D;

  L8C23:
    // SEC
    RndF = 0x80;             // ROR RndF - Assume no filler char (bit 7 set = no filler)
    A    = NxtToken;         // LDA NxtToken - Is sp/cr?
    if (A == 0) goto L8C5F;  // BEQ L8C5F - Yes, no filler

    if (A != 0x01) goto L8C1B;  // CMP #$01 / BNE L8C1B - Not comma, error

    // Save ValExpr (size) on stack
    size = ValExpr_word;  // PHA/PHA

    Y++;  // INY
    Y++;  // INY
    // JSR WhiteSpc - Is it sp/cr after comma?
    A = SrcP_at(Y);
    if (A == SPACE || A == CR) goto L8C44;  // BEQ L8C44 - Yes, error (no filler after comma)

    EvalExpr();              // JSR EvalExpr - 2nd expr -> filler byte
    if (C) goto L8C44;       // BCS L8C44 - Error
    A = NxtToken;            // LDA NxtToken - Make sure nxt char is sp/cr
    if (A != 0) goto L8C44;  // BNE L8C44 - No, error

    // Got filler byte
    RndF   = 0x00;        // CLC / ROR RndF - Flag we will use a filler (bit 7 clear)
    A      = ValExpr;     // LDA ValExpr
    Filler = A;           // STA Filler - byte used to fill
    A      = ValExpr_hi;  // LDA ValExpr+1
    if (A != 0) {         // BEQ L8C59 - Must be 8-bits
      X = 0x28;           // LDX #$28 - byte overflow err
      RegAsmEW(X);        // JSR RegAsmEW
    }

    // Restore size
    ValExpr_word = size;  // PLA/PLA
    goto L8C5F;

  L8C44:
    // Error: dump saved values and report error
    // PLA/PLA (already handled above with local variable)
    goto L8C1B;

  L8C1B:
    X = 0x24;  // LDX #$24 - directive operand err
  L8C1D:
    RegAsmEW(X);  // JSR RegAsmEW
    goto DrtvFin_DS;

  L8C5F:
    // Calculate new PC
    A     = ValExpr;                          // LDA ValExpr - # of bytes to reserve
    sum   = A + (PC & 0xFF);                  // CLC / ADC PC
    NewPC = (NewPC & 0xFF00) | (sum & 0xFF);  // STA NewPC - New PC low byte

    A     = ValExpr_hi;                   // LDA ValExpr+1
    A     = A + (PC >> 8) + (sum >> 8);   // ADC PC+1 (with carry from low byte)
    NewPC = (NewPC & 0x00FF) | (A << 8);  // STA NewPC+1 - New PC high byte

    A = PassNbr;             // LDA PassNbr
    if (A == 0) goto L8CA0;  // BEQ L8CA0 - Pass 1, skip output

    // Pass 2: Output filler bytes or random data
    A         = ValExpr;     // LDA ValExpr
    ERfield   = A;           // STA ERfield - # of bytes reserve (low)
    A         = ValExpr_hi;  // LDA ValExpr+1
    ValExpr_2 = A;           // STA ERfield+1 - # of bytes reserve (high)

    A        = 0x81;  // LDA #%10000001
    LstCodeF = A;     // STA LstCodeF

    // BIT GenF - BMI L8C9D - No obj code output
    if ((int8_t)GenF < 0) goto L8C9D;

    // BIT RndF - BMI L8C9A - No filler (use random)
    if ((int8_t)RndF < 0) goto L8C9A;

    // Fill with filler byte
  L8C84:
    A = ValExpr;             // LDA ValExpr
    A |= ValExpr_hi;         // ORA ValExpr+1
    if (A == 0) goto L8C9D;  // BEQ L8C9D - Done

    A = Filler;  // LDA Filler
    StorByt();   // JSR StorByt

    A = ValExpr;             // LDA ValExpr
    if (A != 0) goto L8C95;  // BNE L8C95
    ValExpr_hi--;            // DEC ValExpr+1
  L8C95:
    ValExpr--;   // DEC ValExpr
    goto L8C84;  // JMP L8C84

  L8C9A:
    // Fill with random data (stubbed - fill with zeros)
  L8CAB:
    A = ValExpr;             // LDA ValExpr
    A |= ValExpr_hi;         // ORA ValExpr+1
    if (A == 0) goto L8C9D;  // BEQ L8C9D - Done
    A = 0x00;                // LDA #$00
    StorByt();               // JSR StorByt
    A = ValExpr;             // LDA ValExpr
    if (A != 0) goto L8CBD;  // BNE L8CBD
    ValExpr_hi--;            // DEC ValExpr+1
  L8CBD:
    ValExpr--;   // DEC ValExpr
    goto L8CAB;  // JMP L8CAB

  L8C9D:
      // JSR PrtAsmLn - print code,stmt (stubbed)
      ;

  L8CA0:
    // Adjust PC and ObjPC
    A  = NewPC & 0xFF;              // LDA NewPC
    PC = (PC & 0xFF00) | A;         // STA PC
    A  = NewPC >> 8;                // LDA NewPC+1
    PC = (PC & 0x00FF) | (A << 8);  // STA PC+1
    // Sync ObjPC with PC
    ObjPC = PC;

  DrtvFin_DS:
    A = ZAB;    // LDA ZAB
    Y = 0;      // LDY #0
    C = false;  // CLC
    return;     // RTS
  }

  //=================================================
  // L8CC3 - DFB/BYTE directive handler (ASM3.S lines 465-550)
  // Define Byte
  // Format: DFB byte1[,byte2[,byte3[,byte4]]]
  // Emits up to 4 bytes per statement
  //=================================================
  void HndlDFB() {
    g_LastDirectiveCalled = "HndlDFB";

  L8CC3:
    // JSR L8D5E - Setup
    X        = 0x21;  // LDX #%00100001 / STX LstCodeF
    LstCodeF = X;
    X        = 0;  // LDX #0
    TotCnt   = X;  // STX TotCnt - cntr

  NxtDFB:
    ByteCnt = X;        // STX ByteCnt - curr cnt
    EvalExpr();         // JSR EvalExpr - rtn will take care of comma
    X      = ByteCnt;   // LDX ByteCnt - double as index
    A      = ValExpr;   // LDA ValExpr - val defined
    GMC[X] = A;         // STA GMC,X
    X++;                // INX
    if (C) goto L8CED;  // BCS L8CED - err during eval

    A = PassNbr;             // LDA PassNbr
    if (A == 0) goto L8CED;  // BEQ L8CED - Pass 1, skip validation

    // Validate byte range (must be 0-255)
    A = ValExpr_hi;  // LDA ValExpr+1
    if (A != 0) {    // Check if high byte is non-zero
      // Byte overflow error
      uint8_t savedX = X;
      uint8_t savedY = Y;
      X              = 0x28;  // LDX #$28 - byte overflow err
      RegAsmEW(X);            // JSR RegAsmEW
      X = savedX;
      Y = savedY;
    }

    A = RelExprF;            // LDA RelExprF - Evaluate fr relocatable expr?
    if (A == 0) goto L8CED;  // BEQ L8CED - No, abs

    // For relocatable expressions, create RLD entry
    // Set up parameters for AddRLDEnt:
    // A = offset within GMC (ByteCnt-1 since we already incremented X)
    // X = size (1 for byte)
    // Y = order (0 for byte operands)
    A = ByteCnt;  // Current offset in GMC
    A--;          // Adjust since X was incremented
    uint8_t save_X = X;
    X              = 1;  // 8-bit value
    Y              = 0;  // No endian issue for single byte
    AddRLDEnt();         // Create RLD entry
    X = save_X;          // Restore X

  L8CED:
    ByteCnt = X;             // STX ByteCnt (updated)
    Length  = X;             // STX Length
    A       = NxtToken;      // LDA NxtToken
    if (A == 0) goto L8D0C;  // BEQ L8D0C - cr/space, done with this batch

    A ^= 0x01;               // EOR #$01
    if (A == 0) goto L8CFD;  // BEQ L8CFD - Comma, more values

    // Error: invalid token
    X = 0x24;     // LDX #$24 - directive operand err
    RegAsmEW(X);  // JSR RegAsmEW
    goto L8D35;

  L8CFD:
    Y++;                  // INY
    Y++;                  // INY
    A = SrcP_at(Y);       // LDA (SrcP),Y
    if (A < SPACE + 1) {  // CMP #SPACE+1 / BCC - Invalid
      X = 0x24;           // Error
      RegAsmEW(X);
      goto L8D35;
    }

    if (X >= 4) {  // CPX #4 - 4 bytes generated?
      // Flush current batch and continue
      AdvSrcP();  // JSR AdvSrcP
      goto L8D0C;
    }
    goto NxtDFB;  // Continue adding bytes

  L8D0C:
    A = PassNbr;             // LDA PassNbr
    if (A == 0) goto L8D20;  // BEQ L8D20 - Pass 1

    // Pass 2: Store bytes
    X = 0;  // LDX #0
    // JSR L8E28 - Store MC, update PC etc (simplified)
    StorGMC();  // Store generated machine code
    // Sync PC with ObjPC after emission
    PC = ObjPC;
    // JSR PrtAsmLn (stubbed)

  L8D15:
    A = NxtToken;            // LDA NxtToken
    if (A == 0) goto L8D26;  // BEQ L8D26 - cr/space -> done

    X = 0;  // LDX #0 - Reset cnt
    Y = 0;  // LDY #0 - & index
    goto NxtDFB;

  L8D20:
    // Pass 1: Just update total count
    // JSR L8D52 - update total cnt
    A      = Length;      // LDA Length
    A      = A + TotCnt;  // CLC / ADC TotCnt
    TotCnt = A;           // STA TotCnt
    Length = 0;           // LDA #0 / STA Length
    goto L8D15;

  L8D26:
    A = PassNbr;                    // LDA PassNbr
    if (A != 0) goto DrtvDone_DFB;  // BNE DrtvDone_DFB - Pass 2

    // Pass 1: Update PC by total count
    A      = Length;      // (already set above)
    A      = A + TotCnt;  // CLC / ADC TotCnt
    TotCnt = A;           // STA TotCnt
    A      = TotCnt;      // LDA TotCnt
    AdvPC();              // JSR AdvPC
    goto DrtvDone_DFB;

  L8D35:
    A = PassNbr;   // LDA PassNbr
    if (A == 0) {  // Pass 1
      A      = Length;
      A      = A + TotCnt;
      TotCnt = A;
      A      = TotCnt;
      AdvPC();
    } else {
      // Pass 2
      StorGMC();  // JSR StorGMC - write instr
      // JSR PrtAsmLn (stubbed)
      // Sync PC with ObjPC after emission
      PC = ObjPC;
    }

  DrtvDone_DFB:
    A      = 0;     // LDA #0
    Length = A;     // STA Length
    A      = 0x83;  // LDA #$83
    ZAB    = A;     // STA ZAB

    Y = 0;      // LDY #0
    A = ZAB;    // LDA ZAB
    C = false;  // CLC
    return;     // RTS
  }

  //=================================================
  // L8D67 - DW/WORD directive handler (ASM3.S lines 563-629)
  // Define Word
  // Format: DW word1[,word2]
  // Emits 16-bit words in little-endian format (low byte first)
  //=================================================
  void HndlDW() {
    g_LastDirectiveCalled = "HndlDW";

  L8D67:
    A = 0x00;  // LDA #$00 - Reverse (little-endian)
    HndlDWCore();
  }

  //=================================================
  // L8D69 - DW/DDB core handler (ASM3.S lines 569-629)
  // Entry: A = EndianF (0x00=DW, 0x01=DDB)
  //=================================================
  void HndlDWCore() {
  L8D69:
    EndianF = A;  // STA EndianF (enter here for DDB)

    // JSR L8D5E - Setup
    X        = 0x21;  // LDX #%00100001
    LstCodeF = X;     // STX LstCodeF
    X        = 0;     // LDX #0
    TotCnt   = X;     // STX TotCnt - cntr

  NxtDW:
    EvalExpr();   // JSR EvalExpr
    SavIndY = Y;  // STY SavIndY - Index into operand field

    Y = ValExpr;             // LDY ValExpr
    X = ValExpr_hi;          // LDX ValExpr+1
    A = EndianF;             // LDA EndianF
    if (A == 0) goto L8D81;  // BEQ L8D81 - Little-endian

    // Big-endian (DDB): high byte first
    GMC[1] = Y;  // STY GMC+1
    GMC[0] = X;  // STX GMC
    goto L8D85;

  L8D81:
    // Little-endian (DW): low byte first
    GMC[0] = Y;  // STY GMC
    GMC[1] = X;  // STX GMC+1

  L8D85:
    if (C) goto L8D98;  // BCS L8D98 - err during eval

    A = PassNbr;             // LDA PassNbr
    if (A == 0) goto L8D98;  // BEQ L8D98 - Pass 1

    A = RelExprF;            // LDA RelExprF
    if (A == 0) goto L8D98;  // BEQ L8D98 - abs expr

    // For relocatable expressions, create RLD entry
    // Set up parameters for AddRLDEnt:
    // A = offset (0 for first byte in GMC)
    // X = size (2 for word)
    // Y = endian (EndianF: 0=DW/little-endian, 1=DDB/big-endian)
    A = 0;        // offset = 0
    X = 2;        // 16-bit value
    Y = EndianF;  // byte order
    AddRLDEnt();  // Create RLD entry

  L8D98:
    Y      = SavIndY;        // LDY SavIndY - Restore index into operand field
    X      = 2;              // LDX #2
    Length = X;              // STX Length
    A      = NxtToken;       // LDA NxtToken
    if (A == 0) goto L8DB4;  // BEQ L8DB4 - cr/space -> done

    A ^= 0x01;               // EOR #$01
    if (A == 0) goto L8DA9;  // BEQ L8DA9 - comma

    // Error
    X = 0x24;
    RegAsmEW(X);
    goto L8D35_DW;

  L8DA9:
    Y++;                  // INY
    Y++;                  // INY
    A = SrcP_at(Y);       // LDA (SrcP),Y
    if (A < SPACE + 1) {  // CMP #SPACE+1 / BCC - Invalid
      X = 0x24;
      RegAsmEW(X);
      goto L8D35_DW;
    }
    AdvSrcP();  // JSR AdvSrcP

  L8DB4:
    A = PassNbr;             // LDA PassNbr
    if (A == 0) goto L8DC7;  // BEQ L8DC7 - Pass 1

    // Pass 2: Store word
    StorGMC();  // JSR L8E28 - Store MC, update PC, print
    // Sync PC with ObjPC after emission
    PC = ObjPC;

  L8DBB:
    A = NxtToken;  // LDA NxtToken - Is cr/space?
    if (A != 0) {  // BNE L8DC2 - No, comma or )
      Y = 0;       // LDY #$00 - Index into src line
      goto NxtDW;
    }
    goto L8D26_DW;  // Done

  L8DC7:
    // Pass 1: Update total count
    A      = Length;      // LDA Length
    A      = A + TotCnt;  // CLC / ADC TotCnt
    TotCnt = A;           // STA TotCnt
    Length = 0;           // LDA #0 / STA Length
    goto L8DBB;

  L8D26_DW:
    A = PassNbr;                   // LDA PassNbr
    if (A != 0) goto DrtvDone_DW;  // BNE DrtvDone_DW - Pass 2

    // Pass 1: Update PC by total count
    A      = Length;
    A      = A + TotCnt;
    TotCnt = A;
    A      = TotCnt;
    AdvPC();
    goto DrtvDone_DW;

  L8D35_DW:
    A = PassNbr;
    if (A == 0) {
      A      = Length;
      A      = A + TotCnt;
      TotCnt = A;
      A      = TotCnt;
    } else {
      StorGMC();
      A = Length;
    }
    AdvPC();

  DrtvDone_DW:
    A      = 0;
    Length = A;
    A      = 0x83;
    ZAB    = A;

    Y = 0;
    A = ZAB;
    C = false;
    return;
  }

  //=================================================
  // L8DD2 - ASC directive handler (ASM3.S lines 634-700)
  // ASCII String
  // Format: ASC "string"
  // Emits ASCII bytes for each character in the string
  //=================================================
  void HndlASC() {
    g_LastDirectiveCalled = "HndlASC";

  L8DD2:
    A       = 0xFF;  // LDA #-1
    StrType = A;     // STA StrType - Flag as ASC (not DCI)

    HndlASC_Core();
    return;
  }

  //=================================================
  // L8DD6 - ASC/DCI core handler
  //=================================================
  void HndlASC_Core() {
    uint8_t bufIdx = 0;

  L8DD6:
    // JSR SkipSpcs
    Y = 0;
    while (SrcP_at(Y) == SPACE) Y++;

    A        = SrcP_at(Y);  // Get delimiter
    Delimitr = A;           // STA Delimitr
    // JSR AdvSrcP - Skip past delimiter
    Y++;

    A = PassNbr;             // LDA PassNbr
    if (A == 0) goto L8E41;  // BEQ L8E41 - Pass 1

    // Pass 2: Emit string bytes
    A        = 0x21;  // LDA #%00100001
    LstCodeF = A;     // STA LstCodeF

    // Y now points past the delimiter, start at byte 1
    bufIdx = 0;  // Track position in GMC buffer

  L8DED:
    A = SrcP_at(Y);                     // LDA (SrcP),Y
    if (A == Delimitr) goto GotDelim2;  // BEQ GotDelim2 - Done
    if (A == CR) goto L8E13;            // BEQ L8E13 - No closing delimiter

    // Store byte in GMC
    A |= msbF;        // ORA msbF - Apply MSB flag if set
    GMC[bufIdx] = A;  // STA GMC-1,Y (using bufIdx instead)
    bufIdx++;
    Y++;  // INY

    if (bufIdx >= 4) {  // CPY #5 - 4 bytes at a time
      Length = bufIdx;  // STY Length
      StorGMC();        // JSR L8E28 - Store MC, update PC etc
      A = Length;
      AdvPC();
      bufIdx = 0;  // Reset buffer index
    }
    goto L8DED;  // Continue

  GotDelim2:
    Y++;             // INY - Skip past 2nd delimiter
    A = SrcP_at(Y);  // JSR WhiteSpc - Do we have a sp/cr?
    if (A != SPACE && A != CR) {
      // Error: invalid delimiter
      X = 0x24;
      RegAsmEW(X);
    }
    // Y--; not needed, bufIdx tracks position

  L8E13:
    // bufIdx now contains the number of remaining bytes
    Length = bufIdx;  // STY Length

    // BIT StrType - ASC?
    if ((int8_t)StrType >= 0) {  // BMI - Yes (StrType = -1 for ASC)
      // DCI: set high bit on last char
      if (bufIdx > 0) {
        A = GMC[bufIdx - 1];  // LDA GMC-1,Y
        A |= 0x80;            // ORA #$80
        GMC[bufIdx - 1] = A;  // STA GMC-1,Y
      }
    }

    if (bufIdx > 0) {
      StorGMC();  // JSR L8E28 - Store MC, update PC etc
      A = Length;
      AdvPC();
    }
    goto DrtvDone_ASC;

  L8E41:
    // Pass 1: Just count bytes
    A = SrcP_at(Y);                 // LDA (SrcP),Y
    if (A == Delimitr) goto L8E4C;  // Found closing delimiter
    if (A == CR) goto L8E4C;        // CR without closing delimiter
    Y++;                            // INY
    goto L8E41;                     // Keep counting

  L8E4C:
    // Y now contains count + 1 (one past delimiter position)
    Y--;    // DEY - Adjust to actual byte count
    A = Y;  // TYA
    if (A > 0) {
      AdvPC();  // JSR AdvPC - (A) has # of bytes to be added to PC
    }

  DrtvDone_ASC:
    A      = 0;
    Length = A;
    A      = 0x83;
    ZAB    = A;

    Y = 0;
    A = ZAB;
    C = false;
    return;
  }

  //=================================================
  // L8E54 - DCI directive handler (ASM3.S lines 704-720)
  // DCI (Inverted Last Character)
  // Format: DCI "string"
  // Emits ASCII bytes, with high bit set on last character
  //=================================================
  void HndlDCI() {
    g_LastDirectiveCalled = "HndlDCI";

  L8E54:
    A                  = msbF;  // LDA msbF
    uint8_t saved_msbF = A;     // PHA - Save current MSB flag

    A    = 0x00;  // LDA #$00
    msbF = A;     // STA msbF - OFF temporarily

    A       = 0x00;  // LDA #$00
    StrType = A;     // STA StrType - Flag as DCI (not ASC)

    HndlASC_Core();  // JSR L8DD6 - Scan as all chars except last as ASC

    // Restore msbF after ASC/DCI handling
    msbF = saved_msbF;  // PLA / STA msbF
    goto DrtvDone_DCI;

  DrtvDone_DCI:
    A = ZAB;
    Y = 0;
    C = false;
    return;
  }

  // .BYTE/.DFB directive handler stub
  void HndlBYTE() {
    g_LastDirectiveCalled = "HndlBYTE";
    // Alias to HndlDFB
    HndlDFB();
  }

  // .WORD/.DW directive handler stub
  void HndlWORD() {
    g_LastDirectiveCalled = "HndlWORD";
    // Alias to HndlDW
    HndlDW();
  }

  // .BLOCK/.DS directive handler stub
  void HndlBLOCK() {
    g_LastDirectiveCalled = "HndlBLOCK";
    // Alias to HndlDS
    HndlDS();
  }

  // .ASCII directive handler stub
  void HndlASCII() {
    g_LastDirectiveCalled = "HndlASCII";
    // Alias to HndlASC
    HndlASC();
  }

  // .DBYTE/.DDB directive handler stub
  void HndlDBYTE() {
    g_LastDirectiveCalled = "HndlDBYTE";

  L8DCD:
    A = 0x01;  // LDA #$01 - Normal (DDB)
    HndlDWCore();
    return;
  }

  //=================================================
  // L8ECA - LST directive handler (ASM3.S lines 778-843)
  // LST [ON|OFF] or LST [+/-][option[,option...]]
  // Controls listing output and options
  // Options (first letter only): C,U,E,W,G,A,V,S
  // Original listing option letters: "CUEWGAVS"
  //=================================================
  void HndlLST() {
    g_LastDirectiveCalled = "HndlLST";

    // L8ECA - LST directive handler
    // Define option letter string (from ASM3.S:857)
    const char* LstOptns = "CUEWGAVS";

    // Array of pointers to the 8 listing flag bytes
    // Maps option letters to their flag bytes
    std::uint8_t* LstFlags[8] = {
        &LstCyc,     // 'C' - List CPU cycle times
        &LstUnAsm,   // 'U' - List unassembled source
        &LstExpMac,  // 'E' - List macro expansion
        &LstWarns,   // 'W' - List warning messages
        &LstGCode,   // 'G' - Generate object code
        &LstASym,    // 'A' - List symbols alphabetically
        &LstVSym,    // 'V' - List symbols by value
        &Lst6Cols    // 'S' - Use 6-column symbol dump
    };

  L8ECA:
    SkipSpcs();         // JSR SkipSpcs
    ChrGot();           // JSR ChrGot
    if (C) goto L8F34;  // BCS L8F34 - non-alphabetic char

    // Check for ON/OFF
    if (A == 'O') {
      ChrGet();           // JSR ChrGet - Get next char
      if (C) goto L8F34;  // BCS L8F34 - non-alphabetic

      if (A == 'N') {
        // ON: Set ListingF MSB
        // Original: SEC; ROR ListingF
        C        = true;
        A        = ListingF;
        A        = (A >> 1) | (C ? 0x80 : 0x00);
        ListingF = A;
        goto L8F22;
      } else if (A == 'F') {
        // OFF: Clear ListingF MSB
        // Original: CLC; ROR ListingF
        C        = false;
        A        = ListingF;
        A        = (A >> 1) | (C ? 0x80 : 0x00);
        ListingF = A;
        goto L8F22;
      } else {
        goto L8F34;  // Invalid after 'O'
      }
    }

    // Parse listing options
    // Implements ASM3.S:793-843 with +/- prefix support
  L8EF3:
    // Default to enable (ON)
    bool enable = true;  // Will be used to set/clear MSB

    // Check for +/- prefix
    ChrGot();  // JSR ChrGot
    if (A == '+') {
      enable = true;      // Enable
      ChrGet();           // JSR ChrGet - Get next char
      if (C) goto L8F34;  // BCS - non-alphabetic
    } else if (A == '-') {
      enable = false;     // Disable
      ChrGet();           // JSR ChrGet - Get next char
      if (C) goto L8F34;  // BCS - non-alphabetic
    } else {
      // No prefix, check if alphabetic
      if (C) goto L8F34;  // Non-alphabetic
    }

    // Look up option letter in LstOptns
    // X will be the index (1-8)
  L8F12:
    X = 8;  // LDX #8 - Start from end
  L8F14:
    if (A == LstOptns[X - 1]) goto L8F1E;  // Match found
    X--;                                   // DEX
    if (X != 0) goto L8F14;                // BNE L8F14
    goto L8F34;                            // No match, error

  L8F1E:
    // Store enable/disable flag to corresponding LstFlags byte
    // Original: LDA OnOffSW; STA LstFlags-1,X
    // We use SEC/ROR or CLC/ROR to set/clear MSB
    A                = *LstFlags[X - 1];
    A                = (A >> 1) | (enable ? 0x80 : 0x00);
    *LstFlags[X - 1] = A;

  L8F22:
    ChrGet();            // JSR ChrGet
    if (!C) goto L8F22;  // BCC L8F22 - alphabetic, continue

    // Check for delimiter or end
    if (A == SPACE) goto L8F37;  // Done
    if (A == CR) goto L8F37;     // Done
    Y++;                         // INY
    if (A == ',') goto L8EF3;    // Next option
    goto L8F34;                  // Invalid

  L8F34:
    // Error: directive operand error
    X = 0x24;     // LDX #$24 - directive operand error
    RegAsmEW(X);  // JSR RegAsmEW

  L8F37:
    // Done
    DrtvDone();  // JMP DrtvDone
  }

  //=================================================
  // HndlLIST - .LIST directive handler
  // Simply enables listing (equivalent to LST ON)
  // Original: ASM3.S has this as a separate handler that does SEC; ROR ListingF
  // No operand parsing - .LIST is a toggle, not an options directive
  //=================================================
  void HndlLIST() {
    g_LastDirectiveCalled = "HndlLIST";

    // Enable listing by setting ListingF MSB
    // Original: SEC; ROR ListingF
    C        = true;
    A        = ListingF;
    A        = (A >> 1) | (C ? 0x80 : 0x00);
    ListingF = A;
    DrtvDone();  // JMP DrtvDone
  }

  //=================================================
  // L8F3A - NOLIST directive handler (ASM3.S lines 846-856)
  // Disables listing output by clearing ListingF MSB
  // Original: ASM3.S:848-856
  // No operand parsing required (ignores any operand)
  //=================================================
  void HndlNOLIST() {
    g_LastDirectiveCalled = "HndlNOLIST";

    // L8F3A - NOLIST directive handler
    // Original: CLC; ROR ListingF; JMP DrtvDone
    // Clear ListingF MSB
    C        = false;
    A        = ListingF;
    A        = (A >> 1) | (C ? 0x80 : 0x00);
    ListingF = A;
    DrtvDone();  // JMP DrtvDone
  }

  //=================================================
  // DoPage - PAGE directive handler (ASM3.S lines 860-871)
  // Inserts a form feed (page break) in listing output
  // Original: ASM3.S:862-871
  // Pass 1: No-op (returns immediately)
  // Pass 2+: Outputs form feed and advances to next record (stubbed for now)
  //=================================================
  void DoPage() {
    g_LastDirectiveCalled = "DoPage";

    // DoPage - PAGE directive handler
    // Original: ASM3.S:862-871

    // Check PassNbr: if Pass 1 (PassNbr == 0), return immediately
    A = PassNbr;  // LDA PassNbr
    if (A == 0) {
      DrtvDone();  // BEQ L8F5E (Pass 1, nothing to do)
      return;
    }

    // Pass 2+: Would output form feed and advance to next record
    // Original code (ASM3.S:865-871):
    //   BIT RVLsting   ; listing output enabled?
    //   BPL L8F5E      ; no, skip
    //   LDA #$0C       ; form feed character
    //   JSR WrtLst     ; write to listing
    //   JMP L9008      ; NextRec (advance to next record)
    //
    // TODO Phase 8: Implement listing output check (RVLsting not yet implemented)
    // TODO Phase 8: Implement form-feed output (WrtLst not yet implemented)
    // TODO Phase 8: Implement NextRec jump (L9008 not yet implemented)
    //
    // For now, stub these operations - they will be filled in Phase 8

    // Stub: Check RVLsting (not yet implemented)
    // if ((int8_t)RVLsting < 0) { ... output form feed ... }

    // Stub: Output form feed (not yet implemented)
    // A = 0x0C;  // Form feed
    // WrtLst(A);

    // Stub: Jump to NextRec (not yet implemented)
    // goto NextRec; // or L9008

    // For now, just fall through to DrtvDone
    DrtvDone();  // Will be replaced with proper flow in Phase 8
  }

  //=================================================
  // HndlSBTL - SBTL directive handler (ASM3.S lines 874-914)
  // Parse and store subtitle string for page headers
  // Original: ASM3.S:874-914, label L8F61
  // Alias: .TITLE dot directive (same handler)
  //
  // SubTtlF flag values:
  //   $00 = No subtitle
  //   $40 = SBTL encountered without string
  //   $FF = String stored in SubTitle buffer
  //
  // Pass 1: Set SubTtlF=$40 and skip to DoPage (optimization)
  // Pass 2+: Parse optional subtitle string with delimiter-based extraction
  //
  // Format: SBTL [/string/]
  //   - First non-space char is delimiter
  //   - String extracted until delimiter repeats or CR
  //   - Max 35 chars for subtitle
  //   - Always ends with page break (DoPage call)
  //=================================================
  void HndlSBTL() {
    // L8F61 - SBTL directive handler
    // Original: ASM3.S:874-914

    g_LastDirectiveCalled = "HndlSBTL";  // Track directive for debugging

    // Set SubTtlF=$40 (marks SBTL encountered)
    A       = 0x40;  // LDA #$40
    SubTtlF = A;     // STA SubTtlF

    // Check if Pass 1
    A = PassNbr;             // LDA PassNbr
    if (A == 0) goto L8FA0;  // BEQ L8FA0 - Pass 1, skip string parsing

    // Pass 2+: Parse optional subtitle string
    SkipSpcs();               // JSR SkipSpcs - Skip to next field
    Delimitr = A;             // STA Delimitr - First char is delimiter
    if (A == CR) goto L8FA0;  // CMP #CR / BEQ L8FA0 - Empty line, no string

    AdvSrcP();    // JSR AdvSrcP - Advance SrcP past delimiter
    SavIndX = X;  // STX SavIndX - Save X

    X = 0;  // LDX #0 - Initialize buffer index
    Y = 0;  // Y already set to 0 by AdvSrcP, but explicit for clarity

  L8F79:
    Y++;                            // INY
    A = SrcP_at(Y);                 // LDA (SrcP),Y - Read character
    if (A == Delimitr) goto L8F8C;  // CMP Delimitr / BEQ L8F8C
    if (A == CR) {                  // CMP #CR - Unterminated string (no closing delimiter)
      // Register directive operand error (0x24) and return
      X = 0x24;     // LDX #$24 - Directive operand error
      RegAsmEW(X);  // JSR RegAsmEW
      X = SavIndX;  // LDX SavIndX - Restore X
      DrtvDone();   // Common directive return
      return;
    }

    SubTitle[X] = A;         // STA SubTitle,X - Store char in buffer
    X++;                     // INX
    if (X < 35) goto L8F79;  // CPX #35 / BCC L8F79 - Continue if < 35

    // X == 35: 35 chars read, next char MUST be delimiter or CR
    Y++;             // INY - Advance to next position
    A = SrcP_at(Y);  // LDA (SrcP),Y - Read next char
    if (A == Delimitr) {
      Y++;  // INY - Advance past delimiter
      goto L8F8D;
    }
    // Not delimiter => error (exceeded max length or unterminated)
    X = 0x24;     // LDX #$24 - Directive operand error
    RegAsmEW(X);  // JSR RegAsmEW
    X = SavIndX;  // LDX SavIndX - Restore X
    DrtvDone();   // Common directive return
    return;

  L8F8C:
    Y++;  // INY - Bump past closing delimiter
    goto L8F8D;

  L8F8D:
    // Common exit point: Y already advanced past delimiter
    // Update source pointer and check for trailing garbage
    AdvSrcP();  // Advance source pointer

    A           = 0;     // LDA #0
    SubTitle[X] = A;     // STA SubTitle,X - Null-terminate string
    A           = 0xFF;  // LDA #$FF
    SubTtlF     = A;     // STA SubTtlF - Flag there is a string (V=1)

    SkipSpcs();               // Skip any spaces after closing delimiter
    if (A == CR) goto L8F9E;  // If CR, valid (end of line)

    // Trailing garbage after delimiter => error
    X = 0x24;     // LDX #$24 - Directive operand error
    RegAsmEW(X);  // JSR RegAsmEW
    X = SavIndX;  // LDX SavIndX - Restore X
    DrtvDone();   // Common directive return
    return;

  L8F9E:
    X = SavIndX;  // LDX SavIndX - Restore X

  L8FA0:
    DoPage();  // JMP DoPage - Always do a form feed (page break)
  }

}  // namespace AsmInternal
