//=================================================
// asm_expr.cpp - Expression Evaluation Functions
// Extracted from asm.cpp #if 0 block (Phase 2)
//=================================================

#include "asm_internal.hpp"

namespace AsmInternal {

  // EvalExpr - Evaluate expressions. No check for numeric overflow
  // Support for +,-,;,/ and bitwise AND ^, OR |,EOR !
  // Ref pg 89 for Expression Syntax adopted for Assembler
  //
  // Expression := [byteopr] Term [opr Term]...]
  // byteopr   := >, <
  //
  // Meaning of:
  // ExprAccF - Expression's accumulated flag bits
  // NxtToken - Use to chk for eo expr(sp/cr), comma, )
  //            Error if -ve
  // SavSEF   - prev subexpr's RelExprF
  // RelExprF - non-zero if subexpr is evaluated fr a relocatable addr
  //          - zero, it's fr an abs addr
  // Ret
  // (A)=
  // C=0 - no errors parsing
  // C=1 - err during eval
  //
  // NB. If relocatable code is generated, only +,- can be used
  void EvalExpr() {
    A        = 0x00;
    RelExprF = A;  // Assume expr's val is absolute not relative
    ExprAccF = A;
    NxtToken = A;
    GblAbsF  = A;  // Assume ZDEF/ZREF
    SkipSpcs();

    // Check for the presence of the byte operators
    // Set (Ret816F)
    // (-1) - 16-bits; 0 - low 8-bits, 1 - hi 8-bits
    A       = -1;  // Default is to ret a 16-bit value
    Ret816F = A;
    A       = SrcP_at(Y);
    if (A != '<') goto L8601;  // EDASM not MERLIN! (BNE)
    Y++;
    Ret816F++;                     // =0 (INC)
    if (Ret816F == 0) goto L8606;  // Proceed to set to 1 (BEQ)
  L8601:
    if (A != '>') goto L8608;  // (BNE)
    Y++;
  L8606:
    Ret816F++;  // 0-lobyte, 1-hibyte (INC)

  L8608:
    if (A == '-') goto L8610;  // unary ops
    if (A != '+') goto L8618;  // Get on with it (BNE)

  L8610:
    A          = 0;
    ValExpr    = A;  // Returned value
    ValExpr_hi = A;
    if (A == 0) goto L8628;  // always (BEQ)

  L8618:
    fprintf(stderr, "    EvalExpr: About to call EvalTerm()\n");
    EvalTerm();  // The leading term is treated differently
    fprintf(stderr, "    EvalExpr: After EvalTerm(), NxtToken=0x%02X\n", NxtToken);
    A = NxtToken;  // Err?
    if ((int8_t)A < 0) {
      fprintf(stderr, "    EvalExpr: Error from EvalTerm(), NxtToken=0x%02X\n", A);
      goto L869C;  // Yes (BMI)
    }

    A          = Accum;
    ValExpr    = A;  // Partial result
    A          = Accum_hi;
    ValExpr_hi = A;
  L8627:
    Y++;  // Eval [opr term]
  L8628:
    A = SrcP_at(Y);
    X = 6;
  L862C:
    if (A == Operators[X]) goto L8636;
    X--;
    if ((int8_t)X >= 0) goto L862C;  // Chk next operator (BPL)
    if ((int8_t)X < 0) goto L8662;   // No hit (BMI)

  L8636:
    if (X < 2) goto L864E;  // +/-? Yes (BCC)

    // Perform additional checks for the operators
    // /,;,&,^,| which cannot operate on rel expr/sub-expr
    A = RelExprF;            // Is it a relative subexpr?
    if (A == 0) goto L864E;  // No, abs (BEQ)
    A = PassNbr;
    if (A == 0) goto L864E;  // (BEQ)
    // BIT RelCodeF - REL code output?
    if ((int8_t)RelCodeF >= 0) goto L864E;  // No, BIN (BPL)
  L8646:
    X = 0x08;  // rel exprn op
    L87FB();
    goto L869C;

  L864E:
    // BIT RelCodeF
    if ((int8_t)RelCodeF >= 0) goto L865C;  // BIN (BPL)
    // BIT Ret816F - Are we returning a 16-bit value?
    if ((int8_t)Ret816F < 0) goto L865C;  // Yes (BMI)
    A = 0x10;                             // Was an EXTeRNal symbol used
    // BIT ExprAccF - during evaln?
    if ((ExprAccF & A) != 0) goto L8646;  // Yes (BNE)
  L865C:
    EvalSExpr();  // Eval new sub expr
    goto L8627;   // Next [opr term]

  L8662:
    X = Ret816F;                    // -1,0,1
    if ((int8_t)X < 0) goto L8674;  // Return 16-bit value (BMI)
    if (X == 0) goto L8670;         // Return val of lobyte (BEQ)

    A       = ValExpr;     // Save lobyte
    Lower8  = A;           // here and
    A       = ValExpr_hi;  // return val of hibyte
    ValExpr = A;           // by storing it here
  L8670:
    A          = 0;
    ValExpr_hi = A;

  L8674:
    fprintf(stderr, "    EvalExpr: About to call GNToken(), Y=%d\n", Y);
    GNToken();  // Chk for comma, ) and cr/space
    fprintf(stderr, "    EvalExpr: After GNToken(), NxtToken=0x%02X\n", NxtToken);
    A |= NxtToken;  // In case of err
    NxtToken = A;
  L867B:
    A = NxtToken;
    Y--;  // Index prev char
    fprintf(stderr, "    EvalExpr: Final check, NxtToken=0x%02X, C will be %d\n", A,
            (A >= 0x80) ? 1 : 0);
    C = (A >= 0x80);  // C=1 if err (CMP #$80)
  }

