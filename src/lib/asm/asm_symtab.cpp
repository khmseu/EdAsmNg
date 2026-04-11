//=================================================
// Symbol Table Management for EdAsmNg Assembler
// Extracted from ASM/ASM1.S
//
// This module contains the symbol table hash functions and node management:
// - FindSym: Lookup symbol in hash table
// - HashFn: Compute hash value for symbol name
// - AddNode: Add new symbol to table
//
// Original code from Apple II EDASM assembler, translated to C++
//=================================================

#include "asm_internal.hpp"

namespace AsmInternal {

  //=================================================
  // ($88C3) FindSym - Symbolic Name Lookup
  // Entry: (Y)=0
  //        (SrcP),Y points to 1st char of symbolic name in source line
  // Exit:  If symbolic name found,
  //          clr(C), (A)=symbol's flag byte, (Y)-> symbol's value field
  //          RelExprF updated based on symbol's relative bit
  //        If not found, set(C)
  //=================================================
  void FindSym() {
    // Declare variables at function scope to avoid goto issues
    std::uint8_t  old_low;
    std::uint8_t  flag;
    std::uint8_t* SymP_ptr;

    HashFn();                // JSR HashFn - Hash the label
    Y = HeaderT_ptr[X + 1];  // LDY HeaderT+1,X - (X)=hashed value x 2 already
    if (Y == 0) goto L8908;  // BEQ L8908 - Empty slot => Not Found!

    // Get ptr to 1st node in singly linked list (chain)
    A = HeaderT_ptr[X];  // LDA HeaderT,X

  FindLoop:
    SymP = A;             // STA SymP
    SymP |= (Y << 8);     // STY SymP+1
    PrvSymP = A;          // STA PrvSymP
    PrvSymP |= (Y << 8);  // STY PrvSymP+1

    Y        = 0;
    SymP_ptr = SimPtrToMemPtr(SymP);  // Convert simulated address to real pointer
    A        = SymP_ptr[Y];           // LDA (SymP),Y
    NxtSymP  = A;                     // STA NxtSymP - Point to next node
    Y++;
    A = SymP_ptr[Y];      // LDA (SymP),Y - in chain
    NxtSymP |= (A << 8);  // STA NxtSymP+1 - If NIL ($0000) end of chain

    old_low = SymP & 0xFF;                                // Save old low byte for carry check
    A       = 2;                                          // LDA #2 - Skip past link pointer
    A    = static_cast<std::uint8_t>(A + (SymP & 0xFF));  // ADC SymP - to point @ the symbolic name
    SymP = (SymP & 0xFF00) | A;                           // STA SymP
    if (A >= old_low) goto L88EC;  // BCC L88EC - This will allow us to use the
    SymP += 0x100;                 // INC SymP+1 - same (Y) index for SrcP & SymP

  L88EC:
    Y        = static_cast<std::uint8_t>(-1);  // LDY #-1 - Prepare to get 1st char of src line
    SymP_ptr = SimPtrToMemPtr(SymP);           // Update pointer after SymP changed

  L88EE:
    ChrGet2();                         // JSR ChrGet2
    A |= 0x80;                         // ORA #$80
    if (A == SymP_ptr[Y]) goto L88EE;  // BEQ L88EE - Got a match, try next char
    A &= 0x7F;                         // AND #$7F - Match last char of symbolic name
    if (A != SymP_ptr[Y]) goto L8902;  // BNE L8902 - No
    ChrGet2();                         // JSR ChrGet2 - Maybe but is next char alphanumeric?
    if (C) goto L890A;                 // BCS L890A - No, probably a CR/SPACE => total match

  L8902:
    A = NxtSymP & 0xFF;         // LDA NxtSymP - On fall thru, a partial match
    Y = (NxtSymP >> 8);         // LDY NxtSymP+1 - End of this chain?
    if (Y != 0) goto FindLoop;  // BNE FindLoop - No, continue with next node in chain

  L8908:
    C = true;  // SEC - Flag symbolic name not found
    return;

  L890A:
    A = SymP_ptr[Y];                // LDA (SymP),Y - Get symbol's flag byte
    if ((int8_t)A < 0) goto L891A;  // BMI L891A - Not defined yet

    A           = A & 0b10111111;        // AND #%10111111 - Set it to referenced
    SymP_ptr    = SimPtrToMemPtr(SymP);  // Ensure pointer is updated
    SymP_ptr[Y] = A;                     // STA (SymP),Y
    flag        = A;                     // PHA - Save flag byte
    A           = A & relative;          // AND #relative - Retain this bit
    A |= RelExprF;                       // ORA RelExprF
    RelExprF = A;                        // STA RelExprF
    A        = flag;                     // PLA - Restore

  L891A:
    C = false;  // CLC - Flag symbolic name found
    Y++;        // INY - Indexing value field
    return;     // RTS - or 1st char in next src line?
  }

