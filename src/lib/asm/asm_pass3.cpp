//=================================================
// Pass 3: Symbol Table Listing
// Extracted from asm.cpp for modularization pilot
// Original: ASM3.S
//=================================================

#include "asm_internal.hpp"

// All Pass 3 code goes in AsmInternal namespace for proper linkage
namespace AsmInternal {

  // Forward declarations for Pass 3 functions
  static void DoPass3_impl();
  static bool LD198();
  static void DoSort();
  static void PrSymTbl();
  static bool AdvRecP();

  //=================================================
  // Pass 3 Main Entry Point
  // Performs symbol table listing (alphabetic and/or by value)
  // Original: ASM3.S lines ~1-350
  //=================================================
  static void DoPass3_impl() {
    std::uint8_t  A, X, Y;
    std::uint8_t* StrtSymT_ptr  = nullptr;
    std::uint8_t* SymNodeP_ptr  = nullptr;
    std::uint8_t* SymP_ptr      = nullptr;
    std::uint8_t* UnsortedP_ptr = nullptr;
    std::uint8_t  orig_strt_lo  = 0;  // For carry detection in LD099
    std::uint8_t  orig_node_lo  = 0;  // For carry detection in LD096

    // DEBUG: Add safety counter for LD076 loop
    int       loop_count = 0;  // Initialize loop counter for safety
    const int MAX_LOOPS  = 1000;

    // Check if alphabetic symbol listing requested
    A = LstASym;
    A |= LstVSym;    // Or value-ordered symbol listing
    if (A & 0x80) {  // BMI ChkPrtCols - Yes, at least one is enabled
      goto ChkPrtCols_label;
    }
    return;  // No symbol listing requested, return

  ChkPrtCols_label:
    // Determine number of printing columns based on output device
    A = 4;                  // Default to 4 columns
    if (Lst6Cols & 0x80) {  // BIT Lst6Cols; BPL UsePrtr - 6-column mode requested?
      A = 6;                // Yes, use 6 columns
    }

    X = PrSlot;    // Check if printer is active
    if (X != 0) {  // BNE SetPtrCols - Yes
      goto SetPtrCols_label;
    }
    A = 2;  // 40-col std video

  SetPtrCols_label:
    NumCols = A;  // # of print cols=2,4,6

    // Find out if the symbol table is empty by checking the header nodes
    A = 0;
    Y = 0;

  ChkLoop:
    if (A != HeaderT_ptr[Y]) {  // Do we have an empty symbol table?
      goto LD025_label;         // No
    }
    Y++;
    if (Y != 0) {
      goto ChkLoop;
    }
    goto LD16E_label;  // All zeroes => Yes, empty symbol table

  LD025_label:
    A     = 0;
    SortF = A;  // Default to sort by symbol

    if (!(SubTtlF & 0x40)) {  // BIT SubTtlF; BVC LD056 - Output subtitle?
      goto LD056_label;       // No
    }

    A       = 0xFF;
    SubTtlF = A;  // There is a subtitle string

    X        = ChnFile;     // Get the src Pathname
    A        = PNTable[X];  // from table of ptrs
    SrcPathP = A;
    A        = PNTable[X + 1];
    SrcPathP = (SrcPathP & 0xFF) | (A << 8);

    // The buffers below are overwritten since we are done w/assembly
    X         = 12;
    ChnPNB[X] = X;

    do {
      A         = SymbolTxt[X - 1];
      ChnPNB[X] = A;
      X--;
    } while (X != 0);

    X = 16;  // Include null char
    do {
      A           = SortedTxt[X];
      SubTitle[X] = A;
      X--;
    } while (X != 0xFF);  // BPL LD04D

  LD056_label:
    A = EndSymT & 0xFF;
    if (A != 0) {
      goto LD05C_label;
    }
    EndSymT = (EndSymT - 0x100) | 0xFF;  // DEC EndSymT+1

  LD05C_label:
    EndSymT--;  // DEC EndSymT

    // BIT DskSrcF
    A = HeaderT & 0xFF;
    Y = (HeaderT >> 8);
    if (DskSrcF & 0x80) {  // BMI LD06A
      goto LD06A_label;    // branch never taken in original
    }

    A = StrtSymT >> 8;
    Y = StrtSymT & 0xFF;

  LD06A_label:
    SymNodeP = (A << 8) | Y;  // Points @ node containing symbolicname
    SavSTS   = SymNodeP;      // Save Start of SymTbl as it will be trashed

    A = 2;  // Skip over ptr to next node
    goto LD099_label;

    // The code below will trash the link ptrs (field) of nodes
    // (StrtSymT) - points @ symbolicname field of the node
    // (SymNodeP) - points @ link field of the same node
    // The purpose is to change the layout of the Nodes
    // removing the link field altogether
    // Layout of record now looks like this:
    //   symbolicname (variable in length)
    //   flagbyte
    //   16-bit value

  LD076_label: {
    loop_count++;
    if (loop_count > MAX_LOOPS) {
      return;
    }
    Y            = 0;
    StrtSymT_ptr = SimPtrToMemPtr(StrtSymT);  // Convert simulated address
    SymNodeP_ptr = SimPtrToMemPtr(SymNodeP);  // Convert simulated address

  LD078_label:
    A               = StrtSymT_ptr[Y];  // Get char fr symbolicname
    SymNodeP_ptr[Y] = A;                // move it forward in node
    if (A & 0x80) {                     // MSB=1, not end of symbolicname yet
      Y++;
      if (Y > 20) {
        return;
      }
      goto LD078_label;
    }

  LD081_label:
    X = 3;
    do {
      Y++;
      A               = StrtSymT_ptr[Y];  // Copy flagbyte and value field
      SymNodeP_ptr[Y] = A;
      X--;
    } while (X != 0);

    Y++;
    A = Y;
    // CLC is implicit

  LD096_label:
    orig_node_lo = (SymNodeP & 0xFF);  // Save original low byte
    A += orig_node_lo;                 // Point @ next node
    SymNodeP = (SymNodeP & 0xFF00) | A;
    if (A >= orig_node_lo) {  // No carry
      goto LD099_label0;
    }
    SymNodeP += 0x100;  // INC SymNodeP+1 - Carry occurred

  LD099_label0:
    Y++;  // Skip over link field
    Y++;
    A = Y;
  }

  LD099_label:
    // CLC is implicit
    orig_strt_lo = (StrtSymT & 0xFF);  // Save original low byte
    A += orig_strt_lo;
    StrtSymT = (StrtSymT & 0xFF00) | A;  // Point @ symbolicname of next node
    if (A >= orig_strt_lo) {             // BCC LD0A2 - No carry
      goto LD0A2_label;
    }
    StrtSymT += 0x100;  // INC StrtSymT+1 - Carry occurred

  LD0A2_label:
    // CMP EndSymT - EO symbol table?
    // Check if StrtSymT < EndSymT (more symbols to process)
    if (StrtSymT < EndSymT) {  // BCC LD076 - Continue loop
      goto LD076_label;
    }

    Y = (SymNodeP >> 8);
    X = (SymNodeP & 0xFF);
    if (X != 0) {
      goto LD0B1_label;
    }
    Y--;

  LD0B1_label:
    X--;
    EndSymT = (Y << 8) | X;  // New end of Symbol table

    A        = (SavSTS & 0xFF);
    StrtSymT = (StrtSymT & 0xFF00) | A;  // Restore Start of symbol table
    A        = (SavSTS >> 8);
    StrtSymT = (StrtSymT & 0xFF) | (A << 8);

    // NOP

  LD0BF_label:
    // The code below builds a fixed array of 2/4-bytes entries for sorting
    A    = (StrtSymT & 0xFF);  // Start w/the 1st rec
    SymP = A;
    A    = (StrtSymT >> 8);
    SymP = (SymP & 0xFF) | (A << 8);

    // Sort by Symbol - array of 2-bytes entries
    // Sort by Address - array of 4-byte entries
    A = 2;                  // Default to 2-byte entries
    if (!(SortF & 0x80)) {  // BIT SortF; BPL LD0CE - Sort by symbol?
      goto LD0CE_label;     // Yes
    }
    A <<= 1;  // Sort by addr - use 4-byte entries (ASL A)

  LD0CE_label:
    // CLC is implicit
    A += (EndSymT & 0xFF);
    UnsortedP = A;
    SortedP   = A;
    A         = (EndSymT >> 8);
    A += 0;  // ADC #0
    UnsortedP = (UnsortedP & 0xFF) | (A << 8);
    SortedP   = (SortedP & 0xFF) | (A << 8);

    A      = 0;
    RecCnt = 0;

    // This loop builds up the aux array
  LD0E3_label:
    PollKbd();  // Check for abort
    // BCC LD0EB
    // JMP AbortAsm

    Y             = 0;
    SymP_ptr      = SimPtrToMemPtr(SymP);
    UnsortedP_ptr = SimPtrToMemPtr(UnsortedP);

  LD0ED_label:
    A = SymP_ptr[Y];    // Looking for eo symbolicname
    if (!(A & 0x80)) {  // BPL LD0F5 - Got it
      goto LD0F5_label;
    }
    Y++;
    goto LD0ED_label;

  LD0F5_label:
    if (SortF & 0x80) {  // BIT SortF; BMI LD108 - Sort by Address?
      goto LD108_label;  // Yes, skip code below
    }

    // Sort by symbol - always done since (SortF) was set to $00 initially
    A |= 0x80;  // set msb on for last char of symbolicname
    SymP_ptr[Y] = A;
    Y++;
    A = SymP_ptr[Y];    // Get symbol's flag byte
    if (!(A & 0x80)) {  // BPL LD108 - Symbol is defined (msb off)
      goto LD108_label;
    }
    A |= 0x7E;  // Retain original $80 and $01 bits
    A ^= 0x80;  // Clear msb to mark symbol is undefined =$7E/$7F
    SymP_ptr[Y] = A;

    // msb of ALL chars of symbolicname are on
    // msb of flagbyte is off ($7x)

  LD108_label:
    Y++;
    A       = SymP_ptr[Y];  // Get associated addr
    SymAddr = A;            // Save here just in case we need it later
    Y++;
    A       = SymP_ptr[Y];
    SymAddr = (SymAddr & 0xFF) | (A << 8);
    SymIdx  = Y;  // Save index for LD198 call

    A                = (SymP & 0xFF);
    Y                = 0;
    UnsortedP_ptr[Y] = A;  // Ptr to symbolic name
    A                = (SymP >> 8);
    Y++;
    UnsortedP_ptr[Y] = A;

    if (!(SortF & 0x80)) {  // BIT SortF; BPL LD12D - Sort by symbol?
      goto LD12D_label;     // Yes
    }

    Y++;
    A                = (SymAddr & 0xFF);
    UnsortedP_ptr[Y] = A;  // Store associated addr
    Y++;
    A                = (SymAddr >> 8);
    UnsortedP_ptr[Y] = A;

  LD12D_label:
    if (!LD198()) {      // Setup envron for next entry
      goto LD0E3_label;  // BCC LD0E3 - Continue building up work array
    }

    // Building of the aux work array of 2/4-byte records is complete
    A = (UnsortedP & 0xFF);
    if (A != 0) {
      goto LD138_label;
    }
    UnsortedP -= 0x100;  // DEC UnsortedP+1

  LD138_label:
    UnsortedP--;  // DEC UnsortedP

    A       = (UnsortedP & 0xFF);
    AuxAryE = A;
    A       = (UnsortedP >> 8);
    AuxAryE = (AuxAryE & 0xFF) | (A << 8);  // Points @ EO array to be sorted

    // Set NumRecs = RecCnt (they are aliases in the original code)
    NumRecs = RecCnt;

    if (!(LstASym & 0x80)) {  // BIT LstASym; BPL LD14C - Alphabetic Symbol listing?
      goto LD14C_label;       // no
    }

    DoSort();
    PrSymTbl();  // Print it

  LD14C_label:
    SortF--;  // DEC SortF
    A = SortF;
    if (A != 0xFF) {     // CMP #$FF; BNE LD16E - Do we need to sort by address?
      goto LD16E_label;  // No -> done
    }

    if (!(SubTtlF & 0x40)) {  // BIT SubTtlF; BVC LD163 -  SBTL directive?
      goto LD163_label;       // No
    }

    // Overwrite 'SYMBOL'00 with 'ADDRESS'00
    X = 7;
    do {
      A                = AddrTxt[X];
      SubTitle[10 + X] = A;
      X--;
    } while (X != 0xFF);  // BPL LD15A

  LD163_label:
    if (!(LstVSym & 0x80)) {  // Value ordered listing?
      goto LD16E_label;       // no
    }

    A       = 0x80;
    LstASym = A;       // Force an Alphabetic sort
    goto LD0BF_label;  // Build aux array again w/(SortF)=$FF

  LD16E_label:
    A       = 0x00;
    SubTtlF = A;  // Reset SBTL flag
  doRtn_label:
    return;
  }