  // Operators table
  const uint8_t Operators[] = {
      0x2B,  // +
      0x2D,  // -
      0x2A,  // ; (multiply)
      0x2F,  // /
      0x21,  // ! EOR
      0x5E,  // ^ AND
      0x7C   // | OR
  };

  // Forward declarations for operator functions
  void ExprADD();
  void ExprSUB();
  void ExprMUL();
  void ExprDIV();
  void ExprEOR();
  void ExprAND();
  void ExprORA();

  // We are going to process a new subexpression
  // after the operator
  // X=0-6
  void EvalSExpr() {
    A        = RelExprF;
    SavSEF   = A;     // Save it
    A        = 0x00;  // Assume absolute value
    RelExprF = A;

    // Get JMP addr-1 and push to prepare for RTS jump
    A              = L8895[X];  // Get JMP addr-1 hibyte
    uint8_t jmp_hi = A;         // (PHA)
    A              = L888E[X];  // lo byte
    uint8_t jmp_lo = A;         // (PHA)

    Y++;
    EvalTerm();
    A = NxtToken;
    if (A != 0) goto L869A;  // (BNE)

    // Combine the 2 subexprs by calling the operator function
    uint16_t jmp_addr = (jmp_hi << 8) | jmp_lo;
    if (jmp_addr == (uint16_t)(ExprADD - 1))
      ExprADD();
    else if (jmp_addr == (uint16_t)(ExprSUB - 1))
      ExprSUB();
    else if (jmp_addr == (uint16_t)(ExprMUL - 1))
      ExprMUL();
    else if (jmp_addr == (uint16_t)(ExprDIV - 1))
      ExprDIV();
    else if (jmp_addr == (uint16_t)(ExprEOR - 1))
      ExprEOR();
    else if (jmp_addr == (uint16_t)(ExprAND - 1))
      ExprAND();
    else if (jmp_addr == (uint16_t)(ExprORA - 1))
      ExprORA();
    return;

  L869A:
    // Dump JMP addr (PLA; PLA)
    goto L869C;
  }

  void L869C() {
    Y = 0;
    // DB $24 - BIT trick to skip next instruction
    goto L869F;
  }

  void L869F() {
    Y++;
    ChrGot2();               // alphanumeric char?
    if (!C) goto L869F;      // Yes, skip (BCC)
    GNToken();               // Is it cr/space,comma,)
    if (A != 0) goto L869F;  // No (BNE)
    if (A == 0) goto L867B;  // always (BEQ)
  }

  // Entry:
  //  (A)=char to check
  // Ret:
  // Z=1, (A)=0 if space/cr (white space)
  //      (A)=1 if char is ,
  //      (A)=2 if char is )
  // Z=0, (A)=err token
  // (Y)-unchanged
  void GNToken() {
    WhiteSpc();
    bool z_flag = Z;  // PHP - Save Z bit
    X           = A;  // Save char in X-reg (TAX)
    A           = 0x00;
    Z           = z_flag;  // PLP - Was is a cr/space?
    if (Z) goto doRet7;    // Yes (BEQ)
    A = 0x01;
    if (X == ',') goto doRet7;  // (CPX #','; BEQ doRet7)
    A = 0x02;
    if (X == ')') goto doRet7;  // (CPX #')'; BEQ doRet7)
    A = 0x34 + 0x80;            // err token
  doRet7:
    return;
  }

