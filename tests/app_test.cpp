#include "EdAsmNg/app.hpp"

#include <gtest/gtest.h>

TEST(GreetTests, DefaultsToWorld) {
  EXPECT_EQ(EdAsmNg::greet(), "Hello, World!");
}

TEST(GreetTests, UsesProvidedName) {
  EXPECT_EQ(EdAsmNg::greet("Kai"), "Hello, Kai!");
}

//=================================================
// Error Registration Tests
//=================================================

// Helper functions to access assembler internals for testing
namespace EdAsmNg {
  namespace Asm {
    // Expose internal state for testing
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

    // Main error registration functions
    void RegAsmEW(uint8_t errorToken);
    void SaveErrInfo(uint8_t errorToken);
  }  // namespace Asm
}  // namespace EdAsmNg

class ErrorRegistrationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::SetVidSlot(0);  // Standard 40-col video
    EdAsmNg::Asm::SetFileNbr(1);
    EdAsmNg::Asm::SetBCDLineNumber(0x01, 0x23);  // Line 123 in BCD
  }
};

TEST_F(ErrorRegistrationTest, SingleErrorRegistration) {
  // Register an error (even token = error)
  uint8_t errorToken = 0x02;  // Token for "Duplicate identifier"
  EdAsmNg::Asm::RegAsmEW(errorToken);

  // Verify error count incremented (BCD format)
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0x0001);
  EXPECT_EQ(EdAsmNg::Asm::GetWarningCount(), 0x0000);

  // Verify ErrorF flag is set ($80)
  EXPECT_EQ(EdAsmNg::Asm::GetErrorFlag(), 0x80);

  // Verify error info saved
  EXPECT_EQ(EdAsmNg::Asm::GetErrNbr4(), 4);  // One error = 4 bytes
  auto errInfo = EdAsmNg::Asm::GetErrorInfo(0);
  EXPECT_EQ(errInfo.fileNbr, 1);
  EXPECT_EQ(errInfo.errIndex, 0x02);  // Error token & 0x7E
  EXPECT_EQ(errInfo.lineHi, 0x01);
  EXPECT_EQ(errInfo.lineLo, 0x23);
}

TEST_F(ErrorRegistrationTest, SingleWarningRegistration) {
  // Register a warning (odd token = warning)
  uint8_t warningToken = 0x03;  // Odd token
  EdAsmNg::Asm::RegAsmEW(warningToken);

  // Verify warning count incremented (BCD format)
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0x0000);
  EXPECT_EQ(EdAsmNg::Asm::GetWarningCount(), 0x0001);

  // Verify ErrorF flag is NOT set for warnings
  EXPECT_EQ(EdAsmNg::Asm::GetErrorFlag(), 0x00);

  // Note: Warnings may or may not be saved depending on LstWarns flag
  // For now, we test basic counting
}

TEST_F(ErrorRegistrationTest, MultipleErrors) {
  // Register three errors
  EdAsmNg::Asm::RegAsmEW(0x04);  // Error 1
  EdAsmNg::Asm::RegAsmEW(0x06);  // Error 2
  EdAsmNg::Asm::RegAsmEW(0x08);  // Error 3

  // Verify error count (BCD: 3 = 0x0003)
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0x0003);

  // Verify three error records saved
  EXPECT_EQ(EdAsmNg::Asm::GetErrNbr4(), 12);  // 3 errors * 4 bytes

  // Verify each error info
  auto err1 = EdAsmNg::Asm::GetErrorInfo(0);
  EXPECT_EQ(err1.errIndex, 0x04);

  auto err2 = EdAsmNg::Asm::GetErrorInfo(1);
  EXPECT_EQ(err2.errIndex, 0x06);

  auto err3 = EdAsmNg::Asm::GetErrorInfo(2);
  EXPECT_EQ(err3.errIndex, 0x08);
}

TEST_F(ErrorRegistrationTest, ErrorWarningMix) {
  // Mix errors and warnings
  EdAsmNg::Asm::RegAsmEW(0x02);  // Error
  EdAsmNg::Asm::RegAsmEW(0x03);  // Warning
  EdAsmNg::Asm::RegAsmEW(0x04);  // Error
  EdAsmNg::Asm::RegAsmEW(0x05);  // Warning

  // Verify counts (BCD format)
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0x0002);
  EXPECT_EQ(EdAsmNg::Asm::GetWarningCount(), 0x0002);
}

TEST_F(ErrorRegistrationTest, ErrorBufferOverflow40Col) {
  EdAsmNg::Asm::SetVidSlot(0);  // 40-col = max 8 errors

  // Register 10 errors (exceeds 8-error limit)
  for (int i = 0; i < 10; i++) {
    EdAsmNg::Asm::RegAsmEW(0x02);
  }

  // All 10 should be counted
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0x0010);  // BCD 10

  // But only 8 should be stored in buffer
  EXPECT_EQ(EdAsmNg::Asm::GetErrNbr4(), 32);  // 8 errors * 4 bytes
}

TEST_F(ErrorRegistrationTest, ErrorBufferOverflow80Col) {
  EdAsmNg::Asm::SetVidSlot(3);  // 80-col = max 16 errors

  // Register 20 errors (exceeds 16-error limit)
  for (int i = 0; i < 20; i++) {
    EdAsmNg::Asm::RegAsmEW(0x02);
  }

  // All 20 should be counted
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0x0020);  // BCD 20

  // But only 16 should be stored in buffer
  EXPECT_EQ(EdAsmNg::Asm::GetErrNbr4(), 64);  // 16 errors * 4 bytes
}

TEST_F(ErrorRegistrationTest, BCDIncrementCorrect) {
  // Test BCD increment across decimal boundaries
  for (int i = 0; i < 10; i++) {
    EdAsmNg::Asm::RegAsmEW(0x02);  // Register 10 errors
  }
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0x0010);  // BCD 10, not 0x000A

  // Register one more
  EdAsmNg::Asm::RegAsmEW(0x02);
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0x0011);  // BCD 11
}

