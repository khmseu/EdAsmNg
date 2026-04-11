//=================================================
// Internal Header for EdAsmNg Assembler
// Shared state declarations for split compilation units
//=================================================

#pragma once

#include <cstdint>
#include <cstring>

// Internal namespace for shared assembler state
// This allows Pass 3 code to access the same variables as the main assembler
namespace AsmInternal {

  // 6502 Register simulation (references to actual variables in asm.cpp anonymous namespace)
  extern std::uint8_t& A;
  extern std::uint8_t& X;
  extern std::uint8_t& Y;
  extern bool&         C;  // Carry flag

  // Symbol Table State (references to actual variables in asm.cpp anonymous namespace)
  extern std::uint16_t&       StrtSymT;     // Start of symbol table
  extern std::uint16_t&       EndSymT;      // End of symbol table
  extern std::uint16_t&       SymP;         // Symbol pointer
  extern std::uint16_t&       SymNodeP;     // Symbol node pointer
  extern std::uint16_t&       SavSTS;       // Saved start of symbol table
  extern std::uint8_t*&       HeaderT_ptr;  // Symbol table hash header
  extern const std::uint16_t& HeaderT;      // Symbol table hash header address
  extern std::uint8_t&        HashIdx;      // Hash table index (0-254, even)
  extern std::uint16_t&       PrvSymP;      // Previous symbol pointer in chain
  extern std::uint16_t&       NxtSymP;      // Next symbol pointer in chain
  extern std::uint16_t&       ZPSaveY;      // Y register temporary save

  // Pass 3 Symbol Table Printing State
  extern std::uint16_t& UnsortedP;  // Unsorted array pointer
  extern std::uint16_t& SortedP;    // Sorted array pointer
  extern std::uint16_t& AuxAryE;    // End of auxiliary array
  extern std::uint16_t& RecCnt;     // Record count
  extern std::uint16_t& NumRecs;    // Number of records (alias)
  extern std::uint8_t&  SortF;      // Sort flag
  extern std::uint8_t&  NumCols;    // Number of print columns
  extern std::uint8_t&  ColCnt;     // Column count
  extern std::uint8_t&  SymRefCh;   // Symbol reference character
  extern std::uint8_t&  IsFwdRef;   // Forward reference flag
  extern std::uint8_t&  SymIdx;     // Symbol index
  extern std::uint16_t& SymAddr;    // Symbol address

  // Sorting State
  extern std::uint16_t& JJJ;
  extern std::uint16_t& III;      // Loop variables
  extern std::uint16_t& Jump;     // Shell sort gap
  extern std::uint16_t& StrtIdx;  // Start index
  extern std::uint16_t& EndIdx;   // End index
  extern std::uint16_t& J_TH;
  extern std::uint16_t& I_TH;  // Element pointers
  extern std::uint16_t& SymPJ;
  extern std::uint16_t& SymPI;  // Symbol pointers for comparison

  // Listing Control Flags
  extern std::uint8_t& LstASym;   // List alphabetically
  extern std::uint8_t& LstVSym;   // List by value
  extern std::uint8_t& Lst6Cols;  // 6-column mode
  extern std::uint8_t& PrSlot;    // Printer slot
  extern std::uint8_t& SubTtlF;   // Subtitle flag

  // Memory and I/O
  extern std::uint16_t& MemTop;    // Top of available memory
  extern std::int8_t&   DskSrcF;   // Disk source flag (signed)
  extern std::uint16_t& SrcP;      // Source pointer
  extern std::uint8_t&  SLTBYT;    // Slot byte
  extern std::uint8_t&  FCTIndex;  // File control table index
  extern std::uint8_t*  XA060;     // Misc array (pointer to array)
  extern const int&     ChnFile;   // Chain file number (const int)
  extern std::uint16_t* PNTable;   // Pathname table (pointer to array)
  extern std::uint16_t& SrcPathP;  // Source path pointer

  // Program State
  extern std::uint16_t& PC;        // Program counter
  extern std::uint16_t& ObjPC;     // Object program counter
  extern std::uint16_t& RLDEnd;    // Relocation dictionary end
  extern std::uint8_t&  PassNbr;   // Current pass number (0/1/2)
  extern std::uint8_t&  DummyF;    // DSECT dummy section flag
  extern std::uint8_t&  RelExprF;  // Relative expression flag
  extern std::uint8_t&  ErrorF;    // Error flag
  extern std::uint8_t&  RelCodeF;  // Relocatable code flag
  extern std::uint8_t&  LabelF;    // Label field presence flag
  extern std::uint8_t&  ListingF;  // Listing output flag

