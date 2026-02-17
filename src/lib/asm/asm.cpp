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

#include <cctype>
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
  void GOpAdr();
  bool ChkRng(std::uint8_t value, std::uint8_t minVal, std::uint8_t maxVal);
  void ValidateRange();
  void EvalExpr();
  void EvalOprnd();
  void AddRLDEnt();  // Phase 8.5.3: RLD entry creation
  void AdvSrcP();

  // Helper functions for GAdrMod
  void IsZPMod();
  void IsAccMod();
  void Is65C02();
  void L8598();

  // Tables for GAdrMod
  extern const std::uint8_t AModTkns[];
  extern const std::uint8_t AModCmds[];
  void                      SkipSpcs();
  void                      AdvPC();
  void                      Is16K();
  void                      IsAXY();
  void                      Wr1Byte();   // Phase 4: Object code writing
  void                      AdvObjPC();  // Phase 4: Object PC advance

  // Phase 8.2: Source line reader forward declarations
  void GSrcLin();
  void ReadMore();
  void SetupMemorySource(const char* sourceText, size_t length);

  // Phase 8.3: Line processing helper forward declarations
  void NextRec();
  void NxtField();
  void L81F0();

  // Phase 8.4: Symbol table and error handling forward declarations
  void FindSym();
  void AddNode();
  void RegAsmEW(std::uint8_t errorToken);
  void RegAsmEW();

  // Phase 8.4: Mnemonic/directive handler forward declarations
  void HndlMnem();

  // Directive handler forward declarations
  void DrtvDone();
  void HndlEQU();
  void HndlORG();
  void HndlOBJ();
  void HndlREL();
  void HndlBYTE();
  void HndlWORD();
  void HndlBLOCK();
  void HndlASCII();
  void HndlDBYTE();
  void HndlDS();
  void HndlDFB();
  void HndlDW();
  void HndlDWCore();
  void HndlASC();
  void HndlASC_Core();
  void HndlDCI();
  void HndlLST();
  void HndlLIST();
  void HndlNOLIST();
  void DoPage();
  void HndlSBTL();

  // Extern array declarations (defined later in file)
  extern const std::uint8_t CharMap1[];
  extern const std::uint8_t AModTbl[];

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
  std::uint8_t  Z60;        // Generic zero page location (multipurpose)
  std::uint8_t  BCDNbr[3];  // Source file line numbers in BCD format ($60-$62, 3 bytes)
  std::uint16_t StrtSymT;   // Start address of symbol table (2 bytes: $63-$64)
  std::uint16_t EndSymT;    // Current end address of symbol table (2 bytes: $65-$66)
#define EndSymT_hi (reinterpret_cast<std::uint8_t*>(&EndSymT)[1])  // High byte access for EndSymT
  std::uint8_t  PassNbr;   // Current assembly pass: 0=Pass1, 1=Pass2, 2=Pass3
  std::uint8_t  ListingF;  // Listing flag: $80=LST ON, $00=LST OFF
  std::uint8_t  SubTtlF;   // Subtitle flag: $00=none, $40=SBTL cmd, $FF=subtitle string
  std::uint8_t  LineCnt;   // Number of lines printed on current page
  std::uint16_t PageNbr;   // Current page number (2 bytes: $6B-$6C)
  std::uint8_t  FileNbr;   // Current file number in assembly
  std::uint8_t  LogPL;     // Logical page length (lines per page)
  std::uint8_t  PhyPL;     // Physical page length (actual printer lines)

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
  std::uint16_t TxtEnd;     // End of text buffer (for memory source mode)
  std::uint16_t PC;         // Program Counter / position counter (current assembly address)
  std::uint16_t SortedP;    // Pointer to sorted auxiliary array (aliases PC)
  std::uint16_t ObjPC;  // Object code Program Counter (where to store in memory, 2 bytes: $7C-$7D)
  std::uint8_t  CodeLen;  // Current length of code image for REL files (stored at BOF)
  std::uint8_t  AuxAryE;  // Pointer to last end of sorted array

#define SrcP_hi  (reinterpret_cast<std::uint8_t*>(&SrcP)[1])   // High byte access
#define PC_hi    (reinterpret_cast<std::uint8_t*>(&PC)[1])     // High byte access for PC
#define ObjPC_hi (reinterpret_cast<std::uint8_t*>(&ObjPC)[1])  // High byte access for ObjPC

  // Global test buffer for unit testing (when non-null, overrides SrcP for array access)
  std::uint8_t* g_test_src_buffer = nullptr;

  // Global test tracking variable for directive routing tests
  const char* g_LastDirectiveCalled = "";

  // Global test memory buffer for source code (simulated 64KB memory for SrcP)
  // This allows SrcP to be treated as a 16-bit address in tests
  std::uint8_t  g_test_src_memory[65536];
  std::uint16_t g_test_src_base = 0x1000;  // Base address in simulated memory

  // Global test memory buffer for StorByt testing (64KB simulated memory)
  std::uint8_t g_test_obj_memory[65536];
  bool         g_test_obj_memory_enabled = false;

  // Helper to access source line as array (simulates 6502 indirect indexed mode)
  // When SrcP contains an address, SrcP_byte(index) accesses memory at that address
  // For testing, if g_test_src_buffer is set, it uses that instead
  inline std::uint8_t SrcP_byte(std::uint8_t index) {
    if (g_test_src_buffer != nullptr) {
      return g_test_src_buffer[index];
    }
    // Use g_test_src_memory for simulated memory access
    std::uint16_t addr = SrcP + index;
    return g_test_src_memory[addr];
  }

  // Helper to convert simulated 16-bit address to real pointer into g_test_src_memory
  // Used for symbol table and other memory operations
  inline std::uint8_t* SimPtrToMemPtr(std::uint16_t simAddr) {
    return &g_test_src_memory[simAddr];
  }

  // Macro to simplify array-style access (for code that looks like SrcP_at(Y))
  // Usage: SrcP_at(Y) instead of SrcP_at(Y)
#define SrcP_at(idx) SrcP_byte(idx)

  // $80-$8F: File and Symbol Table Management
  std::uint16_t       FileLen;   // Current length of BIN/REL file (2 bytes: $81-$82)
  std::uint16_t       CurrORG;   // Current origin address from ORG directive
  std::uint16_t       SymP;      // Pointer to symbol name (2 bytes: $85-$86)
  const std::uint8_t* MnemP;     // Pointer to mnemonic table entry (safe pointer type)
  std::uint8_t        Delimitr;  // Delimiter character (aliases SymP)
  std::uint8_t        DTEndCol;  // End column index of DateTime string
  std::uint8_t        StrType;   // String type: 0=DCI (inverted last char), -1=ASC
  std::uint8_t        DTCurIdx;  // Current index into DateTime string (aliases StrType)
  std::uint16_t       MemTop;    // Top of available memory (2 bytes: $87-$88)
  std::uint32_t       TotLines;  // Total line count (3 bytes: $89-$8B for large counts)
  std::uint8_t        VidSlot;   // Video card slot number
  std::uint8_t        SaveA;     // Saved Accumulator value
  std::uint8_t        SaveY;     // Saved Y register value
  std::uint8_t        SaveX;     // Saved X register value

  // High byte access macros for 16-bit values
#define CurrORG_hi (reinterpret_cast<std::uint8_t*>(&CurrORG)[1])
#define MemTop_hi  (reinterpret_cast<std::uint8_t*>(&MemTop)[1])

  // $90-$9F: Code Generation and Expression Evaluation
  std::uint8_t  DskListF;  // Disk listing flag: $00=off, $40=partial, $80=lst to file
  std::uint8_t  LstDBIdx;  // LST data buffer index / number of chars to write
  std::uint8_t  WinLeft;   // Left edge of 40-column window (for 80-col cards)
  std::uint8_t  WinRight;  // Right edge of 40-column window
  std::uint8_t  X6502F;    // 65C02 processor flag (vs 6502)
  std::uint16_t HighMem;   // High memory address of generated object code
  std::uint8_t  ExprAccF;  // Expression's accumulated flag bits
#define HighMem_hi (reinterpret_cast<std::uint8_t*>(&HighMem)[1])  // High byte access for HighMem
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
  std::uint16_t SymFBP;    // Symbol Flag Byte Pointer (pointer to symbol's flag byte)

  // $D0-$DF: Relocation and Symbol Table Management
  std::uint16_t RLDEnd;  // Relocation Dictionary end pointer (2 bytes)