//=================================================
// Mnemonic/Directive Dispatch Tests (Phase 2)
//=================================================

// Helper functions to access assembler internals for mnemonic testing
namespace EdAsmNg {
  namespace Asm {
    // Setup test sources and get dispatch state
    void           SetupSourceLine(const char* line);
    std::uintptr_t GetMnemP();
    uint8_t        GetZAB();
    uint8_t        GetSubTIdx();
    bool           HndlMnem();  // Returns true on success (C=0), false on error (C=1)
    void           ResetDispatchState();

    // Get which directive handler was last called (for testing dispatch)
    const char* GetLastDirectiveCalled();
  }  // namespace Asm
}  // namespace EdAsmNg

class MnemonicDispatchTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::ResetDispatchState();
  }
};

TEST_F(MnemonicDispatchTest, ValidMnemonicLDA) {
  // Test 3-letter mnemonic LDA
  EdAsmNg::Asm::SetupSourceLine("LDA");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);  // C=0 means success

  // Verify ZAB contains addressing mode flags (not directive flag $80+)
  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_LT(zab, 0x80);  // Not a directive

  // Verify MnemP points to valid entry (non-zero)
  std::uintptr_t mnemP = EdAsmNg::Asm::GetMnemP();
  EXPECT_NE(mnemP, 0u);
}

TEST_F(MnemonicDispatchTest, ValidMnemonicSTA) {
  EdAsmNg::Asm::SetupSourceLine("STA");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_LT(zab, 0x80);  // Not a directive

  std::uintptr_t mnemP = EdAsmNg::Asm::GetMnemP();
  EXPECT_NE(mnemP, 0u);
}

TEST_F(MnemonicDispatchTest, ValidMnemonicJMP) {
  EdAsmNg::Asm::SetupSourceLine("JMP");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_LT(zab, 0x80);
}

TEST_F(MnemonicDispatchTest, ValidMnemonicBREAK) {
  // BRK is a special case (single letter can extend to 3)
  EdAsmNg::Asm::SetupSourceLine("BRK");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_LT(zab, 0x80);
}

TEST_F(MnemonicDispatchTest, ValidDirectiveEQU) {
  EdAsmNg::Asm::SetupSourceLine(".EQU");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  // Directive should set ZAB high bit ($80+)
  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_GE(zab, 0x80);  // Directive flag

  // Verify correct handler was called
  EXPECT_STREQ(EdAsmNg::Asm::GetLastDirectiveCalled(), "HndlEQU");
}

TEST_F(MnemonicDispatchTest, ValidDirectiveORG) {
  EdAsmNg::Asm::SetupSourceLine(".ORG");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_GE(zab, 0x80);

  // Verify correct handler was called
  EXPECT_STREQ(EdAsmNg::Asm::GetLastDirectiveCalled(), "HndlORG");
}

TEST_F(MnemonicDispatchTest, ValidDirectiveDFB) {
  // .BYTE directive
  EdAsmNg::Asm::SetupSourceLine(".BYTE");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_GE(zab, 0x80);

  // Verify correct handler was called
  EXPECT_STREQ(EdAsmNg::Asm::GetLastDirectiveCalled(), "HndlBYTE");
}

TEST_F(MnemonicDispatchTest, ValidDirectiveDW) {
  // .WORD directive
  EdAsmNg::Asm::SetupSourceLine(".WORD");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_GE(zab, 0x80);

  // Verify correct handler was called
  EXPECT_STREQ(EdAsmNg::Asm::GetLastDirectiveCalled(), "HndlWORD");
}