  // Process a term where
  //   Term := Constant, Identifier
  // If a term is an idfer, look up its value
  // Ret:
  //  (Y)=index src line?
  //  (Accum)= Term's 16-bit value
  // NB. Y-reg seems to have a dual purpose. To index the
  // src line and to index an entry of the symbol table
  // Its returned value must be monitored and adjust correctly
  // Todo: Need to check this more closely.
  void EvalTerm() {
    AdvSrcP();  // On ret, (Y)=0
    Y        = 0;
    Accum    = 0;
    Accum_hi = 0;
    ChrGot();                  // Get 1st char
    if (!C && !Z) goto L8716;  // Alphabetic char => idfer
    if (Z) goto L86D6;         // Numeric char
    goto L8781;                // Not alphanumeric

  // Decimal constant
  L86D6:
    A = SrcP_at(Y);  // Get numeric char
    A -= '0';        // $30-$39 -> 0-9 (SEC; SBC #'0')
    A += Accum;      // CLC; ADC Accum
    Accum = A;
    if (!C) goto L86E6;
    Accum_hi++;                     // INC Accum+1
    if (Accum_hi == 0) goto L870E;  // Overflow (BEQ)
  L86E6:
    ChrGet();           // Look 1 char ahead
    if (Z) goto Mul10;  // If numeric, continue (BEQ)
    Y--;                // else move back and ret (DEY)
    return;

  // The next char is numeric so the
  // accumulated result must be x 10
  // before we loop back to process it
  // Logic: 2R x 2 x 2 + 2R = 10R
  Mul10:
    Mul2();                           // 2R
    if (C) goto L870E;                // (BCS)
    A                    = Accum_hi;  // save temporarily (PHA)
    uint8_t saved_acc_hi = A;
    A                    = Accum;
    Mul2();             // 2R x 2
    if (C) goto L870D;  // (BCS)
    Mul2();             // 4R x 2
    if (C) goto L870D;  // (BCS)
    A += Accum;         // + 2R (ADC Accum)
    Accum = A;
    A     = saved_acc_hi;  // (PLA)
    A += Accum_hi;         // (ADC Accum+1)
    Accum_hi = A;
    if (!C) goto L86D6;  // Loop back to process the next char (BCC)
    saved_acc_hi = A;    // overflow (PHA)
  L870D:
    // PLA
  L870E:
    goto L87F9;  // error

  // Identifier
  L8716:
    RsvdId();           // Chk single A,X,Y
    if (C) goto L871C;  // (BCC) - corrected
    return;             // error

  L871C:
    AsmInternal::FindSym();
    if (C) goto L8757;              // Idfer's not in symbol table (BCS)
    X = A;                          // Save idfer's flag byte (TAX)
    C = false;                      // CLC
    if ((int8_t)A < 0) goto L8757;  // Idfer's undefined (BMI)
    A &= external;                  // 0001 0000 Is it declared as an EXTeRNal?
    // BIT ExprAccF
    if ((ExprAccF & A) == 0) goto L8734;  // Ifder is not EXTeRNal (BEQ)
    SavFByt = X;                          // Save flag byte here while (STX SavFByt)
    X       = 0x40;                       // we report Duplicate EXT/ENT (LDX #$40)
    RegAsmEW();
    X = SavFByt;  // Get flagbyte back (LDX SavFByt)
  L8734:
    A = X;  // into (A) (TXA)
  L8735:
    A &= (external | fwdrefd);  // 0001 0001
    A |= ExprAccF;
    ExprAccF = A;
    A        = X;            // Test old flag byte (TXA)
    A &= external;           // 0001 0000
    if (A == 0) goto L874A;  // No, not EXTeRNal (BEQ)
    A = PassNbr;
    if (A == 0) goto L8754;  // (BEQ)
    A       = SymP[Y];       // LoByte of value field (LDA (SymP),Y)
    GblAbsF = A;             // 0=>ZDEF/ZREF
    if (A != 0) goto L8754;  // Its ENTRY/EXTRN (BNE)

  L874A:
    A     = SymP[Y];  // Get value of symbolic idfer (LDA (SymP),Y)
    Accum = A;        // & ret it here
    Y++;
    A        = SymP[Y];
    Accum_hi = A;
    Y--;
  L8754:
    Y--;
    Y--;
    return;

  // Identifer is undefined
  // (A)-symbol's flag byte
  // (Y)-indexing symbol's value field if symbol was found
  // (Y)=0 if symbol not found
  L8757:
    X = PassNbr;
    if (X == 0) goto L876F;  // Its pass 1 (BEQ)
    // BIT Bit02 - Is No-such-label error?
    if ((Bit02 & 0x80) == 0) goto L8764;  // No (BEQ)
    ErrorF++;                             // Flag as err since its pass 2 (INC ErrorF)
    if (ErrorF != 0) goto L8769;          // =1 (BNE)
  L8764:
    A |= nosuchlabel;  // 0000 0010
    Y--;               // (DEY)
    SymP[Y] = A;       // Modified flag byte (STA (SymP),Y)
  L8769:
    X = 0x00;    // Undefined idfer
    Y--;         // what's this for?
    goto L87FB;  // Go report it

  L876F:
    X = A;               // (A)=flag byte (TAX)
    A = fwdrefd;         // 0000 0001
    if (!C) goto L8735;  // Symbol was found but undefined (BCC)
    A |= ExprAccF;       // Symbol not found
    ExprAccF = A;
    A        = undefined | fwdrefd;  // symbol's flag byte
    AsmInternal::AddNode();
    Y--;
    Y--;
    Y--;  // Indexing last char of symbolicname?
    return;

  // 1st char is non-alphanumeric
  // Is it an ASCII char const?
  L8781:
    if (A != '\'') goto L879E;  // Opening single quote? No (BNE)
    Accum_hi = Y;               // Zero the hibyte (STY Accum+1)
    Y++;
    A = SrcP_at(Y);           // Get char within quotes
    if (A == CR) goto L8791;  // (CMP #CR; BNE L8791)
    goto L880F;

  L8791:
    A |= msbF;
    Accum = A;
    Y++;
    A = SrcP_at(Y);              // Look for a
    if (A == '\'') goto doRet8;  // closing single quote (CMP #$27; BEQ doRet8)
    Y--;                         // Move back (DEY)
  doRet8:
    return;

  // Program counter reference
  L879E:
    if (A != '*') goto L87AF;  // Do we have a star? (CMP #'*'; BNE L87AF)

    A        = relative;  // Flag symbol's val is relative
    RelExprF = A;         // & not an absolute addr
    A        = PC;
    Accum    = A;
    A        = PC_hi;
    Accum_hi = A;
    return;

  // Checks for bin/octal/hexdec const
  L87AF:
    if (A != '%') goto L87B9;  // binary (CMP #'%'; BNE L87B9)
    A = '2';
    X = 0x01;
    if (X != 0) goto L87CB;  // always (BNE)

  L87B9:
    if (A != '$') goto L87C3;  // hexdec (CMP #'$'; BNE L87C3)
    A = 0xC0;                  // '@'+$80
    X = 0x04;
    if (X != 0) goto L87CB;  // always (BNE)

  L87C3:
    if (A != '@') goto L880F;  // octal (CMP #'@'; BNE L880F)
    A = '8';
    X = 0x03;
  L87CB:
    RadixCh = A;  // =$32,$38,$C0
    BitsDig = X;  // =$01,$03,$04

  // Conversion starts here
  L87CF:
    ChrGet();                         // Is next char hexdec?
    if ((A & 0x40) == 0) goto L8808;  // No (BVC)
    if (A < '9' + 1) goto L87DA;      // (CMP #'9'+1; BCC L87DA)
    A -= 0x07;                        // 'A'-'F' ($41-$46) -> $3A-$3F (SBC #$07)
  L87DA:
    if (A >= RadixCh) goto L8808;  // Not valid (CMP RadixCh; BCS L8808)
    X = BitsDig;
    if (X == 0x03) goto L87E8;  // Octal (CPX #$03; BEQ L87E8)
    if (X >= 0x03) goto L87E9;  // HexDec (BCS L87E9)

    A <<= 1;  // binary (ASL)
    A <<= 1;  // (ASL)
  L87E8:
    A <<= 1;  // (ASL)
  L87E9:
    A <<= 1;  // (ASL)
    A <<= 1;  // (ASL)
    A <<= 1;  // (ASL)
    A <<= 1;  // (ASL)

  // binary x000 0000, octal xxx0 0000 hex xxxx 0000
  // (X)=# of shifts (% - 1, @ - 3, $ - 4)
  L87ED:
    A <<= 1;  // (ASL)
    uint8_t temp = Accum;
    Accum        = (Accum << 1) | (A >> 7);  // (ROL Accum)
    A            = (A << 1) | (temp >> 7);
    temp         = Accum_hi;
    Accum_hi     = (Accum_hi << 1) | (Accum >> 7);  // (ROL Accum+1)
    C            = (temp & 0x80) != 0;
    if (C) goto L87F9;  // Overflow (BCS)
    X--;
    if (X != 0) goto L87ED;  // (DEX; BNE L87ED)
    if (X == 0) goto L87CF;  // Process another numeral (BEQ)

  L87F9:
    X = 0x06;  // overflow
  L87FB:
    NxtToken = X;
    RegAsmEW();
    A = 0x80;
    A |= NxtToken;
    NxtToken = A;  // $80,$86,$88,$8A
    // NOP
    return;

  L8808:
    Y--;  // Move back (DEY)
    ChrGot();
    if ((A & 0x40) == 0) goto L880F;  // Char is not hexdec (BVC)
    return;

  L880F:
    X = 0x0A;  // expr syntax
    goto L87FB;
  }