  // Expression Evaluation State
  extern std::uint16_t& ValExpr_word;  // Expression value (16-bit)
  extern std::uint8_t&  ValExpr_2;     // Extended byte 2 for mul/div
  extern std::uint8_t&  ValExpr_3;     // Extended byte 3 for mul/div
  extern std::uint16_t& Accum;         // Main accumulator
  extern std::uint8_t&  Accum_2;       // Extended accumulator byte 2
  extern std::uint8_t&  Accum_3;       // Extended accumulator byte 3
  extern std::uint8_t&  ExprAccF;      // Expression accumulator flags
  extern std::uint8_t&  NxtToken;      // Next token type
  extern std::int8_t&   Ret816F;       // Return format flag
  extern std::uint8_t&  GblAbsF;       // Global/Absolute flag
  extern std::uint8_t&  SavSEF;        // Saved sub-expression flag
  extern std::uint8_t&  RadixCh;       // Radix check character
  extern std::uint8_t&  BitsDig;       // Bits per digit
  extern std::uint8_t&  msbF;          // MSB flag
  extern std::uint8_t&  Lower8;        // Low 8 bits
  extern std::uint8_t&  SavFByt;       // Saved flag byte

  // Code Generation State
  extern std::uint8_t&  Length;    // Instruction length
  extern std::uint8_t&  ZAB;       // Generic zero page location
  extern std::uint8_t&  EndianF;   // Endianness flag
  extern std::uint8_t&  LstCodeF;  // Listing code flags
  extern std::uint8_t*  GMC;       // Generated machine code buffer
  extern std::uint8_t&  GMCIdx;    // GMC buffer index
  extern std::uint8_t&  GenF;      // Generation mode flag
  extern std::uint16_t& HighMem;   // High memory limit
  extern std::uint16_t& NewPC;     // New program counter

  // Directive Processing State
  extern std::uint8_t& Delimitr;  // Delimiter character
  extern std::uint8_t& Filler;    // Filler byte for DS directive
  extern std::uint8_t& RndF;      // Random data flag
  extern std::uint8_t& TotCnt;    // Total count
  extern std::uint8_t& ByteCnt;   // Byte count
  extern std::uint8_t& SavIndY;   // Saved Y index
  extern std::uint8_t& SavIndX;   // Saved X index
  extern std::uint8_t& StrType;   // String type flag
  extern std::uint8_t& ERfield;   // Expression result field

  // Additional Listing Control
  extern std::uint8_t& LstCyc;     // List cycle count
  extern std::uint8_t& LstUnAsm;   // List unassembled
  extern std::uint8_t& LstExpMac;  // List expanded macros
  extern std::uint8_t& LstWarns;   // List warnings
  extern std::uint8_t& LstGCode;   // List generated code

  // Buffers (pointers to arrays)
  extern std::uint8_t* ChnPNB;    // Chain pathname buffer
  extern std::uint8_t* SubTitle;  // Subtitle buffer

  // Character classification table
  extern const std::uint8_t* CharMap1;  // Character map for identifier validation

  // Constant string tables (pointers to string literals in asm.cpp)
  extern const char* SymbolTxt;
  extern const char* SortedTxt;
  extern const char* AddrTxt;

  // Symbol flag bit constants
  constexpr std::uint8_t Bit02       = 0x02;
  constexpr std::uint8_t Bit08       = 0x08;
  constexpr std::uint8_t Bit10       = 0x10;
  constexpr std::uint8_t Bit40       = 0x40;
  constexpr std::uint8_t unrefd      = 0x40;
  constexpr std::uint8_t relative    = 0x20;
  constexpr std::uint8_t external    = 0x10;
  constexpr std::uint8_t undefined   = 0x80;
  constexpr std::uint8_t fwdrefd     = 0x01;
  constexpr std::uint8_t entry       = 0x08;
  constexpr std::uint8_t nosuchlabel = 0x02;

  // Helper functions (implemented in asm.cpp)
  std::uint8_t* SimPtrToMemPtr(std::uint16_t simAddr);
  std::uint8_t  SrcP_at(std::uint8_t offset);
  void          NextRec();
  void          PollKbd();
  void          AbortAsm();
  void          PutC(std::uint8_t ch);
  void          PrByte(std::uint8_t value);
  void          PutCR();
  void          PrtFF();
  void          ChrGet();
  void          ChrGot();
  void          ChrGet2();
  void          ChrGot2();
  void          RegAsmEW(std::uint8_t errorCode);
  void          CanclAsm(std::uint8_t code);