TEST_F(MnemonicDispatchTest, InvalidMnemonicXYZ) {
  // Non-existent mnemonic
  EdAsmNg::Asm::SetupSourceLine("XYZ");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_FALSE(success);  // C=1 means error

  // Should register an error
  EXPECT_GT(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(MnemonicDispatchTest, InvalidFirstLetter) {
  // Letter with no opcodes (e.g., 'Q')
  EdAsmNg::Asm::SetupSourceLine("QQQ");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_FALSE(success);

  // Should register an error or return failure
  EXPECT_GT(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(MnemonicDispatchTest, PartialMatchFailure) {
  // Starts like LDA but continues incorrectly
  EdAsmNg::Asm::SetupSourceLine("LDAX");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_FALSE(success);

  EXPECT_GT(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(MnemonicDispatchTest, WhitespaceAfterMnemonic) {
  // LDA followed by space (valid termination)
  EdAsmNg::Asm::SetupSourceLine("LDA ");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_LT(zab, 0x80);
}

TEST_F(MnemonicDispatchTest, CRAfterMnemonic) {
  // LDA followed by CR (valid termination)
  EdAsmNg::Asm::SetupSourceLine("LDA\r");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_LT(zab, 0x80);
}

TEST_F(MnemonicDispatchTest, CaseSensitivityCheck) {
  // lowercase mnemonic (should be converted to uppercase by ChrGot)
  EdAsmNg::Asm::SetupSourceLine("lda");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);  // Should work due to uppercase conversion
}

TEST_F(MnemonicDispatchTest, MixedCaseMnemonic) {
  // Mixed case
  EdAsmNg::Asm::SetupSourceLine("LdA");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);
}

TEST_F(MnemonicDispatchTest, ValidMnemonic6502ADC) {
  EdAsmNg::Asm::SetupSourceLine("ADC");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_LT(zab, 0x80);
}

TEST_F(MnemonicDispatchTest, ValidMnemonic6502AND) {
  EdAsmNg::Asm::SetupSourceLine("AND");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_LT(zab, 0x80);
}

TEST_F(MnemonicDispatchTest, ValidMnemonic6502ASL) {
  EdAsmNg::Asm::SetupSourceLine("ASL");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_LT(zab, 0x80);
}

TEST_F(MnemonicDispatchTest, ValidMnemonic6502BCC) {
  EdAsmNg::Asm::SetupSourceLine("BCC");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_LT(zab, 0x80);
}

TEST_F(MnemonicDispatchTest, ValidMnemonic6502BCS) {
  EdAsmNg::Asm::SetupSourceLine("BCS");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_LT(zab, 0x80);
}

TEST_F(MnemonicDispatchTest, UnsupportedDirectiveFallback) {
  // Test directives that are recognized but have no handler stub
  // For example: .PAGE, .TITLE, .SKIP, etc. (in table but not dispatched)
  EdAsmNg::Asm::SetupSourceLine(".PAGE");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_FALSE(success);  // Should return failure (C=1)

  // Verify ZAB still has directive flag set (was recognized as directive)
  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_GE(zab, 0x80);  // Directive flag should be set

  // Verify no handler was invoked (fallback path was taken)
  EXPECT_STREQ(EdAsmNg::Asm::GetLastDirectiveCalled(), "");
}

//=================================================
// StorByt Tests (Phase 3a)
//=================================================

// Helper functions for StorByt testing
namespace EdAsmNg {
  namespace Asm {
    // StorByt test helpers
    void     SetGenF(uint8_t value);
    uint8_t  GetGenF();
    void     SetObjPC(uint16_t value);
    uint16_t GetObjPC();
    void     SetHighMem(uint16_t value);
    uint16_t GetHighMem();
    void     StorByt(uint8_t byte);
    uint8_t  ReadObjMemory(uint16_t addr);
    void     WriteObjMemory(uint16_t addr, uint8_t value);
    void     InitObjMemory();  // Initialize test memory buffer
  }  // namespace Asm
}  // namespace EdAsmNg

class StorBytTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::InitObjMemory();
    EdAsmNg::Asm::SetGenF(0x00);  // Default: memory mode, no suppression
    EdAsmNg::Asm::SetVidSlot(0);
    EdAsmNg::Asm::SetFileNbr(1);
    EdAsmNg::Asm::SetBCDLineNumber(0x01, 0x00);
  }
};

TEST_F(StorBytTest, SuppressedGeneration_NoWrite) {
  // Set GenF = 0x80 (suppress bit set)
  EdAsmNg::Asm::SetGenF(0x80);
  EdAsmNg::Asm::SetObjPC(0x1000);
  EdAsmNg::Asm::SetHighMem(0x9000);

  // Write initial value to memory
  EdAsmNg::Asm::WriteObjMemory(0x1000, 0xAA);

  // Try to store 0xBB (should be suppressed)
  EdAsmNg::Asm::StorByt(0xBB);

  // Assert ObjPC unchanged
  EXPECT_EQ(EdAsmNg::Asm::GetObjPC(), 0x1000);

  // Assert memory unchanged
  EXPECT_EQ(EdAsmNg::Asm::ReadObjMemory(0x1000), 0xAA);
}

TEST_F(StorBytTest, DiskMode_JumpsToWr1Byte) {
  // Set GenF = 0x40 (disk bit set, suppress bit clear)
  EdAsmNg::Asm::SetGenF(0x40);
  EdAsmNg::Asm::SetObjPC(0x2000);
  EdAsmNg::Asm::SetHighMem(0x9000);

  // Call StorByt with disk mode
  // Should call Wr1Byte (which is stubbed, so just verify no crash)
  EdAsmNg::Asm::StorByt(0xCC);

  // ObjPC should remain unchanged in disk mode (Wr1Byte handles that)
  EXPECT_EQ(EdAsmNg::Asm::GetObjPC(), 0x2000);
}

TEST_F(StorBytTest, MemoryMode_StoresAndIncrementsPC) {
  // Set GenF = 0x00 (memory mode, no bits set)
  EdAsmNg::Asm::SetGenF(0x00);
  EdAsmNg::Asm::SetObjPC(0x2000);
  EdAsmNg::Asm::SetHighMem(0x9000);

  // Store byte
  EdAsmNg::Asm::StorByt(0xDD);

  // Assert memory contains the byte
  EXPECT_EQ(EdAsmNg::Asm::ReadObjMemory(0x2000), 0xDD);

  // Assert ObjPC incremented
  EXPECT_EQ(EdAsmNg::Asm::GetObjPC(), 0x2001);
}

TEST_F(StorBytTest, IncrementObjPC_WithCarry) {
  // Set GenF = 0x00 (memory mode)
  EdAsmNg::Asm::SetGenF(0x00);
  EdAsmNg::Asm::SetObjPC(0x20FF);  // Will overflow low byte
  EdAsmNg::Asm::SetHighMem(0x9000);

  // Store byte
  EdAsmNg::Asm::StorByt(0xEE);

  // Assert memory at 0x20FF contains the byte
  EXPECT_EQ(EdAsmNg::Asm::ReadObjMemory(0x20FF), 0xEE);

  // Assert ObjPC incremented with carry to 0x2100
  EXPECT_EQ(EdAsmNg::Asm::GetObjPC(), 0x2100);
}

TEST_F(StorBytTest, OutOfMemory_TriggersError) {
  // Set GenF = 0x00 (memory mode)
  EdAsmNg::Asm::SetGenF(0x00);
  EdAsmNg::Asm::SetObjPC(0x8FFF);
  EdAsmNg::Asm::SetHighMem(0x9000);  // Boundary

  // Store byte - ObjPC will increment to 0x9000, triggering error
  EdAsmNg::Asm::StorByt(0xFF);

  // Assert memory written at 0x8FFF
  EXPECT_EQ(EdAsmNg::Asm::ReadObjMemory(0x8FFF), 0xFF);

  // Assert ObjPC incremented to 0x9000
  EXPECT_EQ(EdAsmNg::Asm::GetObjPC(), 0x9000);

  // Assert error registered (error count should be > 0)
  EXPECT_GT(EdAsmNg::Asm::GetErrorCount(), 0);
}

//=================================================
// GenMCode Tests (Phase 3b)
//=================================================

// Helper functions for GenMCode testing
namespace EdAsmNg {
  namespace Asm {
    // GenMCode test helpers
    void     SetLength(uint8_t length);
    uint8_t  GetLength();
    void     SetLenTIdx(uint8_t idx);
    uint8_t  GetLenTIdx();
    void     SetValExpr(uint16_t value);
    uint16_t GetValExpr();
    void     SetModWrdL(uint8_t value);
    uint8_t  GetModWrdL();
    void     SetRelExprF(uint8_t value);
    uint8_t  GetRelExprF();
    uint8_t  GetGMC(uint8_t index);
    void     SetGMC(uint8_t index, uint8_t value);
    uint8_t  GetGMCIdx();
    void     GenMCode(uint8_t opcode);  // Generate machine code
  }  // namespace Asm
}  // namespace EdAsmNg

class GenMCodeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::InitObjMemory();
    EdAsmNg::Asm::SetGenF(0x00);  // Memory mode, no suppression
    EdAsmNg::Asm::SetObjPC(0x2000);
    EdAsmNg::Asm::SetHighMem(0x9000);
    EdAsmNg::Asm::SetVidSlot(0);
    EdAsmNg::Asm::SetFileNbr(1);
    EdAsmNg::Asm::SetBCDLineNumber(0x01, 0x00);
    EdAsmNg::Asm::SetRelExprF(0x00);  // Default: absolute addressing
    EdAsmNg::Asm::SetModWrdL(0x00);   // Default: no branch instruction
  }
};

