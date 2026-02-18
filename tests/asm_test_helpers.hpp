#ifndef EDASM_NG_ASM_TEST_HELPERS_HPP
#define EDASM_NG_ASM_TEST_HELPERS_HPP

#include <cstdint>
#include <string>

//=================================================
// Test Helper Declarations for EdAsmNg::Asm
//
// These functions expose internal assembler state and
// functions for testing purposes only.
//=================================================

namespace EdAsmNg {
  namespace Asm {
    //=================================================
    // Error State Management
    //=================================================
    void     ResetErrorState();
    uint16_t GetErrorCount();
    uint16_t GetWarningCount();
    uint8_t  GetErrorFlag();
    uint8_t  GetErrNbr4();
    void     SetVidSlot(uint8_t slot);
    void     SetFileNbr(uint8_t file);
    void     SetBCDLineNumber(uint8_t hi, uint8_t lo);

    struct ErrorInfo {
      uint8_t fileNbr;
      uint8_t errIndex;
      uint8_t lineHi;
      uint8_t lineLo;
    };

    ErrorInfo GetErrorInfo(int index);
    void      RegAsmEW(uint8_t errorToken);
    void      SaveErrInfo(uint8_t errorToken);

    //=================================================
    // Assembler State Management
    //=================================================
    void ResetAsmState();
    void SaveZP();     // Save zero-page workspace
    void RestoreZP();  // Restore zero-page workspace

    //=================================================
    // Instruction Length Test Helpers (GInstLen)
    //=================================================
    void    GInstLen();  // Call instruction length calculator
    void    SetupMnemP(uint8_t* mnemEntry, uint8_t y_offset);
    void    SetupOperandField(const char* operand);
    uint8_t GetY();
    void    SetY(uint8_t value);
    void    SetSubTIdx(uint8_t value);
    uint8_t GetSubTIdx();
    uint8_t GetLength();                           // Get calculated instruction length
    void    SetLength(uint8_t value);              // Set instruction length
    uint8_t GetLenTIdx();                          // Get addressing mode index
    void    SetLenTIdx(uint8_t value);             // Set addressing mode index
    uint8_t GetGMC(uint8_t index);                 // Get byte from GMC buffer
    void    SetGMC(uint8_t index, uint8_t value);  // Set byte in GMC buffer
    uint8_t GetNxtToken();                         // Get NxtToken value
    void    SetNxtToken(uint8_t value);            // Set NxtToken value

    //=================================================
    // Phase 3: Symbol Table Compaction Test Helpers
    //=================================================
    void     ResetPass3State();         // Reset Pass3-relevant state
    bool     IsHeaderTEmpty();          // Check if HeaderT is all zeros
    void     SetLstASym(uint8_t val);   // Set LstASym flag
    void     SetLstVSym(uint8_t val);   // Set LstVSym flag
    uint8_t  GetLst6Cols();             // Get 6-column mode flag
    void     SetLst6Cols(uint8_t val);  // Set 6-column mode flag
    void     DoPass3();                 // Execute Pass 3
    uint16_t GetEndSymT();              // Get end of symbol table pointer
    uint16_t GetStrtSymT();             // Get start of symbol table pointer
    uint16_t GetSymNodeP();             // Get current symbol node pointer
    uint16_t GetSavSTS();               // Get saved start of symbol table
    void AddTestSymbol(const char* name, uint16_t value, uint8_t flags);  // Add symbol for testing

    //=================================================
    // Phase 3: Symbol Table Sorting Test Helpers (Phase 2)
    //=================================================
    uint16_t    GetRecCnt();     // Get record count (number of aux array entries)
    uint16_t    GetSortedP();    // Get SortedP (moving cursor during sort)
    uint16_t    GetUnsortedP();  // Get pointer to end of unsorted array
    uint16_t    GetAuxArrayEntry(int index, bool fourByte);  // Get pointer from aux array entry
    uint16_t    GetAuxArrayAddr(int index);       // Get address from 4-byte aux array entry
    std::string GetSortedSymbolName(int index);   // Get symbol name from sorted array
    uint16_t    GetSortedSymbolValue(int index);  // Get symbol value from sorted array
    uint8_t     GetCompactedSymbolFlags(
            int index);  // Get symbol flags from compacted table (pure read)

    //=================================================
    // Phase 3: Symbol Table Formatting Test Helpers (Phase 3)
    //=================================================
    uint8_t     GetNumCols();               // Get number of print columns (2, 4, or 6)
    uint8_t     GetPrSlot();                // Get printer slot number (0=video)
    void        SetPrSlot(uint8_t value);   // Set printer slot number
    uint8_t     GetSortF();                 // Get current SortF state (alpha/value flow)
    const char* GetSubTitle();              // Get subtitle buffer as C-string (already exists)
    void        SetSubTtlF(uint8_t value);  // Set SubTtlF flag (already exists)

  }  // namespace Asm
}  // namespace EdAsmNg

#endif  // EDASM_NG_ASM_TEST_HELPERS_HPP