  //=================================================
  // Setup enviornment to process next entry of 2/4-byte array
  // Output
  //   C=1 out of mem or EO symbol table
  //=================================================
  static bool LD198() {
    std::uint8_t A;

    // CLC
    A    = SymIdx;                                // Get index into symbol's record
    A    = static_cast<std::uint8_t>(A + 1);      // Skip over hi-byte of assoc addr
    SymP = static_cast<std::uint16_t>(SymP + A);  // Points @ next record

    // Check out of memory
    if (UnsortedP >= MemTop) {
      return true;  // Probably out of mem
    }

    // Advance UnsortedP by entry size
    A = 2;
    if (SortF & 0x80) {  // Sort by Symbol?
      A <<= 1;           // =4
    }
    UnsortedP = static_cast<std::uint16_t>(UnsortedP + A);

    RecCnt++;

    // Check if EO symbol table?
    if (SymP >= EndSymT) {
      return true;  // C=1 => yes
    }

    return false;
  }

  //=================================================
  // Sorting Algorithm
  // See Disassembled ProDOS Linker for more details
  // Before calling, the work array of 2-byte/4-byte
  // entries must be setup
  //=================================================
  static void DoSort() {
    std::uint8_t A, X, Y;

    A    = static_cast<std::uint8_t>(NumRecs & 0xFF);  // Size of unsorted array
    Jump = NumRecs;

    // WHILE Jump <> 0
  WhileLoop:
    Jump >>= 1;  // Jump := Jump DIV 2
    if (Jump == 0) {
      return;
    }

    EndIdx = static_cast<std::uint16_t>(NumRecs - Jump);  // NumRecs-Jump

    X       = 0;
    StrtIdx = 1;  // =1

    // FOR JJJ := 1 to NumRecs-Jump
  ForLoop:
    JJJ = StrtIdx;

    // REPEAT
  RptLoop:
    III = static_cast<std::uint16_t>(JJJ + Jump);  // III := JJJ + Jump

    // Compute ptrs to the i-th & j-th elements
    J_TH = JJJ;
    I_TH = III;

    // Compute offsets based on entry size (2 or 4 bytes)
    std::uint16_t offsetJ = J_TH;
    std::uint16_t offsetI = I_TH;
    if (SortF & 0x80) {  // Sort by address
      offsetJ <<= 2;
      offsetI <<= 2;
    } else {
      offsetJ <<= 1;
      offsetI <<= 1;
    }

    J_TH = static_cast<std::uint16_t>(EndSymT + offsetJ);
    I_TH = static_cast<std::uint16_t>(EndSymT + offsetI);

    if (!(SortF & 0x80)) {  // Sort by Symbol
      goto LD268_label;
    }

    // Sort addresses in ascending order
    SymPJ = static_cast<std::uint16_t>(J_TH + 2);  // Point @ 16-bit address
    SymPI = static_cast<std::uint16_t>(I_TH + 2);

    {
      std::uint8_t* SymPJ_ptr = SimPtrToMemPtr(SymPJ);
      std::uint8_t* SymPI_ptr = SimPtrToMemPtr(SymPI);
      std::uint8_t  hiJ       = SymPJ_ptr[1];
      std::uint8_t  hiI       = SymPI_ptr[1];

      if (hiJ == hiI) {
        std::uint8_t loI = SymPI_ptr[0];
        std::uint8_t loJ = SymPJ_ptr[0];
        if (loI >= loJ) {
          goto LD25A_label;  // less than or equal
        }
      } else if (hiJ < hiI) {
        goto LD25A_label;  // less than
      }
    }

    Y = 3;  // Greater than => swap entries
    goto LD298_label;

  LD25A_label:
    goto NextJ_label;

    // Sort by symbol
  LD268_label: {
    std::uint8_t* J_TH_ptr = SimPtrToMemPtr(J_TH);
    std::uint8_t* I_TH_ptr = SimPtrToMemPtr(I_TH);

    SymPJ = static_cast<std::uint16_t>(J_TH_ptr[0] | (J_TH_ptr[1] << 8));
    SymPI = static_cast<std::uint16_t>(I_TH_ptr[0] | (I_TH_ptr[1] << 8));

    std::uint8_t* SymPJ_ptr = SimPtrToMemPtr(SymPJ);
    std::uint8_t* SymPI_ptr = SimPtrToMemPtr(SymPI);

    Y = 0;
  LD27E_label:
    A = SymPJ_ptr[Y];
    if (A != SymPI_ptr[Y]) {
      if (A > SymPI_ptr[Y]) {
        Y = 1;  // swap two-byte entry
        goto LD298_label;
      }
      goto NextJ_label;
    }
    Y++;
    if (Y >= 14) {
      goto NextJ_label;
    }
    if ((SymPJ_ptr[Y] & SymPI_ptr[Y]) & 0x80) {
      goto LD27E_label;  // All chars match (msb set)
    }
    if (static_cast<std::int8_t>(SymPJ_ptr[Y]) >= 0) {
      goto NextJ_label;
    }
    Y = 1;  // Flag we have to swap ptrs
  }

    // Swap contents of 2/4-byte table
  LD298_label: {
    std::uint8_t* J_TH_ptr = SimPtrToMemPtr(J_TH);
    std::uint8_t* I_TH_ptr = SimPtrToMemPtr(I_TH);
    do {
      std::uint8_t tmp = J_TH_ptr[Y];
      J_TH_ptr[Y]      = I_TH_ptr[Y];
      I_TH_ptr[Y]      = tmp;
    } while (Y-- != 0);
  }

    JJJ = static_cast<std::uint16_t>(JJJ - Jump);  // JJJ := JJJ - Jump
    if (JJJ > Jump) {                              // Is J > Jump?
      goto RptLoop;
    }

    // UNTIL JJJ =< Jump
  NextJ_label:
    StrtIdx++;
    if (StrtIdx <= EndIdx) {  // Start Index =< End Index
      goto ForLoop;
    }
    goto WhileLoop;
  }