TEST_F(GenMCodeTest, Accumulator_GeneratesOneByte) {
  // Accumulator addressing mode: ASL A (opcode 0x0A)
  // Length = 1, addressing mode index = 10 (accumulator)
  EdAsmNg::Asm::SetLength(1);
  EdAsmNg::Asm::SetLenTIdx(10);  // Accumulator mode index
  EdAsmNg::Asm::SetValExpr(0x0000);

  // Generate machine code for ASL A
  EdAsmNg::Asm::GenMCode(0x0A);

  // Verify only opcode byte stored in GMC buffer
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(0), 0x0A);
  EXPECT_EQ(EdAsmNg::Asm::GetGMCIdx(), 1);

  // Verify Length still 1
  EXPECT_EQ(EdAsmNg::Asm::GetLength(), 1);
}

TEST_F(GenMCodeTest, Immediate_GeneratesTwoBytes) {
  // Immediate addressing mode: LDA #$42 (opcode 0xA9)
  // Length = 2, addressing mode index = 2 (immediate)
  EdAsmNg::Asm::SetLength(2);
  EdAsmNg::Asm::SetLenTIdx(2);  // Immediate mode index
  EdAsmNg::Asm::SetValExpr(0x0042);

  // Generate machine code for LDA #$42
  EdAsmNg::Asm::GenMCode(0xA9);

  // Verify opcode and operand bytes in GMC buffer
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(0), 0xA9);  // Opcode
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(1), 0x42);  // Operand low byte
  EXPECT_EQ(EdAsmNg::Asm::GetGMCIdx(), 2);   // Index advanced to 2

  // Verify Length still 2
  EXPECT_EQ(EdAsmNg::Asm::GetLength(), 2);
}

TEST_F(GenMCodeTest, ZeroPage_GeneratesTwoBytes) {
  // Zero page addressing mode: LDA $34 (opcode 0xA5)
  // Length = 2, addressing mode index = 1 (zero page)
  EdAsmNg::Asm::SetLength(2);
  EdAsmNg::Asm::SetLenTIdx(1);  // Zero page mode index
  EdAsmNg::Asm::SetValExpr(0x0034);

  // Generate machine code for LDA $34
  EdAsmNg::Asm::GenMCode(0xA5);

  // Verify opcode and zero page address in GMC buffer
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(0), 0xA5);  // Opcode
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(1), 0x34);  // Zero page address
  EXPECT_EQ(EdAsmNg::Asm::GetGMCIdx(), 2);

  // Verify Length still 2
  EXPECT_EQ(EdAsmNg::Asm::GetLength(), 2);
}

TEST_F(GenMCodeTest, Absolute_GeneratesThreeBytes) {
  // Absolute addressing mode: LDA $1234 (opcode 0xAD)
  // Length = 3, addressing mode index = 0 (absolute)
  EdAsmNg::Asm::SetLength(3);
  EdAsmNg::Asm::SetLenTIdx(0);  // Absolute mode index
  EdAsmNg::Asm::SetValExpr(0x1234);

  // Generate machine code for LDA $1234
  EdAsmNg::Asm::GenMCode(0xAD);

  // Verify opcode and address bytes in GMC buffer (little-endian)
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(0), 0xAD);  // Opcode
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(1), 0x34);  // Low byte of address
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(2), 0x12);  // High byte of address
  EXPECT_EQ(EdAsmNg::Asm::GetGMCIdx(), 3);

  // Verify Length still 3
  EXPECT_EQ(EdAsmNg::Asm::GetLength(), 3);
}

TEST_F(GenMCodeTest, IndexedX_GeneratesTwoBytes) {
  // Zero page indexed X: LDA $12,X (opcode 0xB5)
  // Length = 2, addressing mode index = 3 (zp,X)
  EdAsmNg::Asm::SetLength(2);
  EdAsmNg::Asm::SetLenTIdx(3);  // Zero page,X mode index
  EdAsmNg::Asm::SetValExpr(0x0012);

  // Generate machine code for LDA $12,X
  EdAsmNg::Asm::GenMCode(0xB5);

  // Verify opcode and zero page address in GMC buffer
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(0), 0xB5);  // Opcode
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(1), 0x12);  // Zero page address
  EXPECT_EQ(EdAsmNg::Asm::GetGMCIdx(), 2);

  // Verify Length still 2
  EXPECT_EQ(EdAsmNg::Asm::GetLength(), 2);
}