  // Mul2 - Multiply accumulator by 2
  void Mul2() {
    uint8_t old_accum = Accum;
    Accum <<= 1;  // ASL Accum
    C                    = (old_accum & 0x80) != 0;
    uint8_t old_accum_hi = Accum_hi;
    Accum_hi             = (Accum_hi << 1) | (old_accum >> 7);  // ROL Accum+1
    C                    = (old_accum_hi & 0x80) != 0;
  }

  // AdvSrcP - Advance source pointer
  void AdvSrcP() {
    Y = 0;
  }

  // ; operator
  void ExprMUL() {
    AdvSrcP();
    ValExpr_2 = Y;  // zero these
    ValExpr_3 = Y;
    Y         = 16;  // # of times
  L881D:
    A             = ValExpr;
    uint8_t old_c = C;
    C             = (A & 0x01) != 0;  // Check LSB before shift
    A >>= 1;                          // (LSR)
    if (!C) goto L882E;               // (BCC)

    C = false;  // CLC
    // Add Accum_2 and Accum_3 to ValExpr_2 and ValExpr_3
    A = ValExpr_2;
    A += Accum_2;
    ValExpr_2 = A;
    A         = ValExpr_3;
    if (C) A++;  // Add carry
    A += Accum_3;
    ValExpr_3 = A;

  L882E:
    // Shift right the 4-byte value (ValExpr, ValExpr_hi, ValExpr_2, ValExpr_3)
    // From MSB to LSB
    C = (ValExpr_3 & 0x01) != 0;
    ValExpr_3 >>= 1;  // ROR ValExpr_3

    uint8_t temp = ValExpr_2;
    ValExpr_2    = (ValExpr_2 >> 1) | (C ? 0x80 : 0);  // ROR ValExpr_2
    C            = (temp & 0x01) != 0;

    temp       = ValExpr_hi;
    ValExpr_hi = (ValExpr_hi >> 1) | (C ? 0x80 : 0);  // ROR ValExpr_hi
    C          = (temp & 0x01) != 0;

    ValExpr = (ValExpr >> 1) | (C ? 0x80 : 0);  // ROR ValExpr

    Y--;
    if (Y != 0) goto L881D;  // (BNE)
  }