  //=================================================
  // ($8932) HashFn - Hash function
  // Entry: (Y)=0
  //        (SrcP),Y points to 1st character in source label field
  // Exit:  (X)=hash value x 2, which is the offset into HeaderT
  //        (Y) preserved
  //
  // This function hashes symbolic names using a simple 8-bit hash.
  // The Header Table can have at most 128 entries. Each
  // entry is a 2-byte pointer (called a HEADER NODE) to a
  // singly linked list of nodes (chain of nodes).
  // Only the first 3 chars of a symbolic name are used
  // by this hashing function.
  // Ret:
  // Y - preserved
  // (X)=8-bit value which is used to index HeaderT
  //=================================================
  void HashFn() {
    uint8_t saved_y = Y;                         // TYA / PHA - save (Y)
    A               = 0;                         // LDA #0
    HashIdx         = A;                         // STA HashIdx - zero the hash value
    ChrGot2();                                   // JSR ChrGot2 - 1st char of label
    A &= 0b00000011;                             // AND #%00000011 - 0000 00xx
    bool carry = A & 1;                          // LSR sets carry to bit 0
    A >>= 1;                                     // LSR
    HashIdx = (HashIdx << 1) | (carry ? 1 : 0);  // ROL HashIdx
    carry   = A & 1;                             // LSR sets carry to bit 0
    A >>= 1;                                     // LSR
    HashIdx = (HashIdx << 1) | (carry ? 1 : 0);  // ROL HashIdx - 0000 00xx

    A                  = HashIdx;  // LDA HashIdx
    uint8_t saved_hash = A;        // PHA - save temporarily
    A                  = 0;        // LDA #0
    HashIdx            = A;        // STA HashIdx
    ChrGet2();                     // JSR ChrGet2 - Is 2nd char alphanumeric?
    if (!C) goto L8947;            // BCC L8947 - yes

    // one-char label
    // PLA - Discard hashed value
    Y       = saved_y;     // PLA / TAY - restore Y-reg
    saved_y = Y;           // PHA
    A       = SrcP_at(Y);  // LDA (SrcP),Y - Get char again ($41-$5A)
    A &= 0b00011111;       // AND #%00011111 - $01-$1A (note: A,X,Y may be missing)
    A <<= 1;               // ASL
    A <<= 1;               // ASL
    goto L897D;

  L8947:
    A     = A & 0b00000111;                      // AND #%00000111 - 0000 0yyy
    carry = A & 1;                               // LSR sets carry to bit 0
    A >>= 1;                                     // LSR
    HashIdx = (HashIdx << 1) | (carry ? 1 : 0);  // ROL HashIdx
    carry   = A & 1;                             // LSR sets carry to bit 0
    A >>= 1;                                     // LSR
    HashIdx = (HashIdx << 1) | (carry ? 1 : 0);  // ROL HashIdx
    carry   = A & 1;                             // LSR sets carry to bit 0
    A >>= 1;                                     // LSR
    HashIdx = (HashIdx << 1) | (carry ? 1 : 0);  // ROL HashIdx - =0000 0yyy
    HashIdx <<= 1;                               // ASL HashIdx - =0000 yyy0
    HashIdx <<= 1;                               // ASL HashIdx - =000y yy00
    A = saved_hash;                              // PLA - (A)=0000 00xx
    A ^= HashIdx;                                // EOR HashIdx
    saved_hash = A;                              // PHA - (A)=000y yyxx
    A          = 0x00;                           // LDA #$00
    HashIdx    = A;                              // STA HashIdx
    ChrGet2();                                   // JSR ChrGet2 - Is 3rd char alphanumeric?
    if (!C) goto L8968;                          // BCC L8968 - Yes
    A = saved_hash;                              // PLA - 2-char label field
    A <<= 1;                                     // ASL
    A <<= 1;                                     // ASL - 0yyy xx00
    if ((int8_t)A >= 0) goto L897D;              // BPL L897D - always

  // 3rd char
  L8968:
    A &= 0b00000111;          // AND #%00000111 - 0000 0zzz
    carry = (A & 0x80) != 0;  // ASL x6 - shift left 6 times
    A <<= 1;
    carry = (A & 0x80) != 0;
    A <<= 1;
    carry = (A & 0x80) != 0;
    A <<= 1;
    carry = (A & 0x80) != 0;
    A <<= 1;
    carry = (A & 0x80) != 0;
    A <<= 1;
    carry = (A & 0x80) != 0;
    A <<= 1;                                        // ASL - 6th shift, carry from last ASL
    HashIdx = (HashIdx >> 1) | (carry ? 0x80 : 0);  // ROR HashIdx - (HashIdx)=z000 0000
    carry   = (A & 0x80) != 0;
    A <<= 1;                                        // ASL
    HashIdx = (HashIdx >> 1) | (carry ? 0x80 : 0);  // ROR HashIdx - (HashIdx)=zz00 0000
    carry   = (A & 0x80) != 0;
    A <<= 1;                                        // ASL
    HashIdx = (HashIdx >> 1) | (carry ? 0x80 : 0);  // ROR HashIdx - (HashIdx)=zzz0 0000
    HashIdx >>= 1;                                  // LSR HashIdx - (HashIdx)=0zzz 0000
    A = saved_hash;                                 // PLA - (A)=000y yyxx
    A ^= HashIdx;                                   // EOR HashIdx

  L897D:
    A <<= 1;            // ASL - x2 to make hash value into an index
    HashIdx = A;        // STA HashIdx - 0,2,4,...,254
    X       = A;        // TAX
    Y       = saved_y;  // PLA / TAY - restore (Y)
  }