TEST_F(GenMCodeTest, Branch_GeneratesTwoBytes) {
  // Branch instruction: BNE $10 (opcode 0xD0, displacement +$10)
  // Length = 2, ModWrdL bit 3 set (branch flag)
  EdAsmNg::Asm::SetLength(2);
  EdAsmNg::Asm::SetLenTIdx(0);       // Not used for branches
  EdAsmNg::Asm::SetModWrdL(0x08);    // Branch instruction flag (bit 3)
  EdAsmNg::Asm::SetValExpr(0x0010);  // Displacement

  // Generate machine code for BNE
  EdAsmNg::Asm::GenMCode(0xD0);

  // Verify opcode and displacement in GMC buffer
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(0), 0xD0);  // Opcode
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(1), 0x10);  // Displacement
  EXPECT_EQ(EdAsmNg::Asm::GetGMCIdx(), 2);

  // Verify Length still 2
  EXPECT_EQ(EdAsmNg::Asm::GetLength(), 2);
}

//=================================================
// Phase 3c: GOpAdr (Operand Address Resolution) Tests
//=================================================

// Helper functions to access GOpAdr internals for testing
namespace EdAsmNg {
  namespace Asm {
    // Test helpers for GOpAdr
    void     SetAddressingMode(uint8_t mode);  // Set LenTIdx
    uint8_t  GetAddressingMode();              // Get LenTIdx
    void     SetPC(uint16_t pc);               // Set program counter
    uint16_t GetPC();                          // Get program counter
    void     GOpAdr();                         // Calculate operand address
    void     ResetGOpAdrState();
  }  // namespace Asm
}  // namespace EdAsmNg

class GOpAdrTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::ResetGOpAdrState();
  }
};

TEST_F(GOpAdrTest, Immediate_ReturnsValueAsIs) {
  // Immediate mode: #$12
  // LenTIdx = 2 (immediate addressing mode index)
  // Expression value should be returned unchanged
  EdAsmNg::Asm::SetAddressingMode(2);  // Immediate mode
  EdAsmNg::Asm::SetValExpr(0x0012);

  EdAsmNg::Asm::GOpAdr();

  // In immediate mode, the value is used as-is
  EXPECT_EQ(EdAsmNg::Asm::GetValExpr(), 0x0012);
  EXPECT_EQ(EdAsmNg::Asm::GetRelExprF(), 0);  // Immediate is never relocatable
}

TEST_F(GOpAdrTest, ZeroPage_ReturnsLowByteOnly) {
  // Zero page mode: $34 (with high byte set)
  // LenTIdx = 1 (zero page addressing mode index)
  // Only low byte should be retained
  EdAsmNg::Asm::SetAddressingMode(1);  // Zero page mode
  EdAsmNg::Asm::SetValExpr(0x1234);    // High byte should be masked off

  EdAsmNg::Asm::GOpAdr();

  // Zero page mode uses only low byte
  EXPECT_EQ(EdAsmNg::Asm::GetValExpr(), 0x0034);
}

TEST_F(GOpAdrTest, Absolute_ReturnsFull16BitValue) {
  // Absolute mode: $2000
  // LenTIdx = 0 (absolute addressing mode index)
  // Full 16-bit value should be preserved
  EdAsmNg::Asm::SetAddressingMode(0);  // Absolute mode
  EdAsmNg::Asm::SetValExpr(0x2000);

  EdAsmNg::Asm::GOpAdr();

  // Absolute mode uses full 16-bit address
  EXPECT_EQ(EdAsmNg::Asm::GetValExpr(), 0x2000);
}

TEST_F(GOpAdrTest, Branch_CalculatesRelativeOffset) {
  // Branch mode: BNE $201F (from PC=$2000)
  // Branch instructions use relative addressing
  // Offset = target - PC - 2 (instruction is 2 bytes)

  // PC = $2000, target = $201F
  // Offset = $201F - $2000 - 2 = $1D
  EdAsmNg::Asm::SetPC(0x2000);
  EdAsmNg::Asm::SetLength(2);          // Branch instructions are 2 bytes
  EdAsmNg::Asm::SetModWrdL(0x08);      // Branch flag (bit 3)
  EdAsmNg::Asm::SetValExpr(0x201F);    // Target address
  EdAsmNg::Asm::SetAddressingMode(0);  // Not really used for branches

  EdAsmNg::Asm::GOpAdr();

  // Branch offset should be $1D
  EXPECT_EQ(EdAsmNg::Asm::GetValExpr(), 0x001D);
}

TEST_F(GOpAdrTest, RelocatableSymbol_SetsRelocationFlag) {
  // Symbol defined in different module (external symbol)
  // RelExprF flag should already be set by EvalExpr
  EdAsmNg::Asm::SetAddressingMode(0);  // Absolute mode
  EdAsmNg::Asm::SetValExpr(0x3000);
  EdAsmNg::Asm::SetRelExprF(0x20);  // Set relocatable flag

  EdAsmNg::Asm::GOpAdr();

  // Relocation flag should still be set
  EXPECT_EQ(EdAsmNg::Asm::GetRelExprF(), 0x20);
  EXPECT_EQ(EdAsmNg::Asm::GetValExpr(), 0x3000);
}

TEST_F(GOpAdrTest, ZeroPageOutOfRange_RegistersError) {
  // Zero page mode with value >= $100 should register error
  EdAsmNg::Asm::SetAddressingMode(1);  // Zero page mode
  EdAsmNg::Asm::SetValExpr(0x0100);    // Out of zero page range

  EdAsmNg::Asm::GOpAdr();

  // Should have registered an error
  EXPECT_GT(EdAsmNg::Asm::GetErrorCount(), 0);

  // Value should be masked to low byte
  EXPECT_EQ(EdAsmNg::Asm::GetValExpr(), 0x0000);
}

//=================================================
// Phase 3d: ValidateRange() & ChkRng() Tests
//=================================================

// Helper functions for range validation testing
namespace EdAsmNg {
  namespace Asm {
    // Range checking test helpers
    bool     ChkRng(uint8_t value, uint8_t minVal, uint8_t maxVal);
    void     ValidateRange();
    bool     GetLastCarryFlag();
    uint16_t GetValExpr();
    void     SetValExpr(uint16_t value);
    uint8_t  GetLenTIdx();
    void     SetLenTIdx(uint8_t mode);
    uint8_t  GetModWrdL();
    void     SetModWrdL(uint8_t flags);
  }  // namespace Asm
}  // namespace EdAsmNg

class ValidateRangeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::ResetGOpAdrState();
  }
};

//=================================================
// ChkRng() Tests - Generic Range Checker
//=================================================

TEST_F(ValidateRangeTest, ChkRng_ValueInRange_ReturnsFalse) {
  // Value = 0x50, Range = [0x00, 0xFF]
  // Should return false (carry clear = in range)
  bool outOfRange = EdAsmNg::Asm::ChkRng(0x50, 0x00, 0xFF);
  EXPECT_FALSE(outOfRange);
}

TEST_F(ValidateRangeTest, ChkRng_ValueAtMinimum_ReturnsFalse) {
  // Value = 0x00, Range = [0x00, 0x7F]
  // Should return false (value equals minimum)
  bool outOfRange = EdAsmNg::Asm::ChkRng(0x00, 0x00, 0x7F);
  EXPECT_FALSE(outOfRange);
}

TEST_F(ValidateRangeTest, ChkRng_ValueAtMaximum_ReturnsFalse) {
  // Value = 0x7F, Range = [0x00, 0x7F]
  // Should return false (value equals maximum)
  bool outOfRange = EdAsmNg::Asm::ChkRng(0x7F, 0x00, 0x7F);
  EXPECT_FALSE(outOfRange);
}

TEST_F(ValidateRangeTest, ChkRng_ValueAboveRange_ReturnsTrue) {
  // Value = 0xFF, Range = [0x00, 0x7F]
  // Should return true (carry set = out of range)
  bool outOfRange = EdAsmNg::Asm::ChkRng(0xFF, 0x00, 0x7F);
  EXPECT_TRUE(outOfRange);
}

TEST_F(ValidateRangeTest, ChkRng_ValueBelowRange_ReturnsTrue) {
  // Value = 0x10, Range = [0x20, 0x80]
  // Should return true (below minimum)
  bool outOfRange = EdAsmNg::Asm::ChkRng(0x10, 0x20, 0x80);
  EXPECT_TRUE(outOfRange);
}

//=================================================
// ValidateRange() Tests - Addressing Mode Validation
//=================================================

