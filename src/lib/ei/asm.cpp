//=================================================
// ASM/ASM1.S, ASM2.S, ASM3.S - Assembler Module
// Translated from 6502 assembly to C++
//
// This is the EDASM.ASM assembler module that handles:
// - Symbol table management and printing (Pass 3)
// - Expression evaluation and code generation
// - Mnemonic processing and addressing modes
// - File I/O and listing generation
//
// MODULE LOADING:
// Original: EDASM.ASM loads at $6800-$9EFF (length $4000 = 16K)
// Memory layout:
//   $6800-$77FF: Relocated to $D000 (Language Card Bank 2)
//   $7800-$9EFF: Remains resident in main memory
//
// ASSEMBLY PROCESS OVERVIEW:
// EDASM uses a multi-pass approach:
//   Pass 1: Build symbol table, determine addresses
//   Pass 2: Generate object code, resolve references
//   Pass 3: Print symbol table (optional, if LST requested)
//
// SYMBOL TABLE:
// - Approximately 27K capacity
// - Hash table with linked list chains for collision resolution
// - Flags track symbol properties (undefined, relative, external, etc.)
//=================================================

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

//=================================================
// ASM/EQUATES.S - Zero Page and Symbol Definitions
//=================================================

namespace {
  constexpr std::uint8_t DCI_end(char c) {
    return static_cast<std::uint8_t>(static_cast<std::uint8_t>(c) | 0x80);
  }

  //=================================================
  // 6502 CPU Register Emulation (Global)
  // These simulate the 6502 processor registers and flags
  // used throughout the translated assembly code
  //=================================================
  std::uint8_t A = 0;      // Accumulator
  std::uint8_t X = 0;      // X index register
  std::uint8_t Y = 0;      // Y index register
  bool         C = false;  // Carry flag
  bool         Z = false;  // Zero flag
  bool         N = false;  // Negative flag
  bool         V = false;  // Overflow flag

  //=================================================
  // Forward Declarations (for functions used before defined)
  //=================================================
  void ChrGot();
  void ChrGot2();
  void HashFn();
  void GAdrMod();
  void IsAXY();
  void HndlMnem();
  void WhiteSpc();

  // Extern array declarations (defined later in file)
  extern const std::uint8_t  CharMap1[];
  extern const std::uint8_t  AModTbl[];
  extern const std::uint8_t  LtrA[];
  extern const std::uint8_t  LtrB[];
  extern const std::uint8_t  LtrC[];
  extern const std::uint8_t  LtrD[];
  extern const std::uint8_t  LtrE[];
  extern const std::uint8_t  LtrF[];
  extern const std::uint8_t  DotDrtv[];
  extern const std::uint8_t* Tbl1stLet[];
  extern const std::uint8_t  AModTkns[];
  extern const std::uint8_t  AModCmds[];

  //=================================================
  // File Control Table (FCT) Indices
  // These indices are used to access different file handles in
  // the file control tables. Each file type (OBJ, source, include,
  // macro, listing) has a dedicated slot.
  //=================================================
  constexpr int ObjFile  = 0;  // Object file (BIN/REL/SYS output) FCT index
  constexpr int ChnFile  = 2;  // Chain file (main source file) FCT index
  constexpr int InclFile = 4;  // Include file (INCLUDE directive) FCT index
  constexpr int MacFile  = 6;  // Macro library file FCT index
  constexpr int LstFile  = 8;  // Listing file (LST output) FCT index

  //=================================================
  // Symbol Table Flag Bits (referenced on page 231 of documentation)
  // Each symbol in the symbol table has a flag byte describing its
  // characteristics. Multiple flags can be combined using OR.
  //=================================================
  constexpr std::uint8_t undefined   = 0x80;  // Symbol referenced but not yet defined (forward ref)
  constexpr std::uint8_t unrefd      = 0x40;  // Symbol defined but never referenced (unused label)
  constexpr std::uint8_t relative    = 0x20;  // Symbol has relocatable address (not absolute)
  constexpr std::uint8_t external    = 0x10;  // Symbol defined in another module (EXTRN directive)
  constexpr std::uint8_t entry       = 0x08;  // Symbol exported to other modules (ENTRY directive)
  constexpr std::uint8_t macro       = 0x04;  // Symbol is a macro name
  constexpr std::uint8_t nosuchlabel = 0x02;  // Symbol lookup failed (error condition)
  constexpr std::uint8_t fwdrefd     = 0x01;  // Symbol forward referenced (used before defined)
  constexpr std::uint8_t Bit08       = 0x08;  // Bit mask $08
  constexpr std::uint8_t Bit10       = 0x10;  // Bit mask $10
  constexpr std::uint8_t Bit40       = 0x40;  // Bit mask $40

  //=================================================
  // ASCII Character Constants
  //=================================================
  constexpr std::uint8_t BEL   = 0x07;  // Bell/beep character
  constexpr std::uint8_t CR    = 0x0D;  // Carriage return (Enter)
  constexpr std::uint8_t SPACE = 0x20;  // Space character

  //=================================================
  // ZERO PAGE MEMORY LOCATIONS ($60-$F1)
  // NB: The contents of $60-$F1 are saved by the Assembler on entry
  // and restored on exit, so these locations can be freely used as
  // workspace without affecting other modules.
  //=================================================

  // $60-$6F: General Assembly Control Variables
  std::uint8_t Z60;            // Generic zero page location (multipurpose)
  std::uint8_t BCDNbr[3];      // Source file line numbers in BCD format ($60-$62, 3 bytes)
#define BCDNbr_hi (BCDNbr[1])  // High byte access
  std::uint16_t StrtSymT;      // Start address of symbol table (2 bytes: $63-$64)
  std::uint16_t EndSymT;       // Current end address of symbol table (2 bytes: $65-$66)
  std::uint8_t  PassNbr;       // Current assembly pass: 0=Pass1, 1=Pass2, 2=Pass3
  std::uint8_t  ListingF;      // Listing flag: $80=LST ON, $00=LST OFF
  std::uint8_t  SubTtlF;       // Subtitle flag: $00=none, $40=SBTL cmd, $FF=subtitle string
  std::uint8_t  LineCnt;       // Number of lines printed on current page
  std::uint16_t PageNbr;       // Current page number (2 bytes: $6B-$6C)
  std::uint8_t  FileNbr;       // Current file number in assembly
  std::uint8_t  LogPL;         // Logical page length (lines per page)
  std::uint8_t  PhyPL;         // Physical page length (actual printer lines)

  // $70-$7F: Instruction Processing and Pointers
  std::uint8_t  SavIndX;    // Temporary storage for X register
  std::uint8_t  ByteCnt;    // Number of bytes generated so far (aliases SavIndX)
  std::uint8_t  PrtCol;     // Current printing column position
  std::uint8_t  EIStack;    // EdAsm Interpreter's saved stack pointer
  std::uint8_t  CancelF;    // Cancel flag (user abort via Ctrl-X or other)
  std::uint16_t NbrErrs;    // Number of errors encountered (2 bytes: $74-$75)
  std::uint8_t  PrSlot;     // Printer slot number (0=none, 1-7=slot)
  std::uint8_t  AbortF;     // Abort flag (fatal error encountered)
  std::uint8_t  SavIndY;    // Temporary storage for Y register
  std::uint16_t SrcP;       // Source pointer - points within current source line
  std::uint16_t UnsortedP;  // Pointer to unsorted auxiliary work array (aliases SrcP)
  std::uint16_t Src2P;      // Copy of source pointer used during code listing
  std::uint16_t PC;         // Program Counter / position counter (current assembly address)
  std::uint16_t SortedP;    // Pointer to sorted auxiliary array (aliases PC)
  std::uint8_t  ObjPC;      // Object code Program Counter (where to store in memory)
  std::uint8_t  SymFBP;     // Pointer to symbol's flag byte field (during Pass 1)
  std::uint8_t  CodeLen;    // Current length of code image for REL files (stored at BOF)
  std::uint8_t  AuxAryE;    // Pointer to last end of sorted array

#define SrcP_hi (reinterpret_cast<std::uint8_t*>(&SrcP)[1])  // High byte access

  // Global test buffer for unit testing (when non-null, overrides SrcP for array access)
  std::uint8_t* g_test_src_buffer = nullptr;

  // Helper to access source line as array (simulates 6502 indirect indexed mode)
  // When SrcP contains an address, SrcP_byte(index) accesses memory at that address
  // For testing, if g_test_src_buffer is set, it uses that instead
  inline std::uint8_t SrcP_byte(std::uint8_t index) {
    if (g_test_src_buffer != nullptr) {
      return g_test_src_buffer[index];
    }
    std::uint8_t* ptr = reinterpret_cast<std::uint8_t*>(static_cast<uintptr_t>(SrcP));
    return ptr[index];
  }

  // Macro to simplify array-style access (for code that looks like SrcP_at(Y))
  // Usage: SrcP_at(Y) instead of SrcP_at(Y)
#define SrcP_at(idx) SrcP_byte(idx)

  // $80-$8F: File and Symbol Table Management
  std::uint16_t FileLen;   // Current length of BIN/REL file (2 bytes: $81-$82)
  std::uint16_t CurrORG;   // Current origin address from ORG directive
  std::uint16_t SymP;      // Pointer to symbol name (2 bytes: $85-$86)
  std::uint16_t MnemP;     // Pointer to mnemonic table entry (aliases SymP)
  std::uint8_t  Delimitr;  // Delimiter character (aliases SymP)
  std::uint8_t  DTEndCol;  // End column index of DateTime string
  std::uint8_t  StrType;   // String type: 0=DCI (inverted last char), -1=ASC
  std::uint8_t  DTCurIdx;  // Current index into DateTime string (aliases StrType)
  std::uint16_t MemTop;    // Top of available memory (2 bytes: $87-$88)
  std::uint32_t TotLines;  // Total line count (3 bytes: $89-$8B for large counts)
  std::uint8_t  VidSlot;   // Video card slot number
  std::uint8_t  SaveA;     // Saved Accumulator value
  std::uint8_t  SaveY;     // Saved Y register value
  std::uint8_t  SaveX;     // Saved X register value

  // $90-$9F: Code Generation and Expression Evaluation
  std::uint8_t  DskListF;      // Disk listing flag: $00=off, $40=partial, $80=lst to file
  std::uint8_t  LstDBIdx;      // LST data buffer index / number of chars to write
  std::uint8_t  WinLeft;       // Left edge of 40-column window (for 80-col cards)
  std::uint8_t  WinRight;      // Right edge of 40-column window
  std::uint8_t  X6502F;        // 65C02 processor flag (vs 6502)
  std::uint16_t HighMem;       // High memory address of generated object code
  std::uint8_t  ExprAccF;      // Expression's accumulated flag bits
  std::uint8_t  ColCnt;        // Current print column count (aliases ExprAccF)
  std::uint8_t  SortF;         // Sort flag for symbol table
  std::uint8_t  NxtToken;      // Next token type flag (indicates nature of next char)
  std::uint8_t  LstCodeF;      // Listing control bits for machine code output
  std::uint8_t  SymRefCh;      // Character printed before symbol's address
  std::uint8_t  GMC[4];        // Generated Machine Code buffer ($9A-$9D, 4 bytes)
  std::uint8_t  IsFwdRef;      // Forward reference flag (bit set if forward ref)
  std::uint8_t  NumCols;       // Number of print columns: 2, 4, or 6
  std::uint8_t  SymIdx;        // Index into symbol record
  std::uint8_t  ERfield;       // Expression Result field
  std::uint16_t SymAddr;       // Address associated with symbolic name (2 bytes)
  std::uint16_t ValExpr_word;  // Value of expression as 16-bit word (2 bytes: $9F-$A0)
#define ValExpr    (reinterpret_cast<std::uint8_t*>(&ValExpr_word)[0])  // Low byte access
#define ValExpr_hi (reinterpret_cast<std::uint8_t*>(&ValExpr_word)[1])  // High byte access
  std::uint8_t  ValExpr_2;  // Extended byte 2 for mul/div operations ($A1)
  std::uint8_t  ValExpr_3;  // Extended byte 3 for mul/div operations ($A2)
  std::uint16_t RLDEntP;    // Pointer to RLD (Relocation Dictionary) entry
  std::uint16_t WrkP;       // Work pointer to symbol table entry
  std::uint16_t JJJ;        // Loop variable J (used during sorting algorithms)
  std::uint16_t III;        // Loop variable I (used during sorting algorithms)

  // $A0-$AF: Instruction Encoding and Loop Control
  std::uint8_t  Length;   // Instruction length: 1=1 byte, 2=2 bytes, 3=3 bytes
  std::uint16_t ModWrd;   // Permitted addressing modes for mnemonic (2 bytes)
  std::uint8_t  ModWrdL;  // Low byte of permitted addressing modes
  std::uint8_t  ModWrdH;  // High byte of permitted addressing modes
  std::uint16_t StrtIdx;  // Starting index for FOR loop (aliases Length)
  std::uint16_t EndIdx;   // Ending index for FOR loop
  std::uint8_t  LenTIdx;  // Index into instruction length table
  std::uint8_t  Filler;   // Filler byte for reserved storage (DS directive)
  std::uint8_t  SavLstF;  // Saved listing flags (temporary storage)
  std::uint8_t  GMCIdx;   // Index into GMC buffer during code generation
  std::uint8_t  RadixCh;  // Radix check character during string-to-binary conversion
  std::uint16_t Jump;     // Gap between two elements (shell sort algorithm)
  std::uint8_t  SavFByt;  // Saved symbol's flag byte (temporary)
  std::uint8_t  BitsDig;  // Bits per digit for bin/octal/hex conversion (1/3/4)
  std::uint8_t  LabelF;   // Label field flag: instruction has a label
  std::uint16_t RecCnt;   // Record count for auxiliary array
  std::uint16_t NumRecs;  // Number of records (alias for RecCnt)
  std::uint8_t  SubTIdx;  // Offset into opcode sub-table
  std::uint8_t  ZAB;      // Generic zero page location
  std::uint8_t  ErrorF;   // Error flag: current line is flagged as incorrect
  std::uint8_t  ErrTIdx;  // Error info table index (aliases ErrorF)
  std::uint8_t  msbF;     // MSB flag (most significant byte)
  std::uint16_t J_TH;     // Offset/pointer to j-th element of aux array
  std::uint16_t I_TH;     // Offset/pointer to i-th element of aux array
  std::uint8_t  EndianF;  // Endianness flag: little-endian vs big-endian
  std::uint16_t Accum;    // Main accumulator (2 bytes: $AF-$B0)
#define Accum_hi (reinterpret_cast<std::uint8_t*>(&Accum)[1])  // High byte access
  std::uint8_t  Accum_2;  // Extended byte 2 for mul/div operations
  std::uint8_t  Accum_3;  // Extended byte 3 for mul/div operations
  std::uint16_t NewPC;    // New Program Counter (used by DS directive)
  std::uint16_t SymPJ;    // Symbol pointer J

