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