  //=================================================
  // Print the sorted symbols and addresses
  // (SortedP) should be pointing @ BO aux work array
  //=================================================
  static void PrSymTbl() {
    std::uint8_t A, Y;

    PrtFF();

  LD2D8_label:
    ColCnt = 0;  // # of cols printed
    PollKbd();   // Abort assembling
    // BCC LD2E4
    // JMP AbortAsm

  LD2E4_label: {
    std::uint8_t* SortedP_ptr = SimPtrToMemPtr(SortedP);
    Y                         = 0;
    SymP                      = static_cast<std::uint16_t>(SortedP_ptr[Y]);
    Y++;
    SymP |= static_cast<std::uint16_t>(SortedP_ptr[Y] << 8);
  }

    Y = 0;
  LD2F0_label: {
    std::uint8_t* SymP_ptr = SimPtrToMemPtr(SymP);
    A                      = SymP_ptr[Y];  // Is it the flag byte?
    if (!(A & 0x80)) {                     // BPL LD2F8
      goto LD2F8_label;
    }
    Y++;
    goto LD2F0_label;
  }

  LD2F8_label:
    SymRefCh = ' ';  // Init with a blank
    IsFwdRef = 0;
    {
      std::uint8_t* SymP_ptr = SimPtrToMemPtr(SymP);
      A                      = SymP_ptr[Y];  // Get flag byte
      if (A & 0x01) {                        // forward referenced bit
        IsFwdRef = 0xFF;
      }
      if (A >= 0x7E) {
        SymRefCh = '*';  // Symbol referenced but not defined
      } else if (A & Bit40) {
        SymRefCh = '?';  // Defined but never referenced
      } else if (A & Bit10) {
        SymRefCh = 'X';  // EXTERN
      } else if (A & Bit08) {
        SymRefCh = 'N';  // ENTRY
      }
    }

    PutC(SymRefCh);

    {
      std::uint8_t* SymP_ptr = SimPtrToMemPtr(SymP);
      Y++;
      A                   = SymP_ptr[Y];  // Get symbol's addr low byte
      std::uint8_t addrLo = A;
      Y++;
      A = SymP_ptr[Y];  // Get its hi-byte
      if (A == 0 && IsFwdRef == 0) {
        PutC(' ');
        PutC(' ');
      } else {
        PrByte(A);
      }
      PrByte(addrLo);
      PutC(' ');
    }

    // Print up to 14 chars of the symbolicname
    Y = 0;
  LD356_label: {
    std::uint8_t* SymP_ptr = SimPtrToMemPtr(SymP);
    A                      = SymP_ptr[Y];  // Get char fr symbolicname
    if (!(A & 0x80)) {                     // It is the flagbyte
      goto LD362_label;
    }
    PutC(A);
    Y++;
    if (Y < 14) {
      goto LD356_label;
    }
  }

  LD362_label:
    Y--;
  LD363_label:
    Y++;
    if (Y >= 14) {
      goto LD370_label;
    }
    PutC(' ');
    goto LD363_label;  // Loop back to print more spaces

  LD370_label:
    if (AdvRecP()) {  // Adv to next entry of aux array
      return;         // We're done with table
    }
    ColCnt++;
    if (ColCnt >= NumCols) {
      PutCR();  // Further printing will start on next row
      goto LD2D8_label;
    }
    goto LD2E4_label;  // Further printing is on same row
  }

  //=================================================
  // Adv ptr to next entry of work array
  // On return
  //   C=1 - End of work array
  //=================================================
  static bool AdvRecP() {
    std::uint8_t A;

    A = 2;
    if (SortF & 0x80) {  // Sort by addr?
      A <<= 1;           // =4
    }
    SortedP = static_cast<std::uint16_t>(SortedP + A);
    if (SortedP >= AuxAryE) {
      return true;
    }
    return false;
  }

  //=================================================
  // Public entry point - called from asm.cpp
  //=================================================
  void DoPass3() {
    DoPass3_impl();
  }

}  // namespace AsmInternal