  // $B0-$BF: Symbol Processing and Code Generation
  std::int8_t   Ret816F;     // Return format: -1=16-bit, 0=low 8-bit, 1=high 8-bit
  std::uint16_t SymPI;       // Symbol pointer I
  std::uint8_t  RepChar;     // Repeat character (used by REP directive)
  std::uint8_t  SymNbr;      // Number of symbols declared as EXTRN/ENTRY (DEF/REF)
  std::uint8_t  SymLen;      // Length of symbolic name (aliases SymNbr)
  std::uint16_t SavSTS;      // Saved start of symbol table
  std::uint8_t  GblAbsF;     // Global/Absolute flag: $00=ZDEF/ZREF, $01=DEF/REF
  std::uint8_t  DummyF;      // Dummy section flag (DSECT directive)
  std::uint16_t SavPC;       // Saved Program Counter (2 bytes: $B6-$B7)
  std::uint16_t SavObjPC;    // Saved Object PC (2 bytes: $B8-$B9)
  std::uint16_t CodeImgLen;  // Code image length (aliases SavObjPC)
  std::uint8_t  CondAsmF;    // Conditional Assembly Flag: $00, $40, or $80
  std::uint8_t  TabTIdx;     // Index into Editor's tab table
  std::uint8_t  SymFByte;    // Symbol's current flag byte
  std::uint8_t  RelCodeF;    // Relocatable code flag
  std::int8_t   DskSrcF;     // Disk source flag: -1=disk file, 0=memory buffer
  std::uint8_t  GenF;        // Generation flag: N=1 suppress, V=1 disk, V=0 memory

  // $C0-$CF: File I/O and Macro Processing
  std::uint8_t  ObjDBIdx;  // Object code data buffer index / bytes to write
  std::int8_t   IDskSrcF;  // Include disk source flag: MSB on = from INCLUDE file
  std::uint8_t  MacroF;    // Macro status: $00=not using, $40,$06,$04,$80=file opened
  std::uint8_t  MParmCnt;  // Macro parameter count (0-9)
  std::uint8_t  MacArg;    // Macro argument number (0-9)
  std::uint8_t  ZC5;       // (Not used - reserved)
  std::uint8_t  FCTIndex;  // File Control Table index: 0, 2, 4, 6, or 8
  std::uint16_t PathP;     // Pathname pointer (2 bytes: $C7-$C8)
  std::uint16_t SrcPathP;  // Current source filename pointer (2 bytes: $C9-$CA)
  std::uint8_t  RelExprF;  // Relative expression flag: non-zero = rel addr expr/sub-expr
  std::uint8_t  SavSTE;    // Temporary save for high byte of end-of-symbol-table
  std::uint8_t  SavSEF;    // Previous sub-expression's RelExprF (aliases above)
  std::uint8_t  NewF;      // New file flag: $80 = new file being assembled
  std::uint16_t Msg2P;     // Message pointer (2 bytes: $CE-$CF)
  std::uint8_t  Lower8;    // Low 8 bits of 16-bit value (aliases Msg2P)
  std::uint8_t  ParmBIdx;  // Index into parameter string passed by EI
  std::uint8_t  OnOffSW;   // On/Off switch: $80=ON, $00=OFF
  std::uint16_t SrcP3;     // Pointer to partial source line minus 1
  std::uint8_t  ZCE;       // Generic zero page location
  std::uint8_t  ZCF;       // Generic zero page location
  std::uint16_t SymNodeP;  // Symbol node pointer (used in symbol table printing)
  std::uint8_t  TotCnt;    // Total count of bytes generated

  // $D0-$DF: Relocation and Symbol Table Management
  std::uint16_t RLDEnd;     // Relocation Dictionary end pointer (2 bytes)
  std::uint16_t ZD2;        // (Not used - reserved)
  std::uint8_t  SavGenF;    // Saved GenF when DSECT (dummy section) is declared
  std::uint16_t SBufP;      // Pointer to SBuf/IBuf data buffer (2 bytes)
  std::uint16_t MsgP;       // Message pointer (aliases SBufP)
  std::uint8_t  HashIdx;    // Hash table index (for symbol table lookup)
  std::uint16_t PrvSymP;    // Pointer to previous symbol's node (2 bytes)
  std::uint16_t NxtSymP;    // Pointer to next symbol's node (2 bytes)
  std::uint8_t  NumCycles;  // Instruction's number of CPU cycles
  std::uint16_t NbrWarns;   // Number of warnings (2 bytes: $DE-$DF)

  // $E0-$EF: Listing Control Flags and Miscellaneous
  std::uint8_t  LstFlags[8];  // Base address of listing flags array
  std::uint8_t  LstCyc;       // List CPU cycle times (default: OFF)
  std::uint8_t  LstUnAsm;     // List unassembled source (default: ON)
  std::uint8_t  LstExpMac;    // List macro expansion lines (default: ON)
  std::uint8_t  LstWarns;     // List warning messages (default: ON)
  std::uint8_t  LstGCode;     // Generate object code (default: OFF)
  std::uint8_t  LstASym;      // List symbols alphabetically (default: ON)
  std::uint8_t  LstVSym;      // List symbols by value order (default: OFF)
  std::uint8_t  Lst6Cols;     // Use 6-column symbol dump (default: OFF, uses 4-col)
  std::uint8_t  ZE8;          // (Initialized but not used)
  std::uint8_t  SW16F;        // Sweet16 flag (indicates SW16 virtual machine code)
  std::uint16_t ZPSaveY;      // Zero page save for Y register (2 bytes)
  std::uint8_t  RndF;         // Random data flag: $80=use random, $00=use filler byte
  std::uint8_t  ErrNbr4;      // Error number times 4 (used as table index)
  char          DecimalS[4];  // Decimal string buffer '0000' (4 bytes)
  std::uint16_t ZPRetAdr;     // Zero page return address (2 bytes)
  std::uint8_t  MacPNLen;     // Macro library pathname length byte
  std::uint8_t  SLTBYT;       // Slot ROM presence byte

  //=================================================
  // ASM/EXTERNALS.S - External Memory Buffers
  //=================================================

  // Workspace and Entry Points
  constexpr std::uint16_t X0800 = 0x0800;  // Entry point address / workspace start

  // Macro Processing Buffers
  constexpr std::uint16_t X6E00     = 0x6E00;  // 1024-byte buffer for MACRO definition file
  constexpr std::uint16_t MacExpBuf = 0x7200;  // Macro expansion buffer (128 bytes)
  constexpr std::uint16_t MacStrBuf = 0x7280;  // Macro string parameter buffer (128 bytes)

  // Listing and Data Buffers
  constexpr std::uint16_t OLDataB = 0x7300;  // Online data buffer (256 bytes)
  constexpr std::uint16_t LstDBuf = 0x7300;  // Circular data buffer for LST file
  constexpr std::uint16_t X7400   = 0x7400;  // 1024-byte buffer for LST/MACRO file

  // Inter-Module Communication Buffers
  constexpr std::uint16_t ObjDataB = 0xBD00;  // Object code data buffer (128 bytes)
  constexpr std::uint16_t AsmParmB = 0xBD80;  // Assembler parameter buffer (128 bytes)

  // ProDOS File I/O Buffers (1024 bytes each)
  constexpr std::uint16_t XA100 = 0xA100;  // Object file buffer (1024 bytes)
  constexpr std::uint16_t XA500 = 0xA500;  // Source file buffer (1024 bytes)
  constexpr std::uint16_t XA900 = 0xA900;  // Include file buffer (1024 bytes)

  // Symbol Table Data Structures
  constexpr std::uint16_t HeaderT = 0xBC00;  // Symbol table header array (256 bytes)

  // Memory pointers (these would be actual pointers in C++)
  std::uint8_t* HeaderT_ptr   = nullptr;  // Symbol table hash table pointers
  std::uint8_t* ObjDataB_ptr  = nullptr;  // Object code data buffer
  std::uint8_t* AsmParmB_ptr  = nullptr;  // Assembler parameter buffer
  std::uint8_t* MacExpBuf_ptr = nullptr;  // Macro expansion buffer
  std::uint8_t* MacStrBuf_ptr = nullptr;  // Macro string parameter buffer
  std::uint8_t* OLDataB_ptr   = nullptr;  // Online data buffer
  std::uint8_t* LstDBuf_ptr   = nullptr;  // Listing data buffer
  std::uint8_t* X7400_ptr     = nullptr;  // LST/MACRO file buffer
  std::uint8_t* XA100_ptr     = nullptr;  // Object file I/O buffer
  std::uint8_t* XA500_ptr     = nullptr;  // Source file I/O buffer
  std::uint8_t* XA900_ptr     = nullptr;  // Include file I/O buffer
  std::uint8_t  XA060[10];                // $A060-$A069

  // Error Information Table ($A0B2 in original)
  // Stores first 8 (40-col) or 16 (80-col) errors encountered
  // Each error record is 4 bytes: FileNbr, ErrIndex, LineHi, LineLo
  constexpr int MAX_ERROR_INFO               = 16;  // Maximum 16 errors for 80-col
  std::uint8_t  ErrInfoT[MAX_ERROR_INFO * 4] = {};  // 64 bytes total

  // Text strings for symbol table
  const char SymbolTxt[] = "SYMBOL         TABLE";
  const char SortedTxt[] = "SORTED         BY SYMBOL";
  const char AddrTxt[]   = "ADDRESS";

  // Pathname table
  std::uint16_t PNTable[10];  // Table of pathname pointers

  // Other buffers
  std::uint8_t ChnPNB[256];    // Chain pathname buffer
  std::uint8_t SubTitle[256];  // Subtitle buffer

  //=================================================
  // Forward Declarations
  //=================================================
  void         DoPass3();
  bool         LD198();
  void         DoSort();
  void         PrSymTbl();
  bool         AdvRecP();
  std::uint8_t Chk4ROM(std::uint8_t slot);
  void         LD3B4();
  void         PollKbd();
  void         AbortAsm();
  void         PutC(std::uint8_t ch);
  void         PrByte(std::uint8_t value);
  void         PutCR();
  void         PrtFF();
  void         NextRec();

  //=================================================
  // ASM1.S - Pass 3: Symbol Table Printing
  //=================================================