  //=================================================
  // ($89A9) AddNode - Add Symbol to Table
  // Entry: On Pass 1, after FindSym indicates symbol doesn't
  //        exist (set(C)). On entry, SrcP points to 1st char
  //        in symbolic name (Y=0).
  //        (A)=flag byte to be assigned to new symbol
  //        HashIdx=hashed value for this symbol
  //        PrvSymP=ptr to HEADER NODE (if new chain)
  //              or ptr to last node in chain
  //              (if adding to existing chain)
  // Exit:  clr(C), new node added to end of chain
  //        If sym table full (EndSymT=RLDEnd), set(C) and abort
  //
  // Flag byte Format for symbols (FByte), from MSB to LSB:
  //   Bit 7 - Undefined (1=undefined, 0=defined)
  //   Bit 6 - Unreferenced (1=unrefd, 0=referenced)
  //   Bit 5 - Relative (1=relative, 0=absolute)
  //   Bit 4 - External (1=external, 0=not external)
  //   Bit 3 - Entry point (1=entry, 0=not entry)
  //   Bit 2 - Reserved
  //   Bit 1 - Reserved (used for "no such label" errors)
  //   Bit 0 - Forward reference (1=fwd ref, 0=backward ref)
  //
  // The symbol table grows downward (from StrtSymT toward lower memory).
  // Each symbol node has this structure:
  //   +0,+1: Link to next node (2 bytes, $0000 if end of chain)
  //   +2...: Symbol name (variable length, MSB set except last char)
  //   Last+1: Flag byte (format above)
  //   Last+2,Last+3: Symbol value/address (2 bytes, little-endian)
  //=================================================
  void AddNode() {
    // Declare variables at function scope to avoid goto issues
    uint8_t  flag;
    uint16_t sum_low;
    bool     carry_add;

    flag = A;    // PHA - Save flag byte
    Y    = 0;    // LDY #0
    ChrGot();    // JSR ChrGot
    if (C) {     // BCC L89B4
      C = true;  // SEC
      return;
    }

  L89B4:
    X = HashIdx;                 // LDX HashIdx - Is there already a chain
    A = HeaderT_ptr[X + 1];      // LDA HeaderT+1,X - associated with this value?
    if (A != 0) goto L89C8;      // BNE L89C8 - Yes
    A         = 0x00;            // LDA #<HeaderT - Start a new singly linked list
    sum_low   = A + HashIdx;     // ADC HashIdx
    A         = sum_low & 0xFF;  // Low byte result
    PrvSymP   = A;               // STA PrvSymP - Point @ $BCxx
    carry_add = sum_low > 0xFF;  // Carry from low byte addition
    A         = 0xBC;            // LDA #>HeaderT
    A         = static_cast<std::uint8_t>(A + (carry_add ? 1 : 0));  // ADC #0
    PrvSymP |= (A << 8);                                             // STA PrvSymP+1

    // NB: We assume PrvSymP have been set correctly
    // if it's not a new chain. In order to set this ptr
    // correctly for an existing chain, FindSym should
    // be called before AddNode.
  L89C8:
    A                         = Y;                        // TYA - A=Y=0
    std::uint8_t* EndSymT_ptr = SimPtrToMemPtr(EndSymT);  // Convert simulated address
    EndSymT_ptr[Y]            = A;                        // STA (EndSymT),Y
    A                         = EndSymT & 0xFF;           // LDA EndSymT
    std::uint8_t* PrvSymP_ptr = SimPtrToMemPtr(PrvSymP);  // Convert simulated address
    PrvSymP_ptr[Y]            = A;                        // STA (PrvSymP),Y
    A                         = Y;                        // TYA - A=0
    Y++;                                                  // INY - Y=1
    EndSymT_ptr[Y] = A;             // STA (EndSymT),Y - Set link field to NIL ($0000)
    A              = EndSymT >> 8;  // LDA EndSymT+1
    PrvSymP_ptr[Y] = A;             // STA (PrvSymP),Y - Point @ new entry

    Y--;                                                                       // DEY - =0
    std::uint8_t old_low_1 = EndSymT & 0xFF;                                   // Save old low byte
    A                      = 2;                                                // LDA #2 - Skip past
    A                      = static_cast<std::uint8_t>(A + (EndSymT & 0xFF));  // ADC EndSymT
    EndSymT                = (EndSymT & 0xFF00) | A;  // STA EndSymT - the link field so that
    if (A >= old_low_1) goto L89E3;                   // BCC L89E3 - Y reg can be used to
    EndSymT += 0x100;                                 // INC EndSymT+1 - index both SrcP & EndSymT

    // Labels are stored in the symbol table with
    // msb on except last char.
    // On fall thru, Y=0 for both SrcP and EndSymT.
  L89E3:
    EndSymT_ptr = SimPtrToMemPtr(EndSymT);  // Update pointer after EndSymT changed
    ChrGot();                               // JSR ChrGot

  L89E6:
    A |= 0x80;                                   // ORA #$80 - msb on
    EndSymT_ptr[Y] = A;                          // STA (EndSymT),Y
    ChrGet2();                                   // JSR ChrGet2 - Is char alphanumeric?
    if (!C) goto L89E6;                          // BCC L89E6 - Yes
    Y--;                                         // DEY
    A = EndSymT_ptr[Y];                          // LDA (EndSymT),Y
    A &= 0x7F;                                   // AND #$7F - Remove msb for last char
    EndSymT_ptr[Y] = A;                          // STA (EndSymT),Y
    Y++;                                         // INY - Skip past last char
    A = flag;                                    // PLA - Get flag byte that was passed
    if (A == (undefined | fwdrefd)) goto L8A00;  // CMP #undefined+fwdrefd / BEQ L8A00
    A |= RelExprF;                               // ORA RelExprF - relative if bit20=1
    // Minimal DSECT support: if DummyF is negative (signed), mark new symbols as relative
    if ((int8_t)DummyF < 0) {
      A |= relative;
    }
    A |= unrefd;  // ORA #unrefd - Mark as unreferenced

  L8A00:
    EndSymT_ptr[Y] = A;          // STA (EndSymT),Y - Set flag byte
    Y++;                         // INY
    A              = PC & 0xFF;  // LDA PC - Set addr associated
    EndSymT_ptr[Y] = A;          // STA (EndSymT),Y
    Y++;                         // INY
    A              = PC >> 8;    // LDA PC+1 - w/this symbol
    EndSymT_ptr[Y] = A;          // STA (EndSymT),Y

    A    = EndSymT & 0xFF;                    // LDA EndSymT
    SymP = A;                                 // STA SymP - Point @ symbolic name
    A    = EndSymT >> 8;                      // LDA EndSymT+1
    SymP |= (A << 8);                         // STA SymP+1
    std::uint8_t old_low_2 = EndSymT & 0xFF;  // Save old low byte
    A                      = Y;               // TYA
    A       = static_cast<std::uint8_t>(A + 1 + (EndSymT & 0xFF));  // SEC / ADC EndSymT
    EndSymT = (EndSymT & 0xFF00) | A;                               // STA EndSymT - next availmem
    if (A >= old_low_2) goto L8A1E;                                 // BCC L8A1E - detect carry out
    EndSymT += 0x100;                                               // INC EndSymT+1

  L8A1E:
    // CMP RLDEnd: sets carry if EndSymT_lo >= RLDEnd_lo
    uint8_t cmp_low    = EndSymT & 0xFF;              // LDA EndSymT
    bool    carry_cmp  = cmp_low >= (RLDEnd & 0xFF);  // CMP RLDEnd
    uint8_t cmp_high   = EndSymT >> 8;                // LDA EndSymT+1
    uint8_t rld_high   = RLDEnd >> 8;
    uint8_t sbc_result = cmp_high - rld_high - (carry_cmp ? 0 : 1);  // SBC RLDEnd+1
    // BCC branches if borrow occurred (i.e., EndSymT < RLDEnd unsigned)
    bool borrow = (carry_cmp ? (cmp_high < rld_high) : (cmp_high <= rld_high));
    if (borrow) {  // BCC doRtn3
    doRtn3:
      C = false;
      return;
    }

    X = 0x12;     // LDX #$12 - sym/rld table full!
    RegAsmEW(X);  // JSR RegAsmEW
    CanclAsm(0);  // JMP CanclAsm
  }

}  // namespace AsmInternal
