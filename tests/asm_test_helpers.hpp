#ifndef EDASM_NG_ASM_TEST_HELPERS_HPP
#define EDASM_NG_ASM_TEST_HELPERS_HPP

#include <cstdint>
#include <string>

#include "EdAsmNg/asm.hpp"

//=================================================
// Specialized Test Helper Extensions
//
// This header extends the public EdAsmNg::Asm API
// with specialized test-only inspection functions.
// For standard APIs, use EdAsmNg/asm.hpp directly.
//=================================================

namespace EdAsmNg {
  namespace Asm {
    //=================================================
    // Error State Management - Test Extensions
    //=================================================
    std::uint16_t GetErrorCount();
    std::uint16_t GetWarningCount();
    std::uint8_t  GetErrorFlag();
    std::uint8_t  GetErrNbr4();
    void          SetVidSlot(std::uint8_t slot);
    void          SetFileNbr(std::uint8_t file);
    void          SetBCDLineNumber(std::uint8_t hi, std::uint8_t lo);

    struct ErrorInfo {
      std::uint8_t fileNbr;
      std::uint8_t errIndex;
      std::uint8_t lineHi;
      std::uint8_t lineLo;
    };

    ErrorInfo GetErrorInfo(int index);
    void      RegAsmEW(std::uint8_t errorToken);
    void      SaveErrInfo(std::uint8_t errorToken);

    //=================================================
    // Phase 3: Symbol Table Test Extensions
    //=================================================
    bool          IsHeaderTEmpty();  // Check if HeaderT is all zeros
    std::uint16_t GetSymNodeP();     // Get current symbol node pointer
    std::uint16_t GetSavSTS();       // Get saved start of symbol table

    //=================================================
    // Phase 3: Symbol Table Sorting Test Extensions (Phase 2)
    //=================================================
    std::uint16_t GetRecCnt();     // Get record count (number of aux array entries)
    std::uint16_t GetSortedP();    // Get SortedP (moving cursor during sort)
    std::uint16_t GetUnsortedP();  // Get pointer to end of unsorted array
    std::uint16_t GetAuxArrayEntry(int index, bool fourByte);  // Get pointer from aux array entry
    std::uint16_t GetAuxArrayAddr(int index);       // Get address from 4-byte aux array entry
    std::string   GetSortedSymbolName(int index);   // Get symbol name from sorted array
    std::uint16_t GetSortedSymbolValue(int index);  // Get symbol value from sorted array
    std::uint8_t  GetCompactedSymbolFlags(
         int index);  // Get symbol flags from compacted table (pure read)

    //=================================================
    // Phase 3: Symbol Table Formatting Test Extensions (Phase 3)
    //=================================================
    std::uint8_t GetNumCols();  // Get number of print columns (2, 4, or 6)
    std::uint8_t GetSortF();    // Get current SortF state (alpha/value flow)

  }  // namespace Asm
}  // namespace EdAsmNg

#endif  // EDASM_NG_ASM_TEST_HELPERS_HPP