  //
  // PASS 3: Symbol Table Printing
  // This code section handles printing the symbol table after
  // assembly completes. It can print symbols alphabetically or
  // by value, in 2, 4, or 6 column format.
  //
  void DoPass3() {
    std::uint8_t  A, X, Y;
    std::uint8_t* StrtSymT_ptr  = nullptr;
    std::uint8_t* SymNodeP_ptr  = nullptr;
    std::uint8_t* SymP_ptr      = nullptr;
    std::uint8_t* UnsortedP_ptr = nullptr;

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
    Y            = 0;
    StrtSymT_ptr = reinterpret_cast<std::uint8_t*>(StrtSymT);
    SymNodeP_ptr = reinterpret_cast<std::uint8_t*>(SymNodeP);

  LD078_label:
    A               = StrtSymT_ptr[Y];  // Get char fr symbolicname
    SymNodeP_ptr[Y] = A;                // move it forward in node
    if (!(A & 0x80)) {                  // BPL LD081 - not eo symbolicname yet
      Y++;
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
    A += (SymNodeP & 0xFF);  // Point @ next node
    SymNodeP = (SymNodeP & 0xFF00) | A;
    if (A >= (SymNodeP & 0xFF)) {  // No carry
      goto LD099_label0;
    }
    SymNodeP += 0x100;  // INC SymNodeP+1

  LD099_label0:
    Y++;  // Skip over link field
    Y++;
    A = Y;
  }

  LD099_label:
    // CLC is implicit
    A += (StrtSymT & 0xFF);
    StrtSymT = (StrtSymT & 0xFF00) | A;  // Point @ symbolicname of next node
    if (A >= (StrtSymT & 0xFF)) {        // BCC LD0A2
      goto LD0A2_label;
    }
    StrtSymT += 0x100;  // INC StrtSymT+1

  LD0A2_label:
    // CMP EndSymT - EO symbol table?
    A = (StrtSymT >> 8);
    A -= (EndSymT >> 8);       // SBC EndSymT+1
    if (A < (EndSymT >> 8)) {  // BCC LD076 - No
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
    SymP_ptr      = reinterpret_cast<std::uint8_t*>(SymP);
    UnsortedP_ptr = reinterpret_cast<std::uint8_t*>(UnsortedP);

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
  bool LD198() {
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
  void DoSort() {
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
      std::uint8_t* SymPJ_ptr = reinterpret_cast<std::uint8_t*>(SymPJ);
      std::uint8_t* SymPI_ptr = reinterpret_cast<std::uint8_t*>(SymPI);
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
    std::uint8_t* J_TH_ptr = reinterpret_cast<std::uint8_t*>(J_TH);
    std::uint8_t* I_TH_ptr = reinterpret_cast<std::uint8_t*>(I_TH);

    SymPJ = static_cast<std::uint16_t>(J_TH_ptr[0] | (J_TH_ptr[1] << 8));
    SymPI = static_cast<std::uint16_t>(I_TH_ptr[0] | (I_TH_ptr[1] << 8));

    std::uint8_t* SymPJ_ptr = reinterpret_cast<std::uint8_t*>(SymPJ);
    std::uint8_t* SymPI_ptr = reinterpret_cast<std::uint8_t*>(SymPI);

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
    std::uint8_t* J_TH_ptr = reinterpret_cast<std::uint8_t*>(J_TH);
    std::uint8_t* I_TH_ptr = reinterpret_cast<std::uint8_t*>(I_TH);
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
  void PrSymTbl() {
    std::uint8_t A, Y;

    PrtFF();

  LD2D8_label:
    ColCnt = 0;  // # of cols printed
    PollKbd();   // Abort assembling
    // BCC LD2E4
    // JMP AbortAsm

  LD2E4_label: {
    std::uint8_t* SortedP_ptr = reinterpret_cast<std::uint8_t*>(SortedP);
    Y                         = 0;
    SymP                      = static_cast<std::uint16_t>(SortedP_ptr[Y]);
    Y++;
    SymP |= static_cast<std::uint16_t>(SortedP_ptr[Y] << 8);
  }

    Y = 0;
  LD2F0_label: {
    std::uint8_t* SymP_ptr = reinterpret_cast<std::uint8_t*>(SymP);
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
      std::uint8_t* SymP_ptr = reinterpret_cast<std::uint8_t*>(SymP);
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
      std::uint8_t* SymP_ptr = reinterpret_cast<std::uint8_t*>(SymP);
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
    std::uint8_t* SymP_ptr = reinterpret_cast<std::uint8_t*>(SymP);
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
  bool AdvRecP() {
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
  // Checks if external ROM is present
  // (A)=slot # (1-7)
  // Z=1 yes, Z=0 no
  //=================================================
  std::uint8_t Chk4ROM(std::uint8_t slot) {
    std::uint8_t A = 0x02;
    std::uint8_t X = slot;

    while (X > 1) {
      A <<= 1;
      X--;
    }

    if (SLTBYT & A) {  // Slot ROM present
      return 0;
    }
    return 1;
  }

  //=================================================
  // (Y)=preserved
  // Get ptr to next line record and save it
  //=================================================
  void LD3B4() {
    std::uint16_t savedSrcP = SrcP;
    NextRec();  // Point @ start of next src line

    std::size_t idx = static_cast<std::size_t>(FCTIndex);
    if (idx + 1 < sizeof(XA060)) {
      XA060[idx]     = static_cast<std::uint8_t>(SrcP & 0xFF);
      XA060[idx + 1] = static_cast<std::uint8_t>((SrcP >> 8) & 0xFF);
    }

    SrcP = savedSrcP;
  }

  //=================================================
  // Helper stubs (to be implemented later)
  //=================================================
  void PollKbd() {
    // TODO: Implement keyboard polling for user abort
  }

  void AbortAsm() {
    // TODO: Implement assembly abort
  }

  void PutC(std::uint8_t ch) {
    // TODO: Output a character to the listing output
    (void)ch;
  }

  void PrByte(std::uint8_t value) {
    // TODO: Print a byte in hex
    (void)value;
  }

  void PutCR() {
    // TODO: Output CR
  }

  void PrtFF() {
    // TODO: Output form feed
  }

  void NextRec() {
    // TODO: Advance to next source record
  }

  // ASM2 helper stubs
  void SaveZP() {
    // TODO: Save zero page area
  }

  void SetupVec() {
    // TODO: Setup/reset vectors
  }

  void InitASM() {
    // TODO: Initialize assembler state
  }

  // Forward declaration - full implementation below
  void DoPass1();

  void DoPass2() {
    // TODO: Pass 2
  }

  void ClsFile() {
    // TODO: Close file by FCTIndex
  }

  void ClsFileX() {
    // TODO: Close file by index in X
  }

  void L99DF() {
    // TODO: Flush object code
  }

  void PrSummry() {
    // TODO: Print summary of errors
  }

  void PrtEndAsm() {
    // TODO: Print end of assembly summary
  }

  void CountErr() {
    // TODO: Increment error count
  }

  void IsVideo() {
    // TODO: Check output device
  }

  void VidOut(std::uint8_t ch) {
    // TODO: Output to video
    (void)ch;
  }

  void PrtCR() {
    // TODO: Output CR
  }

  void PrtDecS() {
    // TODO: Print BCD as decimal string
  }

  void L81A3() {
    // TODO: Increment decimal string
  }

  void SaveErrInfo() {
    // TODO: Save error info for current line
  }

  void L986A() {
    // TODO: Output message pointed by X
  }

  void CanclAsm(std::uint8_t keycode) {
    // TODO: Handle cancel/abort
    (void)keycode;
  }

  void GoMon() {
    // TODO: Return to monitor
  }

  //=================================================
  // ASM2.S - Main Entry & Control
  //=================================================

  // Assembly Build Information
  const char DateStr[] = "30-APR-85      22:46";

  // Main Assembler Entry Points
  void Assembler();
  void ColdStrt();
  void ExecAsm();
  void CanclAsm(std::uint8_t keycode);
  void SetupVec();
  void GoMon();

  // ASM2 helper stubs
  void SaveZP();
  void InitASM();
  void DoPass1();
  void DoPass2();
  void ClsFile();
  void ClsFileX();
  void L99DF();
  void PrSummry();
  void PrtEndAsm();
  void CountErr();
  void DoAlert();
  void IsVideo();
  void VidOut(std::uint8_t ch);
  void PrtCR();
  void PrtDecS();
  void L81A3();
  void PrtErrMsg();
  void SaveErrInfo();
  void L986A();

  // Patchable pointer (A,X,Y as label names)
  std::uint16_t IsAXY_addr = 0;

  void Assembler() {
    ColdStrt();
  }

  void ColdStrt() {
    ExecAsm();
  }

  void ExecAsm() {
    SaveZP();
    EIStack = 0;  // TSX / STX EIStack (placeholder)

    SetupVec();
    InitASM();
    DoPass1();
    DoPass2();
    PollKbd();

  CleanUp:
    if ((DskSrcF & 0x80) == 0) {  // Assembling a mem resident src file?
      goto FlushObj;
    }

    if (FCTIndex < InclFile) {
      goto CleanUp2;
    }
    if (FCTIndex == InclFile) {
      goto CleanUp1;
    }
    FCTIndex = MacFile;  // Close Macro file (if any)
    ClsFile();

  CleanUp1:
    FCTIndex = InclFile;  // Close INCLUDE file (if any)
    ClsFile();

  CleanUp2:
    FCTIndex = ChnFile;  // Close CHAIN file (if any)
    ClsFile();

  FlushObj:
    L99DF();  // Flush obj code

  ListSymTbl:
    if ((CancelF & 0x80) == 0) {  // Suppress symbol table listing?
      goto ListErrs;
    }

    SavSTE = static_cast<std::uint8_t>(EndSymT >> 8);
    DoPass3();
    PollKbd();

  AbortAsmLabel:
    EndSymT = static_cast<std::uint16_t>((EndSymT & 0x00FF) | (SavSTE << 8));

  ListErrs:
    if ((NbrErrs | (NbrErrs >> 8)) != 0) {
      PrSummry();
    }

  TellUser:
    PrtEndAsm();

  EndAsm:
    PrtFF();
    if ((DskListF & 0x80) != 0) {  // Listing to file?
      FCTIndex = LstFile;
      ClsFileX();
    }

  ExitASM:
    SaveZP();
    SetupVec();
    return;
  }

  // ($7AA8) DoAlert - Print *****, sound the bell and then print an error message
  // ($7AA8) DoAlert - Print *****, sound the bell and then print an error message
  // (X)=error token
  // (Y) preserved
  // NB:Fall thru to the PrtErrMsg code
  void DoAlert() {
    // TODO: Implement DoAlert - Print alert with asterisks and bell
  }

  // ($7A6B) PrtErrMsg - Print Error or Warnings
  // Entry:
  //  (X)=error token
  //  bits
  //   7
  //  6-1  index into a table of ptrs to Error/Warning Messages
  //   0   on=counted as warning, off=counted as error
  //
  // Range for index $00-$48
  void PrtErrMsg() {
    // TODO: Implement PrtErrMsg
  }

  // SaveErrInfo - Save info for first 8/16 errors encountered
  // (Y) & (X) preserved
  // X=error token
  void SaveErrInfo(std::uint8_t errorToken) {
    // Determine max errors based on video slot
    std::uint8_t maxErrors  = (VidSlot == 0) ? 8 : 16;
    std::uint8_t maxErrNbr4 = maxErrors * 4;

    // Check if error buffer is full
    if (ErrNbr4 >= maxErrNbr4) {
      return;  // Too many errors already stored
    }

    // Store error information in ErrInfoT
    std::uint8_t idx  = ErrNbr4;
    ErrInfoT[idx]     = FileNbr;            // File number
    ErrInfoT[idx + 1] = errorToken & 0x7E;  // Error token index (isolate bits 6-1)
    ErrInfoT[idx + 2] = BCDNbr[1];          // Line number high byte (BCD)
    ErrInfoT[idx + 3] = BCDNbr[0];          // Line number low byte (BCD)

    // Increment error count (4 bytes per error)
    ErrNbr4 += 4;
  }

  // ZeroLnCnt - Init Line Counters & set file cnt to 1
  // X, Y regs not used
  // (A)=0
  void ZeroLnCnt() {
    // TODO: Initialize line counters
  }

  // OpenSrc1 - Open/ReOpen initial SRC file for input
  void OpenSrc1() {
    // TODO: Open source file
  }

  // GetSrcPN - Copy the SRC pathname passed by EdAsm Interpreter
  // using $BD80 buf (only 65 bytes used).
  // Since EdAsm uses only disk source files, DskSrcF
  // once set will always be $80.
  void GetSrcPN() {
    // TODO: Get source pathname
  }

  // ParmErr - Parameter error handler
  void ParmErr() {
    // TODO: Handle parameter error
  }

  // GetObjPN - Get the OBJ pathname (if any)
  // Check for suppression of obj code by looking for
  // '@' in place of object file name pg 76, 96
  void GetObjPN() {
    // TODO: Get object pathname
  }

  // PrtSetup - Setup printing to file/printer
  // The DevCtlS is set using the EdAsm Interpreter
  void PrtSetup() {
    // TODO: Setup printer
  }

  // Dec2Int - Look for a 2-byte dec string and convert into integer
  void Dec2Int() {
    // TODO: Convert decimal string
  }

  // $7E14
  const char OBJ0TXT[] = ".OBJ0";

  // X=# of chars to print
  void L7E19() {
    // TODO: Print characters
  }

  // ToUpper
  void ToUpper() {
    // TODO: Convert to uppercase
  }

  // DoPass1 - Create the symbol table
  // Source code handling, lexical
  // syntactic and semantic analysis
  void DoPass1() {
    // 6502 register emulation (should be declared globally where needed)
    // For now, stub implementation
    // TODO: Implement full Pass 1 logic with proper variable scope

    RelCodeF = 0;
    SymNbr   = 0;  // # of ENTRY/EXTRN
    PassNbr  = 0;
    // A = BINtype / ftypeT[0] = A  - TODO: implement when BINtype/ftypeT are defined
    // OpenSrc1();  // TODO: implement when ready

    // Rest of Pass 1 logic commented out until variables are properly scoped
    /*
  // Assemble each src line
  // This should be our parser
  Pass1Lup:
    GSrcLin();           // Any more?
    if (!C) goto L7E46;  // Yes (BCC)
    return;

  // Init vars before assembling each src line
  L7E46:
    */
    return;  // Stub return for now
  }

  // PHASE1_CODE:   ChkCommLin:
  // PHASE1_CODE:     A = SrcP_at(Y);               // Get 1st char (Y=0)
  // PHASE1_CODE:     if (A == '*') goto L7E74;  // Is it a pure comment line? Yes
  // PHASE1_CODE:     if (A == ';') goto L7E74;  // comment
  // PHASE1_CODE:     goto ChkLabel;             // no
  // PHASE1_CODE:   L7E74:
  // PHASE1_CODE:     goto L7F04;  // ignore
  // PHASE1_CODE:
  // PHASE1_CODE:   ChkLabel:
  // PHASE1_CODE:     A ^= SPACE;
  // PHASE1_CODE:     LabelF = A;  // 0 => no label
  // PHASE1_CODE:     if (A == 0) goto L7EDD;
  // PHASE1_CODE:
  // PHASE1_CODE:     // This part of code checks for an idfer in the LABEL field
  // PHASE1_CODE:     // Should be part of Lexer (Static semantic analysis)
  // PHASE1_CODE:     RsvdId();  // Chk for A,X,Y as 1st char of label
  // PHASE1_CODE:     FindSym();
  // PHASE1_CODE:     if (C) goto NewLabel;           // No such label (BCS)
  // PHASE1_CODE:     X = A;                          // Sym found but has it been defined?
  // PHASE1_CODE:     if ((int8_t)X < 0) goto L7E94;  // No (BMI)
  // PHASE1_CODE:
  // PHASE1_CODE:     X = 0x02;  // Duplicate idfer
  // PHASE1_CODE:     RegAsmEW();
  // PHASE1_CODE:     A      = 0x00;
  // PHASE1_CODE:     LabelF = A;  // Flag no label field
  // PHASE1_CODE:     goto L7EC8;  // (Y)-indexing lobyte?
  // PHASE1_CODE:
  // PHASE1_CODE:   L7E94:
  // PHASE1_CODE:     SymFByte = A;            // Symbol's curr flag byte
  // PHASE1_CODE:     Y--;                     // Index flag byte again
  // PHASE1_CODE:     A &= (entry | fwdrefd);  // 0000 1001
  // PHASE1_CODE:     // BIT DummyF - Are we in a DSECT?
  // PHASE1_CODE:     if ((int8_t)DummyF < 0) goto L7E9F;  // Yes (BMI)
  // PHASE1_CODE:     A |= relative;
  // PHASE1_CODE:   L7E9F:
  // PHASE1_CODE:     SymP[Y] = A;  // save modified status of flag byte
  // PHASE1_CODE:     Y++;
  // PHASE1_CODE:     A       = PC;  // Value associated w/symbol
  // PHASE1_CODE:     SymP[Y] = A;
  // PHASE1_CODE:     Y++;
  // PHASE1_CODE:     A       = PC_hi;
  // PHASE1_CODE:     SymP[Y] = A;
  // PHASE1_CODE:     goto L7EC8;
  // PHASE1_CODE:
  // PHASE1_CODE:   // New label
  // PHASE1_CODE:   NewLabel:
  // PHASE1_CODE:     A        = 0x00;
  // PHASE1_CODE:     SymFByte = A;
  // PHASE1_CODE:     X        = relative;
  // PHASE1_CODE:     // BIT DummyF - Are we in a DUMMY section?
  // PHASE1_CODE:     if ((int8_t)DummyF >= 0) goto L7EBA;  // No (BPL)
  // PHASE1_CODE:
  // PHASE1_CODE:     X = 0x00;  // abs
  // PHASE1_CODE:   L7EBA:
  // PHASE1_CODE:     RelExprF = X;
  // PHASE1_CODE:     A        = 0x00;     // Initial flag byte to be
  // PHASE1_CODE:     AddNode();           // stored into Node
  // PHASE1_CODE:     if (!C) goto L7EC8;  // (Y)-indexing hibyte (BCC)
  // PHASE1_CODE:     X = 0x0E;            // Invalid identifier
  // PHASE1_CODE:     RegAsmEW();
  // PHASE1_CODE:
  // PHASE1_CODE:   L7EC8:
  // PHASE1_CODE:     Y--;
  // PHASE1_CODE:     Y--;  // Indexing flag byte of symtbl entry
  // PHASE1_CODE:     A = Y;
  // PHASE1_CODE:     // CLC
  // PHASE1_CODE:     A += SymP;
  // PHASE1_CODE:     SymFBP = A;  // Point @ the symbol's flag byte
  // PHASE1_CODE:     A      = 0;
  // PHASE1_CODE:     A += SymP_hi;  // with carry
  // PHASE1_CODE:     SymFBP_hi = A;
  // PHASE1_CODE:
  // PHASE1_CODE:     // This part handles the mnemonic/psuedo opcode field
  // PHASE1_CODE:     Y = 0;              // Start w/1st char
  // PHASE1_CODE:     L81F0();            // Skip over non-blanks
  // PHASE1_CODE:     if (Z) goto L7EDD;  // Got a CR (BNE -> BEQ inverted)
  // PHASE1_CODE:
  // PHASE1_CODE:   L7EDD:
  // PHASE1_CODE:     NxtField();  // We got at least 1 space so skip over them
  // PHASE1_CODE:     HndlMnem();
  // PHASE1_CODE:     if (!C) goto L7EF0;  // No errs (BCC)
  // PHASE1_CODE:
  // PHASE1_CODE:   L7EE5:
  // PHASE1_CODE:     X = 0x04;  // undefined opcode
  // PHASE1_CODE:     RegAsmEW();
  // PHASE1_CODE:     A      = 3;
  // PHASE1_CODE:     Length = A;
  // PHASE1_CODE:     if (A != 0) goto L7F01;  // always (BNE)
  // PHASE1_CODE:
  // PHASE1_CODE:   L7EF0:
  // PHASE1_CODE:     if (A == 0xFF) goto L7F04;  // 1st flag byte - Proceed to next line rec
  // PHASE1_CODE:     // BIT ZAB - SW16 opcodes?
  // PHASE1_CODE:     if ((ZAB & 0x40) == 0) goto L7EFE;  // No (BVC)
  // PHASE1_CODE:     // BIT SW16F - Are SW16 ops valid?
  // PHASE1_CODE:     if ((int8_t)SW16F >= 0) goto L7EFE;  // BPL
  // PHASE1_CODE:     SW16F--;                             // When (SW16F)=0, code gen problems may
  // arise PHASE1_CODE: PHASE1_CODE:   L7EFE: PHASE1_CODE:     HndlOpnd(); PHASE1_CODE: PHASE1_CODE:
  // L7F01: PHASE1_CODE:     L7F04();  // increment line counter and fetch next src line
  // PHASE1_CODE:     goto Pass1Lup;
  // PHASE1_CODE:
  // PHASE1_CODE:   L7F04:
  // PHASE1_CODE:     // TODO: increment line counter and fetch next src line
  // PHASE1_CODE:     goto Pass1Lup;
  // PHASE1_CODE:   }
  // PHASE1_CODE:
  //=================================================
  // BCD Arithmetic Helper (for error/warning counters)
  //=================================================
  void IncrementBCD16(std::uint16_t& counter) {
    std::uint8_t lo = counter & 0xFF;
    std::uint8_t hi = (counter >> 8) & 0xFF;

    // BCD increment low byte
    std::uint8_t new_lo = (lo & 0x0F) + 1;
    if (new_lo > 9) new_lo += 6;  // BCD adjust

    std::uint8_t carry = (new_lo & 0xF0) >> 4;
    new_lo &= 0x0F;

    std::uint8_t hi_nibble = (lo >> 4) + carry;
    if (hi_nibble > 9) {
      hi_nibble = (hi_nibble + 6) & 0x0F;
      carry     = 1;
    } else {
      carry = 0;
    }
    lo = new_lo | (hi_nibble << 4);

    // BCD increment high byte if carry
    if (carry) {
      new_lo = (hi & 0x0F) + 1;
      if (new_lo > 9) new_lo += 6;

      carry = (new_lo & 0xF0) >> 4;
      new_lo &= 0x0F;

      hi_nibble = (hi >> 4) + carry;
      if (hi_nibble > 9) hi_nibble = (hi_nibble + 6) & 0x0F;

      hi = new_lo | (hi_nibble << 4);
    }

    counter = (static_cast<std::uint16_t>(hi) << 8) | lo;
  }

  //=================================================
  // RegAsmEW - Register Assembler Error/Warning
  //=================================================
  void RegAsmEW(std::uint8_t errorToken) {
    // Check if line already flagged
    if (ErrorF & 0x80) return;

    // Check if warning (odd token) or error (even token)
    bool isWarning = (errorToken & 0x01) != 0;

    if (isWarning) {
      IncrementBCD16(NbrWarns);
      if ((LstWarns & 0x80) == 0) return;  // Warnings suppressed
      // TODO: DoAlert, doPause stubs
    } else {
      SaveErrInfo(errorToken);
      ErrorF = 0x80;
      IncrementBCD16(NbrErrs);
      // TODO: DoAlert, doPause stubs
    }
  }

  // Stub: GSrcLin - Get source line
  void GSrcLin() {
    // TODO: Get next source line
    // Set C flag when no more lines
    C = true;  // For now, signal end
  }

  //=================================================
  // ChrGet2/ChrGot2 - Character Scanner (using CharMap2)
  // This subrtn is part of Scanner
  // Same logic as ChrGet/ChrGot except CharMap2 is used
  // Entry:
  //  (Y) = index into src line
  // Ret:
  //  (A)=char (uppercase if alphabetic)
  //  C=0 - alphanumeric char
  //  C=1 - non-alphanumeric char
  //  V=0 - non-hexdec char
  //  V=1 - hexdec char
  //  (X) - unchanged
  //=================================================
  void ChrGet2() {
    Y++;
    ChrGot2();
  }

  void ChrGot2() {
    A              = SrcP_at(Y);     // LDA (SrcP),Y
    ZPSaveY        = Y;              // STY ZPSaveY
    uint8_t char_y = A;              // TAY
    if ((int8_t)A >= 0) goto L8227;  // BPL L8227 - Must be std ASCII

    std::abort();  // BRK - source file must be std ASCII

  L8227:
    A             = CharMap1[char_y];  // LDA CharMap1,Y (was CharMap2) - Get the bit flags
    uint8_t flags = A;                 // PHA - Save for later use
    A             = char_y;            // TYA - Get back char
    Y             = ZPSaveY;           // LDY ZPSaveY - restore Y
    // PLP - Pop into Status reg
    N = (flags & 0x80) != 0;
    V = (flags & 0x40) != 0;
    Z = (flags & 0x02) != 0;
    C = (flags & 0x01) != 0;
    if (N) {      // BPL doRet3 - If (A)=$61-$7A (a-z)
      A &= 0xDF;  // AND #$DF - convert to upper case
    }
  }

  //=================================================
  // ($88C3) FindSym - Find symbol in symbol table
  // HeaderT-table of ptrs to singly list of keys with same hash value
  // Ret:
  //   C=1 - Symbol not in table
  //   (Y)=0
  //   C=0 - Symbol in table
  //   (A) = flag byte
  //   (Y)-indexing lobyte value field/indexing next char of src line
  //
  // PrvSymP would be set correctly for existing chains
  //=================================================
  void FindSym() {
    // Declare variables at function scope to avoid goto issues
    std::uint8_t* SymP_ptr = nullptr;
    std::uint8_t  old_low  = 0;
    uint8_t       flag     = 0;

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
    SymP_ptr = reinterpret_cast<std::uint8_t*>(static_cast<uintptr_t>(SymP));
    A        = SymP_ptr[Y];  // LDA (SymP),Y
    NxtSymP  = A;            // STA NxtSymP - Point to next node
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
    SymP_ptr = reinterpret_cast<std::uint8_t*>(static_cast<uintptr_t>(SymP));

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

    A &= 0b10111111;  // AND #%10111111 - Set it to referenced
    SymP_ptr[Y] = A;  // STA (SymP),Y
    flag        = A;  // PHA - Save flag byte
    A &= relative;    // AND #relative - Retain this bit
    A |= RelExprF;    // ORA RelExprF
    RelExprF = A;     // STA RelExprF
    A        = flag;  // PLA - Restore

  L891A:
    C = false;  // CLC - Flag symbolic name found
    Y++;        // INY - Indexing value field
    return;     // RTS - or 1st char in next src line?
  }

  //=================================================
  // HashFn
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
  // RsvdId - Check for reserved identifier
  //=================================================
  void RsvdId() {
    IsAXY();         // JSR IsAXY - Chk if reserved idfer
    if (!C) return;  // BCC doRet9 - No
    X = 0x1E;        // LDX #$1E - Reserved idfer err
    RegAsmEW(0x1E);  // JMP RegAsmEW
  }

  //=================================================
  // IsAXY - Chk for a single 'A','X','Y' in label/operand field
  // C=1 - Yes
  // (X) & (Y) - unchanged
  //=================================================
  void IsAXY() {
    ChrGot2();                 // JSR ChrGot2 - Patch here if we want the letters
    if (A == 'X') goto L899F;  // CMP #'X' / BEQ L899F - A,X,Y to be used as labels/operands
    if (A < 'A') {             // CMP #'A' / BCC doRet9
      C = false;
      return;
    }
    if (A == 'A') goto L899F;  // BEQ L899F
    if (A == 'Y') goto L899F;  // CMP #'Y' / BNE L89A7
    goto L89A7;

  L899F:
    ChrGet2();  // JSR ChrGet2 - Is next char alphanumeric?
    Y--;        // DEY - Backup to 1st char
    if (!C) {   // BCC doRet9 - Yes
      C = false;
      return;
    }
    C = true;  // SEC
    return;

  L89A7:
    C = false;  // CLC
    return;
  }

  //=================================================
  // ($89A9) AddNode - Add a node to symbol table
  // Entry:
  //  (A)=initial value of flag byte of the Symbol
  //      ref pg 231 of manual for details
  // Ret:
  //  C=0 - succ
  // (Y)=index last byte of entry (Hi-byte)
  //  C=1 - fail
  // The bits of the flag byte are defined as follows:
  // $80 - undefined
  // $40 - unreferenced
  // $20 - relative to beginning of module
  // $10 - External
  // $08 - Entry
  // $04 - macro (not implemented)
  // $02 - No such label
  // $01 - forward referenced
  // Layout of Node
  //   ptr to next node in chain (set to NIL)
  //   symbolicname (variable in length)
  //   flag byte
  //   16-bit value
  // NB. 1) msb of all chars of symbolic name
  //     except the last one are on
  //     2) The size of a node structure is not fixed.
  // Symbolic names with the same hash value (collision)
  // are connected together in a singly linked list.
  //=================================================
  void AddNode() {
    // Declare variables at function scope to avoid goto issues
    uint8_t       flag        = A;  // PHA - Save flag byte
    uint16_t      sum_low     = 0;
    bool          carry_add   = false;
    std::uint8_t* EndSymT_ptr = nullptr;
    std::uint8_t* PrvSymP_ptr = nullptr;
    std::uint8_t  old_low_1   = 0;
    std::uint8_t  old_low_2   = 0;

    Y = 0;       // LDY #0
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
    A              = Y;  // TYA - A=Y=0
    EndSymT_ptr    = reinterpret_cast<std::uint8_t*>(static_cast<uintptr_t>(EndSymT));
    EndSymT_ptr[Y] = A;               // STA (EndSymT),Y
    A              = EndSymT & 0xFF;  // LDA EndSymT
    PrvSymP_ptr    = reinterpret_cast<std::uint8_t*>(static_cast<uintptr_t>(PrvSymP));
    PrvSymP_ptr[Y] = A;             // STA (PrvSymP),Y
    A              = Y;             // TYA - A=0
    Y++;                            // INY - Y=1
    EndSymT_ptr[Y] = A;             // STA (EndSymT),Y - Set link field to NIL ($0000)
    A              = EndSymT >> 8;  // LDA EndSymT+1
    PrvSymP_ptr[Y] = A;             // STA (PrvSymP),Y - Point @ new entry

    Y--;                                                          // DEY - =0
    old_low_1 = EndSymT & 0xFF;                                   // Save old low byte
    A         = 2;                                                // LDA #2 - Skip past
    A         = static_cast<std::uint8_t>(A + (EndSymT & 0xFF));  // ADC EndSymT
    EndSymT   = (EndSymT & 0xFF00) | A;  // STA EndSymT - the link field so that
    if (A >= old_low_1) goto L89E3;      // BCC L89E3 - Y reg can be used to
    EndSymT += 0x100;                    // INC EndSymT+1 - index both SrcP & EndSymT

    // Labels are stored in the symbol table with
    // msb on except last char.
    // On fall thru, Y=0 for both SrcP and EndSymT.
  L89E3:
    EndSymT_ptr = reinterpret_cast<std::uint8_t*>(static_cast<uintptr_t>(EndSymT));
    ChrGot();  // JSR ChrGot

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
    A |= unrefd;                                 // ORA #unrefd - Mark as unreferenced

  L8A00:
    EndSymT_ptr[Y] = A;          // STA (EndSymT),Y - Set flag byte
    Y++;                         // INY
    A              = PC & 0xFF;  // LDA PC - Set addr associated
    EndSymT_ptr[Y] = A;          // STA (EndSymT),Y
    Y++;                         // INY
    A              = PC >> 8;    // LDA PC+1 - w/this symbol
    EndSymT_ptr[Y] = A;          // STA (EndSymT),Y

    A    = EndSymT & 0xFF;       // LDA EndSymT
    SymP = A;                    // STA SymP - Point @ symbolic name
    A    = EndSymT >> 8;         // LDA EndSymT+1
    SymP |= (A << 8);            // STA SymP+1
    old_low_2 = EndSymT & 0xFF;  // Save old low byte
    A         = Y;               // TYA
    A         = static_cast<std::uint8_t>(A + 1 + (EndSymT & 0xFF));  // SEC / ADC EndSymT
    EndSymT   = (EndSymT & 0xFF00) | A;                               // STA EndSymT - next availmem
    if (A >= old_low_2) goto L8A1E;  // BCC L8A1E - detect carry out
    EndSymT += 0x100;                // INC EndSymT+1

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

    X = 0x12;        // LDX #$12 - sym/rld table full!
    RegAsmEW(0x12);  // JSR RegAsmEW
    CanclAsm(0);     // JMP CanclAsm
  }
}

// Stub: L81F0 - Skip over non-blanks
void L81F0() {
  // TODO: Skip over non-blank characters
  Z = true;  // For now, simulate CR found
}

// NxtField - On entry (Y)=index into src line
// Ret:
// (Y)=0
// src ptr pointing @ 1st char of the field
// (X) - unchanged
void NxtField() {
NxtField_start:
  A = SrcP_at(Y);
  if (A != SPACE) goto L823D;  // (BNE)
  Y++;
  if (Y != 0) goto NxtField_start;  // (BNE)

L823D:
  // CLC
  A = Y;
  A += SrcP;
  SrcP = A;
  A    = 0;
  Y    = A;      // (Y)=0
  A += SrcP_hi;  // with carry
  SrcP_hi = A;
}

// HndlMnem - Implemented below at line ~3191

// Stub: HndlOpnd - Handle operand
void HndlOpnd() {
  // TODO: Parse and handle operand field
}

// Stub: L80F7 - Check for FIN/ELSE directive
void L80F7() {
  // TODO: Check if current line has FIN/ELSE directive
  C = false;  // For now, simulate "not found"
}

// Stub: Chk4ROM - Check for ROM at slot
void Chk4ROM() {
  // TODO: Check if there's a ROM at the specified slot
  Z = true;  // For now, assume ROM present
}

// Stub: PRODOS8 - ProDOS 8 system call
void PRODOS8() {
  // TODO: Make ProDOS 8 system call
  Z = true;  // For now, simulate success
}

// Stub: DOSErrs - Handle DOS errors
void DOSErrs() {
  // TODO: Handle DOS errors (doesn't return)
  std::abort();
}

// Stub: COUT - Console output
void COUT() {
  // TODO: Output character via user's I/O hooks
}

// Stub: MonCOUT - Monitor console output
void MonCOUT() {
  // TODO: Output character to Apple II monitor
}

// Stub: Open4RW - Open file for reading/writing
void Open4RW() {
  // TODO: Open file for I/O
}

// Stub: L92F0 - Print file messages
void L92F0() {
  // TODO: Print file-related messages
}

// SetupVec - Implemented above at line ~1131

// ($8458) GInstLen - We must determine the address mode of opcode
// Ret:
// Length of instruction opcode
void GInstLen() {
  ModWrdL = A;  // 1st flag byte (STA)
  Y++;
  const std::uint8_t* mnemP_ptr = reinterpret_cast<const std::uint8_t*>(MnemP);
  A                             = mnemP_ptr[Y];  // 2nd flag byte - addr mode bits
  ModWrdH                       = A;             // of this mnemonic
  Y++;
  A       = mnemP_ptr[Y];
  SubTIdx = A;  // Index into sub-table of opcode table
  Y--;
  Y--;         // Moveback to 1st flag byte
  NxtField();  // Point @ operand field

  A          = 0;
  LenTIdx    = A;
  ValExpr    = A;  // val of operand if any
  ValExpr_hi = A;

  A = ModWrdL;
  // BIT ModWrdL
  if ((int8_t)A < 0) goto L84CE;  // Directives/SET (BMI)
  // BIT Bit20
  if ((A & 0x20) != 0) goto L84D4;  // Implied (BNE)
  // BIT Bit08
  if ((A & 0x08) != 0) goto L84D9;  // Branch opcodes (BNE)

  // There are now thirteen X6502 addr modes to consider
  GAdrMod();          // Get an index to addr mode table
  if (C) goto L84FE;  // error (BCS)

// Checks the returned/parsed addr mode against permitted modes
ChkAMod:
  LenTIdx = A;  // =0-12
  X       = A;
  A       = AModTbl[X];  // Get the parsed addr mode

  if (X < 8) goto L849B;  // (BCC)

  // When (X)=8-12, the addressing modes are:
  // (zp), (abs), acc, zp,Y & (abs,X)
  A &= ModWrdL;                     // 1st flag byte
  A &= 0b00000111;                  // Retain only these bits
  if (A == AModTbl[X]) goto L8502;  // Is addr mode valid? Yes (BEQ)
  if (A != AModTbl[X]) goto L849F;  // => Further checks (BNE)

// X=0-7 The bits of ModWrdH (2nd flag byte) are completely defined
L849B:
  // BIT ModWrdH - Is the returned mode valid?
  if ((ModWrdH & A) != 0) goto L8502;  // Yes (BNE)

L849F:
  if (X != 11) goto L84A7;   // Was mode parsed as zp,Y? No (BNE)
  A = 5;                     // Force the mode as
  if (A != 0) goto ChkAMod;  // abs,Y (for LDA/STA) (BNE)

L84A7:
  if (X != 1) goto L84B5;  // Was mode parsed as zp? nope (BNE)
  A = ModWrdL;
  A &= 0b00010000;           // JMP/JSR?
  if (A == 0) goto BadMode;  // No (BEQ)
  A = 0;                     // Allow for JMP/JSR zp but
  if (A == 0) goto ChkAMod;  // convert 'em to JMP/JSR abs (always) (BEQ)

L84B5:
  if (X != 8) goto BadMode;  // Was mode parsed as (zp)? No (BNE)
  A = ModWrdL;
  A &= 0b00010000;           // JMP?
  if (A == 0) goto BadMode;  // No (BEQ)
  A = 9;
  if (A != 0) goto ChkAMod;  // Convert to JMP (abs) (always) (BNE)

BadMode:
  X = 0x1C;  // addr mode error
  RegAsmEW(0x1C);
  A       = 0x00;
  LenTIdx = A;             // Assume abs mode addressing
  if (A == 0) goto L84FE;  // always (BEQ)

L84CE:
  // BVS L8513 - => SET directive
  if ((ModWrdL & 0x40) != 0) goto L8513;
  A = 0x00;                // zero len
  if (A == 0) goto L8511;  // for directives (BEQ)

L84D4:
  A = 1;                   // Single byte opcodes
  if (A != 0) goto L8511;  // always (BNE)

// const uint8_t Bit20 = 0x20; // Already defined

// Branch opcodes (both 65C02/SW16)
L84D9:
  A       = 0x00;  // There are no sub-tables for such
  LenTIdx = A;     // opcodes so set this index to 0
  X       = 2;     // len of instr
  A       = ModWrdL;
  A &= 0x10;               // BSL/BRL?
  if (A == 0) goto L84E6;  // No (BEQ)
  X++;                     // =3
L84E6:
  Length = X;
  EvalExpr();
  A = PassNbr;
  if (A == 0) goto L8513;  // (BEQ)
  if (!C) goto L8513;      // (BCC)
  A = NxtToken;
  if (A != (0x34 | 0x80)) goto L84FE;  // Invalid delimiter (BNE)
  X = A;
  RegAsmEW(0x34);  // Invalid delimiter error
  goto L8513;

L84FE:
  A = 3;                   // len of instruction
  if (A != 0) goto L8511;  // always (BNE)

// X=0-12
L8502:
  A = ModWrdL;
  // BIT Bit40 - sw16?
  if ((A & 0x40) == 0) goto L850E;  // no (BEQ)
  A = L851F[X];                     // Get instr len
  if (A != 0) goto L8511;           // always (BNE)

L850E:
  A = InstLenT[X];  // Get instr len
L8511:
  Length = A;
L8513:
  A = Length;
}

// (X)=index into addr mode table
// Not only this, it can be used to index an opcode within
// a sub-table of opcodes (eg ADCOps) by adding it to SubTIdx
// This table is highly dependent on the meaning of the
// bits of the 2 mnemonic flag bytes
const uint8_t InstLenT[] = {
    0x03,  // abs
    0x02,  // zp
    0x02,  // #
    0x02,  // zp,X
    0x03,  // abs,X
    0x03,  // abs,Y
    0x02,  // (zp),Y
    0x02,  // (zp,X)
    0x02   // (zp)
};

// This sub-table is used by SW16 opcodes
const uint8_t L851F[] = {
    0x03,  // (abs) - CPIM
    0x01,  // acc - SW16 Reg ops
    0x02,  // zp,Y
    0x03   // (abs,X)
};

// bit flags used to check the validity of
// the parsed addressing mode
const uint8_t AModTbl[] = {
    0x01,  // abs
    0x02,  // zp
    0x04,  // imm
    0x08,  // zp,X
    0x10,  // abs,X
    0x20,  // abs,Y
    0x40,  // (zp),y
    0x80,  // (zp,X)
    0x03,  // (zp)
    0x01,  // (abs)
    0x02,  // acc
    0x04,  // zp,Y
    0x01   // (abs,X)
};

// AdvPC - A=# to advance
void AdvPC() {
  // CLC
  A += PC;
  PC = A;
  if (A >= PC) return;  // no carry (BCC doRet4)
  PC_hi++;              // INC PC+1
}

// NextRec - Set SrcP to beginning of next assembly src line
// Source Lines are terminated with a CR
// (X)-unchanged
// Ret with (Y)=0 & src ptr pointing @ 1st char of line
void NextRec() {
  Y = 0;
L824D:
  A = SrcP_at(Y);
  Y++;                      // NB: skip past char
  if (A != CR) goto L824D;  // b4 comparision (BNE)

  // On fall thru, Y=# to advance
  AdvSrcP();
}

// L81A3 - Incr line #s, show user we have assembled
// a chunk of code by printing a dot
void L81A3() {
  // BIT NewF - Assembling new file?
  if ((int8_t)NewF >= 0) goto L81AF;  // No (BPL)

  A         = 0;
  BCDNbr[0] = A;  // line # for new file (low byte)
  BCDNbr_hi = A;  // high byte
  NewF      = A;

L81AF:
  // SED - set decimal mode
  // CLC
  A = TotLines;
  A += 1;  // BCD increment (simplified)
  TotLines = A;
  if (A < 100) goto L81C7;  // (BCC)
  A = TotLines_hi;
  A += 1;  // with carry
  TotLines_hi = A;
  if (A < 100) goto L81C7;  // (BCC)
  A = TotLines_2;
  A += 1;  // with carry
  TotLines_2 = A;

L81C7:
  // CLC
  A = BCDNbr[0];  // Low byte of line number
  A += 1;         // BCD increment (simplified)
  BCDNbr[0] = A;
  if (A < 100) goto L81E4;  // (BCC)
  A = BCDNbr_hi;            // High byte
  A += 1;                   // with carry
  BCDNbr_hi = A;

  // CLD - clear decimal mode
  A = PassNbr;
  if (A == 0) goto L81DF;  // (BEQ)
  // BIT ListingF - listing ON?
  if ((int8_t)ListingF < 0) goto L81E4;  // yes (BMI)

L81DF:
  A = '.' | 0x80;  // show a dot
  VidOut();
L81E4:
  // CLD - clear decimal mode
}

// DoPass2 - Second pass of assembly
void DoPass2() {
  // TODO: Implement full Pass 2 assembly logic
  // Pass 2 generates object code based on Pass 1 symbol table
  // For now, stub implementation to unblock Phase 2 testing
}

//=================================================
// HndlMnem - Process mnemonic/pseudo opcode/directive field
// (ASM2.S line ~2054, label HndlMnem)
//
// SIMPLIFIED STUB for Phase 2 testing
// Full 1:1 translation requires table definitions and proper control flow
//=================================================
void HndlMnem() {
  // TODO: Implement full mnemonic dispatch with letter-by-letter table lookup
  // For now, stub to allow Phase 2 tests to compile

  C       = true;  // Assume not found
  ZAB     = 0x80;  // Initialize flag byte
  MnemP   = 0;
  SubTIdx = 0;

  // In a real implementation, this would:
  // 1. Call ChrGot() to get first character
  // 2. Traverse Tbl1stLet to find subtitle table
  // 3. Scan through mnemonic entries with letter-by-letter comparison
  // 4. Return C=0 on match, populate ZAB/MnemP/SubTIdx
  // 5. Return C=1 on no match, call RegAsmEW() with appropriate error
}

// Stub: VidOut - Video output
void VidOut() {
  // TODO: Output character to video
}

// Stub: L986A - Helper function
void L986A() {
  // TODO: Implement L986A
}

// GAdrMod - Stub (incomplete in original translation)
// This subrtn will parse the addressing mode of the operand
void GAdrMod() {
  // TODO: Implement full addressing mode parsing
  // For now, stub to unblock compilation
  C = false;
}

// EvalExpr - Stub (incomplete in original translation)
// C=0 - no errors parsing
// EvalExpr - Stub (incomplete in original translation)
// Evaluate expressions with operator support
void EvalExpr() {
  // TODO: Implement full expression evaluation
  // For now, stub to unblock compilation
  C          = false;
  ValExpr    = 0;
  ValExpr_hi = 0;
  RelExprF   = 0;
  NxtToken   = 0;
}

// Expression operators - stub
void ExprADD() {
}

void ExprSUB() {
}

void ExprMUL() {
}

void ExprDIV() {
}

void ExprEOR() {
}

void ExprAND() {
}

void ExprORA() {
}

// EvalTerm - Stub (incomplete in original translation)
void EvalTerm() {
  // TODO: Implement term evaluation
  C        = false;
  Accum    = 0;
  Accum_hi = 0;
}

// GNToken - Stub (incomplete in original translation)
void GNToken() {
  // TODO: Get next token, validate syntax
  NxtToken = 0;
}

// ChrGot2 - Stub (incomplete)    A = 0x02;
if (X == ')') goto doRet7;  // (CPX #')'; BEQ doRet7)
A = 0x34 + 0x80;            // err token
doRet7 : return;
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
  FindSym();
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
  AddNode();
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

//=================================================
// START of Assembler's Tables
//=================================================

// ($D3D4) Table of Ptrs to messages incl. error msgs
// Ref page 219 Workbench
const char         LD50F[] = "UNDEFINED      IDENTIFIER";
const char         LD524[] = "DUPLICATE      IDENTIFIER";
const char         LD539[] = "UNDEFINED      OPCODE";
const char         LD54A[] = "OVERFLOW";
const char         LD553[] = "RELATIVE       EXPRSN OPERATOR";
const char         LD56C[] = "EXPRESSION     SYNTAX";
const char         LD57E[] = "EQUATE         SYNTAX";
const char         LD58C[] = "INVALID        IDENTIFIER";
const char         LD59F[] = "DSECT/DEND";
const char         LD5AA[] = "SYMBOL/RLD     TABLE FULL";
const char         LD5C0[] = "ASSEMBLER      PARAMETER";
const char         LD5D4[] = "INCLUDE/CHN    NESTING";
const char         LD5E8[] = "MACRO          NESTING";
const char         LD5F6[] = "MACRO          ARGUMENT";
const char         LD605[] = "ADDRESS        MODE";
const char         LD612[] = "RESERVED       IDENTIFIER";
const char         LD626[] = "MACRO          FILE NOT FOUND";
const char         LD63B[] = "DIRECTIVE      OPERAND";
const char         LD64D[] = "BRANCH         RANGE";
const char         LD65A[] = "BYTE           OVERFLOW";
const char         LD668[] = "INDIRECT       SYNTAX";
const char         LD678[] = "INDEXING       SYNTAX";
const char         LD688[] = "INDIRECT       REQUIRES ZPAGE";
const char         LD6A0[] = "INVALID        AFTER 1ST IDENTIFIER";
const char         LD6BD[] = "SW16           REGISTER";
const char         LD6CB[] = "INVALID        DELIMITER";
const char         LD6DD[] = "OBJ            BUFFER OVERFLOW";
const char         LD6F1[] = "OBJ            BUFFER CONFLICT";
const char         LD705[] = "INVALID        FROM INCLUDE";
const char         LD71A[] = "BUFFER         SIZE";
const char         LD726[] = ">255           EXTRNS/ENTRYS";
const char         LD739[] = "DUPLICATE      EXT/ENT";
const std::uint8_t LD74B[] = {'S', 'W', 'E', 'E', 'T', '1', '6', ' ', ' ', ' ', ' ',
                              ' ', ' ', ' ', 'O', 'P', 'C', 'O', 'D', 'E', 0x01};
const std::uint8_t LD75A[] = {'E', 'X', 'T', 'R', 'N', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 'U',
                              'S', 'E', 'D', ' ', 'A', 'S', ' ', 'Z', 'X', 'T', 'R', 'N', 0x01};
const char         LD76E[] = "ORG            MUST BE > $100";
const char         LD781[] = "6502X          ADRS MODE/OPCODE";

const std::uint8_t* ErrMsgT[] = {
    reinterpret_cast<const std::uint8_t*>(LD50F),  // 0
    reinterpret_cast<const std::uint8_t*>(LD524),  // 2
    reinterpret_cast<const std::uint8_t*>(LD539),  // 4
    reinterpret_cast<const std::uint8_t*>(LD54A),  // 6
    reinterpret_cast<const std::uint8_t*>(LD553),  // 8
    reinterpret_cast<const std::uint8_t*>(LD56C),  // A
    reinterpret_cast<const std::uint8_t*>(LD57E),  // C
    reinterpret_cast<const std::uint8_t*>(LD58C),  // E
    reinterpret_cast<const std::uint8_t*>(LD59F),  // 10
    reinterpret_cast<const std::uint8_t*>(LD5AA),  // 12
    reinterpret_cast<const std::uint8_t*>(LD5C0),  // 14
    reinterpret_cast<const std::uint8_t*>(LD5D4),  // 16
    reinterpret_cast<const std::uint8_t*>(LD5E8),  // 18
    reinterpret_cast<const std::uint8_t*>(LD5F6),  // 1A
    reinterpret_cast<const std::uint8_t*>(LD605),  // 1C
    reinterpret_cast<const std::uint8_t*>(LD612),  // 1E
    reinterpret_cast<const std::uint8_t*>(LD626),  // 20
    nullptr,                                       // 22 - none?
    reinterpret_cast<const std::uint8_t*>(LD63B),  // 24
    reinterpret_cast<const std::uint8_t*>(LD64D),  // 26
    reinterpret_cast<const std::uint8_t*>(LD65A),  // 28
    reinterpret_cast<const std::uint8_t*>(LD668),  // 2A
    reinterpret_cast<const std::uint8_t*>(LD678),  // 2C
    reinterpret_cast<const std::uint8_t*>(LD688),  // 2E
    reinterpret_cast<const std::uint8_t*>(LD6A0),  // 30
    reinterpret_cast<const std::uint8_t*>(LD6BD),  // 32
    reinterpret_cast<const std::uint8_t*>(LD6CB),  // 34
    reinterpret_cast<const std::uint8_t*>(LD6DD),  // 36
    reinterpret_cast<const std::uint8_t*>(LD6F1),  // 38
    reinterpret_cast<const std::uint8_t*>(LD705),  // 3A
    reinterpret_cast<const std::uint8_t*>(LD71A),  // 3C
    reinterpret_cast<const std::uint8_t*>(LD726),  // 3E
    reinterpret_cast<const std::uint8_t*>(LD739),  // 40
    reinterpret_cast<const std::uint8_t*>(LD74B),  // 42
    reinterpret_cast<const std::uint8_t*>(LD75A),  // 44
    reinterpret_cast<const std::uint8_t*>(LD76E),  // 46
    reinterpret_cast<const std::uint8_t*>(LD781),  // 48
};

const std::uint8_t FileTxt[]  = "               # ELIF";  // FILE #
const std::uint8_t ASErrTxt[] = "               ERRORS IN THIS ASSEMBLY";
const std::uint8_t SuccTxt[]  = "**             SUCCESSFUL ASSEMBLY := NO ERRORS";
const std::uint8_t WarnTxt[]  = "               WARNINGS IN THIS ASSEMBLY";
const std::uint8_t CreatTxt[] = {'*', '*', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                                 ' ', ' ', 'A', 'S', 'S', 'E', 'M', 'B', 'L', 'E', 'R', ' ', 'C',
                                 'R', 'E', 'A', 'T', 'E', 'D', ' ', 'O', 'N', ' ', 0x01};
const std::uint8_t FSPCTxt[]  = {'*', '*', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                                 ' ', ' ', 'F', 'R', 'E', 'E', ' ', 'S', 'P', 'A', 'C', 'E', ' ',
                                 'P', 'A', 'G', 'E', ' ', 'C', 'O', 'U', 'N', 'T', 0x01};
const std::uint8_t TotLnTxt[] = {'*', '*', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                                 ' ', ' ', 'T', 'O', 'T', 'A', 'L', ' ', 'L', 'I', 'N', 'E', 'S',
                                 ' ', 'A', 'S', 'S', 'E', 'M', 'B', 'L', 'E', 'D', ' ', 0x01};
const std::uint8_t ContTxt[]  = {'P', 'R', 'E', 'S', 'S', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                                 ' ', ' ', ' ', 'R', 'E', 'T', 'U', 'R', 'N', ' ', 'T', 'O',
                                 ' ', 'C', 'O', 'N', 'T', 'I', 'N', 'U', 'E', 0x01};
const std::uint8_t AbortTxt[] = {'A', 'S', 'S', 'E', 'M', 'B', 'L', 'Y', ' ', ' ', ' ', ' ', ' ',
                                 ' ', ' ', 'A', 'B', 'O', 'R', 'T', 'E', 'D', '.', ' ', 'P', 'R',
                                 'E', 'S', 'S', ' ', 'R', 'E', 'T', 'U', 'R', 'N', 0x01};
const std::uint8_t InLinTxt[] = "ENIL           NI RORRE          ";

const std::uint8_t LD798[] = "-----          NEXT OBJECT FILE NAME IS ";
const std::uint8_t LD7B7[] = {0x0D, 'S', 'O', 'U', 'R', 'C', 'E', ' ', ' ', ' ', ' ',
                              ' ',  ' ', ' ', ' ', 'F', 'I', 'L', 'E', ' ', '#'};
const std::uint8_t LD7C7[] = {0x0D, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                              ' ',  ' ', ' ', ' ', 'I', 'N', 'C', 'L', 'U', 'D',
                              'E',  ' ', 'F', 'I', 'L', 'E', ' ', '#'};

//=================================================
// Table for operand parser (47 bytes)
//=================================================
const std::uint8_t AModTkns[] = {
    0xA3, 0x00, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0,
    0xA0, 0x05, 0xA8, 0x00, 0xAC, 0xD8, 0xA9, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0,
    0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0x02, 0x0F, 0x06, 0x19, 0xA9, 0xAC, 0xD9, 0xA0, 0xA0, 0xA0,
    0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0x02, 0x0D, 0xA0, 0xA0,
    0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0x06, 0x02, 0x11,
    0x13, 0x04, 0x15, 0x8D, 0x15, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0,
    0xA0, 0xA0, 0xA0, 0xA0, 0xBB, 0x15, 0x00, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0,
    0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0x02, 0x03, 0x01, 0xAC, 0xD8, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0,
    0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0x02, 0x07, 0x09, 0xD9, 0xA0, 0xA0,
    0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0x02, 0x17, 0x0B,
};

const std::uint8_t AModCmds[] = {
    0x12, 0x00, 0xB4, 0x00, 0x51, 0x00, 0x2A, 0xAA, 0xAA, 0xB4, 0x28, 0x00, 0xAE, 0x00, 0xAA, 0x3E,
    0xAA, 0xB4, 0xAE, 0x00, 0xB4, 0x50, 0x50, 0x00, 0x00, 0x53, 0x00, 0x55, 0x00, 0x66, 0x00, 0x00,
    0x79, 0x78, 0x00, 0x00, 0xB4, 0x8D, 0xB4, 0x8C, 0x00, 0x00, 0xAC, 0xB4, 0x9F, 0x00, 0x00,
};

//=================================================
// ($D835) 6502/X6502 Opcode Translation table  213 bytes
//=================================================
constexpr std::size_t ADCOps  = 0;
constexpr std::size_t ANDOps  = 9;
constexpr std::size_t ASLOps  = 18;
constexpr std::size_t BITOps  = 26;
constexpr std::size_t CLROps  = 38;
constexpr std::size_t CMPOps  = 42;
constexpr std::size_t CPXOps  = 51;
constexpr std::size_t CPYOps  = 54;
constexpr std::size_t DECOps  = 57;
constexpr std::size_t EOROps  = 64;
constexpr std::size_t INCOps  = 73;
constexpr std::size_t JMPOps  = 80;
constexpr std::size_t JSROps  = 83;
constexpr std::size_t LDAOps  = 84;
constexpr std::size_t LDXOps  = 93;
constexpr std::size_t LDYOps  = 99;
constexpr std::size_t LSROps  = 104;
constexpr std::size_t NOPOps  = 109;
constexpr std::size_t ORAOps  = 110;
constexpr std::size_t PSHOps  = 119;
constexpr std::size_t ROLOps  = 127;
constexpr std::size_t ROROps  = 132;
constexpr std::size_t SBCOps  = 139;
constexpr std::size_t STAOps  = 151;
constexpr std::size_t STXOps  = 160;
constexpr std::size_t STYOps  = 164;
constexpr std::size_t STZOps  = 168;
constexpr std::size_t TRBOps  = 175;
constexpr std::size_t TSBOps  = 177;
constexpr std::size_t SW16Ops = 183;

const std::uint8_t OpcodeT[] = {
    0x6D, 0x65, 0x69, 0x75, 0x7D, 0x79, 0x71, 0x61, 0x72, 0x2D, 0x25, 0x29, 0x35, 0x3D, 0x39, 0x31,
    0x21, 0x32, 0x0E, 0x06, 0x0A, 0x16, 0x1E, 0x90, 0xB0, 0xF0, 0x2C, 0x24, 0x89, 0x34, 0x3C, 0x30,
    0xD0, 0x10, 0x80, 0x00, 0x50, 0x70, 0x18, 0xD8, 0x58, 0xB8, 0xCD, 0xC5, 0xC9, 0xD5, 0xDD, 0xD9,
    0xD1, 0xC1, 0xD2, 0xEC, 0xE4, 0xE0, 0xCC, 0xC4, 0xC0, 0xCE, 0xC6, 0x3A, 0xD6, 0xDE, 0xCA, 0x88,
    0x4D, 0x45, 0x49, 0x55, 0x5D, 0x59, 0x51, 0x41, 0x52, 0xEE, 0xE6, 0x1A, 0xF6, 0xFE, 0xE8, 0xC8,
    0x4C, 0x6C, 0x7C, 0x20, 0xAD, 0xA5, 0xA9, 0xB5, 0xBD, 0xB9, 0xB1, 0xA1, 0xB2, 0xAE, 0xA6, 0xA2,
    0xB6, 0x00, 0xBE, 0xAC, 0xA4, 0xA0, 0xB4, 0xBC, 0x4E, 0x46, 0x4A, 0x56, 0x5E, 0xEA, 0x0D, 0x05,
    0x09, 0x15, 0x1D, 0x19, 0x11, 0x01, 0x12, 0x48, 0x08, 0xDA, 0x5A, 0x68, 0x28, 0xFA, 0x7A, 0x2E,
    0x26, 0x2A, 0x36, 0x3E, 0x6E, 0x66, 0x6A, 0x76, 0x7E, 0x40, 0x60, 0xED, 0xE5, 0xE9, 0xF5, 0xFD,
    0xF9, 0xF1, 0xE1, 0xF2, 0x38, 0xF8, 0x78, 0x8D, 0x85, 0x00, 0x95, 0x9D, 0x99, 0x91, 0x81, 0x92,
    0x8E, 0x86, 0x00, 0x96, 0x8C, 0x84, 0x00, 0x94, 0x9C, 0x64, 0x00, 0x74, 0x9E, 0xAA, 0xA8, 0x1C,
    0x14, 0x0C, 0x04, 0xBA, 0x8A, 0x9A, 0x98, 0xA0, 0x03, 0x0A, 0x05, 0x08, 0x02, 0x09, 0x07, 0x04,
    0x01, 0x0E, 0x0C, 0x0F, 0x06, 0x0D, 0xD0, 0xF0, 0xE0, 0x20, 0x60, 0x40, 0x80, 0xC0, 0x00, 0x0B,
    0x30, 0x50, 0x70, 0x90, 0xB0,
};

//=================================================
// ($D90A) 213 bytes cycle times
//=================================================
const std::uint8_t CycTimes[] = {
    4,    3,    2,    4,    4,    4,    5,    6,    5,    4,    3,    2,    4,    4,    4,    5,
    6,    5,    6,    5,    2,    6,    7,    3,    3,    3,    4,    3,    2,    4,    4,    3,
    3,    3,    3,    7,    3,    3,    2,    2,    2,    2,    4,    3,    2,    4,    4,    4,
    5,    6,    5,    4,    3,    2,    4,    3,    2,    6,    5,    2,    6,    7,    2,    2,
    4,    3,    2,    4,    4,    4,    5,    6,    5,    6,    5,    2,    6,    7,    2,    2,
    3,    5,    6,    6,    4,    3,    2,    4,    4,    4,    5,    6,    5,    4,    3,    2,
    4,    0x29, 4,    4,    3,    2,    4,    4,    6,    5,    2,    6,    7,    2,    4,    3,
    2,    4,    4,    4,    5,    6,    5,    3,    3,    3,    3,    4,    4,    4,    4,    6,
    5,    2,    6,    7,    6,    5,    2,    6,    7,    6,    6,    4,    3,    2,    4,    4,
    4,    5,    6,    5,    2,    2,    2,    4,    3,    0x29, 4,    5,    5,    6,    6,    5,
    4,    3,    0x29, 4,    4,    3,    0x29, 4,    4,    3,    0x29, 4,    5,    2,    2,    6,
    5,    6,    5,    2,    2,    2,    2,    0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29,
    0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29,
    0x29, 0x29, 0x29, 0x29, 0x29,
};

//=================================================
// ($D9DF) Char mapping
//=================================================
const std::uint8_t CharMap1[] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43,
    0x43, 0x43, 0x43, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01, 0x01, 0x01, 0x01, 0x01,
};

//=================================================
// Char mapping
//=================================================
const std::uint8_t CharMap2[] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x42, 0x42, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01, 0x01, 0x01, 0x01, 0x01,
};

//=================================================
// Directives/Mnemonics/Sweet16 table
//=================================================
// MnemTbl

// LtrA
const std::uint8_t LtrA[] = {
    'A', 'D', DCI_end('C'), 0x03, 0xFF, static_cast<std::uint8_t>(ADCOps),
    'A', 'D', DCI_end('D'), 0x40, 0x02, static_cast<std::uint8_t>(SW16Ops),
    'A', 'N', DCI_end('D'), 0x03, 0xFF, static_cast<std::uint8_t>(ANDOps),
    'A', 'S', DCI_end('C'), 0x80, 0x00, 0x00,  // L8DD2-1
    'A', 'S', DCI_end('L'), 0x02, 0x1B, static_cast<std::uint8_t>(ASLOps),
};

// LtrB
const std::uint8_t LtrB[] = {
    'B',
    DCI_end('C'),
    0x48,
    0x03,
    static_cast<std::uint8_t>(SW16Ops + 1),
    'B',
    'C',
    DCI_end('C'),
    0x08,
    0x03,
    0x17,
    'B',
    'C',
    DCI_end('S'),
    0x08,
    0x03,
    0x18,
    'B',
    'E',
    DCI_end('Q'),
    0x08,
    0x03,
    0x19,
    'B',
    'G',
    DCI_end('E'),
    0x08,
    0x03,
    0x18,
    'B',
    'I',
    DCI_end('T'),
    0x00,
    0x1F,
    static_cast<std::uint8_t>(BITOps),
    'B',
    DCI_end('K'),
    0x60,
    0x00,
    static_cast<std::uint8_t>(SW16Ops + 2),
    'B',
    'L',
    DCI_end('T'),
    0x08,
    0x03,
    0x17,
    'B',
    DCI_end('M'),
    0x48,
    0x03,
    static_cast<std::uint8_t>(SW16Ops + 3),
    'B',
    'M',
    DCI_end('1'),
    0x48,
    0x03,
    static_cast<std::uint8_t>(SW16Ops + 4),
    'B',
    'M',
    DCI_end('I'),
    0x08,
    0x03,
    0x1F,
    'B',
    'N',
    DCI_end('C'),
    0x48,
    0x03,
    static_cast<std::uint8_t>(SW16Ops + 5),
    'B',
    'N',
    DCI_end('E'),
    0x08,
    0x03,
    0x20,
    'B',
    'N',
    'M',
    DCI_end('1'),
    0x48,
    0x03,
    static_cast<std::uint8_t>(SW16Ops + 6),
    'B',
    'N',
    DCI_end('Z'),
    0x48,
    0x03,
    static_cast<std::uint8_t>(SW16Ops + 7),
    'B',
    DCI_end('P'),
    0x48,
    0x03,
    static_cast<std::uint8_t>(SW16Ops + 8),
    'B',
    'P',
    DCI_end('L'),
    0x08,
    0x03,
    0x21,
    'B',
    DCI_end('R'),
    0x48,
    0x03,
    static_cast<std::uint8_t>(SW16Ops + 9),
    'B',
    'R',
    DCI_end('A'),
    0x08,
    0x03,
    0x22,
    'B',
    'R',
    DCI_end('K'),
    0x20,
    0x00,
    0x23,
    'B',
    'R',
    DCI_end('L'),
    0x58,
    0x03,
    static_cast<std::uint8_t>(SW16Ops + 10),
    'B',
    DCI_end('S'),
    0x48,
    0x03,
    static_cast<std::uint8_t>(SW16Ops + 11),
    'B',
    'S',
    DCI_end('L'),
    0x58,
    0x03,
    static_cast<std::uint8_t>(SW16Ops + 12),
    'B',
    'V',
    DCI_end('C'),
    0x08,
    0x03,
    0x24,
    'B',
    'V',
    DCI_end('S'),
    0x08,
    0x03,
    0x25,
    'B',
    DCI_end('Z'),
    0x48,
    0x03,
    static_cast<std::uint8_t>(SW16Ops + 13),
};

// LtrC
const std::uint8_t LtrC[] = {
    'C',
    'H',
    DCI_end('N'),
    0x80,
    0x00,
    0x00,  // L928C-1
    'C',
    'H',
    DCI_end('R'),
    0x80,
    0x00,
    0x00,  // L8FC7-1
    'C',
    'L',
    DCI_end('C'),
    0x20,
    0x00,
    0x26,
    'C',
    'L',
    DCI_end('D'),
    0x20,
    0x00,
    0x27,
    'C',
    'L',
    DCI_end('I'),
    0x20,
    0x00,
    0x28,
    'C',
    'L',
    DCI_end('V'),
    0x20,
    0x00,
    0x29,
    'C',
    'M',
    DCI_end('P'),
    0x03,
    0xFF,
    static_cast<std::uint8_t>(CMPOps),
    'C',
    'P',
    'I',
    DCI_end('M'),
    0x50,
    0x01,
    static_cast<std::uint8_t>(SW16Ops + 14),
    'C',
    'P',
    DCI_end('R'),
    0x40,
    0x02,
    static_cast<std::uint8_t>(SW16Ops + 15),
    'C',
    'P',
    DCI_end('X'),
    0x00,
    0x07,
    static_cast<std::uint8_t>(CPXOps),
    'C',
    'P',
    DCI_end('Y'),
    0x00,
    0x07,
    static_cast<std::uint8_t>(CPYOps),
};

// LtrD
const std::uint8_t LtrD[] = {
    'D',          'A',          'T',
    DCI_end('E'), 0x80,         0x00,
    0x00,  // L901D-1
    'D',          DCI_end('B'), 0x80,
    0x00,         0x00,  // L8CC3-1
    'D',          'C',          DCI_end('I'),
    0x80,         0x00,         0x00,  // L8E54-1
    'D',          'C',          DCI_end('R'),
    0x40,         0x02,         static_cast<std::uint8_t>(SW16Ops + 16),
    'D',          'D',          DCI_end('B'),
    0x80,         0x00,         0x00,  // L8DCD-1
    'D',          'E',          DCI_end('C'),
    0x02,         0x1B,         static_cast<std::uint8_t>(DECOps),
    'D',          'E',          DCI_end('F'),
    0x81,         0x00,         0x00,  // L9144-1
    'D',          'E',          'N',
    DCI_end('D'), 0x80,         0x00,
    0x00,  // L908E-1
    'D',          'E',          DCI_end('X'),
    0x20,         0x00,         0x3E,
    'D',          'E',          DCI_end('Y'),
    0x20,         0x00,         0x3F,
    'D',          'F',          DCI_end('B'),
    0x80,         0x00,         0x00,  // L8CC3-1
    'D',          DCI_end('O'), 0x81,
    0x00,         0x00,  // L90B7-1
    'D',          DCI_end('S'), 0x81,
    0x00,         0x00,  // L8C0E-1
    'D',          'S',          'E',
    'C',          DCI_end('T'), 0x80,
    0x00,         0x00,  // L9065-1
    'D',          DCI_end('W'), 0x80,
    0x00,         0x00,  // L8D67-1
};

// LtrE
const std::uint8_t LtrE[] = {
    'E',  'L',  'S',          DCI_end('E'), 0x80,         0x00,
    0x00,  // L90CB-1
    'E',  'N',  'T',          'R',          DCI_end('Y'), 0x81,
    0x00, 0x00,  // L9144-1
    'E',  'O',  DCI_end('R'), 0x03,         0xFF,         static_cast<std::uint8_t>(EOROps),
    'E',  'Q',  DCI_end('U'), 0x81,         0x00,         0x00,  // L8A31-1
    'E',  'X',  'T',          'R',          DCI_end('N'), 0x81,
    0x00, 0x00,  // L91A8-1
};

// LtrF
const std::uint8_t LtrF[] = {
    'F', 'A', 'I',          DCI_end('L'), 0x80, 0x00, 0x00,  // L9215-1
    'F', 'I', DCI_end('N'), 0x80,         0x00, 0x00,        // L90D7-1
};

// LtrI
const std::uint8_t LtrI[] = {
    'I',          'B',          'U',
    'F',          'S',          'I',
    DCI_end('Z'), 0x81,         0x00,
    0x00,  // L93C4-1
    'I',          'D',          'N',
    'U',          DCI_end('M'), 0x80,
    0x00,         0x00,  // L905E-1
    'I',          'F',          'E',
    DCI_end('Q'), 0x81,         0x00,
    0x00,  // L90DE-1
    'I',          'F',          'G',
    DCI_end('E'), 0x81,         0x00,
    0x00,  // L90FC-1
    'I',          'F',          'G',
    DCI_end('T'), 0x81,         0x00,
    0x00,  // L90EB-1
    'I',          'F',          'N',
    DCI_end('E'), 0x81,         0x00,
    0x00,  // L90B7-1
    'I',          'F',          'L',
    DCI_end('E'), 0x81,         0x00,
    0x00,  // L9112-1
    'I',          'F',          'L',
    DCI_end('T'), 0x81,         0x00,
    0x00,  // L9107-1
    'I',          'N',          DCI_end('C'),
    0x02,         0x1B,         static_cast<std::uint8_t>(INCOps),
    'I',          'N',          'C',
    'L',          'U',          'D',
    DCI_end('E'), 0x80,         0x00,
    0x00,  // L9360-1
    'I',          'N',          DCI_end('R'),
    0x40,         0x02,         static_cast<std::uint8_t>(SW16Ops + 17),
    'I',          'N',          'T',
    'E',          'R',          DCI_end('P'),
    0x81,         0x00,         0x00,  // L9131-1
    'I',          'N',          DCI_end('X'),
    0x20,         0x00,         0x4E,
    'I',          'N',          DCI_end('Y'),
    0x20,         0x00,         0x4F,
};

// LtrJ
const std::uint8_t LtrJ[] = {
    'J', 'M', DCI_end('P'), 0x11, 0x01, static_cast<std::uint8_t>(JMPOps),
    'J', 'S', DCI_end('R'), 0x10, 0x01, static_cast<std::uint8_t>(JSROps),
};

// LtrL
const std::uint8_t LtrL[] = {
    'L',  DCI_end('D'), 0x40, 0x02, static_cast<std::uint8_t>(SW16Ops + 18), 'L',
    'D',  DCI_end('A'), 0x03, 0xFF, static_cast<std::uint8_t>(LDAOps),       'L',
    'D',  DCI_end('D'), 0x40, 0x02, static_cast<std::uint8_t>(SW16Ops + 19), 'L',
    'D',  DCI_end('I'), 0x40, 0x02, static_cast<std::uint8_t>(SW16Ops + 20), 'L',
    'D',  DCI_end('P'), 0x40, 0x02, static_cast<std::uint8_t>(SW16Ops + 21), 'L',
    'D',  DCI_end('X'), 0x04, 0x27, static_cast<std::uint8_t>(LDXOps),       'L',
    'D',  DCI_end('Y'), 0x00, 0x1F, static_cast<std::uint8_t>(LDYOps),       'L',
    'S',  DCI_end('L'), 0x02, 0x1B, static_cast<std::uint8_t>(ASLOps),       'L',
    'S',  DCI_end('R'), 0x02, 0x1B, static_cast<std::uint8_t>(LSROps),       'L',
    'S',  DCI_end('T'), 0x80, 0x00,
    0x00,  // L8ECA-1
};

// LtrM
const std::uint8_t LtrM[] = {
    'M', 'A', 'C',          'L',  'I',  DCI_end('B'), 0x80, 0x00, 0x00,  // L937F-1
    'M', 'S', DCI_end('B'), 0x80, 0x00, 0x00,                            // L8E66-1
};

// LtrN
const std::uint8_t LtrN[] = {
    'N', 'O', DCI_end('P'), 0x20, 0x00, 0x6D,
};

// LtrO
const std::uint8_t LtrO[] = {
    'O', 'B', DCI_end('J'), 0x81, 0x00, 0x00,  // L8BAD-1
    'O', 'R', DCI_end('A'), 0x03, 0xFF, static_cast<std::uint8_t>(ORAOps),
    'O', 'R', DCI_end('G'), 0x81, 0x00, 0x00,  // L8A82-1
};

// LtrP
const std::uint8_t LtrP[] = {
    'P',
    'A',
    'G',
    DCI_end('E'),
    0x80,
    0x00,
    0x00,  // DoPage-1
    'P',
    'A',
    'U',
    'S',
    DCI_end('E'),
    0x80,
    0x00,
    0x00,  // L927A-1
    'P',
    'H',
    DCI_end('A'),
    0x20,
    0x00,
    0x77,
    'P',
    'H',
    DCI_end('P'),
    0x20,
    0x00,
    0x78,
    'P',
    'H',
    DCI_end('X'),
    0x20,
    0x00,
    0x79,
    'P',
    'H',
    DCI_end('Y'),
    0x20,
    0x00,
    0x7A,
    'P',
    'L',
    DCI_end('A'),
    0x20,
    0x00,
    0x7B,
    'P',
    'L',
    DCI_end('P'),
    0x20,
    0x00,
    0x7C,
    'P',
    'L',
    DCI_end('X'),
    0x20,
    0x00,
    0x7D,
    'P',
    'L',
    DCI_end('Y'),
    0x20,
    0x00,
    0x7E,
    'P',
    'O',
    DCI_end('P'),
    0x40,
    0x02,
    static_cast<std::uint8_t>(SW16Ops + 21),
    'P',
    'O',
    'P',
    DCI_end('D'),
    0x40,
    0x02,
    static_cast<std::uint8_t>(SW16Ops + 22),
};

// LtrR
const std::uint8_t LtrR[] = {
    'R',
    'E',
    DCI_end('F'),
    0x81,
    0x00,
    0x00,  // L91A8-1
    'R',
    'E',
    DCI_end('L'),
    0x80,
    0x00,
    0x00,  // L9126-1
    'R',
    'E',
    DCI_end('P'),
    0x80,
    0x00,
    0x00,  // L8FA3-1
    'R',
    'O',
    DCI_end('L'),
    0x02,
    0x1B,
    static_cast<std::uint8_t>(ROLOps),
    'R',
    'O',
    DCI_end('R'),
    0x02,
    0x1B,
    static_cast<std::uint8_t>(ROROps),
    'R',
    DCI_end('S'),
    0x60,
    0x00,
    static_cast<std::uint8_t>(SW16Ops + 24),
    'R',
    'T',
    DCI_end('I'),
    0x20,
    0x00,
    0x89,
    'R',
    'T',
    DCI_end('N'),
    0x60,
    0x00,
    0x23,
    'R',
    'T',
    DCI_end('S'),
    0x20,
    0x00,
    0x8A,
};

// LtrS
const std::uint8_t LtrS[] = {
    'S',
    'B',
    DCI_end('C'),
    0x03,
    0xFF,
    static_cast<std::uint8_t>(SBCOps),
    'S',
    'B',
    'T',
    DCI_end('L'),
    0x80,
    0x00,
    0x00,  // L8F61-1
    'S',
    'B',
    'U',
    'F',
    'S',
    'I',
    DCI_end('Z'),
    0x81,
    0x00,
    0x00,  // L93C7-1
    'S',
    'E',
    DCI_end('C'),
    0x20,
    0x00,
    0x94,
    'S',
    'E',
    DCI_end('D'),
    0x20,
    0x00,
    0x95,
    'S',
    'E',
    DCI_end('I'),
    0x20,
    0x00,
    0x96,
    'S',
    'E',
    DCI_end('T'),
    0xC0,
    0x00,
    0x00,  // L94C0-1
    'S',
    'K',
    DCI_end('P'),
    0x80,
    0x00,
    0x00,  // L8FEA-1
    'S',
    DCI_end('T'),
    0x40,
    0x02,
    static_cast<std::uint8_t>(SW16Ops + 25),
    'S',
    'T',
    DCI_end('A'),
    0x03,
    0xFB,
    static_cast<std::uint8_t>(STAOps),
    'S',
    'T',
    DCI_end('D'),
    0x40,
    0x02,
    static_cast<std::uint8_t>(SW16Ops + 27),
    'S',
    'T',
    DCI_end('I'),
    0x40,
    0x02,
    static_cast<std::uint8_t>(SW16Ops + 26),
    'S',
    'T',
    DCI_end('P'),
    0x40,
    0x02,
    static_cast<std::uint8_t>(SW16Ops + 28),
    'S',
    'T',
    DCI_end('R'),
    0x80,
    0x00,
    0x00,  // L8E94-1
    'S',
    'T',
    DCI_end('X'),
    0x04,
    0x03,
    static_cast<std::uint8_t>(STXOps),
    'S',
    'T',
    DCI_end('Y'),
    0x00,
    0x0B,
    static_cast<std::uint8_t>(STYOps),
    'S',
    'T',
    DCI_end('Z'),
    0x00,
    0x1B,
    static_cast<std::uint8_t>(STZOps),
    'S',
    'U',
    DCI_end('B'),
    0x40,
    0x02,
    static_cast<std::uint8_t>(SW16Ops + 29),
    'S',
    'Y',
    DCI_end('S'),
    0x81,
    0x00,
    0x00,  // L9131-1
    'S',
    'W',
    '1',
    DCI_end('6'),
    0xC0,
    0x00,
    0x00,  // L946F-1
};

// LtrT
const std::uint8_t LtrT[] = {
    'T',  'A', DCI_end('X'), 0x20,         0x00, 0xAD,
    'T',  'A', DCI_end('Y'), 0x20,         0x00, 0xAE,
    'T',  'I', 'M',          DCI_end('E'), 0x80, 0x00,
    0x00,  // L905E-1
    'T',  'R', DCI_end('B'), 0x00,         0x03, static_cast<std::uint8_t>(TRBOps),
    'T',  'S', DCI_end('B'), 0x00,         0x03, static_cast<std::uint8_t>(TSBOps),
    'T',  'S', DCI_end('X'), 0x20,         0x00, 0xB3,
    'T',  'X', DCI_end('A'), 0x20,         0x00, 0xB4,
    'T',  'X', DCI_end('S'), 0x20,         0x00, 0xB5,
    'T',  'Y', DCI_end('A'), 0x20,         0x00, 0xB6,
};

// LtrX
const std::uint8_t LtrX[] = {
    'X', '6', '5', '0', DCI_end('2'), 0x81, 0x00, 0x00,  // L9139-1
};

// LtrZ
const std::uint8_t LtrZ[] = {
    'Z', 'D', 'E', DCI_end('F'), 0x81,         0x00, 0x00,        // L9140-1
    'Z', 'R', 'E', DCI_end('F'), 0x81,         0x00, 0x00,        // L91A4-1
    'Z', 'X', 'T', 'R',          DCI_end('N'), 0x81, 0x00, 0x00,  // L91A4-1
};

// Dot directives
const std::uint8_t DotDrtv[] = {
    '.',  'A',  'S',  'C',          'I',          DCI_end('I'), 0x80,         0x00,
    0x00,  // L8DD2-1
    '.',  'B',  'L',  'O',          'C',          DCI_end('K'), 0x81,         0x00,
    0x00,                                                                            // L8C0E-1
    '.',  'B',  'Y',  'T',          DCI_end('E'), 0x80,         0x00,         0x00,  // L8CC3-1
    '.',  'D',  'B',  'Y',          'T',          DCI_end('E'), 0x80,         0x00,
    0x00,                                                              // L8DCD-1
    '.',  'D',  'E',  DCI_end('F'), 0x81,         0x00,         0x00,  // L9144-1
    '.',  'E',  'Q',  DCI_end('U'), 0x81,         0x00,         0x00,  // L8A31-1
    '.',  'I',  'N',  'C',          'L',          'U',          'D',          DCI_end('E'),
    0x80, 0x00, 0x00,                                                                // L9360-1
    '.',  'L',  'I',  'S',          DCI_end('T'), 0x80,         0x00,         0x00,  // L8F3D-1
    '.',  'N',  'O',  'L',          'I',          'S',          DCI_end('T'), 0x80,
    0x00, 0x00,                                                                      // L8F3A-1
    '.',  'O',  'R',  DCI_end('G'), 0x81,         0x00,         0x00,                // L8A82-1
    '.',  'P',  'A',  'G',          DCI_end('E'), 0x80,         0x00,         0x00,  // DoPage-1
    '.',  'R',  'E',  DCI_end('F'), 0x81,         0x00,         0x00,                // L91A8-1
    '.',  'S',  'K',  'I',          DCI_end('P'), 0x80,         0x00,         0x00,  // L8FEA-1
    '.',  'T',  'I',  'T',          'L',          DCI_end('E'), 0x80,         0x00,
    0x00,                                                                            // L8F61-1
    '.',  'W',  'O',  'R',          DCI_end('D'), 0x80,         0x00,         0x00,  // L8D67-1
    0x00, 0x00, 0x00, 0x00,                                                          // unused
};

// Table of ptrs to first letter subtables
const std::uint8_t* Tbl1stLet[] = {
    DotDrtv, LtrA, LtrB,    LtrC,    LtrD,    LtrE,    LtrF, nullptr, nullptr,
    LtrI,    LtrJ, nullptr, LtrL,    LtrM,    LtrN,    LtrO, LtrP,    nullptr,
    LtrR,    LtrS, LtrT,    nullptr, nullptr, nullptr, LtrX, nullptr, LtrZ,
};

// MnemTbl points at LtrA
const std::uint8_t* MnemTbl = LtrA;

// Assembler's creation date and time
const char LDF3F[] = "30-APR-85";
const char LDF48[] = "22:46          ";

// Area to preserve the zero page locations $60-$F1
std::uint8_t SvZPArea[0x92] = {};

// ($DFE0) Unidentified data block
const std::uint16_t DFE0Data[] = {
    0xD581, 0xD589, 0xD58E, 0xD546, 0xA023, 0xA048, 0xA308, 0xDCC7,
    0x9BF8, 0xA071, 0xB9AF, 0x9B18, 0x9B6D, 0xDDC0, 0x9F49, 0xC8D3,
};

}  // namespace

//=================================================
// Test Helper Functions - Exported for Unit Testing
//=================================================
namespace EdAsmNg {
  namespace Asm {

    void ResetErrorState() {
      NbrErrs  = 0;
      NbrWarns = 0;
      ErrorF   = 0;
      ErrNbr4  = 0;
      std::memset(ErrInfoT, 0, sizeof(ErrInfoT));
    }

    uint16_t GetErrorCount() {
      return NbrErrs;
    }

    uint16_t GetWarningCount() {
      return NbrWarns;
    }

    uint8_t GetErrorFlag() {
      return ErrorF;
    }

    uint8_t GetErrNbr4() {
      return ErrNbr4;
    }

    void SetVidSlot(uint8_t slot) {
      VidSlot = slot;
    }

    void SetFileNbr(uint8_t file) {
      FileNbr = file;
    }

    void SetBCDLineNumber(uint8_t hi, uint8_t lo) {
      BCDNbr[1] = hi;
      BCDNbr[0] = lo;
    }

    void SetLstWarns(uint8_t flags) {
      LstWarns = flags;
    }

    struct ErrorInfo {
      uint8_t fileNbr;
      uint8_t errIndex;
      uint8_t lineHi;
      uint8_t lineLo;
    };

    ErrorInfo GetErrorInfo(int index) {
      ErrorInfo info;
      int       offset = index * 4;
      if (offset >= 0 && offset < (int)sizeof(ErrInfoT) - 3) {
        info.fileNbr  = ErrInfoT[offset];
        info.errIndex = ErrInfoT[offset + 1];
        info.lineHi   = ErrInfoT[offset + 2];
        info.lineLo   = ErrInfoT[offset + 3];
      } else {
        info.fileNbr  = 0;
        info.errIndex = 0;
        info.lineHi   = 0;
        info.lineLo   = 0;
      }
      return info;
    }

    void RegAsmEW(uint8_t errorToken) {
      ::RegAsmEW(errorToken);
    }

    void SaveErrInfo(uint8_t errorToken) {
      ::SaveErrInfo(errorToken);
    }

    //=================================================
    // Phase 2: Mnemonic Dispatch Test Helpers
    //=================================================

    // Test source line buffer
    static uint8_t  test_src_buffer[256];
    static uint8_t* test_SrcP_ptr = nullptr;

    void ResetDispatchState() {
      MnemP   = 0;
      ZAB     = 0x80;
      SubTIdx = 0;
      MacroF  = 0;  // No macros for testing
      Y       = 0;
      std::memset(test_src_buffer, 0, sizeof(test_src_buffer));
    }

    void SetupSourceLine(const char* line) {
      // Copy line to test buffer
      size_t len = strlen(line);
      if (len > sizeof(test_src_buffer) - 1) {
        len = sizeof(test_src_buffer) - 1;
      }
      std::memcpy(test_src_buffer, line, len);
      test_src_buffer[len] = CR;  // Add CR terminator

      // Set up global test buffer pointer so SrcP_at() uses it
      g_test_src_buffer = test_src_buffer;

      // Set up SrcP to point to test buffer (for pointer arithmetic)
      test_SrcP_ptr = test_src_buffer;

      // Initialize Y to 0 (start of line)
      Y = 0;
    }

    uint16_t GetMnemP() {
      return MnemP;
    }

    uint8_t GetZAB() {
      return ZAB;
    }

    uint8_t GetSubTIdx() {
      return SubTIdx;
    }

    bool HndlMnem() {
      // Call the internal HndlMnem function
      // It returns via setting the C flag
      // For testing, we need to capture the C flag state

      // Re-route SrcP array access to test buffer
      // This is a workaround since SrcP is uint16_t but used as array
      auto old_SrcP = ::SrcP;

      // Temporarily replace the SrcP array access mechanism
      // by modifying how SrcP_at(Y) is evaluated
      // Since we can't directly change pointer behavior, we'll use a different approach

      // For now, call HndlMnem with Y=0
      ::HndlMnem();

      // Return success status (C flag: false=success, true=error)
      return !C;
    }

  }  // namespace Asm
}  // namespace EdAsmNg