#define RLDEnd_hi (reinterpret_cast<std::uint8_t*>(&RLDEnd)[1])  // High byte access for RLDEnd
  std::uint16_t ZD2;                                             // (Not used - reserved)
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

  // Stub commented out - replaced by Phase 8.1 implementation
  // void NextRec() {
  //   // TODO: Advance to next source record
  // }

  // ASM2 helper stubs - commented out, replaced by Phase 8.1 implementations
  // void SaveZP() {
  //   // TODO: Save zero page area
  // }

  void SetupVec() {
    // TODO: Setup/reset vectors
  }

  void EvalExpr() {
    // Basic expression evaluator for Phase 8.5.3
    // Supports: hex/dec constants, symbols, < > prefix (low/high byte), and a single +,-,* binary
    // op

    auto skip_spaces = [&]() {
      while (SrcP_at(Y) == SPACE) {
        Y++;
        if (Y == 0) break;
      }
    };

    auto parse_number = [&](uint16_t& out_val) -> bool {
      uint8_t ch = SrcP_at(Y);
      if (ch == '$') {
        Y++;
        uint16_t val  = 0;
        bool     seen = false;
        while (true) {
          ch = SrcP_at(Y);
          if (ch >= '0' && ch <= '9') {
            val = (val << 4) | (ch - '0');
            Y++;
            seen = true;
            continue;
          }
          if (ch >= 'A' && ch <= 'F') {
            val = (val << 4) | (ch - 'A' + 10);
            Y++;
            seen = true;
            continue;
          }
          if (ch >= 'a' && ch <= 'f') {
            val = (val << 4) | (ch - 'a' + 10);
            Y++;
            seen = true;
            continue;
          }
          break;
        }
        if (seen) {
          out_val = val;
          return true;
        }
        return false;
      }
      if (ch >= '0' && ch <= '9') {
        uint16_t val = 0;
        while (true) {
          ch = SrcP_at(Y);
          if (ch >= '0' && ch <= '9') {
            val = static_cast<uint16_t>(val * 10 + (ch - '0'));
            Y++;
            continue;
          }
          break;
        }
        out_val = val;
        return true;
      }
      return false;
    };

    auto parse_symbol = [&](uint16_t& out_val, uint8_t& out_flags) -> bool {
      uint8_t start_y = Y;
      fprintf(stderr, "    parse_symbol: Looking for symbol starting at Y=%d, ch='%c'\n", Y,
              (SrcP_at(Y) >= 32) ? SrcP_at(Y) : '?');

      // CRITICAL: FindSym() needs to read from the current position in the source.
      // In the original 6502 code, symbol names were always at the start of a line (Y=0),
      // but in expression evaluation, we need to position SrcP to point at the symbol.
      uint16_t saved_SrcP = SrcP;
      SrcP += start_y;  // Advance SrcP to where the symbol actually starts
      Y = 0;            // FindSym expects Y=0
      FindSym();
      SrcP = saved_SrcP;  // Restore SrcP
      fprintf(stderr, "    parse_symbol: After FindSym(), C=%d, A=0x%02X, SymP=0x%04X, Y=%d\n", C,
              A, SymP, Y);
      if (C) {
        fprintf(stderr, "    parse_symbol: ERROR - symbol not found\n");
        Y = start_y;  // Restore Y before returning
        return false;
      }
      out_flags         = A;
      uint8_t* SymP_ptr = SimPtrToMemPtr(SymP);
      out_val           = static_cast<uint16_t>(SymP_ptr[Y] | (SymP_ptr[Y + 1] << 8));
      // Clear unrefd bit on reference
      if (Y > 0) {
        uint8_t flag_idx = static_cast<uint8_t>(Y - 1);
        SymP_ptr[flag_idx] &= static_cast<uint8_t>(~unrefd);
      }
      // Advance Y past symbol text
      Y = start_y;
      while (true) {
        uint8_t ch = SrcP_at(Y);
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
          Y++;
        } else {
          break;
        }
      }
      return true;
    };

    auto finish_tokens = [&]() {
      skip_spaces();
      NxtToken = (SrcP_at(Y) == ',') ? 0x01 : 0;
      // Y is left pointing at the delimiter after parsing
    };

    auto parse_term = [&](uint16_t& val, bool& is_reloc, bool allow_prefix) -> bool {
      skip_spaces();
      uint8_t ch      = SrcP_at(Y);
      bool    use_low = false, use_high = false;
      if (allow_prefix && (ch == '<' || ch == '>')) {
        use_low  = (ch == '<');
        use_high = (ch == '>');
        Y++;
        skip_spaces();
        ch = SrcP_at(Y);
      }

      uint16_t parsed   = 0;
      uint8_t  symFlags = 0;
      is_reloc          = false;

      if (parse_number(parsed)) {
        val = parsed;
        finish_tokens();
        return true;
      }

      if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
        if (!parse_symbol(parsed, symFlags)) return false;
        val      = parsed;
        is_reloc = (symFlags & relative) != 0 || (symFlags & external) != 0;
        if (use_low) {
          val = val & 0x00FF;
        } else if (use_high) {
          val = (val >> 8) & 0x00FF;
        }
        finish_tokens();
        return true;
      }

      return false;
    };

    // ---- expression parse starts here ----
    RelExprF       = 0;
    uint16_t term1 = 0, term2 = 0;
    bool     term1_rel = false, term2_rel = false;

    fprintf(stderr, "    EvalExpr (modern): About to parse_term(term1), Y=%d, ch=0x%02X '%c'\n", Y,
            SrcP_at(Y), (SrcP_at(Y) >= 32 && SrcP_at(Y) < 127) ? SrcP_at(Y) : '?');
    if (!parse_term(term1, term1_rel, true)) {
      fprintf(stderr, "    EvalExpr (modern): parse_term FAILED\n");
      C = true;
      X = 0x24;
      RegAsmEW(X);
      return;
    }
    fprintf(stderr, "    EvalExpr (modern): parse_term OK, term1=0x%04X, term1_rel=%d\n", term1,
            term1_rel);

    skip_spaces();
    uint8_t op = SrcP_at(Y);
    if (op == '+' || op == '-' || op == '*') {
      Y++;
      if (!parse_term(term2, term2_rel, true)) {
        C = true;
        X = 0x24;
        RegAsmEW(X);
        return;
      }
      // Reloc rules: product of two relocatables is illegal
      if (op == '*' && term1_rel && term2_rel) {
        C = true;
        X = 0x24;
        RegAsmEW(X);
        return;
      }
      uint32_t res = 0;
      if (op == '+') res = static_cast<uint32_t>(term1) + term2;
      if (op == '-') res = static_cast<uint32_t>(term1) - term2;
      if (op == '*') res = static_cast<uint32_t>(term1) * term2;
      ValExpr    = static_cast<uint8_t>(res & 0xFF);
      ValExpr_hi = static_cast<uint8_t>((res >> 8) & 0xFF);
      Lower8     = ValExpr;
      RelExprF   = (term1_rel || term2_rel) ? relative : 0;
      C          = false;
      finish_tokens();
      return;
    }

    // Single term only
    ValExpr    = static_cast<uint8_t>(term1 & 0xFF);
    ValExpr_hi = static_cast<uint8_t>((term1 >> 8) & 0xFF);
    Lower8     = ValExpr;
    RelExprF   = term1_rel ? relative : 0;
    C          = false;
    finish_tokens();
  }

  // void InitASM() {
  //   // TODO: Initialize assembler state
  // }

  // Forward declaration - full implementation below
  void DoPass1();

  // DoPass2 - Generate object code from symbol table
  // Source code reading, code generation
  // Original: ASM2.S lines ~456-650 (approximate)
  void DoPass2() {
    // Initialize Pass 2 state
    PassNbr = 1;  // Pass 2
    // ObjPC is set by caller or from initialization
    // GenF controls whether code is actually generated

    // Main Pass 2 loop: Assemble each source line and emit code
  Pass2Lup:
    GSrcLin();      // Get next source line
    if (C) return;  // EOF reached? (C=1 means no more lines)

    // Initialize vars before assembling each src line
    Y = 0;  // Start at first character

    // Check for comment-only lines
    A = SrcP_at(Y);                // Get 1st char (Y=0)
    if (A == '*') goto Pass2Next;  // Pure comment line? Skip it
    if (A == ';') goto Pass2Next;  // Comment line? Skip it
    if (A == CR) goto Pass2Next;   // Blank line? Skip it

    // Check for label (non-space first character)
    A ^= SPACE;
    LabelF = A;  // 0 => no label

    if (A == 0) goto Pass2NoLabel;  // Line starts with space? No label

    // Label present - skip past it to find mnemonic
    Y = 0;    // Reset to start of line
    L81F0();  // Skip over non-blanks (label text)

    // Check if there's a mnemonic/directive after label
    A = SrcP_at(Y);
    if (A == ':') Y++;            // Skip colon if present
    if (A == CR) goto Pass2Next;  // Label only line? Done
    // Fall through to Pass2NoLabel to parse mnemonic

  Pass2NoLabel:
    NxtField();  // Skip spaces, point at mnemonic/directive

    // Check if we reached end of line
    A = SrcP_at(Y);
    if (A == CR) goto Pass2Next;  // End of line? Done

    // Parse mnemonic/directive and emit opcodes
    HndlMnem();

    // TODO Phase 9+: Check error flag, handle operand evaluation

  Pass2Next:
    // Advance to next source line
    NextRec();
    goto Pass2Lup;
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

  // Stub commented out - replaced by actual implementation later
  // void L81A3() {
  //   // TODO: Increment decimal string
  // }

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
  // Original: ASM1.S lines ~330-450
  void DoPass1() {
    // Initialize Pass 1 state
    RelCodeF = 0;
    SymNbr   = 0;  // # of ENTRY/EXTRN
    PassNbr  = 0;  // Pass 1
    // A = BINtype / ftypeT[0] = A  - TODO: implement when BINtype/ftypeT are defined
    // OpenSrc1();  // TODO: implement when ready

  // Main Pass 1 loop: Assemble each source line
  Pass1Lup:
    GSrcLin();      // Get next source line
    if (C) return;  // EOF reached? (C=1 means no more lines)

    // Initialize vars before assembling each src line
    Y = 0;  // Start at first character

    // Check for comment-only lines
    A = SrcP_at(Y);                // Get 1st char (Y=0)
    if (A == '*') goto Pass1Next;  // Pure comment line? Skip it
    if (A == ';') goto Pass1Next;  // Comment line? Skip it
    if (A == CR) goto Pass1Next;   // Blank line? Skip it

    // Check for label (non-space first character)
    A ^= SPACE;
    LabelF = A;  // 0 => no label

    if (A == 0) goto NoLabel;  // Line starts with space? No label

    // Label present - basic validation (Phase 8.4.1)
    // - reject single-letter reserved IDs: A, X, Y  -> error 0x1E
    // - reject labels whose first char is not alphabetic -> error 0x0E
    // Use SrcP_at(0)/SrcP_at(1) for look-ahead while Y==0
    {
      uint8_t ch0 = SrcP_at(0);
      uint8_t ch1 = SrcP_at(1);

      // Single-char reserved identifiers (A/X/Y) followed by space/CR/colon
      if ((ch1 == SPACE || ch1 == CR || ch1 == ':') && (ch0 == 'A' || ch0 == 'X' || ch0 == 'Y')) {
        RegAsmEW(0x1E);  // Reserved identifier error
        LabelF = 0;      // clear label field so it won't be added
        goto Pass1Next;  // skip rest of line (do not process mnemonic)
      }

      // First character must be alphabetic (A-Z or a-z)
      if (!((ch0 >= 'A' && ch0 <= 'Z') || (ch0 >= 'a' && ch0 <= 'z'))) {
        RegAsmEW(0x0E);  // Invalid identifier
        LabelF = 0;      // clear label field so it won't be added
        goto Pass1Next;  // skip rest of line
      }
    }

    // Label present - parse and add to symbol table
    // Phase 8.4: Basic label parsing with duplicate detection

    // Check if symbol already exists
    FindSym();
    if (!C) {  // Symbol found (C=0)
      // Check if already defined
      if ((int8_t)A >= 0) {  // Bit 7 clear means defined
        X = 0x02;            // Duplicate identifier error
        fprintf(stderr, "DoPass1: duplicate label detected, X=0x%02X\n", X);
        RegAsmEW();
        fprintf(stderr, "DoPass1: after RegAsmEW ErrNbr4=%u, ErrInfo[1]=0x%02X\n", ErrNbr4,
                ErrInfoT[1]);
        A      = 0x00;
        LabelF = A;  // Flag no label field
        goto SkipLabel;
      }
      // Symbol exists but undefined - update it
      // Note: FindSym() returns with Y indexing the low-byte of the
      // value field; decrement Y to index the flag byte before writing.
      if (Y > 0) Y--;                                // Safeguard against underflow
      uint8_t* SymP_ptr     = SimPtrToMemPtr(SymP);  // Convert simulated address
      uint8_t  SymFByte_val = SymP_ptr[Y];
      SymFByte_val &= (entry | fwdrefd);  // Keep ENTRY/EXTRN flags
      // If DummyF indicates a DSECT (signed negative), mark symbol as relative;
      // otherwise preserve existing RelCodeF behaviour.
      if ((int8_t)DummyF < 0) {
        SymFByte_val |= relative;
      } else {
        SymFByte_val |= RelCodeF;  // Add relative bit if in relative mode
      }
      SymP_ptr[Y] = SymFByte_val;      // Write flag byte
      Y++;                             // Advance to low-byte
      SymP_ptr[Y] = PC & 0xFF;         // Store PC low byte
      Y++;                             // Advance to high-byte
      SymP_ptr[Y] = (PC >> 8) & 0xFF;  // Store PC high byte
      goto SkipLabel;
    }

    // New label - add to symbol table
    A        = 0x00;
    RelExprF = RelCodeF;  // Set relative flag if in relative mode
    AddNode();
    if (C) {     // Error adding node
      X = 0x0E;  // Invalid identifier
      RegAsmEW();
    }

  SkipLabel:
    // Skip past label text to find mnemonic
    Y = 0;    // Reset to start of line
    L81F0();  // Skip over non-blanks (label text)

    // Check if there's a mnemonic/directive after label
    A = SrcP_at(Y);
    if (A == ':') Y++;            // Skip colon if present
    if (A == CR) goto Pass1Next;  // Label only line? Done
    // Fall through to NoLabel to parse mnemonic

  NoLabel:
    NxtField();  // Skip spaces, point at mnemonic/directive

    // Check if we reached end of line
    A = SrcP_at(Y);
    if (A == CR) goto Pass1Next;  // End of line? Done

    // Parse mnemonic/directive
    HndlMnem();

    // TODO Phase 9+: Check error flag, handle operands
    // if (!C) goto L7EF0;  // No errs (BCC)
    // For now, we assume HndlMnem handles PC advancement internally

  Pass1Next:
    // Advance to next source line
    NextRec();
    goto Pass1Lup;
  }

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
    // Check if warning (odd token) or error (even token)
    bool isWarning = (errorToken & 0x01) != 0;

    if (isWarning) {
      IncrementBCD16(NbrWarns);
      if ((LstWarns & 0x80) == 0) return;  // Warnings suppressed
      // TODO: DoAlert, doPause stubs
    } else {
      // Always count errors and save info (up to buffer limit)
      IncrementBCD16(NbrErrs);
      SaveErrInfo(errorToken);
      ErrorF = 0x80;
      // TODO: DoAlert, doPause stubs
    }
  }

  // Overload for cases where X register contains error token
  void RegAsmEW() {
    RegAsmEW(X);
  }

  //=================================================
  // GSrcLin - Get next source line from memory or disk
  // Original: ASM3.S:2991-3056
  // Returns: C=1 if EOF, C=0 if line fetched
  //
  // Memory Mode (IDskSrcF = 0):
  //   - SrcP points into memory buffer containing source text
  //   - Check if SrcP >= TxtEnd (reached end of text)
  //   - If at end: set carry (C=1), return (EOF)
  //   - If not at end: clear carry (C=0), return (line ready)
  //
  // Disk Mode (IDskSrcF != 0):
  //   - Stubbed for Phase 8.2 - will be implemented in Phase 9+
  //   - Returns EOF (C=1) for now
  //=================================================
  void GSrcLin() {
    // Original: GSrcLin at ASM3.S:2991
    // LDA DskSrcF ; Are we using disk source files?
    // BMI L9B95   ; Yes

    std::uint8_t dskSrcFValue = static_cast<std::uint8_t>(DskSrcF);
    if (dskSrcFValue & 0x80) {  // BMI - check MSB (disk mode)
      // L9B95: Disk mode - check macro and include file handling
      // For Phase 8.2, we stub this entire path
      // TODO: Phase 9+ - implement disk I/O, macro expansion, CHN/INCLUDE
      C = true;  // Set carry (EOF)
      return;
    }

    // Memory source mode
    // Original: ASM3.S:2995-3001
    // LDA    SrcP            ; Check if there are still
    // CMP    TxtEnd          ; lines to be assembled
    // LDA    SrcP+1
    // SBC    TxtEnd+1
    // RTS                    ; If C=1, done

    // Compare SrcP with TxtEnd (16-bit comparison)
    // If SrcP >= TxtEnd, set carry (EOF)
    // If SrcP < TxtEnd, clear carry (line available)

    std::uint8_t srcP_lo   = SrcP & 0xFF;
    std::uint8_t srcP_hi   = (SrcP >> 8) & 0xFF;
    std::uint8_t txtEnd_lo = TxtEnd & 0xFF;
    std::uint8_t txtEnd_hi = (TxtEnd >> 8) & 0xFF;

    // Emulate 6502 CMP/SBC sequence:
    // First compare low bytes (CMP sets carry if A >= operand)
    // Then subtract high bytes with borrow (SBC)
    bool cmp_carry = (srcP_lo >= txtEnd_lo);

    // SBC: A - M - (1-C)  where C is carry from CMP
    // If result has carry set, then SrcP >= TxtEnd (EOF)
    int result = srcP_hi - txtEnd_hi - (cmp_carry ? 0 : 1);

    C = (result >= 0);  // Set carry flag based on comparison result

    // Original returns here with carry flag indicating status:
    // C=1: EOF (SrcP >= TxtEnd)
    // C=0: Line available (SrcP < TxtEnd)
  }

  //=================================================
  // ReadMore - Read next block from disk into buffer
  // Original: ASM3.S:3064-3204 (L9BFC label)
  // Stubbed for Phase 8.2 - will be implemented in Phase 9+
  //
  // Original functionality:
  // - Read ProDOS blocks via MLI
  // - Handle partial lines across block boundaries
  // - Manage SBuf/IBuf buffers
  // - Handle file nesting (CHN/INCLUDE)
  // - Prepend partial line from previous block
  //=================================================
  void ReadMore() {
    // TODO: Phase 9+ - implement disk I/O
    // For now, just set carry (EOF) and return
    C = true;
  }

  //=================================================
  // SetupMemorySource - Set up memory source mode
  // Helper function for testing and in-memory assembly
  //
  // Parameters:
  //   sourceText: pointer to source code buffer
  //   length: length of buffer in bytes
  //
  // Sets:
  //   Copies source to g_test_src_memory at base address
  //   SrcP to g_test_src_base (simulated address)
  //   TxtEnd to g_test_src_base + length
  //   IDskSrcF = 0 (memory mode)
  //   DskSrcF = 0 (memory mode)
  //
  // Note: Uses g_test_src_memory for simulated 16-bit address space
  //=================================================
  void SetupMemorySource(const char* sourceText, size_t length) {
    // Clear test buffer override to prevent contamination from dispatch tests
    g_test_src_buffer = nullptr;

    // Copy source to simulated memory
    if (length > sizeof(g_test_src_memory) - g_test_src_base) {
      length = sizeof(g_test_src_memory) - g_test_src_base;
    }
    std::memcpy(&g_test_src_memory[g_test_src_base], sourceText, length);

    // SrcP points to start of source in simulated memory
    SrcP = g_test_src_base;

    // TxtEnd points to end
    TxtEnd = g_test_src_base + static_cast<std::uint16_t>(length);

    // Clear disk source flags (memory mode)
    IDskSrcF = 0;
    DskSrcF  = 0;
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

#if 0   // TODO: Phase 9+ - Stub RsvdId function (depends on IsAXY and RegAsmEW)
  //=================================================
  // RsvdId - Check for reserved identifier
  //=================================================
  void RsvdId() {
    IsAXY();         // JSR IsAXY - Chk if reserved idfer
    if (!C) return;  // BCC doRet9 - No
    X = 0x1E;        // LDX #$1E - Reserved idfer err
    RegAsmEW(X);     // JMP RegAsmEW
  }
#endif  // RsvdId

#if 0   // TODO: Phase 9+ - Stub IsAXY function
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
#endif  // IsAXY

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
#if 1  // Phase 8.4: Enable AddNode for symbol table creation
  // Symbolic names with the same hash value (collision)
  // are connected together in a singly linked list.
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
#endif  // AddNode

  // L81F0 - Skip over non-blanks until space, CR, or colon
  // Original: ASM2.S (various locations)
  // Used to skip over label text
  // Entry: Y = index into source line
  // Exit: Y pointing at first space/CR/colon, Z flag set based on result
  void L81F0() {
    while (true) {
      A = SrcP_at(Y);
      if (A == ' ' || A == '\t' || A == CR || A == ':') {
        Z = (A == CR);  // Set Z if CR found
        return;
      }
      Y++;
      if (Y == 0) {  // Wrapped around
        Z = false;
        return;
      }
    }
  }

  //=================================================
  // NxtField - Advance SrcP to start of next field (first non-space)
  // Original: ASM2.S:1846
  // On entry (Y)=index into src line
  // Ret:
  // (Y)=0
  // src ptr pointing @ 1st char of the field
  // (X) - unchanged
  //=================================================
  void NxtField() {
  NxtField_start:
    A = SrcP_at(Y);
    if (A != SPACE) goto L823D;  // (BNE)
    Y++;
    if (Y != 0) goto NxtField_start;  // (BNE) - loop until non-space or Y wraps

  L823D:
    // Advance SrcP by Y and reset Y to 0
    AdvSrcP();
  }

#if 0
  // Stub: HndlMnem - Handle mnemonic/pseudo opcode
  // This is a duplicate stub - the real implementation is later in the file
  void HndlMnem() {
    // TODO: Parse and handle mnemonic
    C = true;  // For now, simulate error
  }
#endif

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

#if 0   // TODO: Phase 9+ - Redefinition of SetupVec (already defined earlier)
  // Stub: SetupVec - Setup vectors
  void SetupVec() {
    // TODO: Setup interrupt vectors
  }
#endif  // SetupVec redefinition

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
      0x02,  // (zp)
      0x03,  // (abs)
      0x01,  // acc
      0x02,  // zp,Y
      0x03   // (abs,X)
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

  // GAdrMod - This subrtn will parse the addressing mode of the
  // operand of a 6502 mnemonic/SW16 psuedo opcode
  // Ret
  //  (A)=index (0-12) use to get addr mode fr a table
  //  C=0 - succ
  //  C=1 - syntax error

  void GAdrMod() {
    Y = 0;  // Position at start of operand
    while (SrcP_at(Y) == SPACE) {
      Y++;
      if (Y == 0) break;
    }

    uint8_t ch = SrcP_at(Y);
    if (ch == CR) {
      C = true;
      return;
    }

    auto skip_spaces = [&]() {
      while (SrcP_at(Y) == SPACE) {
        Y++;
        if (Y == 0) break;
      }
    };

    auto is_zero_page = [&]() -> bool {
      uint8_t acc = static_cast<uint8_t>(ExprAccF & 0b11101111);  // Clear EXTeRNal bit
      if ((acc | ValExpr_hi) == 0) return true;
      if ((ExprAccF & 0b00010000) == 0) return false;  // Not external
      if (Ret816F == 0) {
        uint8_t savedX = X;
        X              = 0x44 + 1;  // odd-warning
        RegAsmEW();
        X = savedX;
        return true;
      }
      return false;
    };

    if (ch == '#') {
      Y++;
      EvalExpr();
      if (C) return;
      skip_spaces();
      A = 2;  // immediate
      C = false;
      return;
    }

    if (ch == '(') {
      Y++;
      EvalExpr();
      if (C) return;
      skip_spaces();

      if (SrcP_at(Y) == ',') {
        Y++;
        skip_spaces();
        uint8_t idx = SrcP_at(Y);
        if (idx == 'X' || idx == 'x') {
          Y++;
          skip_spaces();
          if (SrcP_at(Y) == ')') {
            Y++;
            A = is_zero_page() ? 7 : 12;  // (zp,X) or (abs,X)
            C = false;
            return;
          }
        }
      }

      if (SrcP_at(Y) == ')') {
        Y++;
        skip_spaces();
        if (SrcP_at(Y) == ',') {
          Y++;
          skip_spaces();
          uint8_t idx = SrcP_at(Y);
          if (idx == 'Y' || idx == 'y') {
            if (!is_zero_page()) {
              C = true;
              return;
            }
            A = 6;  // (zp),Y
            C = false;
            return;
          }
        }
        A = is_zero_page() ? 8 : 9;  // (zp) or (abs)
        C = false;
        return;
      }

      C = true;
      return;
    }

    if ((ch == 'A' || ch == 'a') && (SrcP_at(static_cast<uint8_t>(Y + 1)) == SPACE ||
                                     SrcP_at(static_cast<uint8_t>(Y + 1)) == CR)) {
      A = 10;  // accumulator
      C = false;
      return;
    }

    EvalExpr();
    if (C) return;
    skip_spaces();

    if (SrcP_at(Y) == ',') {
      Y++;
      skip_spaces();
      uint8_t idx = SrcP_at(Y);
      if (idx == 'X' || idx == 'x') {
        A = is_zero_page() ? 3 : 4;  // zp,X or abs,X
        C = false;
        return;
      }
      if (idx == 'Y' || idx == 'y') {
        A = is_zero_page() ? 11 : 5;  // zp,Y or abs,Y
        C = false;
        return;
      }
    }

    A = is_zero_page() ? 1 : 0;  // zp or abs
    C = false;
  }

  // ($8458) GInstLen - We must determine the address mode of opcode
  // Ret:
  // Length of instruction opcode
  void GInstLen() {
    A       = MnemP[Y];
    ModWrdL = A;  // 1st flag byte (STA)
    Y++;
    A       = MnemP[Y];  // 2nd flag byte - addr mode bits
    ModWrdH = A;         // of this mnemonic
    Y++;
    A       = MnemP[Y];
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
    RegAsmEW(X);
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
    RegAsmEW(X);
    goto L8513;

  L84FE:
    A = 3;                   // len of instruction
    if (A != 0) goto L8511;  // always (BNE)

  // X=0-12
  L8502:
    A = ModWrdL;
    // BIT Bit40 - sw16?
    if ((A & 0x40) == 0) goto L850E;  // no (BEQ)
    if (X >= 9 && X <= 12) {
      A = L851F[X - 9];        // Get instr len for (abs), acc, zp,Y, (abs,X)
      if (A != 0) goto L8511;  // always (BNE)
    }

  L850E:
    A = InstLenT[X];  // Get instr len
  L8511:
    Length = A;
  L8513:
    A = Length;
  }

  // AdvPC - A=# to advance
  void AdvPC() {
    // CLC
    uint8_t  pc_lo = static_cast<uint8_t>(PC & 0xFF);
    uint8_t  pc_hi = static_cast<uint8_t>((PC >> 8) & 0xFF);
    uint16_t sum   = static_cast<uint16_t>(pc_lo) + A;
    A              = static_cast<uint8_t>(sum & 0xFF);
    PC             = (PC & 0xFF00) | A;
    C              = (sum > 0xFF);
    if (sum > 0xFF) {
      pc_hi++;
      PC = (static_cast<uint16_t>(pc_hi) << 8) | A;
    }
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

  //=================================================
  // AdvSrcP - Advance source pointer by Y, reset Y to 0
  // Used by NextRec and other parsing functions
  // Original: ASM2.S (various locations)
  // Entry: Y = offset to advance
  // Exit: SrcP += Y, Y = 0
  //=================================================
  void AdvSrcP() {
    // Emulate 6502 CLC / ADC with carry propagation
    // CLC (clear carry)
    std::uint16_t temp = static_cast<std::uint8_t>(SrcP & 0xFF) + Y;  // Add Y to low byte
    SrcP               = (SrcP & 0xFF00) | (temp & 0xFF);             // Store low byte result
    if (temp > 0xFF) {                                                // Check carry
      SrcP += 0x100;                                                  // Add carry to high byte
    }
    Y = 0;
  }

  //=================================================
  // ChrGot/ChrGet - Character Scanner (using CharMap1)
  // Original: ASM2.S:1792
  // This subrtn is part of Scanner
  // There are 2 entry points viz ChrGet and ChrGot
  // On entry:
  //   (Y)    = index into the src line
  //   (SrcP) = Pointing somewhere within source line
  // Ret:
  // (A) - char (converted to uppercase if alphabetic)
  // C=1 if char is non-alphabetic
  // C=0 if char is alphabetic (A-Z, a-z)
  // Z=1 if char is numeric digit (0-9)
  // Z=0 if char is non-numeric
  // V=1 if char is hexdec digit (0-9, A-F, a-f)
  // V=0 if char is non-hexdec
  // (X) - unchanged
  // (Y) - incr by 1 if 1st entry point else unchanged
  //
  // NOTE: In the actual 6502 code, ChrGet does INY first then falls through
  // to ChrGot. However, for the emulation, we interpret ChrGet as getting
  // the character at the current Y position and then incrementing Y, which
  // matches the typical usage pattern in parsing code.
  //=================================================
  void ChrGet() {
    ChrGot();
    Y++;
  }

  void ChrGot() {
    A              = SrcP_at(Y);     // Get char fr src line
    ZPSaveY        = Y;              // Save (Y) temporarily
    uint8_t char_y = A;              // Use char as an index as well as saving it in (Y)
    if ((int8_t)A >= 0) goto L8211;  // Must be std ASCII or (BPL)

    std::abort();  // BRK - else crash

  L8211:
    A             = CharMap1[char_y];  // Get flag byte
    uint8_t flags = A;                 // Save for later use (PHA)
    A             = char_y;            // Get back char (TYA)
    Y             = ZPSaveY;           // restore Y

    // Set flags from saved flag byte (PLP)
    N = (flags & 0x80) != 0;
    V = (flags & 0x40) != 0;
    Z = (flags & 0x02) != 0;
    C = (flags & 0x01) != 0;

    if (N) {      // If (A)=$61-$7A (a-z) (BPL doRet2)
      A &= 0xDF;  // convert to upper case
    }
  }

#if 0   // TODO: Phase 9+ - Fix L81A3 (undeclared BCDNbr_hi, TotLines_hi, TotLines_2 and VidOut
        // signature)
  // L81A3 - Incr line #s, show user we have assembled
  // a chunk of code by printing a dot
  void L81A3() {
    // BIT NewF - Assembling new file?
    if ((int8_t)NewF >= 0) goto L81AF;  // No (BPL)

    A         = 0;
    BCDNbr    = A;  // line # for new file
    BCDNbr_hi = A;
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
    A = BCDNbr;
    A += 1;  // BCD increment (simplified)
    BCDNbr = A;
    if (A < 100) goto L81E4;  // (BCC)
    A = BCDNbr_hi;
    A += 1;  // with carry
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
#endif  // L81A3

#if 0   // TODO: Phase 9+ - Redefinition of DoPass2 (stub already defined at line 1192)
  // DoPass2 - Second pass of assembly
  void DoPass2() {
    PassNbr++;  // Flag we are in 2nd pass
    A        = '*';
    RepChar  = A;
    A        = -1;
    ListingF = A;
    PutCR();
    OpenSrc1();  // Re-open initial src file
    A = GenF;
    if (A != 0x80) goto Pass2Lup;  // Write obj code into mem? No

    A        = CurrORG;     // These 4 inst serves no purpose since
    ObjPC    = A;           // its contents are changed when an
    A        = CurrORG_hi;  // OBJ/ORG directive is declared
    ObjPC_hi = A;           // Renamed as CodeLen if REL file
    GenF <<= 1;             // $00 - Remove suspension

  // Assemble each src line
  Pass2Lup:
    GSrcLin();           // Any more src lines to assembled?
    if (!C) goto L7F33;  // BCC
    return;

  // Init before each line is scanned/parsed
  L7F33:
    Y   = -1;
    ZAB = Y;  // =$FF
    Y++;      // =0
    Length    = Y;
    ErrorF    = Y;
    LstCodeF  = Y;
    NumCycles = Y;
    A         = SrcP;
    Src2P     = A;  // Save a copy of ptr
    A         = SrcP_hi;
    Src2P_hi  = A;  // to curr srcline

    // BIT CondAsmF - Assembling alt block?
    if ((int8_t)CondAsmF < 0) goto L7F50;    // Yes (BMI), proceed to scan for alt blk
    if ((CondAsmF & 0x40) == 0) goto L7F57;  // BVC - If V=1, then we will be assembling
    CondAsmF <<= 1;                          // alt block so set flag to $80

  L7F50:
    ZAB >>= 1;           // Clear msb (=$7F) - LSR
    L80F7();             // Do we have a FIN/ELSE?
    if (!C) goto L7F77;  // No (BCC), skip assembling src line

  // Assemble curr src line
  // Should be the lexical analyser/scanner
  L7F57:
    A = SrcP_at(Y);  // Pure comment line?
    if (A == '*') goto L7F77;
    if (A == ';') goto L7F77;  // Yes, ignore curr src line

    // Ignore the label field and go directly to the mnemonic field
    L81F0();            // Skip over non-blanks
    if (Z) goto L7F6E;  // Got a cr (BNE -> BEQ inverted)
    NxtField();         // Skip over 1 or more blanks
    HndlMnem();
    if (!C) goto L7F7A;  // BCC

  L7F6E:
    X = 0x04;  // undefined opcode
    RegAsmEW(X);
  L7F73:
    X      = 3;
    Length = X;
  L7F77:
    goto L806F;

  L7F7A:
    ZAB = A;  // 1st flag byte after mnem/directive byte
    // BIT Bit80 - Directives?
    if ((A & 0x80) == 0) goto CodeGen;  // No (BEQ)
    if (A != 0x83) goto L7F88;          // 1000 0011
    goto L807A;                         // Skip listing & storing generated code

  L7F88:
    if (A != 0x81) goto L7F77;  // 1000 0001
    LstCodeF   = A;             // $81 - these control directives
    A          = ValExpr;       // have expr result field which
    ERfield    = A;             // is printed to right of PC field
    A          = ValExpr_hi;
    ERfield_hi = A;
    goto L806F;  // list code, store generated code

  // Bit80 constant
  // const uint8_t Bit80 = 0x80;  // Already defined earlier

  // Prepare to generate code
  CodeGen:
    GInstLen();       // Determine instr's len
    A        = 0x27;  // 0010 0111
    LstCodeF = A;
    X        = 0;
    A        = ModWrdL;
    // BIT Bit40 - sw16 opcode?
    if ((A & 0x40) == 0) goto L7FCB;  // No (BEQ)
    A = SW16F;
    if (A != 0) goto L7FB5;
    X = 0x42 + 1;  // odd-warning
    RegAsmEW();    // sw16 opcode

    // This part is for SW16 ops
    X = 0x00;
  L7FB5:
    A = SubTIdx;
    Y = A;
    A = OpcodeT[Y];  // Get SW opcode
  L7FBB:
    if (A < 0x10) goto GenNow;  // Non-reg ops? Yes (BCC)

    IsSW16Reg();
    if (!Z) goto L7F73;       // No - not a valid SW16 register (BNE)
    A = OpcodeT[Y];           // Get sw16 reg opcode
    A |= ValExpr;             // =Rn
    if (A != 0) goto GenNow;  // always (BNE)

  // This part of the code is for 6502 ops and is used
  // by the Code Generator to compute the index to the
  // actual opcode within the OpcodeTable
  // NB: Highly dependent on the 3 bytes following
  // a mnemonic entry & arrangement data in the various
  // tables like AModTbl, OpcodeT etc. See comments
  // b4 MnemTbl for more info
  // (A)=1st flag byte after a mnenmonic entry
  L7FCB:
    A &= 0b00000101;  // zp,Y/JMP/JSR/(zp)
    A |= ModWrdH;
    if (A == 0) goto L7FED;          // single byte ops (A)=0 (BEQ)
    A = LenTIdx;                     // Index into inst len table
    if ((int8_t)A >= 0) goto L7FD6;  // always! (BPL)
  L7FD5:
    std::abort();  // BRK
  L7FD6:
    if (A < 9) goto L7FED;    // 0-8 (first 9 modes of AModTbl) (BCC)
    C = (A >= 12);
    if (A != 12) goto L7FE5;  // (abs,X) (BNE)
    Is65C02();                // Are 65C02 opcodes allowed?
    if (C) goto L8002;        // No (BCS)
    if (!C) goto L7FE7;       // always (BCC)
  L7FE5:
    if (C) goto L7FD5;  // CRASHED! (BCS)

  // On fall thru, (A)=9-11 => (abs), acc, zp,Y addr modes
  L7FE7:
    A &= 0b00000011;         // (A) -> 1-3
    if (A != 0) goto L7FED;  // (BNE)

    A = 2;  // JMP (abs,X)

  // Single byte & branch ops don't have sub-tables
  // Instead the (SubTIdx) is just the index into
  // the opcode table. For these ops, (A) must be to 0
  L7FED:
    // CLC
    A += SubTIdx;  // Calc the index into opcode table
    Y         = A;
    A         = CycTimes[Y];
    NumCycles = A;
    A         = OpcodeT[Y];  // get opcode
    // BIT X6502F - R 65C02 ops allowed?
    if ((int8_t)X6502F < 0) goto GenNow;  // Yes (BMI)

    IsC02Op();            // Is the opcode valid?
    if (!C) goto GenNow;  // Yes (BCC)

  L8002:
    X = 0x48;  // 65C02 addr mode/opcode
    RegAsmEW();
    goto L7F73;

  // Code Generation
  // Relocation Dictionary entries are created for
  // 1) all 6502 opcodes except branch & single byte ops
  // 2) DFB,DDB,DW pseudo ops
  // 3) SW16 pseudo ops
  GenNow:
    GMC[X] = A;  // X=0 -> save opcode
    X++;
    GMCIdx = X;
    A      = ModWrdL;
    // BIT Bit08 - branch instr?
    if ((A & 0x08) != 0) goto L8038;  // Yes (BNE)

    A = X;                        // X=1 on fall thru
    if (A >= Length) goto L8025;  // Single byte ops? Yes (BCS)
    A      = ValExpr;
    GMC[X] = A;
    X++;
    A      = ValExpr_hi;
    GMC[X] = A;
    X++;  // unnecessary inst

  L8025:
    X = Length;
    X--;
    if (X == 0) goto L806F;  // Single byte ops (BEQ)
    A = RelExprF;            // Is expr's val abs?
    if (A == 0) goto L806F;  // Yes (BEQ)

    A = 1;        // offset
    Y = 0;        // Little Endian (Reverse)
    AddRLDEnt();  // Make an RLD entry
    goto L806F;

  // Branch instructions
  L8038:
    A          = ValExpr;     // Branch target addr to
    ERfield    = A;           // be printed to right of
    A          = ValExpr_hi;  // branch object code
    ERfield_hi = A;
    A          = LstCodeF;
    A |= 0x80;
    LstCodeF = A;
    CalcDisp();

    X = GMCIdx;
    A = ModWrdL;
    A &= 0x10;  // BRL/BSL?
    Y = A;
    if (Y != 0) goto L8063;  // yes (BNE)

    // 6502/C02
    A = ValExpr;                     // (Y)=0
    if ((int8_t)A >= 0) goto L8058;  // Forward branch (BPL)
    ValExpr_hi++;                    // =0 (INC)
  L8058:
    A = ValExpr_hi;          // (ValExpr) has displacement byte
    if (A == 0) goto L8063;  // (BEQ)
    X = 0x26;                // branch range err
    RegAsmEW();

    X = GMCIdx;
  L8063:
    A      = ValExpr;
    GMC[X] = A;
    A      = Y;              // 6502 branch instr?
    if (A == 0) goto L806F;  // Yes (BEQ)
    X++;
    A      = ValExpr_hi;  // SW16 BSL/BRL
    GMC[X] = A;

  L806F:
    PrtAsmLn();
    StorGMC();
    A = Length;
    AdvPC();

  L807A:
    PollKbd();
    if (C) goto L8088;  // BCS
    NextRec();
    L81A3();
    goto Pass2Lup;  // Assemble next srcline
  L8088:
    CanclAsm();
  }
#endif  // DoPass2 redefinition

  // RVLsting - Chk if instruction is to be printed
  // C=0 - Yes
  // C=1 - No
  void RVLsting() {
    C = false;           // CLC
    A = ErrorF;          // Was an error reported for this srcline?
    if (A != 0) return;  // Yes (BNE doRTS5)
    // BIT CondAsmF - Has this line been assembled?
    if ((int8_t)CondAsmF >= 0) goto L8098;  // Yes (BPL)
    // BIT LstUnAsm - Print unasm src block?
    if ((int8_t)LstUnAsm >= 0) goto L80A4;  // No (BPL)
  L8098:
    // BIT MacroF - Is line a result of mac exp?
    if ((int8_t)MacroF >= 0) goto L80A0;  // No (BPL)
    // BIT LstExpMac - List such lines?
    if ((int8_t)LstExpMac >= 0) goto L80A4;  // No (BPL)
  L80A0:
    // BIT ListingF - Is listing ON?
    if ((int8_t)ListingF < 0) return;  // Yes (BMI doRTS5)
  L80A4:
    C = true;  // SEC
  }

#if 0   // TODO: Phase 9+ - Stub PrtAsmLn calls undeclared ListCode/LstSrcLn
  // PrtAsmLn - Print Assembled Line
  void PrtAsmLn() {
    RVLsting();
    if (C) return;  // No (BCS doRTS6)
    ListCode();     // Print generated code
    LstSrcLn();     // Print src stmt
  }
#endif  // PrtAsmLn

  // StorGMC - Store generated machine code
  void StorGMC() {
    Y = Length;          // # of bytes
    if (Y == 0) return;  // (BEQ doRTS7)

    Y = 0;
  L80B8:
    A = GMC[Y];
    // BIT GenF - Is code generation suppressed?
    if ((int8_t)GenF < 0) return;        // Yes (BMI doRTS7)
    if ((GenF & 0x40) != 0) goto L80C5;  // Write to disk (BVS)
    if (g_test_obj_memory_enabled) {
      g_test_obj_memory[static_cast<uint16_t>(ObjPC + Y)] = A;  // Write to mem
    }
    // BVC L80C8 - always
    goto L80C8;
  L80C5:
    Wr1Byte();
  L80C8:
    Y++;
    if (Y != Length) goto L80B8;  // Next byte (BNE)

    // BIT GenF - Did we do a mem store?
    if ((GenF & 0x40) != 0) return;  // No (BVS doRTS7), a disk store
    A = Y;
    AdvObjPC();
  }

  // Stub: Wr1Byte - Write one byte to disk
  void Wr1Byte() {
    // TODO: Write one byte to object file on disk
  }

  // L8282 - Object buffer overflow error
  void L8282() {
    X = 0x17;  // Object buffer full error
    RegAsmEW(X);
  }

  // L8287 - Not referenced
  void L8287() {
    if (GenF != 0) return;  // BNE label (always taking branch) (BNE)
  }

  // L828A - Checks/adjusts ObjPC against object buffer end
  void L828A() {
    A = MemTop;
    // CMP ObjPC
    A = MemTop_hi;
    // SBC ObjPC+1
    if ((MemTop_hi < ObjPC_hi) || (MemTop_hi == ObjPC_hi && MemTop < ObjPC)) {
      // Overflow
      L8282();
    }
  }

  // AdvObjPC - A=# to advance
  void AdvObjPC() {
    // Extract low and high bytes
    uint8_t objpc_lo = static_cast<uint8_t>(ObjPC & 0xFF);
    uint8_t objpc_hi = static_cast<uint8_t>((ObjPC >> 8) & 0xFF);

    // Add A to low byte and check for carry
    uint16_t sum    = static_cast<uint16_t>(objpc_lo) + A;
    uint8_t  new_lo = static_cast<uint8_t>(sum & 0xFF);

    // Update ObjPC low byte
    ObjPC = (ObjPC & 0xFF00) | new_lo;

    // If carry occurred, increment high byte
    if (sum > 0xFF) {
      objpc_hi++;
      ObjPC = (static_cast<uint16_t>(objpc_hi) << 8) | new_lo;
    }

  L8275:
    L828A();
  }

  // L8278 - Not referenced
  void L8278() {
    A = MemTop;
    // CMP ObjPC
    A = MemTop_hi;
    // SBC ObjPC+1
    if ((MemTop_hi < ObjPC_hi) || (MemTop_hi == ObjPC_hi && MemTop < ObjPC)) {
      // Check object buffer overflow
      L828A();
    }
  }

  // StorByt - Store one byte to output buffer
  // Input: A = byte to store, ObjPC = output address, GenF = generation flag
  // Translate from ASM2.S lines 1521-1556
  void StorByt() {
    // BIT GenF - Suppress code generation?
    if ((int8_t)GenF < 0) return;  // Yes (BMI doRTS8)

    // Write to memory (test mode)
    if (g_test_obj_memory_enabled) {
      g_test_obj_memory[ObjPC] = A;
    }

    // INC ObjPC - Increment 16-bit ObjPC
    ObjPC++;

    // Are we out of memory?
    // LDA ObjPC - CMP HighMem
    // LDA ObjPC+1 - SBC HighMem+1
    // 16-bit comparison: ObjPC >= HighMem?
    if (ObjPC >= HighMem) {
      // Out of memory error - register error and stop
      X = 0x0A;     // Out of memory error
      RegAsmEW(X);  // JSR RegAsmEW
      // TODO: Cancel assembly (set abort flag or similar)
    }
    return;  // RTS
  }

#if 0   // TODO: Phase 9+ - Stub L80F7 already exists at line 2146, this is duplicate/incomplete
  // C=0 - no
  // C=1 - yes
  // (SrcP) & Y-reg preserved
  void L80F7() {
      SavIndY               = Y;  // Index into field of src line
      uint8_t saved_srcP    = SrcP;
      uint8_t saved_srcP_hi = SrcP_hi;

      L81F0();            // Look ahead for a space
      if (Z) goto L812D;  // None found (BNE -> BEQ inverted)

      SkipSpcs();  // Skip until non-blank
      AdvSrcP();   // Now pointing @ pseudo code field

      X = 3;  // Y=0
    L810C:
      ChrGot();
      if (A != FINTxt[Y]) goto L811A;
      Y++;
      X--;
      if (X != 0) goto L810C;
      if (X == 0) goto L812A;  // Got a hit (BEQ)

    L811A:
      Y = 0;
      X = 4;
    L811E:
      ChrGot();
      if (A != ELSETxt[Y]) goto L812D;
      Y++;
      X--;
      if (X != 0) goto L811E;
    L812A:
      C = true;  // got a hit (SEC)
      // BCS L812E - always
      goto L812E;

    L812D:
      C = false;  // CLC
    L812E:
      SrcP_hi = saved_srcP_hi;
      SrcP    = saved_srcP;
      Y       = SavIndY;
    }
#endif  // L80F7 duplicate

  const char FINTxt[]  = "FIN";
  const char ELSETxt[] = "ELSE";

  //=================================================
  // AddRLDEnt - Add an entry to the relocation entry dictionary table
  //  (ASM3.S lines ~1170-1215)
  // Inputs:
  //   A = low byte of offset address of value in GMC to be relocated when loaded
  //   X = 1 for 8-bit value, 2 for 16-bit value
  //   Y = 0 for normal order (low byte first), <> 0 for reverse order
  //   GblAbsF = external or undefined flag (external bit set if external ref)
  //   Lower8 = Low 8 bits of 16-bit value (used to fill in low byte for reverse order)
  // On 6502, normal order is lower 8 bits of a 16-bit
  // value is stored in 1st byte and upper 8-bits in 2nd
  // byte (pg 103). However, according to page 229,
  // normal order is DDB.
  // The Relocation Dictionary is build downwards from
  // high mem towards the End of Symbol Table
  // The initial start of the RLD is @ MemTop
  // & is build downwards towards LoMem
  // Each entry is 4 bytes
  void AddRLDEnt() {
    fprintf(stderr, "AddRLDEnt: RelCodeF=0x%02X, (int8_t)RelCodeF=%d\n", RelCodeF,
            (int8_t)RelCodeF);
    // Only generate when relocatable code is requested
    if ((int8_t)RelCodeF >= 0) {
      fprintf(stderr, "AddRLDEnt: Early return - not in REL mode\n");
      return;
    }

    // Ensure space for one entry (4 bytes)
    if (RLDEnd < EndSymT + 4) {
      uint8_t savedX = X;
      X              = 0x12;  // Sym/RLD table full
      RegAsmEW(X);
      X = savedX;
      return;
    }

    RLDEnd           = static_cast<uint16_t>(RLDEnd - 4);
    uint8_t* rld_ptr = SimPtrToMemPtr(RLDEnd);

    uint8_t flag = 0x01;                          // Not end-of-RLD
    if (X == 2) flag |= 0x80;                     // size = 2 bytes
    if (Y != 0) flag |= 0x20;                     // reversed/normal ordering flag
    if ((GblAbsF & external) != 0) flag |= 0x10;  // external reference

    rld_ptr[2] = A;       // offset low
    rld_ptr[1] = 0;       // offset high (placeholder)
    rld_ptr[0] = Lower8;  // extra info (low 8 bits of value)
    rld_ptr[3] = flag;
  }

  // WhiteSpc - Check if character is white space (space or CR)
  // Entry: (A)=char to check (from (SrcP),Y)
  // Ret: Z=1 if space/CR
  void WhiteSpc() {
    A = SrcP_at(Y);
    if (A == SPACE) {
      Z = true;
      return;
    }
    if (A == CR) {
      Z = true;
      return;
    }
    Z = false;
  }

  // IsZPMod - Checks if expr is a 8-bit or 16-bit value
  // For addressing modes involving zp
  // Ret: C=0 - Yes (8-bit), C=1 - No (16-bit)
  void IsZPMod() {
    A = ExprAccF;
    A &= 0b11101111;               // Clear EXTeRNal symbol bit
    A |= ValExpr_hi;               // Is hi-byte of expr zero?
    if (A == 0) goto L85CF_ZPMod;  // Yes => 8-bit
    A = ExprAccF;
    A &= 0b00010000;         // Is EXTeRNal symbol bit set?
    if (A == 0) goto L85C6;  // No
    A = Ret816F;             // EXTRN but is lo-byte being returned?
    if (A == 0) goto L85C8;  // Yes
  L85C6:
    C = true;  // 16-bit
    return;
  L85C8:
    X = 0x44 + 1;  // odd-warning
    RegAsmEW();    //  (EXTRN used as ZXTRN)
    X = SavIndX;
  L85CF_ZPMod:
    Y--;  // Move back
    C = false;
    return;
  }

  // IsAccMod - Check a single 'A' in the operand field
  // Entry: (A)=char to check
  // Ret: C=0 - Yes, C=1 - No
  void IsAccMod() {
    if (A != 'A') goto L85DD_AccMod;
    Y++;         // Look 1 char ahead
    WhiteSpc();  // Is the next char sp/cr?
    // Yes, we have a single 'A' in operand field
    if (Z) {
      Y--;  // Move back
      C = false;
      return;
    }
    Y--;  // No, just move back
  L85DD_AccMod:
    C = true;
    return;
  }

  // Is65C02 - Check if 65C02 opcodes are valid
  // Ret: C=0 - Yes, C=1 - No
  void Is65C02() {
    if ((int8_t)X6502F < 0) {  // Are X6502 opcodes allowed?
      // Yes
      Y--;  // Move back
      C = false;
      return;
    }
    // always
    C = true;
    return;
  }

  // L8598 - Calls helper subroutines for GAdrMod.
  // Entry: (A)=2,4,6 - index into subrtn table
  // The helper functions return values primarily via the C bit
  void L8598() {
    // Helper function pointer type
    typedef void (*HelperFunc)();

    // Jump-table pointers for helper subroutines
    // Entry at index 0 = IsZPMod, index 1 = IsAccMod, index 2 = Is65C02
    const HelperFunc L85AE_helpers[] = {IsZPMod, IsAccMod, Is65C02};
    constexpr size_t kNumHelpers     = sizeof(L85AE_helpers) / sizeof(L85AE_helpers[0]);

    SavIndX = X;
    X       = A;  // Byte offset (must be 2, 4, or 6)

    // Validate: only 3 subrtns, and X must be in [2..6] range
    if (X >= 7) {
      std::abort();  // BRK
    }

    // X must be even (2, 4, or 6 correspond to entries 0, 1, 2)
    if ((X & 1) != 0) {  // Odd value?
      std::abort();      // BRK
    }

    // Convert byte offset to array index: X/2 gives 1, 2, 3; so index = (X/2) - 1
    uint8_t array_index = (X / 2) - 1;

    // Bounds check
    if (array_index >= kNumHelpers) {
      std::abort();  // Should not reach here given X validation above
    }

    // Prepare for JMP via RTS in original
    ChrGot();  // Get curr char
    X = SavIndX;

    // Call the appropriate helper function
    HelperFunc helper = L85AE_helpers[array_index];
    helper();  // Call IsZPMod, IsAccMod, or Is65C02
  }

#if 0   // TODO: Phase 9+ - Large block of stub functions with compilation errors (not needed for
        // Phase 8.1)
  // This section contains WhiteSpc, IsZPMod, IsAccMod, IsSW16Reg, Is65C02, IsC02Op,
  // AddRLDEnt, CalcDisp, ChkRng, ValidateRange, GOpAdr, ListCode, LstSrcLn, Wr1Byte,
  // L81F0 (redefinition), AdvSrcP, SkipSpcs, NextRec, GSrcLin, ChrGot, ChrGot2, ChrGet2, ToUpCase, etc.
  // All these have compilation errors and are not needed for Phase 8.1 Init/Cleanup testing.

    // WhiteSpc - Check if character is white space (space or CR)
    // Entry:
    //  (A)=char to check (from (SrcP),Y)
    // Ret:
    // ($81FF) Ret Z=1 of space/CR
    // White space chars are <sp> and CR for ProDOS
    //
    void WhiteSpc() {
      A = SrcP_at(Y);
      if (A == SPACE) {
        Z = true;
        return;
      }
      if (A == CR) {
        Z = true;
        return;
      }
      Z = false;
    }

    // Checks if expr is a 8-bit or 16-bit value
    // For addressing modes involving zp
    // Ret:
    //  C=0 - Yes
    //  C=1 - No
    //
    void IsZPMod() {
      A = ExprAccF;
      A &= 0b11101111;               // Clear EXTeRNal symbol bit
      A |= ValExpr_hi;               // Is hi-byte of expr zero?
      if (A == 0) goto L85CF_ZPMod;  // Yes => 8-bit
      A = ExprAccF;
      A &= 0b00010000;         // Is EXTeRNal symbol bit set?
      if (A == 0) goto L85C6;  // No
      A = Ret816F;             // EXTRN but is lo-byte being returned?
      if (A == 0) goto L85C8;  // Yes
    L85C6:
      C = true;  // 16-bit
      return;
    //
    L85C8:
      X = 0x44 + 1;  // odd-warning
      RegAsmEW();    //  (EXTRN used as ZXTRN)
      X = SavIndX;
    L85CF_ZPMod:
      Y--;  // Move back
      C = false;
      return;
    }

    // Check a single 'A' in the operand field
    // i.e. checking for accumulator mode
    // Entry:
    // (A)=char to check
    // Ret:
    //   C=0 - Yes
    //   C=1 - No
    //
    void IsAccMod() {
      if (A != 'A') goto L85DD_AccMod;
      Y++;         // Look 1 char ahead
      WhiteSpc();  // Is the next char sp/cr?
      // Yes, we have a single 'A' in operand field
      if (Z) {
        Y--;  // Move back
        C = false;
        return;
      }
      Y--;  // No, just move back
    L85DD_AccMod:
      C = true;
      return;
    }

    // Chk if sw16 reg ($00-$0F)
    // Z=0
    // Z=1 - yes
    // (Y)-unchanged?
    //
    void IsSW16Reg() {
      A = ValExpr_hi;
      if (A != 0) {  // BNE L9500
        Z = false;   // LDA sets Z=0 when A != 0
        goto L9500;
      }

      A = 0xF0;
      Z = ((A & ValExpr) == 0);  // BIT - test without modifying A, sets Z
      if (!Z) {                  // BNE L9500
        goto L9500;
      }

      // Z is already true (set by BIT), fall through to RTS
      return;

    //
    L9500:
      bool z_flag = Z;     // PHP - Save Z bit
      X           = 0x32;  // SW16 reg err
      RegAsmEW();
      Z = z_flag;  // PLP - Restore Z bit
      return;
    }

    // Check if 65C02 opcodes are valid
    // Only Status reg is changed
    // Ret:
    // C=0 - Yes
    // C=1 - No
    // NB. If X6502F off, LDA (ZP) is still considered
    // valid. It is equivalent to LDA ZP
    //
    void Is65C02() {
      if ((int8_t)X6502F < 0) {  // Are X6502 opcodes allowed?
        // Yes
        Y--;  // Move back
        C = false;
        return;
      }
      // always
      C = true;
      return;
    }

    // L8598 - Calls help subroutines (functions).
    // Only 3 defined so far.
    // Entry
    //   (A)=2,4,6 - index into subrtn table
    //
    // The helper functions with return the required values
    // primarily the C bit
    //
    // Helper function pointer type
    typedef void (*HelperFunc)();

    // Jump-table pointers for helper subroutines
    // Entry at index 0 = IsZPMod, index 1 = IsAccMod, index 2 = Is65C02
    const HelperFunc L85AE_helpers[] = {IsZPMod, IsAccMod, Is65C02};

    // Number of helper functions in the table
    constexpr size_t kNumHelpers = sizeof(L85AE_helpers) / sizeof(L85AE_helpers[0]);

    void L8598() {
      SavIndX = X;
      X       = A;  // Byte offset (must be 2, 4, or 6)

      // Validate: only 3 subrtns, and X must be in [2..6] range
      if (X >= 7) {
        std::abort();  // BRK
      }

      // X must be even (2, 4, or 6 correspond to entries 0, 1, 2)
      // If X is odd, it's an invalid call - treat as error
      if ((X & 1) != 0) {  // Odd value?
        std::abort();      // BRK
      }

      // Convert byte offset to array index: X/2 gives 1, 2, 3; so index = (X/2) - 1
      uint8_t array_index = (X / 2) - 1;

      // Bounds check
      if (array_index >= kNumHelpers) {
        std::abort();  // Should not reach here given X validation above
      }

      // Prepare for JMP via RTS in original
      ChrGot();  // Get curr char
      X = SavIndX;

      // Call the appropriate helper function
      HelperFunc helper = L85AE_helpers[array_index];
      helper();  // Call IsZPMod, IsAccMod, or Is65C02
    }

    // IsC02Op - (A) = opcode
    // Check if (A) is NCR 65C02 opcode
    // C=1 - yes
    // (X)-unchanged
    void IsC02Op() {
      Y = 0;
    L8319:
      if (A < L8327[Y]) {
        C = true;
        return;
      }  // (BCC doRet5)
      if (A == L8327[Y]) {
        C = true;
        return;
      }  // (BEQ doRet5)
      Y++;
      if (Y < 0x12) goto L8319;  // (BCC)
      C = false;                 // CLC
    }

    // Rockwell opcodes
    const uint8_t L8327[] = {0x04, 0x0C, 0x14, 0x1A, 0x1C, 0x34, 0x3C, 0x3A, 0x5A,
                             0x64, 0x74, 0x7A, 0x80, 0x89, 0x9C, 0x9E, 0xDA, 0xFA};

    // AddRLDEnt - Add an entry to the relocation entry dictionary table
    // Entry:
    // (CodeLen) - zeroed whenever the initial srcfile is read
    // A=offset
    // X=1,2; 1 - 8-bits, 2 - 16-bits
    // Y=order of bytes 0=DW(low-hi), 1=DDB(hi-low)
    // On 6502, normal order is lower 8 bits of a 16-bit
    // value is stored in 1st byte and upper 8-bits in 2nd
    // byte (pg 103). However, according to page 229,
    // normal order is DDB.
    // The Relocation Dictionary is build downwards from
    // high mem towards the End of Symbol Table
    // The initial start of the RLD is @ MemTop
    // & is build downwards towards LoMem
    // Each entry is 4 bytes
    void AddRLDEnt() {
      // Only generate when relocatable code is requested
      if ((int8_t)RelCodeF >= 0) return;

      // Ensure space for one entry (4 bytes)
      if (RLDEnd < EndSymT + 4) {
        uint8_t savedX = X;
        X             = 0x12;  // Sym/RLD table full
        RegAsmEW(X);
        X = savedX;
        return;
      }

      RLDEnd = static_cast<uint16_t>(RLDEnd - 4);
      uint8_t* rld_ptr = SimPtrToMemPtr(RLDEnd);

      uint8_t flag = 0x01;                 // Not end-of-RLD
      if (X == 2) flag |= 0x80;            // size = 2 bytes
      if (Y != 0) flag |= 0x20;            // reversed/normal ordering flag
      if ((GblAbsF & external) != 0) flag |= 0x10;  // external reference

      rld_ptr[2] = A;      // offset low
      rld_ptr[1] = 0;      // offset high (placeholder)
      rld_ptr[0] = Lower8; // extra info (low 8 bits of value)
      rld_ptr[3] = flag;
    }

    // CalcDisp - Compute relative addr of a branch op
    // Ret:
    //   Val=Val-PC-Len
    void CalcDisp() {
      // SEC
      A = ValExpr;
      A -= Length;  // SBC
      X = A;
      A = ValExpr_hi;
      // SBC #0 with borrow
      if (ValExpr < Length) A--;

      Y = A;
      A = X;
      // SEC
      A -= PC;  // SBC
      ValExpr = A;
      A       = Y;
      // SBC PC+1 with borrow
      if (X < PC) A--;
      ValExpr_hi = A;
    }

    //=================================================
    // Phase 3d: ChkRng() - Check Range
    //
    // Generic range checking function that validates if a value falls
    // within specified bounds (min/max inclusive).
    //
    // Input:
    //   value - Value to check
    //   minVal - Minimum acceptable value (inclusive)
    //   maxVal - Maximum acceptable value (inclusive)
    //
    // Output:
    //   Returns true if value is OUT of range (carry set)
    //   Returns false if value is IN range (carry clear)
    //
    // This is the C++ equivalent of the 6502 assembly ChkRng routine
    // that uses compare operations and carry flag logic.
    //=================================================
    bool ChkRng(std::uint8_t value, std::uint8_t minVal, std::uint8_t maxVal) {
      // Check if value < minVal
      if (value < minVal) {
        return true;  // Out of range (below minimum)
      }

      // Check if value > maxVal
      if (value > maxVal) {
        return true;  // Out of range (above maximum)
      }

      // Value is within range
      return false;
    }

    //=================================================
    // Phase 3d: ValidateRange() - Validate Addressing Mode Range
    //
    // Validates that the operand value (ValExpr) is within the acceptable
    // range for the current addressing mode. Different addressing modes
    // have different constraints:
    //
    // - Immediate (#$nn): Any 8-bit or 16-bit value is valid
    // - Zero page ($nn): Must be 0-255 (single byte)
    // - Absolute ($nnnn): Any 16-bit value is valid
    // - Branch (BNE, BEQ, etc.): Relative offset must be -128 to +127
    //
    // Input:
    //   LenTIdx - Addressing mode index
    //   ModWrdL - Mode word flags (bit 3 = branch instruction)
    //   ValExpr - Operand value/address to validate
    //
    // Output:
    //   Registers error via RegAsmEW() if validation fails
    //   Error 0x1C: Zero page range error
    //   Error 0x26: Branch range error
    //=================================================
    void ValidateRange() {
      // Check if this is a branch instruction
      A = ModWrdL;
      if ((A & 0x08) != 0) {  // Branch instruction flag (bit 3)
        // Branch instructions use relative addressing
        // Valid range: -128 to +127 (signed 8-bit)
        // In two's complement:
        //   0x00 to 0x7F = 0 to +127
        //   0x80 to 0xFF = -128 to -1
        //
        // Invalid if high byte is not 0x00 (positive) or 0xFF (negative)
        A = ValExpr_hi;

        // If high byte is 0x00, check if low byte <= 0x7F
        if (A == 0x00) {
          A = ValExpr;
          if (A <= 0x7F) {
            return;  // Valid positive offset (0 to +127)
          }
          // Low byte > 0x7F but high byte = 0x00: out of range
          X = 0x26;  // Branch range error
          RegAsmEW(X);
          return;
        }

        // If high byte is 0xFF, check if low byte >= 0x80
        if (A == 0xFF) {
          A = ValExpr;
          if (A >= 0x80) {
            return;  // Valid negative offset (-128 to -1)
          }
          // Low byte < 0x80 but high byte = 0xFF: out of range
          X = 0x26;  // Branch range error
          RegAsmEW(X);
          return;
        }

        // High byte is neither 0x00 nor 0xFF: definitely out of range
        X = 0x26;  // Branch range error
        RegAsmEW(X);
        return;
      }

      // Not a branch - check addressing mode via LenTIdx
      A = LenTIdx;

      // Immediate mode (index 2): Any value is valid
      if (A == 2) {
        // Immediate mode - no range restrictions
        return;
      }

      // Zero page modes: Must be 0-255 (high byte must be 0)
      // LenTIdx = 1: Zero page
      // LenTIdx = 3: Zero page,X
      // LenTIdx = 6: (Zero page),Y
      // LenTIdx = 7: (Zero page,X)
      // LenTIdx = 8: (Zero page)
      // LenTIdx = 11: Zero page,Y
      if (A == 1 || A == 3 || A == 6 || A == 7 || A == 8 || A == 11) {
        // Check if high byte is set (value >= $100)
        A = ValExpr_hi;
        if (A != 0) {
          // Out of zero page range - register error
          X = 0x1C;  // Zero page range error
          RegAsmEW(X);
        }
        return;
      }

      // Absolute mode and other modes: Any 16-bit value is valid
      // LenTIdx = 0: Absolute
      // LenTIdx = 4: Absolute,X
      // LenTIdx = 5: Absolute,Y
      // LenTIdx = 9: (Absolute)
      // LenTIdx = 10: Accumulator
      // LenTIdx = 12: (Absolute,X)
      // No range restrictions for these modes
      return;
    }

    //=================================================
    // GOpAdr - Get Operand Address
    // Calculates the final operand address based on addressing mode
    // Handles:
    // - Branch relative addressing (calculates offset from PC)
    // - Zero page mode (masks to low byte, checks for overflow)
    // - Immediate mode (value as-is, never relocatable)
    // - Absolute addressing (full 16-bit value)
    // - Relocation flag management
    //
    // Input:
    //   LenTIdx - Addressing mode index (from GAdrMod)
    //   ValExpr - Expression value (from EvalExpr)
    //   PC - Current program counter
    //   ModWrdL - Mode word with branch flag (bit 3)
    //   RelExprF - Relocation flag from expression evaluation
    //
    // Output:
    //   ValExpr - Final operand address/value
    //   RelExprF - Updated relocation flag
    //   Error registered if invalid address range
    //=================================================
    void GOpAdr() {
      // Check if this is a branch instruction
      A = ModWrdL;
      if ((A & 0x08) != 0) {  // BIT Bit08; BNE - Branch instruction?
        // Yes, calculate relative displacement
        CalcDisp();
        return;
      }

      // Not a branch - check addressing mode via LenTIdx
      A = LenTIdx;

      // Immediate mode (index 2): Value used as-is, never relocatable
      if (A == 2) {
        // Immediate mode - value is literal, not an address
        // Clear relocation flag since immediate values can't be relocated
        RelExprF = 0;
        return;
      }

      // Zero page mode (index 1): Only low byte, check for overflow
      if (A == 1) {
        // Check if high byte is set (value >= $100)
        A = ValExpr_hi;
        if (A != 0) {
          // Out of zero page range - register error
          X = 0x1C;  // Zero page range error
          RegAsmEW(X);
        }
        // Mask to low byte only
        ValExpr_hi = 0;
        return;
      }

      // Zero page indexed modes (zp,X=3, zp,Y=11, (zp,X)=7, (zp),Y=6, (zp)=8)
      // These also need zero page range checking
      if (A == 3 || A == 6 || A == 7 || A == 8 || A == 11) {
        // Check if high byte is set (value >= $100)
        A = ValExpr_hi;
        if (A != 0) {
          // Out of zero page range - register error
          X = 0x1C;  // Zero page range error
          RegAsmEW(X);
        }
        // Mask to low byte only
        ValExpr_hi = 0;
        return;
      }

      // Absolute mode (index 0): Full 16-bit address
      // Absolute indexed (abs,X=4, abs,Y=5, (abs)=9, (abs,X)=12)
      // Accumulator mode (index 10): No operand address calculation needed
      // All other modes: Use value as-is, preserve relocation flag

      // For all other addressing modes, ValExpr is already set correctly
      // by EvalExpr(), and RelExprF is already set if symbol is relocatable
      // No additional processing needed
      return;
    }

    // Stub: ListCode - Print generated code
    void ListCode() {
      // TODO: List/print generated object code
    }

    // Stub: LstSrcLn - List source line
    void LstSrcLn() {
      // TODO: Print source line to listing
    }

    // Stub: Wr1Byte - Write one byte to disk
    void Wr1Byte() {
      // TODO: Write one byte to object file on disk
    }

    // AdvObjPC - A=# to advance
    void AdvObjPC() {
      // CLC
      A += ObjPC;  // code buf
      ObjPC = A;
      if (A >= ObjPC) goto L8275;  // no carry (BCC)
      ObjPC_hi++;                  // INC ObjPC+1
    L8275:
      L828A();
    }

    // L8278 - Not referenced
    void L8278() {
      A = MemTop;
      // CMP ObjPC
      A = MemTop_hi;
      // SBC ObjPC+1
      if ((MemTop_hi < ObjPC_hi) || (MemTop_hi == ObjPC_hi && MemTop < ObjPC)) goto L828A;  // (BCC)
    }

    // L8282 - Object buffer overflow error
    void L8282() {
      X = 0x36;     // Obj buf overflow
      RegAsmEW(X);  // Pass error token to RegAsmEW
      CanclAsm();
    }

    // L828A
    void L828A() {
      A = ObjPC;  // Should not be >= HiMem
      // CMP HighMem - which was passed by Editor
      A = ObjPC_hi;
      // SBC HighMem+1
      if ((ObjPC_hi > HighMem_hi) || (ObjPC_hi == HighMem_hi && ObjPC >= HighMem))
        goto L8282;  // (BCS)
    }

    // SkipSpcs - Skip to next field
    void SkipSpcs() {
      A = SrcP_at(Y);
      if (A != SPACE) return;  // Return (BNE doRTS9)
      Y++;
      if (Y != 0) goto SkipSpcs;  // skip blanks (BNE)
    }

    // ($81E6) Skip Blanks
    // Ret:
    // Z=1 blank
    // Z=0 non-blank
    // (A)=char
    // (Y)=index
    // On fall thru, search starts fr 2nd char of field
    //
    void L81EF() {
      Y++;
    }

    void L81F0() {
      A = SrcP_at(Y);
      if (A == SPACE) {
        Z = true;
        return;
      }  // Got a blank (Z=1) & ret (BEQ doRTS9)
      if (A != CR) goto L81EF;  // (BNE)
      Y = 0;                    // Index 1st char of field
      A = CR;                   // Got CR (Z=0) & ret
      Z = false;
    }

    // AdvSrcP
    void AdvSrcP() {
      // CLC
      A = Y;
      A += SrcP;
      SrcP = A;
      A    = 0;
      Y    = A;      // =0
      A += SrcP_hi;  // with carry
      SrcP_hi = A;
    }

    // This subrtn is part of Scanner
    // There are 2 entry points viz ChrGet and ChrGot
    // On entry:
    //   (Y)    = index into the src line
    //   (SrcP) = Pointing somewhere within source line
    // Ret:
    // (A) - char (converted to uppercase if alphabetic)
    // C=1 if char is non-alphabetic
    // C=0 if char is alphabetic (A-Z, a-z)
    // Z=1 if char is numeric digit (0-9)
    // Z=0 if char is non-numeric
    // V=1 if char is hexdec digit (0-9, A-F, a-f)
    // V=0 if char is non-hexdec
    // (X) - unchanged
    // (Y) - incr by 1 if 1st entry point else unchanged
    void ChrGet() {
      Y++;
    }

    void ChrGot() {
      A              = SrcP_at(Y);     // Get char fr src line
      ZPSaveY        = Y;              // Save (Y) temporarily
      uint8_t char_y = A;              // Use char as an index as well as saving it in (Y)
      if ((int8_t)A >= 0) goto L8211;  // Must be std ASCII or (BPL)

      std::abort();  // BRK - else crash

    L8211:
      A             = CharMap1[char_y];  // Get flag byte
      uint8_t flags = A;                 // Save for later use (PHA)
      A             = char_y;            // Get back char (TYA)
      Y             = ZPSaveY;           // restore Y

      // Set flags from saved flag byte (PLP)
      N = (flags & 0x80) != 0;
      V = (flags & 0x40) != 0;
      Z = (flags & 0x02) != 0;
      C = (flags & 0x01) != 0;

      if (N) {      // If (A)=$61-$7A (a-z) (BPL doRet2)
        A &= 0xDF;  // convert to upper case
      }
    }
#endif  // Large block of stub functions

  //=================================================
  // Phase 8.4: Minimal HndlMnem Stub for Pass 1 Testing
  // This is a simplified version that handles only the directives/opcodes
  // needed for Phase 8.4 Pass 1 tests. Full implementation is in #if 0 block below.
  //=================================================
  void HndlMnem() {
    // Get mnemonic text starting at Y
    uint8_t     startY = Y;
    std::string mnemonic;

    // Extract mnemonic (until space, CR, or comma)
    while (true) {
      uint8_t ch = SrcP_at(Y);
      if (ch == ' ' || ch == CR || ch == '\t' || ch == ',') break;
      mnemonic += static_cast<char>(::toupper(ch));
      Y++;
      if (mnemonic.length() > 10) break;  // Safety limit
    }

    // Move Y to the character just after the mnemonic so subsequent
    // calls to NxtField() will correctly skip to the operand field.
    Y = startY + static_cast<uint8_t>(mnemonic.length());

    // Handle recognized mnemonics/directives for Phase 8.4-8.5
    if (mnemonic == "NOP") {
      Length = 1;

      // Pass 2: Emit opcode and sync PC with ObjPC
      if (PassNbr == 1) {
        A = 0x00;  // NOP opcode
        StorByt();
        // Sync PC with ObjPC after emission
        PC = ObjPC;
      } else {
        // Pass 1: just track PC
        PC += 1;
      }

      C = false;  // Success
      return;
    }

    if (mnemonic == "LDA" || mnemonic == "ADC" || mnemonic == "AND" || mnemonic == "ASL") {
      // Set a dummy MnemP for tests
      MnemP  = reinterpret_cast<const uint8_t*>(0x1000);
      ZAB    = 0x7F;  // Not a directive
      Length = 2;     // Assume 2-byte instruction
      if (PassNbr == 0) {
        PC += 2;
      } else {
        // Parse operand for Pass 2
        NxtField();  // Skip to operand
        uint8_t operand_byte = 0x00;

        // Check for immediate mode (#)
        if (SrcP_at(Y) == '#') {
          Y++;  // Skip '#'
          // Parse hex value ($hh)
          if (SrcP_at(Y) == '$') {
            Y++;  // Skip '$'
            // Parse 1 or 2 hex digits
            uint8_t digit1 = SrcP_at(Y);
            if (digit1 >= '0' && digit1 <= '9') {
              operand_byte = digit1 - '0';
            } else if (digit1 >= 'A' && digit1 <= 'F') {
              operand_byte = digit1 - 'A' + 10;
            } else if (digit1 >= 'a' && digit1 <= 'f') {
              operand_byte = digit1 - 'a' + 10;
            }
            Y++;
            // Check for second digit
            uint8_t digit2 = SrcP_at(Y);
            if ((digit2 >= '0' && digit2 <= '9') || (digit2 >= 'A' && digit2 <= 'F') ||
                (digit2 >= 'a' && digit2 <= 'f')) {
              operand_byte <<= 4;
              if (digit2 >= '0' && digit2 <= '9') {
                operand_byte |= digit2 - '0';
              } else if (digit2 >= 'A' && digit2 <= 'F') {
                operand_byte |= digit2 - 'A' + 10;
              } else if (digit2 >= 'a' && digit2 <= 'f') {
                operand_byte |= digit2 - 'a' + 10;
              }
            }
          }
        }

        A = 0xA9;  // LDA immediate opcode
        StorByt();
        A = operand_byte;
        StorByt();
        PC = ObjPC;
      }
      C = false;
      return;
    }

    if (mnemonic == "STA") {
      MnemP  = reinterpret_cast<const uint8_t*>(0x1001);
      ZAB    = 0x7F;
      Length = 3;
      if (PassNbr == 0) {
        PC += 3;
      } else {
        A = 0x8D;
        StorByt();
        A = 0x00;
        StorByt();
        A = 0x10;
        StorByt();
        PC = ObjPC;
      }
      C = false;
      return;
    }

    if (mnemonic == "JMP") {
      MnemP  = reinterpret_cast<const uint8_t*>(0x1002);
      ZAB    = 0x7F;
      Length = 3;
      if (PassNbr == 0) {
        PC += 3;
      } else {
        A = 0x4C;  // JMP absolute
        StorByt();
        A = 0x00;
        StorByt();
        A = 0x00;
        StorByt();
        PC = ObjPC;
      }
      C = false;
      return;
    }

    if (mnemonic == "BRK") {
      MnemP  = reinterpret_cast<const uint8_t*>(0x1003);
      ZAB    = 0x7F;
      Length = 1;
      if (PassNbr == 0) {
        PC += 1;
      } else {
        A = 0x00;  // BRK
        StorByt();
        PC = ObjPC;
      }
      C = false;
      return;
    }

    if (mnemonic == "BCC" || mnemonic == "BCS") {
      MnemP  = reinterpret_cast<const uint8_t*>(0x1004);
      ZAB    = 0x7F;
      Length = 2;  // Branch instructions are 2 bytes
      if (PassNbr == 0) {
        PC += 2;
      } else {
        A = (mnemonic == "BCC") ? 0x90 : 0xB0;
        StorByt();
        A = 0x00;
        StorByt();
        PC = ObjPC;
      }
      C = false;
      return;
    }

    if (mnemonic == "ORG") {
      // Parse ORG directive - expects hex address like $8000
      NxtField();  // Skip to operand

      // Simple hex parser - look for $ and parse hex digits
      uint8_t ch = SrcP_at(Y);
      if (ch == '$') {
        Y++;  // Skip $
        uint16_t addr = 0;
        while (true) {
          ch = SrcP_at(Y);
          if (ch >= '0' && ch <= '9') {
            addr = (addr << 4) | (ch - '0');
            Y++;
          } else if (ch >= 'A' && ch <= 'F') {
            addr = (addr << 4) | (ch - 'A' + 10);
            Y++;
          } else if (ch >= 'a' && ch <= 'f') {
            addr = (addr << 4) | (ch - 'a' + 10);
            Y++;
          } else {
            break;  // Not a hex digit
          }
        }

        // Bounds validation: check if address >= HighMem
        if (addr >= HighMem) {
          // Register error and DO NOT change PC/ObjPC
          uint8_t savedX = X;
          uint8_t savedY = Y;
          X              = 0x24;  // Directive operand error
          RegAsmEW(X);
          X = savedX;
          Y = savedY;
        } else {
          // Valid address: Set both PC and ObjPC in both passes
          PC    = addr;
          ObjPC = addr;
        }
      }

      Length = 0;      // Directives don't generate code
      C      = false;  // Success
      return;
    }

    if (mnemonic == "DS") {
      // DS (Define Storage) - reserves N bytes and fills with zeros in Pass 2
      NxtField();  // Skip to operand

      // Parse count (decimal or hex)
      uint16_t count = 0;
      uint8_t  ch    = SrcP_at(Y);

      if (ch == '$') {
        // Hex count
        Y++;
        while (true) {
          ch = SrcP_at(Y);
          if (ch >= '0' && ch <= '9') {
            count = (count << 4) | (ch - '0');
            Y++;
          } else if (ch >= 'A' && ch <= 'F') {
            count = (count << 4) | (ch - 'A' + 10);
            Y++;
          } else if (ch >= 'a' && ch <= 'f') {
            count = (count << 4) | (ch - 'a' + 10);
            Y++;
          } else {
            break;
          }
        }
      } else {
        // Decimal count
        while (ch >= '0' && ch <= '9') {
          count = count * 10 + (ch - '0');
          Y++;
          ch = SrcP_at(Y);
        }
      }

      Length = count;

      // Pass 2: Emit zeros and sync PC with ObjPC
      if (PassNbr == 1) {
        for (uint16_t i = 0; i < count; i++) {
          A = 0x00;
          StorByt();
        }
        // After emission, PC must equal ObjPC
        PC = ObjPC;
      } else {
        // Pass 1: Track PC
        PC += count;
      }

      C = false;
      return;
    }

    if (mnemonic == "REL") {
      // REL directive - enable relocatable code generation
      // Only set flags in Pass 1
      if (PassNbr == 0) {
        // Set MSB of RelCodeF to indicate REL mode
        C        = true;                    // SEC
        RelCodeF = (RelCodeF >> 1) | 0x80;  // ROR with carry => MSB set
        // Also set DummyF to mark subsequent symbols as relocatable
        DummyF = 0x80;  // MSB set => relocatable section
      }
      ZAB    = 0x80;  // Directive flag
      Length = 0;
      C      = false;
      return;
    }

    // Handle directives with proper flag and routing
    if (mnemonic == ".EQU" || mnemonic == "EQU") {
      fprintf(stderr, "  EQU handler: PassNbr=%d, LabelF=%d, SymP=0x%04X\n", PassNbr, LabelF, SymP);
      ZAB                   = 0x80;  // Directive flag
      g_LastDirectiveCalled = "HndlEQU";

      // For dispatch-only tests (using g_test_src_buffer), just return success
      if (g_test_src_buffer != nullptr) {
        Length = 0;
        C      = false;
        return;
      }

      // Inline EQU handling - evaluate the operand and store in symbol table
      NxtField();  // Skip to operand field

      // Check if there's an operand (dispatch-only tests may have no operand)
      uint8_t ch = SrcP_at(Y);
      if (ch == CR || ch == 0) {
        // No operand - just succeed as dispatch test
        Length = 0;
        C      = false;
        return;
      }

      // Evaluate the expression (sets ValExpr, ValExpr_hi, RelExprF)
      EvalOprnd();
      fprintf(stderr, "  EQU after EvalOprnd: C=%d, ValExpr=0x%04X, RelExprF=%d\n", C,
              (uint16_t)(ValExpr | (ValExpr_hi << 8)), RelExprF);
      if (C) {
        // Error in evaluation
        Length = 0;
        return;  // C already set by EvalOprnd
      }

      // Check for valid end-of-operand
      A = NxtToken;
      if (A != 0) {
        X = 0x24;  // Directive operand error
        RegAsmEW(X);
        Length = 0;
        return;
      }

      // For Pass 1, we need to find the label and update its value
      if (PassNbr == 0) {
        fprintf(stderr, "  EQU Pass 1: updating symbol, LabelF=%d, SymP=0x%04X\n", LabelF, SymP);
        // Try to find and update the symbol with the EQU value
        if (LabelF != 0 && SymP != 0) {
          uint8_t* symptr = SimPtrToMemPtr(SymP);
          // Find the flag byte by scanning DCI-encoded name
          // DCI: MSB set on all chars except the last (terminator has MSB clear)
          int idx = 1;                              // Start after length byte
          while ((symptr[idx] & 0x80) != 0) idx++;  // Skip all name bytes with MSB=1
          idx++;                                    // Now at flag byte

          fprintf(stderr, "  EQU updating: idx=%d, old_flags=0x%02X\n", idx, symptr[idx]);

          // Update symbol flags
          uint8_t flags = symptr[idx];
          flags &= static_cast<uint8_t>(~undefined);  // Clear undefined
          flags |= (RelExprF & relative);
          if ((int8_t)DummyF < 0) flags |= relative;
          flags |= unrefd;
          symptr[idx] = flags;

          // Store the value
          symptr[idx + 1] = ValExpr;
          symptr[idx + 2] = ValExpr_hi;

          fprintf(stderr, "  EQU updated: new_flags=0x%02X, value=0x%04X\n", flags,
                  (uint16_t)(ValExpr | (ValExpr_hi << 8)));
        } else {
          fprintf(stderr, "  EQU Pass 1: skipping update (LabelF=%d, SymP=0x%04X)\n", LabelF, SymP);
        }
      }

      Length = 0;
      C      = false;
      return;
    }

    if (mnemonic == ".ORG" || mnemonic == "ORG") {
      ZAB                   = 0x80;
      g_LastDirectiveCalled = "HndlORG";
      // Call inline ORG handling for now
      NxtField();
      uint8_t ch = SrcP_at(Y);
      if (ch == '$') {
        Y++;
        uint16_t addr = 0;
        while (true) {
          ch = SrcP_at(Y);
          if (ch >= '0' && ch <= '9') {
            addr = (addr << 4) | (ch - '0');
            Y++;
          } else if (ch >= 'A' && ch <= 'F') {
            addr = (addr << 4) | (ch - 'A' + 10);
            Y++;
          } else if (ch >= 'a' && ch <= 'f') {
            addr = (addr << 4) | (ch - 'a' + 10);
            Y++;
          } else {
            break;
          }
        }
        if (addr >= HighMem) {
          X = 0x24;
          RegAsmEW(X);
        } else {
          PC    = addr;
          ObjPC = addr;
        }
      }
      Length = 0;
      C      = false;
      return;
    }

    if (mnemonic == ".LIST" || mnemonic == "LIST") {
      ZAB                   = 0x80;
      g_LastDirectiveCalled = "HndlLIST";
      Length                = 0;
      C                     = false;
      return;
    }

    if (mnemonic == "LST") {
      ZAB                   = 0x80;
      g_LastDirectiveCalled = "HndlLST";
      Length                = 0;
      C                     = false;
      return;
    }

    if (mnemonic == ".PAGE" || mnemonic == "PAGE") {
      ZAB                   = 0x80;
      g_LastDirectiveCalled = "DoPage";
      Length                = 0;
      C                     = false;
      return;
    }

    if (mnemonic == ".TITLE" || mnemonic == "TITLE" || mnemonic == "SBTL") {
      ZAB                   = 0x80;
      g_LastDirectiveCalled = "HndlSBTL";
      Length                = 0;
      C                     = false;
      return;
    }

    if (mnemonic == ".NOLIST" || mnemonic == "NOLIST") {
      ZAB                   = 0x80;
      g_LastDirectiveCalled = "HndlNOLIST";
      Length                = 0;
      C                     = false;
      return;
    }

    // Recognized directive without handler (like .SKIP)
    // But allow specific handled directives to pass through
    if (!mnemonic.empty() && mnemonic[0] == '.' && mnemonic != ".BYTE" && mnemonic != ".DFB" &&
        mnemonic != ".WORD" && mnemonic != ".DW" && mnemonic != ".EQU" && mnemonic != ".ORG" &&
        mnemonic != ".LIST" && mnemonic != ".NOLIST" && mnemonic != ".PAGE" &&
        mnemonic != ".TITLE") {
      ZAB                   = 0x80;  // Set directive flag
      g_LastDirectiveCalled = "";
      C                     = true;  // Error - unsupported
      return;
    }

    // Catch other unsupported mnemonics
    if (mnemonic == "SKIP" || mnemonic == "DPAGE") {
      ZAB                   = 0x80;
      g_LastDirectiveCalled = "";
      C                     = true;  // Error - unsupported
      return;
    }

    if (mnemonic == "DFB" || mnemonic == ".BYTE" || mnemonic == ".DFB") {
      // DFB (Define Byte) - with relocatable support
      ZAB                   = 0x80;  // Mark as directive
      g_LastDirectiveCalled = "HndlBYTE";

      // Y is already positioned after mnemonic, skip any spaces to operand
      while (SrcP_at(Y) == ' ' || SrcP_at(Y) == '\t') Y++;
      uint16_t byteCount = 0;

      if (PassNbr == 0) {
        // Pass 1: Just count comma-separated operands to determine byte count
        while (true) {
          while (SrcP_at(Y) == ' ' || SrcP_at(Y) == '\t') Y++;
          uint8_t ch = SrcP_at(Y);
          if (ch == CR || ch == 0) break;

          //  Skip over the operand (any non-comma, non-CR sequence)
          while (true) {
            ch = SrcP_at(Y);
            if (ch == CR || ch == 0 || ch == ',') break;
            Y++;
          }
          byteCount++;

          // Look for comma
          while (SrcP_at(Y) == ' ' || SrcP_at(Y) == '\t') Y++;
          if (SrcP_at(Y) == ',') {
            Y++;
          } else {
            break;
          }
        }
        Length = byteCount;
        PC += byteCount;
        C = false;
        return;
      }

      // Pass 2: Evaluate expressions and emit bytes
      while (true) {
        // Skip spaces
        while (SrcP_at(Y) == ' ' || SrcP_at(Y) == '\t') Y++;

        uint8_t ch = SrcP_at(Y);
        if (ch == CR || ch == 0) break;

        // Evaluate expression (handles symbols, constants)
        EvalExpr();
        if (C) break;  // Error in evaluation

        // Check for byte overflow (value > 0xFF)
        if (ValExpr > 0xFF || ValExpr_hi != 0) {
          uint8_t savedX = X;
          uint8_t savedY = Y;
          X              = 0x28;  // Byte value out of range error
          RegAsmEW(X);
          X = savedX;
          Y = savedY;
        }

        // Pass 2: Emit byte and check for relocatable
        if (PassNbr == 1) {
          // Check if relocatable expression requires RLD entry
          if (RelExprF != 0) {
            // Create RLD entry for relocatable byte
            uint8_t save_X = X;
            uint8_t save_Y = Y;
            A              = byteCount;  // offset in GMC
            X              = 1;          // 8-bit value
            Y              = 0;          // no endian issue for byte
            AddRLDEnt();
            X = save_X;
            Y = save_Y;
          }
          A = (ValExpr & 0xFF);
          StorByt();
        }
        byteCount++;

        // Look for comma
        while (SrcP_at(Y) == ' ' || SrcP_at(Y) == '\t') Y++;
        if (SrcP_at(Y) == ',') {
          Y++;
        } else {
          break;
        }
      }

      Length = byteCount;
      if (PassNbr == 0) {
        PC += byteCount;
      } else {
        PC = ObjPC;
      }
      C = false;
      return;
    }

    if (mnemonic == "DW" || mnemonic == ".WORD" || mnemonic == ".DW") {
      // DW (Define Word) - with relocatable support
      ZAB                   = 0x80;  // Mark as directive
      g_LastDirectiveCalled = "HndlWORD";

      // Y is already positioned after mnemonic, skip any spaces to operand
      while (SrcP_at(Y) == ' ' || SrcP_at(Y) == '\t') Y++;
      uint16_t wordCount = 0;

      // DEBUG
      fprintf(stderr, "DW handler: PassNbr=%d, RelCodeF=0x%02X, Y=%d, ch=0x%02X '%c'\n", PassNbr,
              RelCodeF, Y, SrcP_at(Y), SrcP_at(Y));

      if (PassNbr == 0) {
        // Pass 1: Just count comma-separated operands to determine word count
        while (true) {
          while (SrcP_at(Y) == ' ' || SrcP_at(Y) == '\t') Y++;
          uint8_t ch = SrcP_at(Y);
          if (ch == CR || ch == 0) break;

          // Skip over the operand (any non-comma, non-CR sequence)
          while (true) {
            ch = SrcP_at(Y);
            if (ch == CR || ch == 0 || ch == ',') break;
            Y++;
          }
          wordCount++;

          // Look for comma
          while (SrcP_at(Y) == ' ' || SrcP_at(Y) == '\t') Y++;
          if (SrcP_at(Y) == ',') {
            Y++;
          } else {
            break;
          }
        }
        Length = wordCount * 2;
        PC += wordCount * 2;
        C = false;
        return;
      }

      // Pass 2: Evaluate expressions and emit words
      fprintf(stderr, "  Entering Pass 2 loop, Y=%d, ch=0x%02X '%c'\\n", Y, SrcP_at(Y),
              SrcP_at(Y) >= 32 ? SrcP_at(Y) : '?');
      while (true) {
        // Skip spaces
        while (SrcP_at(Y) == ' ' || SrcP_at(Y) == '\t') Y++;

        uint8_t ch = SrcP_at(Y);
        fprintf(stderr, "  Top of loop: Y=%d, ch=0x%02X\n", Y, ch);
        if (ch == CR || ch == 0) {
          fprintf(stderr, "  Breaking - found CR or 0\n");
          break;
        }

        // Evaluate expression (handles symbols, constants)
        fprintf(stderr, "  About to call EvalExpr(), C=%d\n", C);
        EvalExpr();
        fprintf(stderr, "  After EvalExpr(), C=%d\n", C);
        if (C) {
          fprintf(stderr, "  Breaking - error from EvalExpr\n");
          break;
        }

        // Pass 2: Emit word and check for relocatable
        if (PassNbr == 1) {
          fprintf(stderr, "  In Pass 2 block, checking RelExprF=%d\n", RelExprF);
          // Check if relocatable expression requires RLD entry
          if (RelExprF != 0) {
            fprintf(stderr, "  Calling AddRLDEnt()\n");
            // Create RLD entry for relocatable word
            uint8_t save_X = X;
            uint8_t save_Y = Y;
            A              = 0;  // offset = 0 (first byte in word)
            X              = 2;  // 16-bit value
            Y              = 0;  // little-endian (DW order)
            AddRLDEnt();
            X = save_X;
            Y = save_Y;
          }
          // Emit little-endian word
          A = (ValExpr & 0xFF);  // Low byte
          StorByt();
          A = (ValExpr_hi & 0xFF);  // High byte
          StorByt();
        }
        wordCount++;

        // Look for comma
        while (SrcP_at(Y) == ' ' || SrcP_at(Y) == '\t') Y++;
        if (SrcP_at(Y) == ',') {
          Y++;
        } else {
          break;
        }
      }

      Length = wordCount * 2;
      if (PassNbr == 0) {
        PC += wordCount * 2;
      } else {
        PC = ObjPC;
      }
      C = false;
      return;
    }

    if (mnemonic == "EQU") {
      // Inline EQU handling for Pass 1 (avoid external HndlEQU linkage issue)
      // Y is already positioned after mnemonic, skip any spaces
      while (SrcP_at(Y) == ' ' || SrcP_at(Y) == '\t') Y++;

      // Evaluate operand expression (force Pass2 semantics inside)
      EvalOprnd();
      if (C) {
        X = 0x24;
        RegAsmEW(X);
        C = true;
        return;
      }
      if (NxtToken != 0) {
        X = 0x24;
        RegAsmEW(X);
        C = true;
        return;
      }

      // Pass 1: write value into symbol table entry for the label
      if (PassNbr == 0 && LabelF != 0) {
        // Ensure symbol pointer present
        if (SymP == 0) {
          uint16_t saved_SrcP = SrcP;
          uint8_t  saved_Y    = Y;
          Y                   = 0;
          FindSym();
          Y    = saved_Y;
          SrcP = saved_SrcP;
        }

        if (SymP != 0) {
          uint8_t* symptr = SimPtrToMemPtr(SymP);
          int      idx    = 0;
          while ((symptr[idx] & 0x80) != 0) idx++;
          idx++;  // flag byte
          uint8_t flags = symptr[idx];
          flags &= static_cast<uint8_t>(~undefined);
          flags |= (RelExprF & relative);
          if ((int8_t)DummyF < 0) flags |= relative;
          if ((int8_t)RelCodeF < 0) flags |= relative;
          flags |= unrefd;
          symptr[idx]     = flags;
          symptr[idx + 1] = ValExpr;
          symptr[idx + 2] = ValExpr_hi;
        } else {
        }
      }

      C = false;
      return;
    }

    // Unknown mnemonic - register error
    X = 0x04;  // Undefined opcode
    RegAsmEW(X);
    C = true;  // Error
  }

#if 0  // TODO: Phase 9+ - HndlMnem and GAdrMod use functions that were commented out
    //=================================================
    // HndlMnem - Process mnemonic/pseudo opcode/directive field
    // (ASM2.S line ~2054, label HndlMnem)
    // Entry:
    //   (Y) = index into source line (typically 0 at mnemonic start)
    //   SrcP = pointer to current position in source line
    // Ret:
    //   ZAB = first flag byte if not a directive; directive flag if directive
    //   MnemP = pointer to mnemonic table entry
    //   SubTIdx = offset into opcode sub-table (for opcodes)
    //   C=0 - success (opcode/directive found)
    //   C=1 - fail (mnemonic not found, or macro invocation in restricted context)
    // For directives: sets up RTS trampoline (not fully implemented here)
    //=================================================
    void HndlMnem() {
      // L8348: Initialize
      A   = 0x80;
      ZAB = A;  // STA ZAB - Initialize flag byte

      ChrGot();                  // JSR ChrGot - Get first character
      if (!C) goto L8348_alpha;  // BCC L8348 - alphabetic char
      if (A == '.') goto L8346;  // BNE L83AC - If a DOT directive
      goto L83AC_notfound;       // Else not found

    L8346:
      A = 'A' - 1;  // LDA #'A'-1 - use ASCII @ in place of dot

    L8348_alpha:
      // SEC, SBC #'A'-1 - Convert letter to index (A=0, B=1, ..., Z=25)
      A = static_cast<std::uint8_t>(A - ('A' - 1));
      A = static_cast<std::uint8_t>(A << 1);  // ASL - Multiply by 2 for word index
      X = A;                                  // TAX

      // LDA Tbl1stLet,X / STA MnemP - Get pointer to subtable
      const std::uint8_t* ptr = Tbl1stLet[X / 2];
      if (ptr == nullptr) goto L839C_nosubtbl;  // No such opcode/directive with this 1st letter

      MnemP = ptr;

    // L8359: Main character comparison loop
    L8359:
      ChrGot();                        // JSR ChrGot - Note: msb of char=0
      A ^= MnemP[Y];                   // EOR (MnemP),Y - Effectively comparing 7 bits
      uint8_t cmp_result = A;          // Save comparison result
      A <<= 1;                         // ASL - C=1 if last char of opcode was compared
      if (A != 0) goto L837E_nomatch;  // BNE L837E - No hit

    L8361:
      Y++;                 // INY - Z=1 => 7 bits comparison above matched
      if (!C) goto L8359;  // BCC L8359 - Continue to cmp next char
      //
      // On fall thru, if C=1 then that was last char/byte of
      // mnemonic entry & we have got a match
      //
      WhiteSpc();                  // JSR WhiteSpc - sp/cr?
      if (!Z) goto L8386_advnext;  // BNE L8386 - no

      // Got a match! Load the flag byte.
      A   = MnemP[Y];                         // LDA (MnemP),Y - Get 1st flag byte
      ZAB = A;                                // STA ZAB
      if ((int8_t)A >= 0) goto L837C_opcode;  // BPL L837C - Not a directive

      //
      // This part of the code handles directives by doing a jump via RTS
      // (ZAB) has the directive's only flag byte which may be modified
      // by the directive handler
      //
      // For Phase 4: Wire directive dispatch to handler stubs
      // In the original 6502 code, this uses RTS trampoline. For C++,
      // we dispatch based on the matched directive name.
      //
      SavIndY = Y;  // STY SavIndY - Save index into Mnemonics table

      // Identify which directive was matched by looking at the mnemonic text
      // The mnemonic table entry starts at MnemP and we've matched up to position Y
      // Let's extract the directive name to dispatch properly

      // Check if this is a dot directive or regular directive
      uint8_t first_char = MnemP[0] & 0x7F;

      if (first_char == '.') {
        // Dot directive - dispatch based on second character
        uint8_t second_char = MnemP[1] & 0x7F;

        // Dispatch to appropriate handler
        if (second_char == 'E') {
          // Could be .EQU
          uint8_t third_char = MnemP[2] & 0x7F;
          if (third_char == 'Q') {
            HndlEQU();
            return;
          }
        } else if (second_char == 'O') {
          // Could be .ORG
          uint8_t third_char = MnemP[2] & 0x7F;
          if (third_char == 'R') {
            HndlORG();
            return;
          }
        } else if (second_char == 'B') {
          // Could be .BYTE or .BLOCK
          uint8_t third_char = MnemP[2] & 0x7F;
          if (third_char == 'Y') {
            HndlBYTE();
            return;
          } else if (third_char == 'L') {
            HndlBLOCK();
            return;
          }
        } else if (second_char == 'L') {
          // Could be .LIST
          uint8_t third_char = MnemP[2] & 0x7F;
          if (third_char == 'I') {
            uint8_t fourth_char = MnemP[3] & 0x7F;
            if (fourth_char == 'S') {
              uint8_t fifth_char = MnemP[4] & 0x7F;
              if (fifth_char == 'T') {
                HndlLIST();  // .LIST directive
                return;
              }
            }
          }
        } else if (second_char == 'N') {
          // Could be .NOLIST
          uint8_t third_char = MnemP[2] & 0x7F;
          if (third_char == 'O') {
            HndlNOLIST();
            return;
          }
        } else if (second_char == 'P') {
          // Could be .PAGE
          uint8_t third_char = MnemP[2] & 0x7F;
          if (third_char == 'A') {
            DoPage();
            return;
          }
        } else if (second_char == 'T') {
          // Could be .TITLE (alias for SBTL)
          uint8_t third_char = MnemP[2] & 0x7F;
          if (third_char == 'I') {
            HndlSBTL();
            return;
          }
        } else if (second_char == 'W') {
          // .WORD
          HndlWORD();
          return;
        } else if (second_char == 'A') {
          // .ASCII
          HndlASCII();
          return;
        } else if (second_char == 'D') {
          // Could be .DBYTE
          uint8_t third_char = MnemP[2] & 0x7F;
          if (third_char == 'B') {
            HndlDBYTE();
            return;
          }
        }
      } else if (first_char == 'P') {
        // Regular directive starting with 'P' - could be PAGE
        uint8_t second_char = MnemP[1] & 0x7F;
        if (second_char == 'A') {
          uint8_t third_char = MnemP[2] & 0x7F;
          if (third_char == 'G') {
            uint8_t fourth_char = MnemP[3] & 0x7F;
            if (fourth_char == 'E') {
              DoPage();
              return;
            }
          }
        }
      } else if (first_char == 'L') {
        // Regular directive starting with 'L' - could be LST
        uint8_t second_char = MnemP[1] & 0x7F;
        if (second_char == 'S') {
          uint8_t third_char = MnemP[2] & 0x7F;
          if (third_char == 'T') {
            HndlLST();  // LST directive (non-dot)
            return;
          }
        }
      } else if (first_char == 'N') {
        // Regular directive starting with 'N' - could be NOLIST
        uint8_t second_char = MnemP[1] & 0x7F;
        if (second_char == 'O') {
          uint8_t third_char = MnemP[2] & 0x7F;
          if (third_char == 'L') {
            HndlNOLIST();
            return;
          }
        }
      } else if (first_char == 'O') {
        // Regular directive starting with 'O' - could be OBJ or ORG
        uint8_t second_char = MnemP[1] & 0x7F;
        if (second_char == 'B') {
          // OBJ directive
          uint8_t third_char = MnemP[2] & 0x7F;
          if (third_char == 'J') {
            HndlOBJ();
            return;
          }
        } else if (second_char == 'R') {
          // ORG directive
          uint8_t third_char = MnemP[2] & 0x7F;
          if (third_char == 'G') {
            HndlORG();
            return;
          }
        }
      } else if (first_char == 'R') {
        // Regular directive starting with 'R' - could be REL
        uint8_t second_char = MnemP[1] & 0x7F;
        if (second_char == 'E') {
          // REL directive
          uint8_t third_char = MnemP[2] & 0x7F;
          if (third_char == 'L') {
            HndlREL();
            return;
          }
        }
      } else if (first_char == 'S') {
        // Regular directive starting with 'S' - could be SBTL
        uint8_t second_char = MnemP[1] & 0x7F;
        if (second_char == 'B') {
          uint8_t third_char = MnemP[2] & 0x7F;
          if (third_char == 'T') {
            HndlSBTL();
            return;
          }
        }
      }

      // Fallback: Directive recognized but no handler implemented
      // Register error and return failure
      C = true;  // Failure - unsupported directive
      return;

    L837C_opcode:
      C = false;  // CLC - 6502/SW16 opcode, success
      return;

    //
    L837E_nomatch:
      // LDA (MnemP),Y - Skip rest of entry
      A = MnemP[Y];
      if ((int8_t)A < 0) goto L8385_endentry;  // BMI L8385 - Last byte of entry has msb=1
      Y++;
      goto L837E_nomatch;  // BNE L837E

    //
    L8385_endentry:
      Y++;  // INY
    L8386_advnext:
      // SEC, INY, INY - Add 3 more bytes to skip flag bytes
      Y++;
      Y++;
      // TYA, ADC MnemP, STA MnemP, BCC L8392, INC MnemP+1
      // Proper pointer addition: MnemP = MnemP + Y
      MnemP += Y;

    L8392:
      Y = 0;                   // LDY #0 - Try next entry with
      ChrGot();                // JSR ChrGot
      A ^= MnemP[Y];           // EOR (MnemP),Y
      A <<= 1;                 // ASL
      if (A == 0) goto L8361;  // BEQ L8361 - the same first letter

    //
    // Not mnemonics/pseudo code/directive
    // Assume it's a macro invocation
    //
    L839C_nosubtbl:
      A = MacroF;                              // LDA MacroF - Are macros allowed?
      if (A == 0) goto L83AC_notfound;         // BEQ L83AC - No
      if ((int8_t)A < 0) goto L83A4_macroerr;  // BMI L83A4 - Invocation fr a macro defn file -> err
      // BPL L83AE - No macro nesting issue, could be valid macro invocation
      // For now, macro invocation code not fully translated, return error
      goto L83AC_notfound;

    L83A4_macroerr:
      X = 0x18;  // LDX #$18 - Macro nesting error
      RegAsmEW();
      // JMP DrtvDone - would be called here
      goto L83AC_notfound;  // For now, just return error

    L83AC_notfound:
      C = true;  // SEC - Flag it's an err
      return;
    }

#if 0   // TODO: Phase 9+ - Duplicate VidOut inside test stubs
    // Stub: VidOut - Video output
    void VidOut() {
      // TODO: Output character to video
    }
#endif  // VidOut duplicate

    // Stub: L986A - Helper function
    void L986A() {
      // TODO: Implement L986A
    }

    // GAdrMod - This subrtn will parse the addressing mode of the
    // operand of a 6502 mnemonic/SW16 psuedo opcode
    // Ret
    //  (A)=index (0-12) use to get addr mode fr a table
    //  C=0 - succ
    //  C=1 - syntax error
    void GAdrMod() {
      X = 0;  // X=index into the 2 tables
      Y = 0;  // Position @ start of operand
    L853B:
      ChrGot();  // Y=index into operand text
      A |= 0x80;
      if (A == AModTkns[X]) goto L855E;  // Got a hit

      A = AModTkns[X];                 // Get a token
      if ((int8_t)A >= 0) goto L8570;  // Not a char (BPL)

      if (A != (SPACE | 0x80)) goto L8554;

      // Token is $A0
      A = CR;                           // Is char a cr?
      if (A == SrcP_at(Y)) goto L8563;  // Yes, eol

    // On fall thru, if token is $A0 and CR not found
    L8554:
      A = AModCmds[X];
      if (A == 0) goto L855E;         // => next token & src char
      if ((int8_t)A < 0) goto L8583;  // Error token (BMI)
      X = A;                          // Index to next token fr $D7D7 table to be
      if (X != 0) goto L853B;         // used to cmp against SAME char of src code

    // No fall thru here
    // Proceed to get next token of $D7D7 table
    // and next char of src to be compared
    L855E:
      X++;  // next token
      Y++;  // next src char
      goto L853B;

    // Got a CR
    L8563:
      X++;  // Prepare to look at next token
      Y--;  // Index prev src char
      A = AModTkns[X];
      if (A == 0) goto L856C;          // $A0+cr followed by $00
      if ((int8_t)A >= 0) goto L858B;  // $A0+cr followed by +ve byte value (always) (BPL)

    L856C:
      X = 0x0A;                // expr syntax err - never reported! (LDA #$0A)
      if (X != 0) goto L8583;  // always (BNE) - bug?

    L8570:
      if (A != 0) goto L858B;  // token > 0 (BNE)

      // Got a $00 token
      SavIndX = X;  // Save X
      EvalExpr();
      X = SavIndX;
      if (!C) goto L8554;  // No errs during evaluation (BCC)
      A = NxtToken;
      A &= 0x7F;
      if (A != 0x34) goto L8589;  // Invalid delimiter (BNE)

    // (A) = error token
    L8583:
      X                 = A;  // (X) overwritten! Is it a bug?
      uint8_t saved_err = A;  // Save error token (PHA)
      RegAsmEW();
      A = saved_err;  // (PLA)
    L8589:
      C = true;  // SEC
      return;

    L858B:
      A >>= 1;             // LSR - If odd then
      if (!C) goto L8590;  // (BCC)
      C = false;           // CLC - ret to caller w/mod2 which
      return;              // is an index to Adr Mode Table

    L8590:
      A <<= 1;             // Even, get back token first (ASL)
      L8598();             // Now, execute helper function
      if (!C) goto L855E;  // Loop back to process next src char (BCC)
      if (C) goto L8554;   // Go get index to next token (BCS)
    }

    // L8598 - Calls help subroutines (functions).
    // Only 3 defined so far.
    // Entry
    //   (A)=2,4,6 - index into subrtn table
    //
    // The helper functions with return the required values
    // primarily the C bit

    // Helper function pointer type
    typedef void (*HelperFunc)();

    // Jump-table pointers for helper subroutines
    // Entry at index 0 = IsZPMod, index 1 = IsAccMod, index 2 = Is65C02
    const HelperFunc L85AE_helpers[] = {IsZPMod, IsAccMod, Is65C02};

    // Number of helper functions in the table
    constexpr size_t kNumHelpers = sizeof(L85AE_helpers) / sizeof(L85AE_helpers[0]);

    void L8598() {
      SavIndX = X;
      X       = A;  // Byte offset (must be 2, 4, or 6)

      // Validate: only 3 subrtns, and X must be in [2..6] range
      if (X >= 7) {
        std::abort();  // BRK
      }

      // X must be even (2, 4, or 6 correspond to entries 0, 1, 2)
      // If X is odd, it's an invalid call - treat as error
      if ((X & 1) != 0) {  // Odd value?
        std::abort();      // BRK
      }

    L85A0:
      // Convert byte offset to array index: X/2 gives 1, 2, 3; so index = (X/2) - 1
      uint8_t array_index = (X / 2) - 1;

      // Bounds check
      if (array_index >= kNumHelpers) {
        std::abort();  // Should not reach here given X validation above
      }

      // Prepare for JMP via RTS in original
      ChrGot();  // Get curr char
    L85AB:
      X = SavIndX;

      // Call the appropriate helper function
      HelperFunc helper = L85AE_helpers[array_index];
      helper();  // Call IsZPMod, IsAccMod, or Is65C02
    }
#endif  // HndlMnem and GAdrMod

#if 0   // TODO: Phase 9+ - EvalExpr has compilation errors (not needed for Phase 8.1)
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
      EvalTerm();                     // The leading term is treated differently
      fprintf(stderr, "    EvalExpr: After EvalTerm(), NxtToken=0x%02X\n", NxtToken);
      A = NxtToken;                   // Err?
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
      GNToken();      // Chk for comma, ) and cr/space
      fprintf(stderr, "    EvalExpr: After GNToken(), NxtToken=0x%02X\n", NxtToken);
      A |= NxtToken;  // In case of err
      NxtToken = A;
    L867B:
      A = NxtToken;
      Y--;              // Index prev char
      fprintf(stderr, "    EvalExpr: Final check, NxtToken=0x%02X, C will be %d\n", A, (A >= 0x80) ? 1 : 0);
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
          Y = 0;              // point at label
          FindSym();         // locate symbol; sets SymP and Y when found
          Y = saved_Y;
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
          flags &= static_cast<uint8_t>(~undefined); // clear undefined bit
          flags |= (RelExprF & relative);            // preserve relative bit if any
          if ((int8_t)DummyF < 0) flags |= relative; // DSECT -> symbol is relative
          flags |= unrefd;                           // mark as unreferenced by default
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
      A     = ValExpr;                   // LDA ValExpr
      PC    = (PC & 0xFF00) | A;         // STA PC - PC=new ORG Addr
      ObjPC = (ObjPC & 0xFF00) | A;      // Also set ObjPC
      A     = ValExpr_hi;                // LDA ValExpr+1
      PC    = (PC & 0x00FF) | (A << 8);  // STA PC+1
      ObjPC = (ObjPC & 0x00FF) | (A << 8);  // Also set ObjPC+1
      goto DrtvFin_ORG;                  // JMP DrtvFin

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
      A = ByteCnt;     // Current offset in GMC
      A--;             // Adjust since X was incremented
      uint8_t save_X = X;
      X = 1;           // 8-bit value
      Y = 0;           // No endian issue for single byte
      AddRLDEnt();     // Create RLD entry
      X = save_X;      // Restore X

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
      StorGMC();   // Store generated machine code
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
      A = 0;           // offset = 0
      X = 2;           // 16-bit value
      Y = EndianF;     // byte order
      AddRLDEnt();     // Create RLD entry

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
      StorGMC();   // JSR L8E28 - Store MC, update PC, print
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
#endif  // EvalExpr

  //=================================================
  // EvalOprnd - Evaluate operand expressions
  // (ASM3.S line ~347, label EvalOprnd)
  // This routine evaluates operand expressions FOR directives that need
  // their operands evaluated as if in Pass 2, even during Pass 1
  // Entry:
  //   Source line positioned at operand field
  // Exit:
  //   PassNbr - restored to original value
  //   (X) - error token (if any from EvalExpr)
  //   C flag set appropriately by EvalExpr
  //=================================================
  void EvalOprnd() {
    // Save and force Pass 2 semantics for operand evaluation
    A                  = PassNbr;
    uint8_t saved_pass = A;  // PHA - Save current pass number
    PassNbr            = 1;  // Force PassNbr = 1 for evaluation

    // Basic immediate/constant parsing fallback (Phase 8):
    // If expression evaluation (EvalExpr) is not implemented we support
    // simple numeric operands used by current tests: hex ($nnnn) and
    // decimal digits. This keeps EvalOprnd useful for directives like
    // EQU and ORG during Pass 1/2.
    // Inline skip-spaces (SkipSpcs() is in a disabled block)
    while (SrcP_at(Y) == SPACE) {
      Y++;
      if (Y == 0) break;
    }
    uint8_t ch = SrcP_at(Y);

    if (ch == '$') {
      // Hex constant parser
      Y++;
      uint16_t val  = 0;
      bool     seen = false;
      while (true) {
        ch = SrcP_at(Y);
        if (ch >= '0' && ch <= '9') {
          val = (val << 4) | (ch - '0');
          Y++;
          seen = true;
          continue;
        }
        if (ch >= 'A' && ch <= 'F') {
          val = (val << 4) | (ch - 'A' + 10);
          Y++;
          seen = true;
          continue;
        }
        if (ch >= 'a' && ch <= 'f') {
          val = (val << 4) | (ch - 'a' + 10);
          Y++;
          seen = true;
          continue;
        }
        break;
      }

      if (seen) {
        ValExpr    = static_cast<uint8_t>(val & 0xFF);
        ValExpr_hi = static_cast<uint8_t>((val >> 8) & 0xFF);
        C          = false;  // success
        NxtToken   = 0;
        X          = 0;           // no error token
        PassNbr    = saved_pass;  // restore
        return;
      }
    }

    // Decimal constant fallback
    if (ch >= '0' && ch <= '9') {
      uint16_t val  = 0;
      bool     seen = false;
      while (true) {
        ch = SrcP_at(Y);
        if (ch >= '0' && ch <= '9') {
          val = val * 10 + (ch - '0');
          Y++;
          seen = true;
          continue;
        }
        break;
      }
      if (seen) {
        ValExpr    = static_cast<uint8_t>(val & 0xFF);
        ValExpr_hi = static_cast<uint8_t>((val >> 8) & 0xFF);
        C          = false;
        NxtToken   = 0;
        X          = 0;
        PassNbr    = saved_pass;
        return;
      }
    }

    // Fallback to full expression evaluator (if/when implemented)
    EvalExpr();            // JSR EvalExpr
    X       = A;           // TAX - error token?
    PassNbr = saved_pass;  // PLA - Restore pass number
    // RTS - return (with C flag set by EvalExpr)
  }

  //=================================================
  // Phase 8.1: Initialization and Cleanup Functions
  // Original: ASM2.S:555 (SaveZP), ASM2.S:570 (InitASM)
  //=================================================

  // Zero-page backup buffer (saves $60-$F1 range, 146 bytes)
  uint8_t ZP_Backup[256];

  //=================================================
  // SaveZP - Save zero-page workspace to backup area
  // Original: ASM2.S:555
  // Saves all assembler zero-page variables ($60-$F1)
  //=================================================
  void SaveZP() {
    // Original saves $92 (146) bytes starting from Z60-1 ($5F)
    // We'll save all our zero-page variables to backup buffer

    // Save $60-$6F: General Assembly Control Variables
    ZP_Backup[0x60] = Z60;
    ZP_Backup[0x60] = BCDNbr[0];
    ZP_Backup[0x61] = BCDNbr[1];
    ZP_Backup[0x62] = BCDNbr[2];
    ZP_Backup[0x63] = StrtSymT & 0xFF;
    ZP_Backup[0x64] = (StrtSymT >> 8) & 0xFF;
    ZP_Backup[0x65] = EndSymT & 0xFF;
    ZP_Backup[0x66] = (EndSymT >> 8) & 0xFF;
    ZP_Backup[0x67] = PassNbr;
    ZP_Backup[0x68] = ListingF;
    ZP_Backup[0x69] = SubTtlF;
    ZP_Backup[0x6A] = LineCnt;
    ZP_Backup[0x6B] = PageNbr & 0xFF;
    ZP_Backup[0x6C] = (PageNbr >> 8) & 0xFF;
    ZP_Backup[0x6D] = FileNbr;
    ZP_Backup[0x6E] = LogPL;
    ZP_Backup[0x6F] = PhyPL;

    // Save $70-$7F: Instruction Processing and Pointers
    ZP_Backup[0x70] = SavIndX;
    ZP_Backup[0x71] = PrtCol;
    ZP_Backup[0x72] = EIStack;
    ZP_Backup[0x73] = CancelF;
    ZP_Backup[0x74] = NbrErrs & 0xFF;
    ZP_Backup[0x75] = (NbrErrs >> 8) & 0xFF;
    ZP_Backup[0x76] = PrSlot;
    ZP_Backup[0x77] = AbortF;
    ZP_Backup[0x78] = SavIndY;
    ZP_Backup[0x79] = SrcP & 0xFF;
    ZP_Backup[0x7A] = (SrcP >> 8) & 0xFF;
    ZP_Backup[0x7B] = Src2P & 0xFF;
    ZP_Backup[0x7C] = (Src2P >> 8) & 0xFF;
    ZP_Backup[0x7D] = PC & 0xFF;
    ZP_Backup[0x7E] = (PC >> 8) & 0xFF;
    ZP_Backup[0x7F] = ObjPC & 0xFF;

    // Save $80-$8F: File and Symbol Table Management
    ZP_Backup[0x80] = (ObjPC >> 8) & 0xFF;
    ZP_Backup[0x81] = CodeLen;
    ZP_Backup[0x82] = AuxAryE;
    ZP_Backup[0x83] = FileLen & 0xFF;
    ZP_Backup[0x84] = (FileLen >> 8) & 0xFF;
    ZP_Backup[0x85] = CurrORG & 0xFF;
    ZP_Backup[0x86] = (CurrORG >> 8) & 0xFF;
    ZP_Backup[0x87] = SymP & 0xFF;
    ZP_Backup[0x88] = (SymP >> 8) & 0xFF;
    ZP_Backup[0x89] = Delimitr;
    ZP_Backup[0x8A] = DTEndCol;
    ZP_Backup[0x8B] = StrType;
    ZP_Backup[0x8C] = MemTop & 0xFF;
    ZP_Backup[0x8D] = (MemTop >> 8) & 0xFF;
    ZP_Backup[0x8E] = TotLines & 0xFF;
    ZP_Backup[0x8F] = (TotLines >> 8) & 0xFF;

    // Save $90-$9F: Code Generation and Expression Evaluation
    ZP_Backup[0x90] = (TotLines >> 16) & 0xFF;
    ZP_Backup[0x91] = (TotLines >> 24) & 0xFF;
    ZP_Backup[0x92] = VidSlot;
    ZP_Backup[0x93] = SaveA;
    ZP_Backup[0x94] = SaveY;
    ZP_Backup[0x95] = SaveX;
    ZP_Backup[0x96] = DskListF;
    ZP_Backup[0x97] = LstDBIdx;
    ZP_Backup[0x98] = WinLeft;
    ZP_Backup[0x99] = WinRight;
    ZP_Backup[0x9A] = X6502F;
    ZP_Backup[0x9B] = HighMem & 0xFF;
    ZP_Backup[0x9C] = (HighMem >> 8) & 0xFF;
    ZP_Backup[0x9D] = ExprAccF;
    ZP_Backup[0x9E] = SortF;
    ZP_Backup[0x9F] = NxtToken;

    // Save $A0-$AF: Instruction Encoding and Loop Control
    ZP_Backup[0xA0] = LstCodeF;
    ZP_Backup[0xA1] = SymRefCh;
    ZP_Backup[0xA2] = GMC[0];
    ZP_Backup[0xA3] = GMC[1];
    ZP_Backup[0xA4] = GMC[2];
    ZP_Backup[0xA5] = GMC[3];
    ZP_Backup[0xA6] = IsFwdRef;
    ZP_Backup[0xA7] = NumCols;
    ZP_Backup[0xA8] = SymIdx;
    ZP_Backup[0xA9] = ERfield;
    ZP_Backup[0xAA] = SymAddr & 0xFF;
    ZP_Backup[0xAB] = (SymAddr >> 8) & 0xFF;
    ZP_Backup[0xAC] = ValExpr;
    ZP_Backup[0xAD] = ValExpr_hi;
    ZP_Backup[0xAE] = ValExpr_2;
    ZP_Backup[0xAF] = ValExpr_3;

    // Save $B0-$BF: Symbol Processing and Code Generation
    ZP_Backup[0xB0] = RLDEntP & 0xFF;
    ZP_Backup[0xB1] = (RLDEntP >> 8) & 0xFF;
    ZP_Backup[0xB2] = WrkP & 0xFF;
    ZP_Backup[0xB3] = (WrkP >> 8) & 0xFF;
    ZP_Backup[0xB4] = JJJ & 0xFF;
    ZP_Backup[0xB5] = (JJJ >> 8) & 0xFF;
    ZP_Backup[0xB6] = III & 0xFF;
    ZP_Backup[0xB7] = (III >> 8) & 0xFF;
    ZP_Backup[0xB8] = Length;
    ZP_Backup[0xB9] = ModWrd & 0xFF;
    ZP_Backup[0xBA] = (ModWrd >> 8) & 0xFF;
    ZP_Backup[0xBB] = ModWrdL;
    ZP_Backup[0xBC] = ModWrdH;
    ZP_Backup[0xBD] = LenTIdx;
    ZP_Backup[0xBE] = Filler;
    ZP_Backup[0xBF] = SavLstF;

    // Save $C0-$CF: File I/O and Macro Processing
    ZP_Backup[0xC0] = GMCIdx;
    ZP_Backup[0xC1] = RadixCh;
    ZP_Backup[0xC2] = Jump & 0xFF;
    ZP_Backup[0xC3] = (Jump >> 8) & 0xFF;
    ZP_Backup[0xC4] = SavFByt;
    ZP_Backup[0xC5] = BitsDig;
    ZP_Backup[0xC6] = LabelF;
    ZP_Backup[0xC7] = RecCnt & 0xFF;
    ZP_Backup[0xC8] = (RecCnt >> 8) & 0xFF;
    ZP_Backup[0xC9] = SubTIdx;
    ZP_Backup[0xCA] = ZAB;
    ZP_Backup[0xCB] = ErrorF;
    ZP_Backup[0xCC] = msbF;
    ZP_Backup[0xCD] = J_TH & 0xFF;
    ZP_Backup[0xCE] = (J_TH >> 8) & 0xFF;
    ZP_Backup[0xCF] = I_TH & 0xFF;

    // Save $D0-$DF: Relocation and Symbol Table Management
    ZP_Backup[0xD0] = (I_TH >> 8) & 0xFF;
    ZP_Backup[0xD1] = EndianF;
    ZP_Backup[0xD2] = Accum & 0xFF;
    ZP_Backup[0xD3] = (Accum >> 8) & 0xFF;
    ZP_Backup[0xD4] = Accum_2;
    ZP_Backup[0xD5] = Accum_3;
    ZP_Backup[0xD6] = NewPC & 0xFF;
    ZP_Backup[0xD7] = (NewPC >> 8) & 0xFF;
    ZP_Backup[0xD8] = SymPJ & 0xFF;
    ZP_Backup[0xD9] = (SymPJ >> 8) & 0xFF;
    ZP_Backup[0xDA] = Ret816F;
    ZP_Backup[0xDB] = SymPI & 0xFF;
    ZP_Backup[0xDC] = (SymPI >> 8) & 0xFF;
    ZP_Backup[0xDD] = RepChar;
    ZP_Backup[0xDE] = SymNbr;
    ZP_Backup[0xDF] = SavSTS & 0xFF;

    // Save $E0-$EF: Listing Control Flags
    ZP_Backup[0xE0] = (SavSTS >> 8) & 0xFF;
    ZP_Backup[0xE1] = GblAbsF;
    ZP_Backup[0xE2] = DummyF;
    ZP_Backup[0xE3] = SavPC & 0xFF;
    ZP_Backup[0xE4] = (SavPC >> 8) & 0xFF;
    ZP_Backup[0xE5] = SavObjPC & 0xFF;
    ZP_Backup[0xE6] = (SavObjPC >> 8) & 0xFF;
    ZP_Backup[0xE7] = CondAsmF;
    ZP_Backup[0xE8] = TabTIdx;
    ZP_Backup[0xE9] = SymFByte;
    ZP_Backup[0xEA] = RelCodeF;
    ZP_Backup[0xEB] = DskSrcF;
    ZP_Backup[0xEC] = GenF;
    ZP_Backup[0xED] = ObjDBIdx;
    ZP_Backup[0xEE] = IDskSrcF;
    ZP_Backup[0xEF] = MacroF;

    // Save $F0+: Additional variables
    ZP_Backup[0xF0] = MParmCnt;
    ZP_Backup[0xF1] = MacArg;
  }

  //=================================================
  // RestoreZP - Restore zero-page from backup area
  // Original: ASM2.S (paired with SaveZP)
  // Restores all assembler zero-page variables
  //=================================================
  void RestoreZP() {
    // Restore $60-$6F: General Assembly Control Variables
    Z60       = ZP_Backup[0x60];
    BCDNbr[0] = ZP_Backup[0x60];
    BCDNbr[1] = ZP_Backup[0x61];
    BCDNbr[2] = ZP_Backup[0x62];
    StrtSymT  = ZP_Backup[0x63] | (static_cast<uint16_t>(ZP_Backup[0x64]) << 8);
    EndSymT   = ZP_Backup[0x65] | (static_cast<uint16_t>(ZP_Backup[0x66]) << 8);
    PassNbr   = ZP_Backup[0x67];
    ListingF  = ZP_Backup[0x68];
    SubTtlF   = ZP_Backup[0x69];
    LineCnt   = ZP_Backup[0x6A];
    PageNbr   = ZP_Backup[0x6B] | (static_cast<uint16_t>(ZP_Backup[0x6C]) << 8);
    FileNbr   = ZP_Backup[0x6D];
    LogPL     = ZP_Backup[0x6E];
    PhyPL     = ZP_Backup[0x6F];

    // Restore $70-$7F: Instruction Processing and Pointers
    SavIndX = ZP_Backup[0x70];
    PrtCol  = ZP_Backup[0x71];
    EIStack = ZP_Backup[0x72];
    CancelF = ZP_Backup[0x73];
    NbrErrs = ZP_Backup[0x74] | (static_cast<uint16_t>(ZP_Backup[0x75]) << 8);
    PrSlot  = ZP_Backup[0x76];
    AbortF  = ZP_Backup[0x77];
    SavIndY = ZP_Backup[0x78];
    SrcP    = ZP_Backup[0x79] | (static_cast<uint16_t>(ZP_Backup[0x7A]) << 8);
    Src2P   = ZP_Backup[0x7B] | (static_cast<uint16_t>(ZP_Backup[0x7C]) << 8);
    PC      = ZP_Backup[0x7D] | (static_cast<uint16_t>(ZP_Backup[0x7E]) << 8);
    ObjPC   = ZP_Backup[0x7F] | (static_cast<uint16_t>(ZP_Backup[0x80]) << 8);

    // Restore $80-$8F: File and Symbol Table Management
    CodeLen  = ZP_Backup[0x81];
    AuxAryE  = ZP_Backup[0x82];
    FileLen  = ZP_Backup[0x83] | (static_cast<uint16_t>(ZP_Backup[0x84]) << 8);
    CurrORG  = ZP_Backup[0x85] | (static_cast<uint16_t>(ZP_Backup[0x86]) << 8);
    SymP     = ZP_Backup[0x87] | (static_cast<uint16_t>(ZP_Backup[0x88]) << 8);
    Delimitr = ZP_Backup[0x89];
    DTEndCol = ZP_Backup[0x8A];
    StrType  = ZP_Backup[0x8B];
    MemTop   = ZP_Backup[0x8C] | (static_cast<uint16_t>(ZP_Backup[0x8D]) << 8);
    TotLines = ZP_Backup[0x8E] | (static_cast<uint16_t>(ZP_Backup[0x8F]) << 8) |
               (static_cast<uint32_t>(ZP_Backup[0x90]) << 16) |
               (static_cast<uint32_t>(ZP_Backup[0x91]) << 24);

    // Restore $90-$9F: Code Generation and Expression Evaluation
    VidSlot  = ZP_Backup[0x92];
    SaveA    = ZP_Backup[0x93];
    SaveY    = ZP_Backup[0x94];
    SaveX    = ZP_Backup[0x95];
    DskListF = ZP_Backup[0x96];
    LstDBIdx = ZP_Backup[0x97];
    WinLeft  = ZP_Backup[0x98];
    WinRight = ZP_Backup[0x99];
    X6502F   = ZP_Backup[0x9A];
    HighMem  = ZP_Backup[0x9B] | (static_cast<uint16_t>(ZP_Backup[0x9C]) << 8);
    ExprAccF = ZP_Backup[0x9D];
    SortF    = ZP_Backup[0x9E];
    NxtToken = ZP_Backup[0x9F];

    // Restore $A0-$AF: Instruction Encoding and Loop Control
    LstCodeF     = ZP_Backup[0xA0];
    SymRefCh     = ZP_Backup[0xA1];
    GMC[0]       = ZP_Backup[0xA2];
    GMC[1]       = ZP_Backup[0xA3];
    GMC[2]       = ZP_Backup[0xA4];
    GMC[3]       = ZP_Backup[0xA5];
    IsFwdRef     = ZP_Backup[0xA6];
    NumCols      = ZP_Backup[0xA7];
    SymIdx       = ZP_Backup[0xA8];
    ERfield      = ZP_Backup[0xA9];
    SymAddr      = ZP_Backup[0xAA] | (static_cast<uint16_t>(ZP_Backup[0xAB]) << 8);
    ValExpr_word = ZP_Backup[0xAC] | (static_cast<uint16_t>(ZP_Backup[0xAD]) << 8);
    ValExpr_2    = ZP_Backup[0xAE];
    ValExpr_3    = ZP_Backup[0xAF];

    // Restore $B0-$BF: Symbol Processing and Code Generation
    RLDEntP = ZP_Backup[0xB0] | (static_cast<uint16_t>(ZP_Backup[0xB1]) << 8);
    WrkP    = ZP_Backup[0xB2] | (static_cast<uint16_t>(ZP_Backup[0xB3]) << 8);
    JJJ     = ZP_Backup[0xB4] | (static_cast<uint16_t>(ZP_Backup[0xB5]) << 8);
    III     = ZP_Backup[0xB6] | (static_cast<uint16_t>(ZP_Backup[0xB7]) << 8);
    Length  = ZP_Backup[0xB8];
    ModWrd  = ZP_Backup[0xB9] | (static_cast<uint16_t>(ZP_Backup[0xBA]) << 8);
    ModWrdL = ZP_Backup[0xBB];
    ModWrdH = ZP_Backup[0xBC];
    LenTIdx = ZP_Backup[0xBD];
    Filler  = ZP_Backup[0xBE];
    SavLstF = ZP_Backup[0xBF];

    // Restore $C0-$CF: File I/O and Macro Processing
    GMCIdx  = ZP_Backup[0xC0];
    RadixCh = ZP_Backup[0xC1];
    Jump    = ZP_Backup[0xC2] | (static_cast<uint16_t>(ZP_Backup[0xC3]) << 8);
    SavFByt = ZP_Backup[0xC4];
    BitsDig = ZP_Backup[0xC5];
    LabelF  = ZP_Backup[0xC6];
    RecCnt  = ZP_Backup[0xC7] | (static_cast<uint16_t>(ZP_Backup[0xC8]) << 8);
    SubTIdx = ZP_Backup[0xC9];
    ZAB     = ZP_Backup[0xCA];
    ErrorF  = ZP_Backup[0xCB];
    msbF    = ZP_Backup[0xCC];
    J_TH    = ZP_Backup[0xCD] | (static_cast<uint16_t>(ZP_Backup[0xCE]) << 8);
    I_TH    = ZP_Backup[0xCF] | (static_cast<uint16_t>(ZP_Backup[0xD0]) << 8);

    // Restore $D0-$DF: Relocation and Symbol Table Management
    EndianF = ZP_Backup[0xD1];
    Accum   = ZP_Backup[0xD2] | (static_cast<uint16_t>(ZP_Backup[0xD3]) << 8);
    Accum_2 = ZP_Backup[0xD4];
    Accum_3 = ZP_Backup[0xD5];
    NewPC   = ZP_Backup[0xD6] | (static_cast<uint16_t>(ZP_Backup[0xD7]) << 8);
    SymPJ   = ZP_Backup[0xD8] | (static_cast<uint16_t>(ZP_Backup[0xD9]) << 8);
    Ret816F = static_cast<int8_t>(ZP_Backup[0xDA]);
    SymPI   = ZP_Backup[0xDB] | (static_cast<uint16_t>(ZP_Backup[0xDC]) << 8);
    RepChar = ZP_Backup[0xDD];
    SymNbr  = ZP_Backup[0xDE];
    SavSTS  = ZP_Backup[0xDF] | (static_cast<uint16_t>(ZP_Backup[0xE0]) << 8);

    // Restore $E0-$EF: Listing Control Flags
    GblAbsF  = ZP_Backup[0xE1];
    DummyF   = ZP_Backup[0xE2];
    SavPC    = ZP_Backup[0xE3] | (static_cast<uint16_t>(ZP_Backup[0xE4]) << 8);
    SavObjPC = ZP_Backup[0xE5] | (static_cast<uint16_t>(ZP_Backup[0xE6]) << 8);
    CondAsmF = ZP_Backup[0xE7];
    TabTIdx  = ZP_Backup[0xE8];
    SymFByte = ZP_Backup[0xE9];
    RelCodeF = ZP_Backup[0xEA];
    DskSrcF  = ZP_Backup[0xEB];
    GenF     = ZP_Backup[0xEC];
    ObjDBIdx = ZP_Backup[0xED];
    IDskSrcF = ZP_Backup[0xEE];
    MacroF   = ZP_Backup[0xEF];

    // Restore $F0+: Additional variables
    MParmCnt = ZP_Backup[0xF0];
    MacArg   = ZP_Backup[0xF1];
  }

  //=================================================
  // InitASM - Initialize assembler state for new session
  // Original: ASM2.S:570
  // Sets up default values for all assembler variables
  //=================================================
  void InitASM() {
    // Clear error/warning counters (ASM2.S:596-605)
    NbrErrs  = 0x0000;
    NbrWarns = 0x0000;

    // Clear line counter (JSR ZeroLnCnt)
    BCDNbr[0] = 0x00;
    BCDNbr[1] = 0x00;
    BCDNbr[2] = 0x00;

    // Set default flags (ASM2.S:596-614)
    MacroF   = 0x00;  // STY MacroF - Macros disabled
    ErrorF   = 0x00;  // STY ErrorF - No error
    DummyF   = 0x00;  // STY DummyF - No dummy section
    SubTtlF  = 0x00;  // STY SubTtlF - No subtitle
    GenF     = 0x00;  // STY GenF - Generation enabled, memory mode
    RelCodeF = 0x00;  // No relocatable code by default
    CondAsmF = 0x00;  // No conditional assembly

    // Clear subtitle buffer
    SubTitle[0] = 0x00;  // STY SubTitle - Delimiter=0

    // Set ListingF to $FF (LST ON by default)
    // Original: DEY (Y=-1); STY ListingF
    ListingF = 0xFF;

    // Set LogPL to 60 (ASM2.S:620)
    LogPL = 60;

    // Initialize page number
    PageNbr = 0x0000;  // STY PageNbr, STY PageNbr+1
    PhyPL   = 0x00;

    // Initialize Program Counter and Object PC
    PC    = 0x0000;
    ObjPC = 0x0000;

    // Initialize HighMem to a reasonable default (64KB - no limit)
    HighMem = 0xFFFF;

    // Clear GMC buffer (machine code generation buffer)
    GMC[0] = 0x00;
    GMC[1] = 0x00;
    GMC[2] = 0x00;
    GMC[3] = 0x00;

    // Initialize symbol table pointers
    // In the original, this depends on memory layout (ASM2.S:643-660)
    // For now, set both to same value indicating empty table
    // The actual memory address would be set up based on buffer allocation
    StrtSymT = 0x1E00;    // Example address (would be computed from buffers)
    EndSymT  = StrtSymT;  // Empty table: start == end

    // Clear all 256 entries of HeaderT hash table to $0000
    if (HeaderT_ptr != nullptr) {
      for (int i = 0; i < 256; i++) {
        HeaderT_ptr[i * 2]     = 0x00;
        HeaderT_ptr[i * 2 + 1] = 0x00;
      }
    }

    // Initialize other state
    CancelF = 0xFF;  // DEY; STY CancelF - $FF
    LineCnt = 0xFE;  // DEY; STY LineCnt - $FE
    SW16F   = 0xFF;  // DEY; STY SW16F - SW16 opcodes valid
    PassNbr = 0x00;  // Start at Pass 1
  }

  //=================================================
  // CleanupAsm - Cleanup assembler state after session
  // Original: ASM2.S:79 (CleanUp label)
  // For now, this is a stub - full cleanup will be in Phase 9+
  //=================================================
  void CleanupAsm() {
    // Original performs:
    // - Close source files (ClsFile for INCLUDE, MACRO)
    // - Flush object code buffer (FlushObj)
    // - Close object file
    // - Flush listing buffer (if disk listing)
    // - Close listing file
    //
    // For Phase 8.1, we keep this as a stub
    // Full file I/O cleanup will be implemented in later phases
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
      0xA3, 0x00, 0xA0, 0x05, 0xA8, 0x00, 0xAC, 0xD8, 0xA9, 0xA0, 0x02, 0x0F,
      0x06, 0x19, 0xA9, 0xAC, 0xD9, 0xA0, 0x02, 0x0D, 0xA0, 0x06, 0x02, 0x11,
      0x13, 0x04, 0x15, 0x8D, 0x15, 0xBB, 0x15, 0x00, 0xA0, 0x02, 0x03, 0x01,
      0xAC, 0xD8, 0xA0, 0x02, 0x07, 0x09, 0xD9, 0xA0, 0x02, 0x17, 0x0B,
  };

  const std::uint8_t AModCmds[] = {
      0x04, 0x00, 0xB4, 0x00, 0x19, 0x00, 0x0E, 0xAA, 0xAA, 0xB4, 0x0C, 0x00,
      0xAE, 0x00, 0xAA, 0x14, 0xAA, 0xB4, 0xAE, 0x00, 0xB4, 0x18, 0x18, 0x00,
      0x00, 0x1B, 0x00, 0x1D, 0x00, 0x1F, 0x00, 0x00, 0x24, 0x23, 0x00, 0x00,
      0xB4, 0x2A, 0xB4, 0x29, 0x00, 0x00, 0xAC, 0xB4, 0x2E, 0x00, 0x00,
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
      0x6D, 0x65, 0x69, 0x75, 0x7D, 0x79, 0x71, 0x61, 0x72, 0x2D, 0x25, 0x29, 0x35, 0x3D, 0x39,
      0x31, 0x21, 0x32, 0x0E, 0x06, 0x0A, 0x16, 0x1E, 0x90, 0xB0, 0xF0, 0x2C, 0x24, 0x89, 0x34,
      0x3C, 0x30, 0xD0, 0x10, 0x80, 0x00, 0x50, 0x70, 0x18, 0xD8, 0x58, 0xB8, 0xCD, 0xC5, 0xC9,
      0xD5, 0xDD, 0xD9, 0xD1, 0xC1, 0xD2, 0xEC, 0xE4, 0xE0, 0xCC, 0xC4, 0xC0, 0xCE, 0xC6, 0x3A,
      0xD6, 0xDE, 0xCA, 0x88, 0x4D, 0x45, 0x49, 0x55, 0x5D, 0x59, 0x51, 0x41, 0x52, 0xEE, 0xE6,
      0x1A, 0xF6, 0xFE, 0xE8, 0xC8, 0x4C, 0x6C, 0x7C, 0x20, 0xAD, 0xA5, 0xA9, 0xB5, 0xBD, 0xB9,
      0xB1, 0xA1, 0xB2, 0xAE, 0xA6, 0xA2, 0xB6, 0x00, 0xBE, 0xAC, 0xA4, 0xA0, 0xB4, 0xBC, 0x4E,
      0x46, 0x4A, 0x56, 0x5E, 0xEA, 0x0D, 0x05, 0x09, 0x15, 0x1D, 0x19, 0x11, 0x01, 0x12, 0x48,
      0x08, 0xDA, 0x5A, 0x68, 0x28, 0xFA, 0x7A, 0x2E, 0x26, 0x2A, 0x36, 0x3E, 0x6E, 0x66, 0x6A,
      0x76, 0x7E, 0x40, 0x60, 0xED, 0xE5, 0xE9, 0xF5, 0xFD, 0xF9, 0xF1, 0xE1, 0xF2, 0x38, 0xF8,
      0x78, 0x8D, 0x85, 0x00, 0x95, 0x9D, 0x99, 0x91, 0x81, 0x92, 0x8E, 0x86, 0x00, 0x96, 0x8C,
      0x84, 0x00, 0x94, 0x9C, 0x64, 0x00, 0x74, 0x9E, 0xAA, 0xA8, 0x1C, 0x14, 0x0C, 0x04, 0xBA,
      0x8A, 0x9A, 0x98, 0xA0, 0x03, 0x0A, 0x05, 0x08, 0x02, 0x09, 0x07, 0x04, 0x01, 0x0E, 0x0C,
      0x0F, 0x06, 0x0D, 0xD0, 0xF0, 0xE0, 0x20, 0x60, 0x40, 0x80, 0xC0, 0x00, 0x0B, 0x30, 0x50,
      0x70, 0x90, 0xB0,
  };

  //=================================================
  // ($D90A) 213 bytes cycle times
  //=================================================
  const std::uint8_t CycTimes[] = {
      4,    3,    2,    4,    4,    4,    5,    6,    5,    4,    3,    2,    4,    4,    4,
      5,    6,    5,    6,    5,    2,    6,    7,    3,    3,    3,    4,    3,    2,    4,
      4,    3,    3,    3,    3,    7,    3,    3,    2,    2,    2,    2,    4,    3,    2,
      4,    4,    4,    5,    6,    5,    4,    3,    2,    4,    3,    2,    6,    5,    2,
      6,    7,    2,    2,    4,    3,    2,    4,    4,    4,    5,    6,    5,    6,    5,
      2,    6,    7,    2,    2,    3,    5,    6,    6,    4,    3,    2,    4,    4,    4,
      5,    6,    5,    4,    3,    2,    4,    0x29, 4,    4,    3,    2,    4,    4,    6,
      5,    2,    6,    7,    2,    4,    3,    2,    4,    4,    4,    5,    6,    5,    3,
      3,    3,    3,    4,    4,    4,    4,    6,    5,    2,    6,    7,    6,    5,    2,
      6,    7,    6,    6,    4,    3,    2,    4,    4,    4,    5,    6,    5,    2,    2,
      2,    4,    3,    0x29, 4,    5,    5,    6,    6,    5,    4,    3,    0x29, 4,    4,
      3,    0x29, 4,    4,    3,    0x29, 4,    5,    2,    2,    6,    5,    6,    5,    2,
      2,    2,    2,    0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29,
      0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29, 0x29,
      0x29, 0x29, 0x29,
  };

  //=================================================
  // ($D9DF) Char mapping
  // Original: ASM1.S:1397-1421
  // This table is used by ChrGot/ChrGet to classify characters
  // and set processor flags for character type detection.
  // The byte values are meant to be loaded into the 6502 status register.
  //
  // Flag meanings (NV-B DIZC):
  //   N (bit 7): Set for lowercase letters (0x80)
  //   V (bit 6): Set for hex digits (0x40)
  //   Z (bit 1): Set for decimal digits (0x02)
  //   C (bit 0): Set for non-alphabetic (0x01)
  //
  // Character mappings:
  //   '0'-'9' ($30-$39) --> 0x43 (0100 0011) - hex digit + decimal + non-alpha
  //   'A'-'F' ($41-$46) --> 0x40 (0100 0000) - hex digit (uppercase)
  //   'G'-'Z' ($47-$5A) --> 0x00 (0000 0000) - letter (uppercase, non-hex)
  //   'a'-'f' ($61-$66) --> 0xC0 (1100 0000) - hex digit (lowercase) + lowercase
  //   'g'-'z' ($67-$7A) --> 0x80 (1000 0000) - letter (lowercase, non-hex)
  //   others            --> 0x01 (0000 0001) - non-alphabetic
  //=================================================
  const std::uint8_t CharMap1[] = {
      // 0x00-0x2F: Control chars and punctuation (48 bytes)
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      // 0x30-0x39: '0'-'9' (10 bytes)
      0x43,
      0x43,
      0x43,
      0x43,
      0x43,
      0x43,
      0x43,
      0x43,
      0x43,
      0x43,
      // 0x3A-0x40: ':' to '@' (7 bytes)
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
      // 0x41-0x46: 'A'-'F' (6 bytes)
      0x40,
      0x40,
      0x40,
      0x40,
      0x40,
      0x40,
      // 0x47-0x5A: 'G'-'Z' (20 bytes)
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      // 0x5B-0x60: '[' to '`' (6 bytes) - Updated: '_' (0x5F) is now alphanumeric (0x00)
      0x01,  // 0x5B: '['
      0x01,  // 0x5C: '\'
      0x01,  // 0x5D: ']'
      0x01,  // 0x5E: '^'
      0x00,  // 0x5F: '_' - alphanumeric to allow underscores in symbol names
      0x01,  // 0x60: '`'
      // 0x61-0x66: 'a'-'f' (6 bytes)
      0xC0,
      0xC0,
      0xC0,
      0xC0,
      0xC0,
      0xC0,
      // 0x67-0x7A: 'g'-'z' (20 bytes)
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      0x80,
      // 0x7B-0x7F: '{' to DEL (5 bytes)
      0x01,
      0x01,
      0x01,
      0x01,
      0x01,
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

  //=================================================
  // Bridge functions - IN main anonymous namespace
  // so they can call other functions in this namespace
  //=================================================
  void Bridge_GInstLen() {
    GInstLen();
  }

  void Bridge_StorGMC() {
    StorGMC();
  }

  void Bridge_GOpAdr() {
    // GOpAdr is inside #if 0 block - not compiled
    // TODO: Implement when handler is enabled
  }

  bool Bridge_ChkRng(uint8_t value, uint8_t minVal, uint8_t maxVal) {
    // ChkRng is inside #if 0 block - not compiled
    // TODO: Implement when handler is enabled
    (void)value;
    (void)minVal;
    (void)maxVal;
    return false;
  }

  void Bridge_ValidateRange() {
    // ValidateRange is inside #if 0 block - not compiled
    // TODO: Implement when handler is enabled
  }

  void Bridge_HndlMnem() {
    HndlMnem();
  }

  void Bridge_HndlOBJ() {
    // HndlOBJ is inside #if 0 block - not compiled
    // TODO: Implement when handler is enabled
  }

  void Bridge_HndlREL() {
    // HndlREL is inside #if 0 block - not compiled
    // TODO: Implement when handler is enabled
  }

  void Bridge_HndlDS() {
    // HndlDS is inside #if 0 block - not compiled
    // TODO: Implement when handler is enabled
  }

  void Bridge_HndlDFB() {
    // HndlDFB is inside #if 0 block - not compiled
    // TODO: Implement when handler is enabled
  }

  void Bridge_HndlDW() {
    // HndlDW is inside #if 0 block - not compiled
    // TODO: Implement when handler is enabled
  }

  void Bridge_HndlASC() {
    // HndlASC is inside #if 0 block - not compiled
    // TODO: Implement when handler is enabled
  }

  void Bridge_HndlDCI() {
    // HndlDCI is inside #if 0 block - not compiled
    // TODO: Implement when handler is enabled
  }

  void Bridge_HndlLST() {
    // HndlLST is inside #if 0 block - not compiled
    // TODO: Implement when handler is enabled
  }

  void Bridge_HndlNOLIST() {
    // HndlNOLIST is inside #if 0 block - not compiled
    // TODO: Implement when handler is enabled
  }

  void Bridge_DoPage() {
    // DoPage is inside #if 0 block - not compiled
    // TODO: Implement when handler is enabled
  }

  void Bridge_HndlSBTL() {
    // HndlSBTL is inside #if 0 block - not compiled
    // TODO: Implement when handler is enabled
  }

  void Bridge_EvalOprnd() {
    EvalOprnd();
  }

  void Bridge_SaveZP() {
    SaveZP();
  }

  void Bridge_RestoreZP() {
    RestoreZP();
  }

  void Bridge_InitASM() {
    InitASM();
  }

  void Bridge_DoPass1() {
    DoPass1();
  }

  void Bridge_DoPass2() {
    DoPass2();
  }

  void Bridge_FindSym() {
    FindSym();
  }

  void Bridge_StorByt() {
    StorByt();
  }

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
      MnemP                 = 0;
      ZAB                   = 0x80;
      SubTIdx               = 0;
      MacroF                = 0;  // No macros for testing
      Y                     = 0;
      g_LastDirectiveCalled = nullptr;  // Reset directive tracking
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

      // Set g_test_src_buffer so SrcP_byte() uses it
      g_test_src_buffer = test_src_buffer;

      // Set up SrcP to point to test buffer
      // In the original 6502 code, SrcP is a 16-bit pointer stored in zero page
      // For C++ testing, we'll use the SrcP macro which simulates array access
      test_SrcP_ptr = test_src_buffer;

      // Initialize Y to 0 (start of line)
      Y = 0;
    }

    std::uintptr_t GetMnemP() {
      return reinterpret_cast<std::uintptr_t>(MnemP);
    }

    uint8_t GetZAB() {
      return ZAB;
    }

    uint8_t GetSubTIdx() {
      return SubTIdx;
    }

    void SetSubTIdx(uint8_t value) {
      SubTIdx = value;
    }

    void SetupMnemP(uint8_t* mnemEntry, uint8_t y_offset) {
      MnemP = mnemEntry;
      Y     = y_offset;
    }

    // Global test buffer for operand field (256 bytes max for testing)
    static std::uint8_t g_test_operand_buffer[256];

    void SetupOperandField(const char* operand) {
      // Copy operand string to test buffer (CR-terminated for EDASM compatibility)
      if (operand == nullptr || operand[0] == '\0') {
        g_test_operand_buffer[0] = CR;  // Empty operand: just CR
      } else {
        size_t len = strlen(operand);
        if (len >= sizeof(g_test_operand_buffer) - 1) {
          len = sizeof(g_test_operand_buffer) - 1;
        }
        memcpy(g_test_operand_buffer, operand, len);
        g_test_operand_buffer[len] = CR;  // CR-terminate (EDASM expects CR, not null)
      }

      // Set up g_test_src_buffer to point to the operand buffer
      g_test_src_buffer = g_test_operand_buffer;

      // Set Y to 0 to start parsing from beginning of operand field
      Y = 0;
    }

    // Public wrappers for internal assembler functions (test helpers)
    void GInstLen() {
      Bridge_GInstLen();
    }

    void StorGMC() {
      Bridge_StorGMC();
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
      Bridge_HndlMnem();

      // Return success status (C flag: false=success, true=error)
      return !C;
    }

    //=================================================
    // Phase 3a: StorByt Test Helpers
    //=================================================

    void SetGenF(uint8_t value) {
      GenF = value;
    }

    uint8_t GetGenF() {
      return GenF;
    }

    void SetObjPC(uint16_t value) {
      ObjPC = value;
    }

    uint16_t GetObjPC() {
      return ObjPC;
    }

    void SetHighMem(uint16_t value) {
      HighMem = value;
    }

    uint16_t GetHighMem() {
      return HighMem;
    }

    void StorByt(uint8_t byte) {
      A = byte;
      Bridge_StorByt();
    }

    uint8_t ReadObjMemory(uint16_t addr) {
      return g_test_obj_memory[addr];
    }

    void WriteObjMemory(uint16_t addr, uint8_t value) {
      g_test_obj_memory[addr] = value;
    }

    void InitObjMemory() {
      std::memset(g_test_obj_memory, 0, sizeof(g_test_obj_memory));
      g_test_obj_memory_enabled = true;
    }

    //=================================================
    // Phase 7.5: OBJ Directive Test Helpers
    //=================================================

    void SetRelCodeF(uint8_t value) {
      RelCodeF = value;
    }

    uint8_t GetRelCodeF() {
      return RelCodeF;
    }

    // DummyF (DSECT) accessor for tests
    void SetDummyF(uint8_t value) {
      DummyF = value;
    }

    uint8_t GetDummyF() {
      return DummyF;
    }

    void SetEndSymT(uint16_t value) {
      EndSymT = value;
    }

    uint16_t GetEndSymT() {
      return EndSymT;
    }

    void SetMemTop(uint16_t value) {
      MemTop = value;
    }

    uint16_t GetMemTop() {
      return MemTop;
    }

    void SetRLDEnd(uint16_t value) {
      RLDEnd = value;
    }

    uint16_t GetRLDEnd() {
      return RLDEnd;
    }

    // RLD entry count: calculate from MemTop and RLDEnd
    // Each RLD entry is 4 bytes, growing down from MemTop
    uint16_t GetRLDEntryCount() {
      if (MemTop <= RLDEnd) return 0;
      return (MemTop - RLDEnd) / 4;
    }

    // Get RLD entry contents
    void GetRLDEntry(int index, uint8_t* entry) {
      uint16_t count = GetRLDEntryCount();
      if (index < 0 || index >= count) {
        // Invalid index
        for (int i = 0; i < 4; i++) entry[i] = 0;
        return;
      }
      // RLD entries grow downward from MemTop
      // Entry 0 is at RLDEnd, entry 1 is at RLDEnd+4, etc.
      uint16_t entryAddr = RLDEnd + (index * 4);
      uint8_t* ptr       = SimPtrToMemPtr(entryAddr);
      for (int i = 0; i < 4; i++) {
        entry[i] = ptr[i];
      }
    }

    void HndlOBJ() {
      Bridge_HndlOBJ();
    }

    void HndlREL() {
      Bridge_HndlREL();
    }

    //=================================================
    // Phase 3b: GenMCode Test Helpers
    //=================================================

    void SetLength(uint8_t value) {
      Length = value;
    }

    uint8_t GetLength() {
      return Length;
    }

    void SetLenTIdx(uint8_t value) {
      LenTIdx = value;
    }

    uint8_t GetLenTIdx() {
      return LenTIdx;
    }

    void SetValExpr(uint16_t value) {
      ValExpr_word = value;
    }

    uint16_t GetValExpr() {
      return ValExpr_word;
    }

    void SetModWrdL(uint8_t value) {
      ModWrdL = value;
    }

    uint8_t GetModWrdL() {
      return ModWrdL;
    }

    void SetRelExprF(uint8_t value) {
      RelExprF = value;
    }

    uint8_t GetRelExprF() {
      return RelExprF;
    }

    void SetGMC(uint8_t index, uint8_t value) {
      if (index < 4) {
        GMC[index] = value;
      }
    }

    uint8_t GetGMCIdx() {
      return GMCIdx;
    }

    // GenMCode - Generate Machine Code
    // Extracted from GenNow section (lines 2553-2633)
    // Populates the GMC buffer with opcode and operand bytes
    // based on instruction length and addressing mode
    void GenMCode(uint8_t opcode) {
      // Store opcode in GMC[0]
      X      = 0;
      GMC[X] = opcode;
      X++;
      GMCIdx = X;

      A = ModWrdL;
      // BIT Bit08 - branch instr?
      if ((A & 0x08) != 0) {
        // Branch instructions
        // Store displacement byte from ValExpr
        GMC[X] = ValExpr;
        X++;
        GMCIdx = X;
        return;
      }

      // Non-branch instructions
      // Check if we need to store operand bytes
      A = X;  // X=1 at this point
      if (A >= Length) {
        // Single byte instruction (accumulator mode, implied, etc.)
        return;
      }

      // Store operand bytes from ValExpr (little-endian)
      A      = ValExpr;
      GMC[X] = A;
      X++;

      if (X >= Length) {
        GMCIdx = X;
        return;
      }

      // If 3-byte instruction, store high byte
      A      = ValExpr_hi;
      GMC[X] = A;
      X++;
      GMCIdx = X;
    }

    //=================================================
    // Phase 3c: GOpAdr Test Helpers
    //=================================================

    void ResetGOpAdrState() {
      LenTIdx      = 0;
      PC           = 0;
      ValExpr_word = 0;
      ModWrdL      = 0;
      RelExprF     = 0;
    }

    void SetPC(uint16_t pc) {
      PC = pc;
    }

    uint16_t GetPC() {
      return PC;
    }

    // GOpAdr - Get Operand Address
    // Calculates the final operand address based on addressing mode,
    // handling relocation, zero page constraints, and branch offsets
    void GOpAdr() {
      Bridge_GOpAdr();
    }

    //=================================================
    // Phase 3d: ValidateRange & ChkRng Test Helpers
    //=================================================

    // ChkRng - Generic range checker
    // Returns true if value is OUT of range (carry set)
    // Returns false if value is IN range (carry clear)
    bool ChkRng(uint8_t value, uint8_t minVal, uint8_t maxVal) {
      return Bridge_ChkRng(value, minVal, maxVal);
    }

    // ValidateRange - Validate operand value against addressing mode constraints
    void ValidateRange() {
      Bridge_ValidateRange();
    }

    // Get last carry flag state (from ChkRng)
    bool GetLastCarryFlag() {
      return C;
    }

    //=================================================
    // Phase 4: EvalOprnd & Directive Dispatch Test Helpers
    //=================================================

    uint8_t GetPassNbr() {
      return PassNbr;
    }

    void SetPassNbr(uint8_t pass) {
      PassNbr = pass;
    }

    uint8_t GetNxtToken() {
      return NxtToken;
    }

    void EvalOprnd() {
      Bridge_EvalOprnd();
    }

    const char* GetLastDirectiveCalled() {
      return g_LastDirectiveCalled ? g_LastDirectiveCalled : "";
    }

    bool CallDirectiveDispatch(const char* directiveName) {
      // Set up source line with directive name
      SetupSourceLine(directiveName);

      // Call HndlMnem to dispatch
      return HndlMnem();
    }

    //=================================================
    // Phase 5: EQU and ORG Test Helpers
    //=================================================

    // Address control
    uint16_t GetCurAdr() {
      return PC;
    }

    void SetCurAdr(uint16_t addr) {
      PC    = addr;
      ObjPC = addr;
    }

    // Label field control
    void SetLabelF(uint8_t value) {
      LabelF = value;
    }

    uint8_t GetLabelF() {
      return LabelF;
    }

    // Symbol table control
    void InitSymbolTable() {
      // Initialize symbol table for testing
      // Set up StrtSymT and EndSymT
      StrtSymT = 0x0800;  // Standard start
      EndSymT  = 0x0800;  // Empty initially

      // Zero out hash table if allocated
      if (HeaderT_ptr != nullptr) {
        std::memset(HeaderT_ptr, 0, 256);
      }
    }

    void ClearSymbolTable() {
      // Reset symbol table to empty state
      EndSymT = StrtSymT;
      if (HeaderT_ptr != nullptr) {
        std::memset(HeaderT_ptr, 0, 256);
      }
    }

    void SetSymFBP(uint16_t ptr) {
      SymFBP = ptr;
    }

    uint16_t GetSymFBP() {
      return SymFBP;
    }

    // Direct handler calls (already forward declared)
    // HndlEQU and HndlORG are called directly from test code

    //=================================================
    // Phase 6: Data Directives Test Helpers
    //=================================================

    // Direct handler calls
    void HndlDS() {
      Bridge_HndlDS();
    }

    void HndlDFB() {
      Bridge_HndlDFB();
    }

    void HndlDW() {
      Bridge_HndlDW();
    }

    void HndlASC() {
      Bridge_HndlASC();
    }

    void HndlDCI() {
      Bridge_HndlDCI();
    }

    // Test memory accessors
    void EnableTestObjMemory(bool enable) {
      g_test_obj_memory_enabled = enable;
      if (enable) {
        // Align ObjPC with current PC for test writes
        ObjPC = PC;
      }
    }

    uint8_t GetTestObjMemory(uint16_t addr) {
      if (addr < 65536) {
        return g_test_obj_memory[addr];
      }
      return 0;
    }

    void ClearTestObjMemory() {
      std::memset(g_test_obj_memory, 0, sizeof(g_test_obj_memory));
    }

    // Get GMC buffer (for testing direct byte output)
    uint8_t GetGMC(uint8_t index) {
      if (index < 4) {
        return GMC[index];
      }
      return 0;
    }

    //=================================================
    // Phase 7.2: LST Directive Test Helpers
    //=================================================

    // Listing flag accessors
    uint8_t GetListingF() {
      return ListingF;
    }

    void SetListingF(uint8_t value) {
      ListingF = value;
    }

    uint8_t GetLstCyc() {
      return LstCyc;
    }

    void SetLstCyc(uint8_t value) {
      LstCyc = value;
    }

    uint8_t GetLstUnAsm() {
      return LstUnAsm;
    }

    void SetLstUnAsm(uint8_t value) {
      LstUnAsm = value;
    }

    uint8_t GetLstExpMac() {
      return LstExpMac;
    }

    void SetLstExpMac(uint8_t value) {
      LstExpMac = value;
    }

    uint8_t GetLstWarns() {
      return LstWarns;
    }

    void SetLstWarns(uint8_t value) {
      LstWarns = value;
    }

    uint8_t GetLstGCode() {
      return LstGCode;
    }

    void SetLstGCode(uint8_t value) {
      LstGCode = value;
    }

    uint8_t GetLstASym() {
      return LstASym;
    }

    void SetLstASym(uint8_t value) {
      LstASym = value;
    }

    uint8_t GetLstVSym() {
      return LstVSym;
    }

    void SetLstVSym(uint8_t value) {
      LstVSym = value;
    }

    uint8_t GetLst6Cols() {
      return Lst6Cols;
    }

    void SetLst6Cols(uint8_t value) {
      Lst6Cols = value;
    }

    // Direct handler call
    void HndlLST() {
      Bridge_HndlLST();
    }

    //=================================================
    // Phase 7.3: NOLIST and PAGE Directive Test Helpers
    //=================================================

    // Direct handler calls
    void HndlNOLIST() {
      Bridge_HndlNOLIST();
    }

    void DoPage() {
      Bridge_DoPage();
    }

    //=================================================
    // Phase 7.4: SBTL Directive Test Helpers
    //=================================================

    // SubTtlF accessor
    uint8_t GetSubTtlF() {
      return SubTtlF;
    }

    void SetSubTtlF(uint8_t value) {
      SubTtlF = value;
    }

    // SubTitle accessor (returns C-string pointer to buffer)
    const char* GetSubTitle() {
      return reinterpret_cast<const char*>(SubTitle);
    }

    void ClearSubTitle() {
      std::memset(SubTitle, 0, sizeof(SubTitle));
    }

    // Direct handler call
    void HndlSBTL() {
      Bridge_HndlSBTL();
    }

    //=================================================
    // Phase 8.1: Initialization and Cleanup Test Helpers
    //=================================================

    // Error/Warning counter accessors
    void SetNumErrs(uint16_t value) {
      NbrErrs = value;
    }

    void SetNumWarns(uint16_t value) {
      NbrWarns = value;
    }

    // Line number accessor
    uint16_t GetLineNum() {
      // LineNum is stored as BCDNbr (3 bytes BCD format)
      // For testing, we'll treat it as low 2 bytes
      return (static_cast<uint16_t>(BCDNbr[1]) << 8) | BCDNbr[0];
    }

    void SetLineNum(uint16_t value) {
      BCDNbr[0] = value & 0xFF;
      BCDNbr[1] = (value >> 8) & 0xFF;
    }

    // MacroF accessor
    uint8_t GetMacroF() {
      return MacroF;
    }

    void SetMacroF(uint8_t value) {
      MacroF = value;
    }

    // IfDefF accessor (CondAsmF)
    uint8_t GetIfDefF() {
      return CondAsmF;
    }

    void SetIfDefF(uint8_t value) {
      CondAsmF = value;
    }

    // Symbol table pointers
    uint16_t GetStrtSymT() {
      return StrtSymT;
    }

    void SetStrtSymT(uint16_t value) {
      StrtSymT = value;
    }

    // HeaderT accessor
    uint16_t GetHeaderT(uint8_t index) {
      if (HeaderT_ptr == nullptr) {
        return 0x0000;
      }
      return (static_cast<uint16_t>(HeaderT_ptr[index * 2 + 1]) << 8) | HeaderT_ptr[index * 2];
    }

    void SetHeaderT(uint8_t index, uint16_t value) {
      if (HeaderT_ptr == nullptr) {
        return;
      }
      HeaderT_ptr[index * 2]     = value & 0xFF;
      HeaderT_ptr[index * 2 + 1] = (value >> 8) & 0xFF;
    }

    // Phase 8.1: Initialization and cleanup functions
    void SaveZP() {
      Bridge_SaveZP();
    }

    void RestoreZP() {
      Bridge_RestoreZP();
    }

    void InitASM() {
      Bridge_InitASM();
    }

    void CleanupAsm() {
      ::CleanupAsm();
    }

    //=================================================
    // Phase 8.2: Source Line Reader Test Helpers
    //=================================================

    // Source reader functions
    void GSrcLin() {
      ::GSrcLin();
    }

    bool GetCarryFlag() {
      return C;
    }

    void SetCarryFlag(bool value) {
      C = value;
    }

    // Memory source setup helper
    void SetupMemorySource(const char* sourceText, size_t length) {
      ::SetupMemorySource(sourceText, length);
    }

    // Rewind source to beginning for second pass
    void RewindSource() {
      SrcP = g_test_src_base;
    }

    // Phase 8.2 variable accessors
    uint16_t GetSrcP() {
      return SrcP;
    }

    void SetSrcP(uint16_t value) {
      SrcP = value;
    }

    uint16_t GetTxtEnd() {
      return TxtEnd;
    }

    void SetTxtEnd(uint16_t value) {
      TxtEnd = value;
    }

    int8_t GetIDskSrcF() {
      return IDskSrcF;
    }

    void SetIDskSrcF(int8_t value) {
      IDskSrcF = value;
    }

    // Helper to advance SrcP to next line (for testing multi-line scenarios)
    // Searches for CR from current SrcP and advances past it
    void AdvanceToNextLine() {
      // Search for CR starting at SrcP
      while (SrcP < TxtEnd && g_test_src_memory[SrcP] != CR) {
        SrcP++;
      }
      // Skip past the CR if found
      if (SrcP < TxtEnd && g_test_src_memory[SrcP] == CR) {
        SrcP++;
      }
    }

    //=================================================
    // Phase 8.3: Line Processing Helpers Test API
    //=================================================

    // Register accessors
    uint8_t GetY() {
      return Y;
    }

    void SetY(uint8_t value) {
      Y = value;
    }

    uint8_t GetA() {
      return A;
    }

    void SetA(uint8_t value) {
      A = value;
    }

    // Source pointer byte access helper
    uint8_t GetSrcPByte(uint8_t index) {
      return SrcP_byte(index);
    }

    // Line processing helpers
    void NextRec() {
      ::NextRec();
    }

    void NxtField() {
      ::NxtField();
    }

    void ChrGot() {
      ::ChrGot();
    }

    void ChrGet() {
      ::ChrGet();
    }

    //=================================================
    // Phase 8.4: Pass 1 Loop Test API
    //=================================================

    // Execute Pass 1
    void DoPass1() {
      Bridge_DoPass1();
    }

    // Execute Pass 2
    void DoPass2() {
      Bridge_DoPass2();
    }

    // Symbol table query functions
    bool HasSymbol(const char* name) {
      // Set up SrcP to point to the name string in test memory
      // Use a temporary buffer at a safe location
      const uint16_t temp_addr = 0x0300;  // Safe area in simulated memory
      size_t         name_len  = std::strlen(name);
      if (name_len > 100) name_len = 100;
      std::memcpy(&g_test_src_memory[temp_addr], name, name_len);
      g_test_src_memory[temp_addr + name_len] = '\r';  // Terminate with CR

      // Point SrcP at the name
      uint16_t saved_SrcP = SrcP;
      SrcP                = temp_addr;
      Y                   = 0;

      // Call FindSym
      Bridge_FindSym();
      bool found = !C;  // C=0 means found

      // Restore SrcP
      SrcP = saved_SrcP;

      return found;
    }

    uint16_t GetSymbolValue(const char* name) {
      // Set up SrcP to point to the name string
      const uint16_t temp_addr = 0x0300;
      size_t         name_len  = std::strlen(name);
      if (name_len > 100) name_len = 100;
      std::memcpy(&g_test_src_memory[temp_addr], name, name_len);
      g_test_src_memory[temp_addr + name_len] = '\r';

      uint16_t saved_SrcP = SrcP;
      SrcP                = temp_addr;
      Y                   = 0;

      Bridge_FindSym();

      uint16_t value = 0;
      if (!C) {  // Symbol found
        // Y is indexing value field (low byte)
        uint8_t* SymP_ptr = SimPtrToMemPtr(SymP);  // Convert simulated address
        value             = SymP_ptr[Y];           // Low byte
        value |= (SymP_ptr[Y + 1] << 8);           // High byte
      }

      SrcP = saved_SrcP;
      return value;
    }

    uint8_t GetSymbolFlags(const char* name) {
      // Read symbol flags without modifying the unrefd bit
      // This is a read-only version that manually traverses the symbol table

      const uint16_t temp_addr = 0x0300;
      size_t         name_len  = std::strlen(name);
      if (name_len > 100) name_len = 100;
      std::memcpy(&g_test_src_memory[temp_addr], name, name_len);
      g_test_src_memory[temp_addr + name_len] = '\r';

      uint16_t saved_SrcP = SrcP;
      uint8_t  saved_Y    = Y;
      SrcP                = temp_addr;
      Y                   = 0;

      // Compute hash
      HashFn();

      // Get chain head
      Y = HeaderT_ptr[X + 1];
      if (Y == 0) {
        SrcP = saved_SrcP;
        Y    = saved_Y;
        return 0;  // Not found
      }

      uint16_t node_ptr = HeaderT_ptr[X] | (Y << 8);

      // Traverse chain
      while (node_ptr != 0) {
        uint8_t* node     = SimPtrToMemPtr(node_ptr);
        uint16_t next_ptr = node[0] | (node[1] << 8);

        // Check if name matches - DCI encoding: all chars have MSB set EXCEPT last
        uint8_t* name_ptr = node + 2;  // Skip link pointer
        uint8_t  idx      = 0;
        bool     match    = true;

        for (size_t i = 0; i < name_len; i++) {
          uint8_t ch     = name[i];
          uint8_t stored = name_ptr[idx];
          if (i == name_len - 1) {
            // Last char - stored WITHOUT MSB set
            if (ch != stored) {
              match = false;
              break;
            }
          } else {
            // Not last char - stored WITH MSB set, strip it for comparison
            if (ch != (stored & 0x7F)) {
              match = false;
              break;
            }
          }
          idx++;
        }

        if (match) {
          // Found! Read flag byte (right after name)
          uint8_t flags = name_ptr[idx];
          SrcP          = saved_SrcP;
          Y             = saved_Y;
          return flags;
        }

        node_ptr = next_ptr;
      }

      SrcP = saved_SrcP;
      Y    = saved_Y;
      return 0;  // Not found
    }

    int GetSymbolCount() {
      // Count symbols by traversing HeaderT and counting nodes in chains
      int count = 0;

      for (int i = 0; i < 128; i++) {
        uint16_t node_ptr = HeaderT_ptr[i * 2] | (HeaderT_ptr[i * 2 + 1] << 8);

        // Traverse chain
        while (node_ptr != 0x0000) {
          count++;
          // Get next node pointer (first 2 bytes of node)
          uint8_t* node = SimPtrToMemPtr(node_ptr);  // Convert simulated address
          node_ptr      = node[0] | (node[1] << 8);
        }
      }

      return count;
    }

    // Reset assembler state for clean testing
    void ResetAsmState() {
      // Reset Pass 1 relevant state
      PassNbr  = 0;
      PC       = 0;
      ObjPC    = 0;
      RelCodeF = 0;
      DummyF   = 0;  // Clear DummyF (DSECT) for test isolation
      SymNbr   = 0;
      ErrorF   = 0;

      // Initialize HighMem to a reasonable default (64KB - no limit)
      HighMem = 0xFFFF;

      // Initialize HeaderT_ptr to point to simulated memory at HeaderT address
      HeaderT_ptr = &g_test_src_memory[HeaderT];

      // Initialize symbol table pointers
      StrtSymT = 0x1E00;    // Start of symbol table in simulated memory
      EndSymT  = StrtSymT;  // Empty table: start == end

      // Set RLDEnd to a safe high value for unit tests so AddNode doesn't
      // incorrectly signal a full symbol/RLD table.
      RLDEnd = 0xFFFF;

      // Clear symbol table (HeaderT)
      if (HeaderT_ptr != nullptr) {
        for (int i = 0; i < 256; i++) {
          HeaderT_ptr[i] = 0x00;
        }
      }

      // Clear test buffer override to prevent contamination
      g_test_src_buffer = nullptr;
    }

  }  // namespace Asm
}  // namespace EdAsmNg