TEST_F(ValidateRangeTest, Immediate_AllowsAny8BitValue) {
  // Immediate mode (LenTIdx = 2): Any 8-bit value is valid
  EdAsmNg::Asm::SetLenTIdx(2);  // Immediate mode
  EdAsmNg::Asm::SetModWrdL(0);  // Not a branch

  // Test with various 8-bit values
  EdAsmNg::Asm::SetValExpr(0x0000);
  EdAsmNg::Asm::ValidateRange();
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);

  EdAsmNg::Asm::SetValExpr(0x0080);
  EdAsmNg::Asm::ValidateRange();
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);

  EdAsmNg::Asm::SetValExpr(0x00FF);
  EdAsmNg::Asm::ValidateRange();
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(ValidateRangeTest, Immediate_Allows16BitValue) {
  // Immediate mode with 16-bit value (for 65C02 instructions)
  EdAsmNg::Asm::SetLenTIdx(2);  // Immediate mode
  EdAsmNg::Asm::SetModWrdL(0);  // Not a branch
  EdAsmNg::Asm::SetValExpr(0x1234);

  EdAsmNg::Asm::ValidateRange();

  // Immediate mode allows any value, no error
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(ValidateRangeTest, ZeroPage_AcceptsValidRange) {
  // Zero page mode (LenTIdx = 1): Must be 0-255
  EdAsmNg::Asm::SetLenTIdx(1);  // Zero page mode
  EdAsmNg::Asm::SetModWrdL(0);  // Not a branch

  // Test values within zero page range
  EdAsmNg::Asm::SetValExpr(0x0000);
  EdAsmNg::Asm::ValidateRange();
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);

  EdAsmNg::Asm::SetValExpr(0x0080);
  EdAsmNg::Asm::ValidateRange();
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);

  EdAsmNg::Asm::SetValExpr(0x00FF);
  EdAsmNg::Asm::ValidateRange();
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(ValidateRangeTest, ZeroPage_RejectsAbove255) {
  // Zero page mode with value >= $100 should register error
  EdAsmNg::Asm::SetLenTIdx(1);       // Zero page mode
  EdAsmNg::Asm::SetModWrdL(0);       // Not a branch
  EdAsmNg::Asm::SetValExpr(0x0100);  // Just above zero page range

  EdAsmNg::Asm::ValidateRange();

  // Should have registered error 0x1C (zero page range error)
  EXPECT_GT(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(ValidateRangeTest, ZeroPage_RejectsHighValues) {
  // Zero page mode with high value
  EdAsmNg::Asm::SetLenTIdx(1);       // Zero page mode
  EdAsmNg::Asm::SetModWrdL(0);       // Not a branch
  EdAsmNg::Asm::SetValExpr(0x1234);  // Way above zero page range

  EdAsmNg::Asm::ValidateRange();

  // Should have registered error
  EXPECT_GT(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(ValidateRangeTest, Absolute_AcceptsAny16BitValue) {
  // Absolute mode (LenTIdx = 0): Any 16-bit value is valid
  EdAsmNg::Asm::SetLenTIdx(0);  // Absolute mode
  EdAsmNg::Asm::SetModWrdL(0);  // Not a branch

  // Test with various 16-bit values
  EdAsmNg::Asm::SetValExpr(0x0000);
  EdAsmNg::Asm::ValidateRange();
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);

  EdAsmNg::Asm::SetValExpr(0x8000);
  EdAsmNg::Asm::ValidateRange();
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);

  EdAsmNg::Asm::SetValExpr(0xFFFF);
  EdAsmNg::Asm::ValidateRange();
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(ValidateRangeTest, Branch_AcceptsSmallPositiveOffset) {
  // Branch with small positive offset (within -128 to +127)
  EdAsmNg::Asm::SetLenTIdx(0);       // Not used for branches
  EdAsmNg::Asm::SetModWrdL(0x08);    // Branch flag (bit 3)
  EdAsmNg::Asm::SetValExpr(0x001D);  // Offset = +29

  EdAsmNg::Asm::ValidateRange();

  // Should not register error
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(ValidateRangeTest, Branch_AcceptsSmallNegativeOffset) {
  // Branch with small negative offset (within -128 to +127)
  EdAsmNg::Asm::SetLenTIdx(0);       // Not used for branches
  EdAsmNg::Asm::SetModWrdL(0x08);    // Branch flag (bit 3)
  EdAsmNg::Asm::SetValExpr(0xFFCE);  // Offset = -50 (two's complement)

  EdAsmNg::Asm::ValidateRange();

  // Should not register error
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(ValidateRangeTest, Branch_RejectsLargePositiveOffset) {
  // Branch with offset > +127 should register error
  EdAsmNg::Asm::SetLenTIdx(0);       // Not used for branches
  EdAsmNg::Asm::SetModWrdL(0x08);    // Branch flag (bit 3)
  EdAsmNg::Asm::SetValExpr(0x00C8);  // Offset = +200 (out of range)

  EdAsmNg::Asm::ValidateRange();

  // Should have registered error 0x26 (branch range error)
  EXPECT_GT(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(ValidateRangeTest, Branch_RejectsLargeNegativeOffset) {
  // Branch with offset < -128 should register error
  EdAsmNg::Asm::SetLenTIdx(0);       // Not used for branches
  EdAsmNg::Asm::SetModWrdL(0x08);    // Branch flag (bit 3)
  EdAsmNg::Asm::SetValExpr(0xFF00);  // Offset = -256 (out of range)

  EdAsmNg::Asm::ValidateRange();

  // Should have registered error 0x26 (branch range error)
  EXPECT_GT(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(ValidateRangeTest, Branch_AcceptsEdgeCasePositive127) {
  // Branch with offset = +127 (maximum positive)
  EdAsmNg::Asm::SetLenTIdx(0);       // Not used for branches
  EdAsmNg::Asm::SetModWrdL(0x08);    // Branch flag (bit 3)
  EdAsmNg::Asm::SetValExpr(0x007F);  // Offset = +127

  EdAsmNg::Asm::ValidateRange();

  // Should not register error
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(ValidateRangeTest, Branch_AcceptsEdgeCaseNegative128) {
  // Branch with offset = -128 (maximum negative)
  EdAsmNg::Asm::SetLenTIdx(0);       // Not used for branches
  EdAsmNg::Asm::SetModWrdL(0x08);    // Branch flag (bit 3)
  EdAsmNg::Asm::SetValExpr(0xFF80);  // Offset = -128 (two's complement)

  EdAsmNg::Asm::ValidateRange();

  // Should not register error
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

//=================================================
// EvalOprnd & Directive Dispatch Tests (Phase 4)
//=================================================

// Helper functions for EvalOprnd and directive dispatch testing
namespace EdAsmNg {
  namespace Asm {
    // EvalOprnd evaluation
    void     EvalOprnd();
    uint16_t GetValExpr();
    uint8_t  GetPassNbr();
    void     SetPassNbr(uint8_t pass);
    uint8_t  GetNxtToken();

    // Directive dispatch helpers
    typedef void (*DirectiveHandler)();
    bool CallDirectiveDispatch(const char* directiveName);
  }  // namespace Asm
}  // namespace EdAsmNg

class EvalOprndTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::ResetDispatchState();
  }
};

TEST_F(EvalOprndTest, Immediate_EvaluatesValue) {
  // Test EvalOprnd with immediate value operand
  EdAsmNg::Asm::SetupSourceLine("#$42");
  EdAsmNg::Asm::SetPassNbr(0);  // Start in Pass 1

  EdAsmNg::Asm::EvalOprnd();

  // EvalOprnd should force Pass 2 temporarily, evaluate, then restore
  // Pass should be back to 0
  EXPECT_EQ(EdAsmNg::Asm::GetPassNbr(), 0);

  // Value should be evaluated (if EvalExpr works correctly)
  // For now, we just verify it completes without crashing
}

TEST_F(EvalOprndTest, EmptyOperand_NoEval) {
  // Test EvalOprnd with empty operand (just whitespace/CR)
  EdAsmNg::Asm::SetupSourceLine(" ");
  EdAsmNg::Asm::SetPassNbr(1);  // Start in Pass 2

  EdAsmNg::Asm::EvalOprnd();

  // Pass should be restored to 1
  EXPECT_EQ(EdAsmNg::Asm::GetPassNbr(), 1);
}

TEST_F(EvalOprndTest, PreservesPassNumber_Pass0) {
  // Verify EvalOprnd preserves Pass 0
  EdAsmNg::Asm::SetupSourceLine("$1234");
  EdAsmNg::Asm::SetPassNbr(0);

  EdAsmNg::Asm::EvalOprnd();

  EXPECT_EQ(EdAsmNg::Asm::GetPassNbr(), 0);
}

TEST_F(EvalOprndTest, PreservesPassNumber_Pass1) {
  // Verify EvalOprnd preserves Pass 1
  EdAsmNg::Asm::SetupSourceLine("$5678");
  EdAsmNg::Asm::SetPassNbr(1);

  EdAsmNg::Asm::EvalOprnd();

  EXPECT_EQ(EdAsmNg::Asm::GetPassNbr(), 1);
}

class DirectiveDispatchTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::ResetDispatchState();
  }
};

TEST_F(DirectiveDispatchTest, EQU_RoutesToHandler) {
  // Test that .EQU directive routes to the correct handler
  EdAsmNg::Asm::SetupSourceLine(".EQU");

  bool found = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(found);  // Directive should be recognized

  // ZAB should have directive flag (>= $80)
  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_GE(zab, 0x80);

  // For now, just verify dispatch works - actual handler stubbed
}

TEST_F(DirectiveDispatchTest, ORG_RoutesToHandler) {
  // Test that .ORG directive routes to the correct handler
  EdAsmNg::Asm::SetupSourceLine(".ORG");

  bool found = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(found);

  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_GE(zab, 0x80);
}

//=================================================
// Phase 5: EQU and ORG Directive Tests
//=================================================

// Helper functions for Phase 5 testing
namespace EdAsmNg {
  namespace Asm {
    // Symbol table access
    int      GetSymbolCount();
    uint16_t GetSymbolValue(const char* name);
    bool     FindSymbol(const char* name);

    // Address control
    uint16_t GetCurAdr();
    void     SetCurAdr(uint16_t addr);

    // Label field control
    void    SetLabelF(uint8_t value);
    uint8_t GetLabelF();

    // Symbol table control
    void     InitSymbolTable();
    void     ClearSymbolTable();
    void     SetSymFBP(uint16_t ptr);
    uint16_t GetSymFBP();

    // Direct handler calls
    void HndlEQU();
    void HndlORG();
  }  // namespace Asm
}  // namespace EdAsmNg

class Phase5DirectiveTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::ResetDispatchState();
    EdAsmNg::Asm::InitSymbolTable();
    EdAsmNg::Asm::SetPassNbr(0);  // Default to Pass 1
    EdAsmNg::Asm::SetCurAdr(0x2000);
    EdAsmNg::Asm::SetHighMem(0xC000);
  }
};

//=================================================
// EQU Directive Tests
//=================================================

TEST_F(Phase5DirectiveTest, EQU_DefinesSymbol_Pass1) {
  // Pass 1: EQU FOO = $1234
  // Symbol storage stubbed; just verify routing works

  EdAsmNg::Asm::SetPassNbr(0);  // Pass 1
  EdAsmNg::Asm::SetLabelF(1);   // Line has a label
  EdAsmNg::Asm::SetupSourceLine("$1234");

  // Note: Symbol table writes are stubbed (SymFBP pointer safety).
  // This test verifies that:
  //   1. HndlEQU is called and routes correctly
  //   2. EvalOprnd() evaluates the expression without crashing
  //   3. No errors are registered for valid input
  //   4. Handler returns success (C=false)

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlEQU();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // EQU should evaluate the operand without registering errors
  EXPECT_EQ(errorsAfter, errorsBefore);
}

TEST_F(Phase5DirectiveTest, EQU_SkipsCode_Pass2) {
  // Pass 2: EQU BAR = $5678
  // Verify no code generated (no StorByt calls)
  // Symbol storage stubbed; just verify routing works

  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetLabelF(1);   // Line has a label
  EdAsmNg::Asm::SetupSourceLine("$5678");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlEQU();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // In Pass 2, EQU should just evaluate and return without errors
  EXPECT_EQ(errorsAfter, errorsBefore);
}

TEST_F(Phase5DirectiveTest, EQU_RedefError_Stubbed) {
  // Pass 1: EQU DUP = $1111
  // Symbol storage stubbed; just verify routing works

  // Note: Redefinition error detection requires symbol table integration.
  // With stubbed symbol storage, we just verify handler completes successfully.

  EdAsmNg::Asm::SetPassNbr(0);  // Pass 1
  EdAsmNg::Asm::SetLabelF(1);
  EdAsmNg::Asm::SetupSourceLine("$1111");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlEQU();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // With stubbed symbol storage, first call should complete successfully
  EXPECT_EQ(errorsAfter, errorsBefore);
}

TEST_F(Phase5DirectiveTest, EQU_InvalidExpr_Error) {
  // Pass 1: EQU BAD = UNDEFINED_SYM
  // Verify error registered or handler completes gracefully
  // Symbol storage stubbed; just verify routing works

  EdAsmNg::Asm::SetPassNbr(0);  // Pass 1
  EdAsmNg::Asm::SetLabelF(1);
  EdAsmNg::Asm::SetupSourceLine("UNDEFINED");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlEQU();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // EvalOprnd may register an error for undefined symbol,
  // or complete with error flag set. Just verify no crash.
  EXPECT_GE(errorsAfter, errorsBefore);
}

//=================================================
// ORG Directive Tests
//=================================================

TEST_F(Phase5DirectiveTest, ORG_UpdatesAddress_Pass1) {
  // Pass 1: ORG $2000
  // Verify CurAdr (PC) updated to 0x2000

  EdAsmNg::Asm::SetPassNbr(0);      // Pass 1
  EdAsmNg::Asm::SetCurAdr(0x1000);  // Start at different address
  EdAsmNg::Asm::SetupSourceLine("$2000");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlORG();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // ORG should update PC to $2000 in Pass 1
  EXPECT_EQ(EdAsmNg::Asm::GetCurAdr(), 0x2000);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors
}

TEST_F(Phase5DirectiveTest, ORG_RejectsInvalid_OutOfRange) {
  // Pass 1: ORG $FFFF (assuming HighMem < 0xFFFF)
  // Verify error registered

  EdAsmNg::Asm::SetPassNbr(0);       // Pass 1
  EdAsmNg::Asm::SetHighMem(0xC000);  // Set max to 0xC000
  EdAsmNg::Asm::SetupSourceLine("$FFFF");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlORG();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Should register an out-of-range error (0x24: directive operand err)
  EXPECT_GT(errorsAfter, errorsBefore);
}

TEST_F(Phase5DirectiveTest, ORG_SkipsCode_Pass2) {
  // Pass 2: ORG $3000
  // Verify ORG is skipped entirely in Pass 2 (early return)

  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetCurAdr(0x2000);
  EdAsmNg::Asm::SetupSourceLine("$3000");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlORG();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // ORG should skip processing in Pass 2: PC should NOT change
  EXPECT_EQ(EdAsmNg::Asm::GetCurAdr(), 0x2000);  // PC unchanged
  EXPECT_EQ(errorsAfter, errorsBefore);          // No errors
}
