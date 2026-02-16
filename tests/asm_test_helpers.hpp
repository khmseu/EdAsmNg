#ifndef EDASM_NG_ASM_TEST_HELPERS_HPP
#define EDASM_NG_ASM_TEST_HELPERS_HPP

#include <cstdint>

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

  }  // namespace Asm
}  // namespace EdAsmNg

#endif  // EDASM_NG_ASM_TEST_HELPERS_HPP