  // / operator
  void ExprDIV() {
    AdvSrcP();
    ValExpr_2 = Y;
    ValExpr_3 = Y;  // zero these
    Y         = 16;
  L8842:
    uint8_t old_val = ValExpr;
    ValExpr <<= 1;  // ASL ValExpr - dividend
    C          = (old_val & 0x80) != 0;
    old_val    = ValExpr_hi;
    ValExpr_hi = (ValExpr_hi << 1) | (C ? 1 : 0);  // ROL ValExpr+1
    C          = (old_val & 0x80) != 0;
    old_val    = ValExpr_2;
    ValExpr_2  = (ValExpr_2 << 1) | (C ? 1 : 0);  // ROL ValExpr+2
    C          = (old_val & 0x80) != 0;
    old_val    = ValExpr_3;
    ValExpr_3  = (ValExpr_3 << 1) | (C ? 1 : 0);  // ROL ValExpr+3
    C          = (old_val & 0x80) != 0;

    C = true;  // SEC
    A = ValExpr_2;
    A -= Accum;  // SBC Accum - divisor
    X = A;       // (TAX)
    A = ValExpr_3;
    A -= Accum_hi;      // SBC Accum+1
    if (C) goto L885C;  // (BCC L885C - inverted)
    ValExpr_2 = X;      // (STX ValExpr+2)
    ValExpr_3 = A;
    ValExpr++;  // INC ValExpr
  L885C:
    Y--;
    if (Y != 0) goto L8842;  // (BNE)
  }

