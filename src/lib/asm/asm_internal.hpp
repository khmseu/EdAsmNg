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
  extern std::uint16_t& RLDEnd;    // Relocation dictionary end
  extern std::uint8_t&  PassNbr;   // Current pass number (0/1/2)
  extern std::uint8_t&  DummyF;    // DSECT dummy section flag
  extern std::uint8_t&  RelExprF;  // Relative expression flag
  extern std::uint8_t&  ErrorF;    // Error flag
  extern std::uint8_t&  RelCodeF;  // Relocatable code flag
  extern std::uint8_t&  LabelF;    // Label field presence flag

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

}  // namespace AsmInternal