  // Pass 3 entry point (implemented in asm_pass3.cpp)
  void DoPass3();

  // Symbol table functions (implemented in asm_symtab.cpp)
  void FindSym();
  void HashFn();
  void AddNode();

  // Expression evaluation functions (implemented in asm_expr.cpp)
  void EvalExpr();   // Evaluate expression
  void EvalTerm();   // Evaluate term
  void EvalSExpr();  // Evaluate sub-expression
  void EvalOprnd();  // Evaluate operand
  void GNToken();    // Get next token

  // Expression operators (implemented in asm_expr.cpp)
  void ExprADD();  // Addition operator
  void ExprSUB();  // Subtraction operator
  void ExprMUL();  // Multiplication operator
  void ExprDIV();  // Division operator
  void ExprEOR();  // Exclusive OR operator
  void ExprAND();  // Bitwise AND operator
  void ExprORA();  // Bitwise OR operator

  // Expression helper functions (implemented in asm_expr.cpp)
  void Mul2();     // Multiply by 2
  void AdvSrcP();  // Advance source pointer
  void Is16K();    // Check if > 16K

  // Code generation functions (implemented in asm.cpp)
  void StorByt();    // Store byte
  void StorGMC();    // Store generated machine code
  void AddRLDEnt();  // Add RLD entry
  void AdvPC();      // Advance program counter

  // Directive handlers (implemented in asm_directives.cpp)
  void DrtvDone();      // Common directive completion
  void HndlEQU();       // EQU directive handler
  void HndlORG();       // ORG directive handler
  void HndlOBJ();       // OBJ directive handler
  void HndlREL();       // REL directive handler
  void HndlDS();        // DS/BLOCK directive handler
  void HndlDFB();       // DFB/BYTE directive handler
  void HndlDW();        // DW/WORD directive handler
  void HndlDWCore();    // DW/DDB core handler
  void HndlASC();       // ASC/ASCII directive handler
  void HndlASC_Core();  // ASC/DCI core handler
  void HndlDCI();       // DCI directive handler
  void HndlBYTE();      // .BYTE directive handler
  void HndlWORD();      // .WORD directive handler
  void HndlBLOCK();     // .BLOCK directive handler
  void HndlASCII();     // .ASCII directive handler
  void HndlDBYTE();     // .DBYTE/DDB directive handler
  void HndlLST();       // LST directive handler
  void HndlLIST();      // .LIST directive handler
  void HndlNOLIST();    // NOLIST directive handler
  void DoPage();        // PAGE directive handler
  void HndlSBTL();      // SBTL/.TITLE directive handler

  // Additional expression and error handling functions
  void SkipSpcs();   // Skip whitespace characters
  void L87FB();      // Expression error handler
  void EvalOprnd();  // Evaluate operand (duplicate declaration for clarity)

  // Storage for operator dispatch tables (defined in asm_expr.cpp)
  extern const std::uint8_t  Operators[];  // Operator token table
  extern const std::uint16_t L888E[];      // Operator JMP address table (low bytes)
  extern const std::uint16_t L8895[];      // Operator JMP address table (high bytes)

  // Last directive tracking for special handling
  extern std::uint8_t g_LastDirectiveCalled;

  // Inline getter functions for macro forwarding
  inline std::uint8_t& ValExpr_word_lo();
  inline std::uint8_t& ValExpr_word_hi();
  inline std::uint8_t& Accum_hi_val();
  inline std::uint8_t& ObjPC_hi_val();
  inline std::uint8_t& HighMem_hi_val();
  inline std::uint8_t& EndSymT_hi_val();
  inline std::uint8_t& MemTop_hi_val();

}  // namespace AsmInternal

//=============================================================================
// Macro Forwarding for Legacy Byte Access Patterns
// These macros forward legacy 6502-style byte access to the AsmInternal bridges
// They enable extracted modules to work seamlessly with the state bridge pattern
//=============================================================================

// Legacy expression value access (ValExpr_word low/high byte split)
#define ValExpr    (AsmInternal::ValExpr_word_lo())
#define ValExpr_hi (AsmInternal::ValExpr_word_hi())

// Legacy accumulator high byte access
#define Accum_hi (AsmInternal::Accum_hi_val())

// Legacy 16-bit variable high byte access
#define ObjPC_hi   (AsmInternal::ObjPC_hi_val())
#define HighMem_hi (AsmInternal::HighMem_hi_val())
#define EndSymT_hi (AsmInternal::EndSymT_hi_val())
#define MemTop_hi  (AsmInternal::MemTop_hi_val())