  // - operator
  void ExprSUB() {
    A = RelExprF;
    A ^= SavSEF;  // prev subexpr's RelExprF (EOR SavSEF)
    RelExprF = A;
    A        = Accum_hi;  // Do 1's complement (LDA Accum+1)
    A ^= 0xFF;            // (EOR #$FF)
    Accum_hi = A;
    A        = Accum;
    A ^= 0xFF;          // (EOR #$FF)
    C = true;           // SEC - Proceed to add 1
    if (C) goto L887C;  // giving 2's complement (BCS - always)
  }

  // + operator
  void ExprADD() {
    A = SavSEF;  // Get prev subexpr's RelExprF
    A |= RelExprF;
    RelExprF = A;
    C        = false;  // CLC
    A        = Accum;
  L887C:
    A += ValExpr;  // (ADC ValExpr)
    ValExpr = A;
    A       = Accum_hi;
    A += ValExpr_hi;  // (ADC ValExpr+1)
    ValExpr_hi = A;
  }

  // This table of JMP (via RTS) addresses is split into 2 parts
  const uint8_t L888E[] = {
      static_cast<uint8_t>((reinterpret_cast<uintptr_t>(ExprADD) - 1) & 0xFF),  // lobyte
      static_cast<uint8_t>((reinterpret_cast<uintptr_t>(ExprSUB) - 1) & 0xFF),
      static_cast<uint8_t>((reinterpret_cast<uintptr_t>(ExprMUL) - 1) & 0xFF),
      static_cast<uint8_t>((reinterpret_cast<uintptr_t>(ExprDIV) - 1) & 0xFF),
      static_cast<uint8_t>((reinterpret_cast<uintptr_t>(ExprEOR) - 1) & 0xFF),
      static_cast<uint8_t>((reinterpret_cast<uintptr_t>(ExprAND) - 1) & 0xFF),
      static_cast<uint8_t>((reinterpret_cast<uintptr_t>(ExprORA) - 1) & 0xFF)};
  const uint8_t L8895[] = {
      static_cast<uint8_t>((reinterpret_cast<uintptr_t>(ExprADD) - 1) >> 8),  // hibyte
      static_cast<uint8_t>((reinterpret_cast<uintptr_t>(ExprSUB) - 1) >> 8),
      static_cast<uint8_t>((reinterpret_cast<uintptr_t>(ExprMUL) - 1) >> 8),
      static_cast<uint8_t>((reinterpret_cast<uintptr_t>(ExprDIV) - 1) >> 8),
      static_cast<uint8_t>((reinterpret_cast<uintptr_t>(ExprEOR) - 1) >> 8),
      static_cast<uint8_t>((reinterpret_cast<uintptr_t>(ExprAND) - 1) >> 8),
      static_cast<uint8_t>((reinterpret_cast<uintptr_t>(ExprORA) - 1) >> 8)};

  // | operator bitwise OR
  void ExprORA() {
    A = Accum;
    A |= ValExpr;
    ValExpr = A;
    A       = Accum_hi;
    A |= ValExpr_hi;
    ValExpr_hi = A;
  }

  // ^ operator bitwise AND
  void ExprAND() {
    A = Accum;
    A &= ValExpr;
    ValExpr = A;
    A       = Accum_hi;
    A &= ValExpr_hi;
    ValExpr_hi = A;
  }

  // ! operator - bitwise EOR
  void ExprEOR() {
    A = Accum;
    A ^= ValExpr;
    ValExpr = A;
    A       = Accum_hi;
    A ^= ValExpr_hi;
    ValExpr_hi = A;
  }

}  // namespace AsmInternal
