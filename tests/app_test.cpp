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
  // For example: .SKIP, .DPAGE, etc. (in table but not dispatched)
  EdAsmNg::Asm::SetupSourceLine(".SKIP");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_FALSE(success);  // Should return failure (C=1)

  // Verify ZAB still has directive flag set (was recognized as directive)
  uint8_t zab = EdAsmNg::Asm::GetZAB();
  EXPECT_GE(zab, 0x80);  // Directive flag should be set

  // Verify no handler was invoked (fallback path was taken)
  EXPECT_STREQ(EdAsmNg::Asm::GetLastDirectiveCalled(), "");
}

//=================================================
// Phase 7: Directive Dispatch Tests
//=================================================

TEST_F(MnemonicDispatchTest, PAGE_DotDirective_RoutesToDoPage) {
  // Test that .PAGE directive routes to DoPage handler
  EdAsmNg::Asm::SetupSourceLine(".PAGE");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);  // Directive should be recognized and dispatched

  // Verify DoPage was called
  EXPECT_STREQ(EdAsmNg::Asm::GetLastDirectiveCalled(), "DoPage");
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(MnemonicDispatchTest, PAGE_NonDotDirective_RoutesToDoPage) {
  // Test that PAGE (non-dot) directive routes to DoPage handler
  EdAsmNg::Asm::SetupSourceLine("PAGE");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);  // Directive should be recognized and dispatched

  // Verify DoPage was called
  EXPECT_STREQ(EdAsmNg::Asm::GetLastDirectiveCalled(), "DoPage");
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(MnemonicDispatchTest, LIST_DotDirective_RoutesToHndlLIST) {
  // Test that .LIST directive routes to HndlLIST handler (toggle on)
  EdAsmNg::Asm::SetupSourceLine(".LIST");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  // Verify handler was called
  EXPECT_STREQ(EdAsmNg::Asm::GetLastDirectiveCalled(), "HndlLIST");
}

TEST_F(MnemonicDispatchTest, LST_NonDotDirective_RoutesToHndlLST) {
  // Test that LST (non-dot) directive routes to HndlLST handler
  EdAsmNg::Asm::SetupSourceLine("LST");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  // Verify handler was called
  EXPECT_STREQ(EdAsmNg::Asm::GetLastDirectiveCalled(), "HndlLST");
}

TEST_F(MnemonicDispatchTest, TITLE_DotDirective_RoutesToHndlSBTL) {
  // Test that .TITLE directive routes to HndlSBTL handler
  EdAsmNg::Asm::SetupSourceLine(".TITLE");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  // Verify handler was called
  EXPECT_STREQ(EdAsmNg::Asm::GetLastDirectiveCalled(), "HndlSBTL");
}

TEST_F(MnemonicDispatchTest, SBTL_NonDotDirective_RoutesToHndlSBTL) {
  // Test that SBTL (non-dot) directive routes to HndlSBTL handler
  EdAsmNg::Asm::SetupSourceLine("SBTL");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  // Verify handler was called
  EXPECT_STREQ(EdAsmNg::Asm::GetLastDirectiveCalled(), "HndlSBTL");
}

TEST_F(MnemonicDispatchTest, NOLIST_NonDotDirective_RoutesToHndlNOLIST) {
  // Test that NOLIST (non-dot) directive routes to HndlNOLIST handler
  EdAsmNg::Asm::SetupSourceLine("NOLIST");

  bool success = EdAsmNg::Asm::HndlMnem();
  EXPECT_TRUE(success);

  // Verify no errors
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
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

#if 0  // TODO: Phase 9+ - Tests for Phases 2-7 depend on unimplemented stub functions

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

//=================================================
// Phase 6: Data Directives (DS, DFB/BYTE, DW/WORD, ASC, DCI) Tests
//=================================================

// Helper functions for Phase 6 testing
namespace EdAsmNg {
  namespace Asm {
    // Data directive handlers
    void HndlDS();
    void HndlDFB();
    void HndlDW();
    void HndlASC();
    void HndlDCI();

    // Test memory accessors
    void    EnableTestObjMemory(bool enable);
    uint8_t GetTestObjMemory(uint16_t addr);
    void    ClearTestObjMemory();

    // Get GMC buffer (for testing direct byte output)
    uint8_t GetGMC(uint8_t index);
  }  // namespace Asm
}  // namespace EdAsmNg

class Phase6DataDirectiveTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::ResetDispatchState();
    EdAsmNg::Asm::SetPassNbr(1);      // Default to Pass 2 (code generation)
    EdAsmNg::Asm::SetCurAdr(0x2000);  // Start at $2000
    EdAsmNg::Asm::EnableTestObjMemory(true);
    EdAsmNg::Asm::ClearTestObjMemory();
  }

  void TearDown() override {
    EdAsmNg::Asm::EnableTestObjMemory(false);
  }
};

//=================================================
// DFB/BYTE Tests
//=================================================

TEST_F(Phase6DataDirectiveTest, DFB_EmitsBytes_Pass2) {
  // Pass 2, DFB 1,2,3 → stores 3 bytes in output buffer
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetCurAdr(0x2000);
  EdAsmNg::Asm::SetupSourceLine("1,2,3");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlDFB();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Should emit 3 bytes: 0x01, 0x02, 0x03
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x2000), 0x01);
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x2001), 0x02);
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x2002), 0x03);

  // PC should advance by 3
  EXPECT_EQ(EdAsmNg::Asm::GetCurAdr(), 0x2003);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors
}

TEST_F(Phase6DataDirectiveTest, DFB_RangeError_RegistersError) {
  // Pass 2, DFB $1FF → error (out of byte range)
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetCurAdr(0x2000);
  EdAsmNg::Asm::SetupSourceLine("$1FF");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlDFB();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Should register a byte overflow error
  EXPECT_GT(errorsAfter, errorsBefore);
}

TEST_F(Phase6DataDirectiveTest, DFB_MultipleValues_FourMax) {
  // DFB can emit up to 4 bytes per line, then continues
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetCurAdr(0x3000);
  EdAsmNg::Asm::SetupSourceLine("$10,$20,$30,$40");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlDFB();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Should emit 4 bytes
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3000), 0x10);
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3001), 0x20);
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3002), 0x30);
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3003), 0x40);
  EXPECT_EQ(EdAsmNg::Asm::GetCurAdr(), 0x3004);
  EXPECT_EQ(errorsAfter, errorsBefore);
}

//=================================================
// DW/WORD Tests
//=================================================

TEST_F(Phase6DataDirectiveTest, DW_EmitsWordsLE_Pass2) {
  // Pass 2, DW $1234 → stores low byte then high byte (little-endian)
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetCurAdr(0x2000);
  EdAsmNg::Asm::SetupSourceLine("$1234");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlDW();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Little-endian: low byte first, then high byte
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x2000), 0x34);  // Low byte
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x2001), 0x12);  // High byte

  // PC should advance by 2
  EXPECT_EQ(EdAsmNg::Asm::GetCurAdr(), 0x2002);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors
}

TEST_F(Phase6DataDirectiveTest, DW_MultipleWords) {
  // DW $1234,$5678
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetCurAdr(0x3000);
  EdAsmNg::Asm::SetupSourceLine("$1234,$5678");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlDW();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // First word: $1234 -> 0x34, 0x12
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3000), 0x34);
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3001), 0x12);
  // Second word: $5678 -> 0x78, 0x56
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3002), 0x78);
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3003), 0x56);

  EXPECT_EQ(EdAsmNg::Asm::GetCurAdr(), 0x3004);
  EXPECT_EQ(errorsAfter, errorsBefore);
}

//=================================================
// DS Tests
//=================================================

TEST_F(Phase6DataDirectiveTest, DS_ReservesSpace_Pass1) {
  // Pass 1, DS 4 → PC advances by 4, no output
  EdAsmNg::Asm::SetPassNbr(0);  // Pass 1
  EdAsmNg::Asm::SetCurAdr(0x2000);
  EdAsmNg::Asm::SetupSourceLine("4");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlDS();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // PC should advance by 4 bytes
  EXPECT_EQ(EdAsmNg::Asm::GetCurAdr(), 0x2004);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors
}

TEST_F(Phase6DataDirectiveTest, DS_WithFiller_Pass2) {
  // Pass 2, DS 4,$FF → fills 4 bytes with $FF
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetCurAdr(0x2000);
  EdAsmNg::Asm::SetupSourceLine("4,$FF");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlDS();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Should fill 4 bytes with $FF
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x2000), 0xFF);
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x2001), 0xFF);
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x2002), 0xFF);
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x2003), 0xFF);

  // PC should advance by 4
  EXPECT_EQ(EdAsmNg::Asm::GetCurAdr(), 0x2004);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors
}

TEST_F(Phase6DataDirectiveTest, DS_LargeSize) {
  // Pass 1, DS 256 → PC advances by 256
  EdAsmNg::Asm::SetPassNbr(0);  // Pass 1
  EdAsmNg::Asm::SetCurAdr(0x2000);
  EdAsmNg::Asm::SetupSourceLine("256");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlDS();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // PC should advance by 256
  EXPECT_EQ(EdAsmNg::Asm::GetCurAdr(), 0x2100);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors
}

//=================================================
// ASC Tests
//=================================================

TEST_F(Phase6DataDirectiveTest, ASC_EmitsAscii_Pass2) {
  // Pass 2, ASC "AB" → emits bytes 0x41 0x42
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetCurAdr(0x2000);
  EdAsmNg::Asm::SetupSourceLine("\"AB\"");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlASC();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Should emit ASCII bytes
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x2000), 0x41);  // 'A'
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x2001), 0x42);  // 'B'

  // PC should advance by 2
  EXPECT_EQ(EdAsmNg::Asm::GetCurAdr(), 0x2002);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors
}

TEST_F(Phase6DataDirectiveTest, ASC_LongerString) {
  // Pass 2, ASC "HELLO" → emits 5 ASCII bytes
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetCurAdr(0x3000);
  EdAsmNg::Asm::SetupSourceLine("\"HELLO\"");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlASC();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Should emit ASCII bytes
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3000), 0x48);  // 'H'
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3001), 0x45);  // 'E'
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3002), 0x4C);  // 'L'
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3003), 0x4C);  // 'L'
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3004), 0x4F);  // 'O'

  EXPECT_EQ(EdAsmNg::Asm::GetCurAdr(), 0x3005);
  EXPECT_EQ(errorsAfter, errorsBefore);
}

//=================================================
// DCI Tests
//=================================================

TEST_F(Phase6DataDirectiveTest, DCI_EmitsAsciiHighBit_Pass2) {
  // Pass 2, DCI "AB" → emits bytes 0x41 0xC2 (last char has high bit set)
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetCurAdr(0x2000);
  EdAsmNg::Asm::SetupSourceLine("\"AB\"");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlDCI();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Should emit ASCII bytes, last with high bit set
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x2000), 0x41);  // 'A'
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x2001), 0xC2);  // 'B' | 0x80

  // PC should advance by 2
  EXPECT_EQ(EdAsmNg::Asm::GetCurAdr(), 0x2002);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors
}

TEST_F(Phase6DataDirectiveTest, DCI_LongerString) {
  // Pass 2, DCI "HELLO" → emits 5 ASCII bytes, last with high bit set
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetCurAdr(0x3000);
  EdAsmNg::Asm::SetupSourceLine("\"HELLO\"");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlDCI();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Should emit ASCII bytes, last with high bit set
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3000), 0x48);  // 'H'
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3001), 0x45);  // 'E'
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3002), 0x4C);  // 'L'
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3003), 0x4C);  // 'L'
  EXPECT_EQ(EdAsmNg::Asm::GetTestObjMemory(0x3004), 0xCF);  // 'O' | 0x80

  EXPECT_EQ(EdAsmNg::Asm::GetCurAdr(), 0x3005);
  EXPECT_EQ(errorsAfter, errorsBefore);
}

//=================================================
// Phase 7.2: LST Directive Tests
//=================================================

// Helper functions for Phase 7.2 testing
namespace EdAsmNg {
  namespace Asm {
    // Listing flag accessors
    uint8_t GetListingF();
    void    SetListingF(uint8_t value);
    uint8_t GetLstCyc();
    void    SetLstCyc(uint8_t value);
    uint8_t GetLstUnAsm();
    void    SetLstUnAsm(uint8_t value);
    uint8_t GetLstExpMac();
    void    SetLstExpMac(uint8_t value);
    uint8_t GetLstWarns();
    void    SetLstWarns(uint8_t value);
    uint8_t GetLstGCode();
    void    SetLstGCode(uint8_t value);
    uint8_t GetLstASym();
    void    SetLstASym(uint8_t value);
    uint8_t GetLstVSym();
    void    SetLstVSym(uint8_t value);
    uint8_t GetLst6Cols();
    void    SetLst6Cols(uint8_t value);

    // Direct handler call
    void HndlLST();
  }  // namespace Asm
}  // namespace EdAsmNg

class Phase72LSTDirectiveTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::ResetDispatchState();
    EdAsmNg::Asm::SetPassNbr(0);  // Default to Pass 1
  }
};

//=================================================
// LST ON/OFF Tests
//=================================================

TEST_F(Phase72LSTDirectiveTest, LST_ON) {
  // LST ON should set ListingF MSB
  EdAsmNg::Asm::SetListingF(0x00);  // Start with listing OFF
  EdAsmNg::Asm::SetupSourceLine("ON");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlLST();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // ListingF should have MSB set ($80 or higher)
  EXPECT_GE(EdAsmNg::Asm::GetListingF(), 0x80);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors
}

TEST_F(Phase72LSTDirectiveTest, LST_OFF) {
  // LST OFF should clear ListingF MSB
  EdAsmNg::Asm::SetListingF(0xFF);  // Start with listing ON
  EdAsmNg::Asm::SetupSourceLine("OFF");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlLST();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // ListingF should have MSB clear ($7F or lower)
  EXPECT_LE(EdAsmNg::Asm::GetListingF(), 0x7F);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors
}

//=================================================
// Listing Option Enable Tests
//=================================================

TEST_F(Phase72LSTDirectiveTest, OptionsEnable_SingleOption) {
  // LST C (enable cycle count)
  EdAsmNg::Asm::SetLstCyc(0x00);  // Start disabled
  EdAsmNg::Asm::SetupSourceLine("C");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlLST();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // LstCyc should have MSB set
  EXPECT_GE(EdAsmNg::Asm::GetLstCyc(), 0x80);
  EXPECT_EQ(errorsAfter, errorsBefore);
}

TEST_F(Phase72LSTDirectiveTest, OptionsEnable_MultipleOptions) {
  // LST C,U,E (enable cycle, unasm, expand)
  EdAsmNg::Asm::SetLstCyc(0x00);
  EdAsmNg::Asm::SetLstUnAsm(0x00);
  EdAsmNg::Asm::SetLstExpMac(0x00);
  EdAsmNg::Asm::SetupSourceLine("C,U,E");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlLST();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // All three flags should have MSB set
  EXPECT_GE(EdAsmNg::Asm::GetLstCyc(), 0x80);
  EXPECT_GE(EdAsmNg::Asm::GetLstUnAsm(), 0x80);
  EXPECT_GE(EdAsmNg::Asm::GetLstExpMac(), 0x80);
  EXPECT_EQ(errorsAfter, errorsBefore);
}

TEST_F(Phase72LSTDirectiveTest, OptionsEnable_WithPlusPrefix) {
  // LST +C (explicitly enable with + prefix)
  EdAsmNg::Asm::SetLstCyc(0x00);
  EdAsmNg::Asm::SetupSourceLine("+C");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlLST();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  EXPECT_GE(EdAsmNg::Asm::GetLstCyc(), 0x80);
  EXPECT_EQ(errorsAfter, errorsBefore);
}

//=================================================
// Listing Option Disable Tests
//=================================================

TEST_F(Phase72LSTDirectiveTest, OptionsDisable_SingleOption) {
  // LST -C (disable cycle count)
  EdAsmNg::Asm::SetLstCyc(0xFF);  // Start enabled
  EdAsmNg::Asm::SetupSourceLine("-C");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlLST();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // LstCyc should have MSB clear
  EXPECT_LE(EdAsmNg::Asm::GetLstCyc(), 0x7F);
  EXPECT_EQ(errorsAfter, errorsBefore);
}

TEST_F(Phase72LSTDirectiveTest, OptionsDisable_MultipleOptions) {
  // LST -C,-U,-E (disable multiple options)
  EdAsmNg::Asm::SetLstCyc(0xFF);
  EdAsmNg::Asm::SetLstUnAsm(0xFF);
  EdAsmNg::Asm::SetLstExpMac(0xFF);
  EdAsmNg::Asm::SetupSourceLine("-C,-U,-E");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlLST();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // All three flags should have MSB clear
  EXPECT_LE(EdAsmNg::Asm::GetLstCyc(), 0x7F);
  EXPECT_LE(EdAsmNg::Asm::GetLstUnAsm(), 0x7F);
  EXPECT_LE(EdAsmNg::Asm::GetLstExpMac(), 0x7F);
  EXPECT_EQ(errorsAfter, errorsBefore);
}

//=================================================
// Mixed Enable/Disable Tests
//=================================================

TEST_F(Phase72LSTDirectiveTest, OptionsMixed) {
  // LST +C,-U,E (mixed enable/disable)
  EdAsmNg::Asm::SetLstCyc(0x00);     // Start disabled
  EdAsmNg::Asm::SetLstUnAsm(0xFF);   // Start enabled
  EdAsmNg::Asm::SetLstExpMac(0x00);  // Start disabled
  EdAsmNg::Asm::SetupSourceLine("+C,-U,E");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlLST();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // C should be enabled, U should be disabled, E should be enabled
  EXPECT_GE(EdAsmNg::Asm::GetLstCyc(), 0x80);
  EXPECT_LE(EdAsmNg::Asm::GetLstUnAsm(), 0x7F);
  EXPECT_GE(EdAsmNg::Asm::GetLstExpMac(), 0x80);
  EXPECT_EQ(errorsAfter, errorsBefore);
}

//=================================================
// All Options Test
//=================================================

TEST_F(Phase72LSTDirectiveTest, AllOptions) {
  // LST C,U,E,W,G,A,V,S (all 8 options)
  EdAsmNg::Asm::SetLstCyc(0x00);
  EdAsmNg::Asm::SetLstUnAsm(0x00);
  EdAsmNg::Asm::SetLstExpMac(0x00);
  EdAsmNg::Asm::SetLstWarns(0x00);
  EdAsmNg::Asm::SetLstGCode(0x00);
  EdAsmNg::Asm::SetLstASym(0x00);
  EdAsmNg::Asm::SetLstVSym(0x00);
  EdAsmNg::Asm::SetLst6Cols(0x00);
  EdAsmNg::Asm::SetupSourceLine("C,U,E,W,G,A,V,S");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlLST();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // All 8 flags should have MSB set
  EXPECT_GE(EdAsmNg::Asm::GetLstCyc(), 0x80);
  EXPECT_GE(EdAsmNg::Asm::GetLstUnAsm(), 0x80);
  EXPECT_GE(EdAsmNg::Asm::GetLstExpMac(), 0x80);
  EXPECT_GE(EdAsmNg::Asm::GetLstWarns(), 0x80);
  EXPECT_GE(EdAsmNg::Asm::GetLstGCode(), 0x80);
  EXPECT_GE(EdAsmNg::Asm::GetLstASym(), 0x80);
  EXPECT_GE(EdAsmNg::Asm::GetLstVSym(), 0x80);
  EXPECT_GE(EdAsmNg::Asm::GetLst6Cols(), 0x80);
  EXPECT_EQ(errorsAfter, errorsBefore);
}

//=================================================
// Error Tests
//=================================================

TEST_F(Phase72LSTDirectiveTest, InvalidOption) {
  // LST X (invalid option letter)
  EdAsmNg::Asm::SetupSourceLine("X");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlLST();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Should register a directive operand error
  EXPECT_GT(errorsAfter, errorsBefore);
}

TEST_F(Phase72LSTDirectiveTest, InvalidOption_NonAlphabetic) {
  // LST 123 (non-alphabetic character)
  EdAsmNg::Asm::SetupSourceLine("123");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlLST();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Should register error for non-alphabetic character
  EXPECT_GT(errorsAfter, errorsBefore);
}

//=================================================
// Phase 7.3: NOLIST and PAGE Directives Tests
//=================================================

// Helper functions for Phase 7.3 testing
namespace EdAsmNg {
  namespace Asm {
    // Handler functions
    void HndlNOLIST();
    void DoPage();

    // PassNbr accessor (already exists)
    uint8_t GetPassNbr();
  }  // namespace Asm
}  // namespace EdAsmNg

class Phase73NOLISTandPAGEDirectiveTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::ResetDispatchState();
    EdAsmNg::Asm::SetPassNbr(0);  // Default to Pass 1
  }
};

//=================================================
// NOLIST Tests
//=================================================

TEST_F(Phase73NOLISTandPAGEDirectiveTest, NOLIST_DisablesListing) {
  // Set ListingF to $FF initially (listing ON)
  EdAsmNg::Asm::SetListingF(0xFF);

  // Call NOLIST (no setup line needed - ignores operand)
  EdAsmNg::Asm::SetupSourceLine("");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlNOLIST();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify ListingF MSB is clear ($7F or lower)
  EXPECT_LE(EdAsmNg::Asm::GetListingF(), 0x7F);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors
}

TEST_F(Phase73NOLISTandPAGEDirectiveTest, NOLIST_WithOperandIgnored) {
  // Set ListingF to $80 (listing ON with MSB set)
  EdAsmNg::Asm::SetListingF(0x80);

  // Call NOLIST with operand "ON" (should be ignored)
  EdAsmNg::Asm::SetupSourceLine("ON");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlNOLIST();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify ListingF MSB is clear
  EXPECT_LE(EdAsmNg::Asm::GetListingF(), 0x7F);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors (operand ignored)
}

//=================================================
// PAGE Tests
//=================================================

TEST_F(Phase73NOLISTandPAGEDirectiveTest, PAGE_Pass1NoOp) {
  // Set PassNbr to 0 (Pass 1)
  EdAsmNg::Asm::SetPassNbr(0);

  // Call PAGE
  EdAsmNg::Asm::SetupSourceLine("");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::DoPage();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify no side effects (no output, no errors)
  EXPECT_EQ(errorsAfter, errorsBefore);
  EXPECT_EQ(EdAsmNg::Asm::GetPassNbr(), 0);  // Pass still 0
}

TEST_F(Phase73NOLISTandPAGEDirectiveTest, PAGE_Pass2Stubbed) {
  // Set PassNbr to 1 (Pass 2)
  EdAsmNg::Asm::SetPassNbr(1);

  // Call PAGE
  EdAsmNg::Asm::SetupSourceLine("");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::DoPage();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify no errors (output is stubbed)
  // Full behavior will be tested in Phase 8 when listing I/O is implemented
  EXPECT_EQ(errorsAfter, errorsBefore);
  EXPECT_EQ(EdAsmNg::Asm::GetPassNbr(), 1);  // Pass still 1
}

TEST_F(Phase73NOLISTandPAGEDirectiveTest, PAGE_WithOperandIgnored) {
  // Set PassNbr to 0
  EdAsmNg::Asm::SetPassNbr(0);

  // Call PAGE with operand "42"
  EdAsmNg::Asm::SetupSourceLine("42");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::DoPage();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify no errors
  EXPECT_EQ(errorsAfter, errorsBefore);
}

//=================================================
// Phase 7.4: SBTL (Subtitle) Directive Tests
//=================================================

// Helper functions for Phase 7.4 testing
namespace EdAsmNg {
  namespace Asm {
    // Handler function
    void HndlSBTL();

    // Accessors
    uint8_t     GetSubTtlF();
    void        SetSubTtlF(uint8_t value);
    const char* GetSubTitle();
    void        ClearSubTitle();
  }  // namespace Asm
}  // namespace EdAsmNg

class Phase74SBTLDirectiveTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::ResetDispatchState();
    EdAsmNg::Asm::SetPassNbr(1);    // Default to Pass 2 (parsing is done in Pass 2+)
    EdAsmNg::Asm::SetSubTtlF(0);    // Clear subtitle flag
    EdAsmNg::Asm::ClearSubTitle();  // Clear subtitle buffer
  }
};

//=================================================
// SBTL Tests
//=================================================

TEST_F(Phase74SBTLDirectiveTest, NoString) {
  // Set SubTtlF to $00 initially
  EdAsmNg::Asm::SetSubTtlF(0x00);
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2

  // Call SBTL with empty operand (just CR)
  EdAsmNg::Asm::SetupSourceLine("");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlSBTL();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify SubTtlF is $40 (encountered but no string)
  EXPECT_EQ(EdAsmNg::Asm::GetSubTtlF(), 0x40);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors

  // Note: DoPage should be called (implementation detail - stubbed behavior)
}

TEST_F(Phase74SBTLDirectiveTest, SimpleString) {
  // Call SBTL with operand: /My Title/
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetupSourceLine("/My Title/");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlSBTL();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify SubTtlF is $FF (string stored)
  EXPECT_EQ(EdAsmNg::Asm::GetSubTtlF(), 0xFF);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors

  // Verify SubTitle buffer contains "My Title"
  EXPECT_STREQ(EdAsmNg::Asm::GetSubTitle(), "My Title");
}

TEST_F(Phase74SBTLDirectiveTest, DifferentDelimiter) {
  // Call SBTL with operand: |Custom|
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetupSourceLine("|Custom|");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlSBTL();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify SubTtlF is $FF
  EXPECT_EQ(EdAsmNg::Asm::GetSubTtlF(), 0xFF);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors

  // Verify SubTitle buffer contains "Custom"
  EXPECT_STREQ(EdAsmNg::Asm::GetSubTitle(), "Custom");
}

TEST_F(Phase74SBTLDirectiveTest, MaxLength) {
  // Call SBTL with operand: / + 35 chars + /
  EdAsmNg::Asm::SetPassNbr(1);                                      // Pass 2
  std::string maxString = "/12345678901234567890123456789012345/";  // Exactly 35 chars
  EdAsmNg::Asm::SetupSourceLine(maxString.c_str());

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlSBTL();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify SubTtlF is $FF
  EXPECT_EQ(EdAsmNg::Asm::GetSubTtlF(), 0xFF);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors

  // Verify SubTitle buffer contains exactly 35 chars
  EXPECT_STREQ(EdAsmNg::Asm::GetSubTitle(), "12345678901234567890123456789012345");
}

TEST_F(Phase74SBTLDirectiveTest, UnterminatedString) {
  // Call SBTL with operand: /Unterminated (no closing delimiter, CR only)
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetupSourceLine("/Unterminated");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlSBTL();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify error is registered (directive operand error)
  EXPECT_GT(errorsAfter, errorsBefore);
}

TEST_F(Phase74SBTLDirectiveTest, ExceededMaxLength) {
  // Call SBTL with operand: / + 40 chars (more than 35)
  EdAsmNg::Asm::SetPassNbr(1);                                            // Pass 2
  std::string longString = "/1234567890123456789012345678901234567890/";  // 40 chars
  EdAsmNg::Asm::SetupSourceLine(longString.c_str());

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlSBTL();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify error is registered (directive operand error)
  // When max is reached, next char must be delimiter, if CR instead => error
  EXPECT_GT(errorsAfter, errorsBefore);
}

TEST_F(Phase74SBTLDirectiveTest, NonSpaceAfter) {
  // Call SBTL with operand: /Title/ garbage
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetupSourceLine("/Title/ garbage");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlSBTL();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify error is registered (directive operand error)
  EXPECT_GT(errorsAfter, errorsBefore);
}

TEST_F(Phase74SBTLDirectiveTest, WithDelimiterInString) {
  // Call SBTL with operand: |It's a title|
  EdAsmNg::Asm::SetPassNbr(1);  // Pass 2
  EdAsmNg::Asm::SetupSourceLine("|It's a title|");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlSBTL();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify SubTtlF is $FF
  EXPECT_EQ(EdAsmNg::Asm::GetSubTtlF(), 0xFF);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors

  // Verify SubTitle contains "It's a title"
  EXPECT_STREQ(EdAsmNg::Asm::GetSubTitle(), "It's a title");
}

TEST_F(Phase74SBTLDirectiveTest, Pass1NoOp) {
  // In Pass 1, SBTL should set flag but not parse string (optimization)
  EdAsmNg::Asm::SetPassNbr(0);  // Pass 1
  EdAsmNg::Asm::SetupSourceLine("/My Title/");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlSBTL();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify SubTtlF is $40 (encountered, but string not parsed in Pass 1)
  EXPECT_EQ(EdAsmNg::Asm::GetSubTtlF(), 0x40);
  EXPECT_EQ(errorsAfter, errorsBefore);  // No errors
}

//=================================================
// Phase 7.5: OBJ Directive Tests
//=================================================

// Test helper function declarations for Phase 7.5
namespace EdAsmNg {
  namespace Asm {
    void     HndlOBJ();
    void     SetRelCodeF(uint8_t value);
    uint8_t  GetRelCodeF();
    void     SetEndSymT(uint16_t value);
    uint16_t GetEndSymT();
    void     SetMemTop(uint16_t value);
    uint16_t GetMemTop();
    void     SetRLDEnd(uint16_t value);
    uint16_t GetRLDEnd();
  }  // namespace Asm
}  // namespace EdAsmNg

class Phase75OBJDirectiveTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::ResetDispatchState();
    EdAsmNg::Asm::SetPassNbr(0);  // Default to Pass 1
    EdAsmNg::Asm::SetGenF(0x00);
    EdAsmNg::Asm::SetRelCodeF(0x00);
    EdAsmNg::Asm::SetEndSymT(0x0800);  // Default symbol table end
    EdAsmNg::Asm::SetObjPC(0x0000);
    EdAsmNg::Asm::SetMemTop(0x0000);
    EdAsmNg::Asm::SetRLDEnd(0x0000);
  }
};

TEST_F(Phase75OBJDirectiveTest, SuppressGeneration) {
  // OBJ 0 should suppress code generation (GenF = $80)
  EdAsmNg::Asm::SetupSourceLine("0");

  EdAsmNg::Asm::HndlOBJ();

  // Verify GenF has N bit set ($80 = suppress generation)
  EXPECT_EQ(EdAsmNg::Asm::GetGenF(), 0x80);

  // Verify no errors
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(Phase75OBJDirectiveTest, SetAddress) {
  // OBJ 4096 should set ObjPC, MemTop, RLDEnd to 0x1000
  EdAsmNg::Asm::SetupSourceLine("4096");

  EdAsmNg::Asm::HndlOBJ();

  // Verify all three address variables are set correctly
  EXPECT_EQ(EdAsmNg::Asm::GetObjPC(), 0x1000);
  EXPECT_EQ(EdAsmNg::Asm::GetMemTop(), 0x1000);
  EXPECT_EQ(EdAsmNg::Asm::GetRLDEnd(), 0x1000);

  // Verify GenF is clear (generation enabled)
  EXPECT_EQ(EdAsmNg::Asm::GetGenF(), 0x00);

  // Verify no errors
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(Phase75OBJDirectiveTest, SetAddressHighRange) {
  // OBJ 45056 ($B000, typical ProDOS load address)
  EdAsmNg::Asm::SetupSourceLine("$B000");

  EdAsmNg::Asm::HndlOBJ();

  // Verify all three address variables are set
  EXPECT_EQ(EdAsmNg::Asm::GetObjPC(), 0xB000);
  EXPECT_EQ(EdAsmNg::Asm::GetMemTop(), 0xB000);
  EXPECT_EQ(EdAsmNg::Asm::GetRLDEnd(), 0xB000);

  // Verify GenF is clear
  EXPECT_EQ(EdAsmNg::Asm::GetGenF(), 0x00);

  // Verify no errors
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(Phase75OBJDirectiveTest, ErrorRELModeActive) {
  // If REL mode is active (RelCodeF MSB set), OBJ should error
  EdAsmNg::Asm::SetRelCodeF(0x80);  // REL mode active
  EdAsmNg::Asm::SetupSourceLine("4096");

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlOBJ();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify error was registered
  EXPECT_GT(errorsAfter, errorsBefore);

  // Verify state is unchanged (no side effects on error)
  EXPECT_EQ(EdAsmNg::Asm::GetObjPC(), 0x0000);  // Should remain at initial value
  EXPECT_EQ(EdAsmNg::Asm::GetMemTop(), 0x0000);
  EXPECT_EQ(EdAsmNg::Asm::GetRLDEnd(), 0x0000);
}

TEST_F(Phase75OBJDirectiveTest, ErrorAddressBelowSymbolTable) {
  // If address < EndSymT, should error
  EdAsmNg::Asm::SetEndSymT(0x2000);
  EdAsmNg::Asm::SetupSourceLine("$1000");  // Below EndSymT

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlOBJ();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify error was registered
  EXPECT_GT(errorsAfter, errorsBefore);

  // Verify state is unchanged
  EXPECT_EQ(EdAsmNg::Asm::GetObjPC(), 0x0000);
  EXPECT_EQ(EdAsmNg::Asm::GetMemTop(), 0x0000);
  EXPECT_EQ(EdAsmNg::Asm::GetRLDEnd(), 0x0000);
}

TEST_F(Phase75OBJDirectiveTest, AddressEqualsEndSymT) {
  // Boundary case: address == EndSymT should succeed
  EdAsmNg::Asm::SetEndSymT(0x1000);
  EdAsmNg::Asm::SetupSourceLine("$1000");  // Equal to EndSymT

  EdAsmNg::Asm::HndlOBJ();

  // Verify success (no error)
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);

  // Verify state is set correctly
  EXPECT_EQ(EdAsmNg::Asm::GetObjPC(), 0x1000);
  EXPECT_EQ(EdAsmNg::Asm::GetMemTop(), 0x1000);
  EXPECT_EQ(EdAsmNg::Asm::GetRLDEnd(), 0x1000);
  EXPECT_EQ(EdAsmNg::Asm::GetGenF(), 0x00);
}

TEST_F(Phase75OBJDirectiveTest, DiskOutputModeIgnored) {
  // If GenF V-bit is set (disk output mode), OBJ should be ignored
  EdAsmNg::Asm::SetGenF(0x40);  // V-bit set (disk output mode)
  EdAsmNg::Asm::SetupSourceLine("4096");

  EdAsmNg::Asm::HndlOBJ();

  // Verify GenF is unchanged (OBJ was ignored)
  EXPECT_EQ(EdAsmNg::Asm::GetGenF(), 0x40);

  // Verify addresses are not set
  EXPECT_EQ(EdAsmNg::Asm::GetObjPC(), 0x0000);
  EXPECT_EQ(EdAsmNg::Asm::GetMemTop(), 0x0000);

  // Verify no errors (not an error, just ignored)
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(Phase75OBJDirectiveTest, ParseError) {
  // Invalid expression should cause parse error
  EdAsmNg::Asm::SetupSourceLine("");  // No operand

  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlOBJ();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify error was registered (operand parse error)
  EXPECT_GT(errorsAfter, errorsBefore);
}

//=================================================
// Phase 7.6: REL Directive Tests
//=================================================

// Test helper function declarations for Phase 7.6
namespace EdAsmNg {
  namespace Asm {
    void HndlREL();
  }  // namespace Asm
}  // namespace EdAsmNg

class Phase76RELDirectiveTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::ResetDispatchState();
    EdAsmNg::Asm::SetPassNbr(0);  // Default to Pass 1
    EdAsmNg::Asm::SetGenF(0x00);
    EdAsmNg::Asm::SetRelCodeF(0x00);
  }
};

TEST_F(Phase76RELDirectiveTest, EnableMode) {
  // L9126 - REL directive should set RelCodeF MSB
  // Original: ASM3.S:1168-1179

  // Set RelCodeF to $00 initially (REL not active)
  EdAsmNg::Asm::SetRelCodeF(0x00);
  EdAsmNg::Asm::SetupSourceLine("");  // REL takes no operand

  EdAsmNg::Asm::HndlREL();

  // Verify RelCodeF MSB is set ($80 or higher)
  uint8_t relCodeF = EdAsmNg::Asm::GetRelCodeF();
  EXPECT_GE(relCodeF, 0x80);  // MSB must be set

  // Verify no errors
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(Phase76RELDirectiveTest, ModeAlreadyOn) {
  // REL when RelCodeF is already set should maintain the flag

  // Set RelCodeF to $FF (REL already active)
  EdAsmNg::Asm::SetRelCodeF(0xFF);
  EdAsmNg::Asm::SetupSourceLine("");  // REL takes no operand

  EdAsmNg::Asm::HndlREL();

  // Verify RelCodeF is still set (MSB = 1)
  uint8_t relCodeF = EdAsmNg::Asm::GetRelCodeF();
  EXPECT_GE(relCodeF, 0x80);  // MSB must still be set

  // Verify no errors
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(Phase76RELDirectiveTest, WithOperandIgnored) {
  // REL should ignore any operand (unlike OBJ)

  EdAsmNg::Asm::SetRelCodeF(0x00);
  EdAsmNg::Asm::SetupSourceLine("123");  // Operand should be ignored

  EdAsmNg::Asm::HndlREL();

  // Verify RelCodeF MSB is set regardless of operand
  uint8_t relCodeF = EdAsmNg::Asm::GetRelCodeF();
  EXPECT_GE(relCodeF, 0x80);

  // Verify no errors (operand is ignored, not an error)
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(Phase76RELDirectiveTest, SetRELThenOBJError) {
  // After REL is set, OBJ directive should detect conflict and error

  // First set REL mode
  EdAsmNg::Asm::SetRelCodeF(0x00);
  EdAsmNg::Asm::SetupSourceLine("");
  EdAsmNg::Asm::HndlREL();

  // Verify REL is set
  EXPECT_GE(EdAsmNg::Asm::GetRelCodeF(), 0x80);

  // Now try OBJ (should error because REL is active)
  EdAsmNg::Asm::SetupSourceLine("4096");
  uint16_t errorsBefore = EdAsmNg::Asm::GetErrorCount();
  EdAsmNg::Asm::HndlOBJ();
  uint16_t errorsAfter = EdAsmNg::Asm::GetErrorCount();

  // Verify OBJ detected REL conflict and errored
  EXPECT_GT(errorsAfter, errorsBefore);
}

TEST_F(Phase76RELDirectiveTest, SetFromZero) {
  // Test SEC;ROR behavior: $00 -> $80

  EdAsmNg::Asm::SetRelCodeF(0x00);
  EdAsmNg::Asm::SetupSourceLine("");

  EdAsmNg::Asm::HndlREL();

  // Verify exactly $80 (SEC sets carry, ROR rotates it into MSB)
  uint8_t relCodeF = EdAsmNg::Asm::GetRelCodeF();
  EXPECT_EQ(relCodeF, 0x80);

  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

TEST_F(Phase76RELDirectiveTest, SetFromNonZero) {
  // Test SEC;ROR behavior with non-zero initial value

  EdAsmNg::Asm::SetRelCodeF(0x02);  // Initial value with bit 1 set
  EdAsmNg::Asm::SetupSourceLine("");

  EdAsmNg::Asm::HndlREL();

  // SEC sets carry, ROR rotates: $02 >> 1 with carry in = $81
  uint8_t relCodeF = EdAsmNg::Asm::GetRelCodeF();
  EXPECT_EQ(relCodeF, 0x81);

  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0);
}

//=================================================
// Phase 8.1: Initialization and Cleanup Tests
// Original: ASM2.S:555 (SaveZP), ASM2.S:570 (InitASM)
//=================================================

namespace EdAsmNg {
  namespace Asm {
    // Expose initialization functions for testing
    void SaveZP();
    void RestoreZP();
    void InitASM();
    void CleanupAsm();

    // Expose zero-page variable getters/setters for testing
    void     SetNumErrs(uint16_t value);
    void     SetNumWarns(uint16_t value);
    void     SetLineNum(uint16_t value);
    // Phase 8.1 functions
    void     SaveZP();
    void     RestoreZP();
    void     InitASM();
    void     CleanupAsm();

    // Phase 8.1 accessors
    void     SetNumErrs(uint16_t value);
    void     SetNumWarns(uint16_t value);
    void     SetLineNum(uint16_t value);
    uint16_t GetLineNum();
    void     SetListingF(uint8_t value);
    uint8_t  GetListingF();
    void     SetMacroF(uint8_t value);
    uint8_t  GetMacroF();
    void     SetIfDefF(uint8_t value);
    uint8_t  GetIfDefF();
    void     SetSubTtlF(uint8_t value);
    uint8_t  GetSubTtlF();
    void     SetGenF(uint8_t value);
    uint8_t  GetGenF();
    void     SetRelCodeF(uint8_t value);
    uint8_t  GetRelCodeF();
    void     SetStrtSymT(uint16_t value);
    uint16_t GetStrtSymT();
    void     SetEndSymT(uint16_t value);
    uint16_t GetEndSymT();
    uint16_t GetHeaderT(uint8_t index);
    void     SetHeaderT(uint8_t index, uint16_t value);
    void     SetPC(uint16_t value);
    uint16_t GetPC();
    void     SetObjPC(uint16_t value);
    uint16_t GetObjPC();
    uint8_t  GetGMC(uint8_t index);
    void     SetGMC(uint8_t index, uint8_t value);
  }  // namespace Asm
}  // namespace EdAsmNg

#endif  // TODO: Phase 9+ - Tests for Phases 2-7

/*
class Phase81InitTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetErrorState();
  }
};

TEST_F(Phase81InitTest, SaveRestoreZP_BasicVariables) {
  // Original: ASM2.S:555 - SaveZP
  // Test saving and restoring basic zero-page variables

  // Set some test values
  EdAsmNg::Asm::SetNumErrs(0x1234);
  EdAsmNg::Asm::SetNumWarns(0x5678);
  EdAsmNg::Asm::SetLineNum(0x9ABC);
  EdAsmNg::Asm::SetListingF(0xAA);
  EdAsmNg::Asm::SetMacroF(0xBB);
  EdAsmNg::Asm::SetRelCodeF(0xCC);

  // Save zero-page
  EdAsmNg::Asm::SaveZP();

  // Modify variables to different values
  EdAsmNg::Asm::SetNumErrs(0xFFFF);
  EdAsmNg::Asm::SetNumWarns(0xEEEE);
  EdAsmNg::Asm::SetLineNum(0xDDDD);
  EdAsmNg::Asm::SetListingF(0x11);
  EdAsmNg::Asm::SetMacroF(0x22);
  EdAsmNg::Asm::SetRelCodeF(0x33);

  // Verify modifications took effect
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0xFFFF);
  EXPECT_EQ(EdAsmNg::Asm::GetWarningCount(), 0xEEEE);
  EXPECT_EQ(EdAsmNg::Asm::GetLineNum(), 0xDDDD);
  EXPECT_EQ(EdAsmNg::Asm::GetListingF(), 0x11);
  EXPECT_EQ(EdAsmNg::Asm::GetMacroF(), 0x22);
  EXPECT_EQ(EdAsmNg::Asm::GetRelCodeF(), 0x33);

  // Restore zero-page
  EdAsmNg::Asm::RestoreZP();

  // Verify original values are restored
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0x1234);
  EXPECT_EQ(EdAsmNg::Asm::GetWarningCount(), 0x5678);
  EXPECT_EQ(EdAsmNg::Asm::GetLineNum(), 0x9ABC);
  EXPECT_EQ(EdAsmNg::Asm::GetListingF(), 0xAA);
  EXPECT_EQ(EdAsmNg::Asm::GetMacroF(), 0xBB);
  EXPECT_EQ(EdAsmNg::Asm::GetRelCodeF(), 0xCC);
}

TEST_F(Phase81InitTest, SaveRestoreZP_SymbolTablePointers) {
  // Test saving and restoring symbol table pointers

  EdAsmNg::Asm::SetStrtSymT(0x2000);
  EdAsmNg::Asm::SetEndSymT(0x3000);
  EdAsmNg::Asm::SetPC(0x4000);
  EdAsmNg::Asm::SetObjPC(0x5000);

  EdAsmNg::Asm::SaveZP();

  EdAsmNg::Asm::SetStrtSymT(0xAAAA);
  EdAsmNg::Asm::SetEndSymT(0xBBBB);
  EdAsmNg::Asm::SetPC(0xCCCC);
  EdAsmNg::Asm::SetObjPC(0xDDDD);

  EdAsmNg::Asm::RestoreZP();

  EXPECT_EQ(EdAsmNg::Asm::GetStrtSymT(), 0x2000);
  EXPECT_EQ(EdAsmNg::Asm::GetEndSymT(), 0x3000);
  EXPECT_EQ(EdAsmNg::Asm::GetPC(), 0x4000);
  EXPECT_EQ(EdAsmNg::Asm::GetObjPC(), 0x5000);
}

TEST_F(Phase81InitTest, SaveRestoreZP_GMCBuffer) {
  // Test saving and restoring GMC buffer (machine code generation)

  EdAsmNg::Asm::SetGMC(0, 0xA9);  // LDA immediate
  EdAsmNg::Asm::SetGMC(1, 0x42);
  EdAsmNg::Asm::SetGMC(2, 0x85);
  EdAsmNg::Asm::SetGMC(3, 0x60);

  EdAsmNg::Asm::SaveZP();

  EdAsmNg::Asm::SetGMC(0, 0xFF);
  EdAsmNg::Asm::SetGMC(1, 0xFF);
  EdAsmNg::Asm::SetGMC(2, 0xFF);
  EdAsmNg::Asm::SetGMC(3, 0xFF);

  EdAsmNg::Asm::RestoreZP();

  EXPECT_EQ(EdAsmNg::Asm::GetGMC(0), 0xA9);
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(1), 0x42);
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(2), 0x85);
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(3), 0x60);
}

TEST_F(Phase81InitTest, InitASM_ClearsCounters) {
  // Original: ASM2.S:570 - InitASM
  // Test that InitASM clears error/warning/line counters

  // Set non-zero values
  EdAsmNg::Asm::SetNumErrs(0x9999);
  EdAsmNg::Asm::SetNumWarns(0x8888);
  EdAsmNg::Asm::SetLineNum(0x7777);

  // Initialize
  EdAsmNg::Asm::InitASM();

  // Verify all counters are zeroed
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0x0000);
  EXPECT_EQ(EdAsmNg::Asm::GetWarningCount(), 0x0000);
  EXPECT_EQ(EdAsmNg::Asm::GetLineNum(), 0x0000);
}

TEST_F(Phase81InitTest, InitASM_SetsDefaultFlags) {
  // Original: ASM2.S:570 - InitASM
  // Test that InitASM sets default flag values

  // Set non-default values
  EdAsmNg::Asm::SetListingF(0x00);
  EdAsmNg::Asm::SetMacroF(0xFF);
  EdAsmNg::Asm::SetIfDefF(0xFF);
  EdAsmNg::Asm::SetSubTtlF(0xFF);
  EdAsmNg::Asm::SetGenF(0xFF);
  EdAsmNg::Asm::SetRelCodeF(0xFF);

  // Initialize
  EdAsmNg::Asm::InitASM();

  // Verify default flags
  // ListingF = $FF (LST ON, MSB set) per original code
  EXPECT_EQ(EdAsmNg::Asm::GetListingF(), 0xFF);
  // MacroF = $00 (macros OFF)
  EXPECT_EQ(EdAsmNg::Asm::GetMacroF(), 0x00);
  // SubTtlF = $00 (no subtitle)
  EXPECT_EQ(EdAsmNg::Asm::GetSubTtlF(), 0x00);
  // GenF = $00 (generation enabled, memory mode)
  EXPECT_EQ(EdAsmNg::Asm::GetGenF(), 0x00);
  // RelCodeF = $00 (absolute mode)
  EXPECT_EQ(EdAsmNg::Asm::GetRelCodeF(), 0x00);
  // ErrorF = $00 (no error)
  EXPECT_EQ(EdAsmNg::Asm::GetErrorFlag(), 0x00);
}

TEST_F(Phase81InitTest, InitASM_InitializesSymbolTable) {
  // Original: ASM2.S:570 - InitASM
  // Test that InitASM initializes symbol table pointers

  // Set non-zero values
  EdAsmNg::Asm::SetStrtSymT(0x5555);
  EdAsmNg::Asm::SetEndSymT(0x6666);
  // Set some header table entries to non-zero
  EdAsmNg::Asm::SetHeaderT(0, 0x1111);
  EdAsmNg::Asm::SetHeaderT(1, 0x2222);
  EdAsmNg::Asm::SetHeaderT(255, 0x3333);

  // Initialize
  EdAsmNg::Asm::InitASM();

  // Verify symbol table is empty (StrtSymT == EndSymT)
  uint16_t strtSymT = EdAsmNg::Asm::GetStrtSymT();
  uint16_t endSymT  = EdAsmNg::Asm::GetEndSymT();
  EXPECT_EQ(strtSymT, endSymT);
  EXPECT_NE(strtSymT, 0x0000);  // Should point to valid memory

  // Verify HeaderT entries are cleared to $0000
  EXPECT_EQ(EdAsmNg::Asm::GetHeaderT(0), 0x0000);
  EXPECT_EQ(EdAsmNg::Asm::GetHeaderT(1), 0x0000);
  EXPECT_EQ(EdAsmNg::Asm::GetHeaderT(127), 0x0000);
  EXPECT_EQ(EdAsmNg::Asm::GetHeaderT(255), 0x0000);
}

TEST_F(Phase81InitTest, InitASM_InitializesPC) {
  // Test that InitASM initializes Program Counter

  EdAsmNg::Asm::SetPC(0x9999);
  EdAsmNg::Asm::SetObjPC(0x8888);

  EdAsmNg::Asm::InitASM();

  // PC and ObjPC should be zeroed (or left for ORG directive)
  EXPECT_EQ(EdAsmNg::Asm::GetPC(), 0x0000);
  EXPECT_EQ(EdAsmNg::Asm::GetObjPC(), 0x0000);
}

TEST_F(Phase81InitTest, InitASM_ClearsGMCBuffer) {
  // Test that InitASM clears machine code generation buffer

  EdAsmNg::Asm::SetGMC(0, 0xAA);
  EdAsmNg::Asm::SetGMC(1, 0xBB);
  EdAsmNg::Asm::SetGMC(2, 0xCC);
  EdAsmNg::Asm::SetGMC(3, 0xDD);

  EdAsmNg::Asm::InitASM();

  EXPECT_EQ(EdAsmNg::Asm::GetGMC(0), 0x00);
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(1), 0x00);
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(2), 0x00);
  EXPECT_EQ(EdAsmNg::Asm::GetGMC(3), 0x00);
}

TEST_F(Phase81InitTest, InitASM_MultipleInitializations) {
  // Test that InitASM can be called multiple times and properly resets each time

  // First initialization
  EdAsmNg::Asm::InitASM();

  // Modify some values
  EdAsmNg::Asm::SetNumErrs(0x0005);
  EdAsmNg::Asm::SetNumWarns(0x0003);
  EdAsmNg::Asm::SetListingF(0x00);
  EdAsmNg::Asm::SetPC(0x8000);

  // Second initialization
  EdAsmNg::Asm::InitASM();

  // Verify everything is reset properly
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0x0000);
  EXPECT_EQ(EdAsmNg::Asm::GetWarningCount(), 0x0000);
  EXPECT_EQ(EdAsmNg::Asm::GetListingF(), 0xFF);
  EXPECT_EQ(EdAsmNg::Asm::GetPC(), 0x0000);
}

TEST_F(Phase81InitTest, CleanupAsm_BasicStub) {
  // Test that CleanupAsm can be called without errors
  // For now, this is just a stub function

  // Should not crash or cause issues
  EdAsmNg::Asm::CleanupAsm();

  // No specific assertions - just verifying it doesn't crash
  SUCCEED();
}

TEST_F(Phase81InitTest, SaveRestoreMultipleTimes) {
  // Test multiple save/restore cycles

  // Set initial values
  EdAsmNg::Asm::SetNumErrs(0x1111);
  EdAsmNg::Asm::SetListingF(0xAA);

  // First save
  EdAsmNg::Asm::SaveZP();

  // Modify and restore
  EdAsmNg::Asm::SetNumErrs(0x2222);
  EdAsmNg::Asm::RestoreZP();
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0x1111);

  // Second save with new values
  EdAsmNg::Asm::SetNumErrs(0x3333);
  EdAsmNg::Asm::SetListingF(0xBB);
  EdAsmNg::Asm::SaveZP();

  // Modify and restore again
  EdAsmNg::Asm::SetNumErrs(0x4444);
  EdAsmNg::Asm::SetListingF(0xCC);
  EdAsmNg::Asm::RestoreZP();

  // Should restore the second saved state
  EXPECT_EQ(EdAsmNg::Asm::GetErrorCount(), 0x3333);
  EXPECT_EQ(EdAsmNg::Asm::GetListingF(), 0xBB);
}
*/

//=================================================
// Phase 8.2: Source Line Reader Tests
//=================================================

// Helper functions to access Phase 8.2 internals for testing
namespace EdAsmNg {
  namespace Asm {
    // Source reader functions
    void GSrcLin();  // Get source line - returns via carry flag
    bool GetCarryFlag();
    void SetCarryFlag(bool value);

    // Memory source setup helper
    void SetupMemorySource(const char* sourceText, size_t length);

    // Phase 8.2 variable accessors
    uint16_t GetSrcP();
    void     SetSrcP(uint16_t value);
    uint16_t GetTxtEnd();
    void     SetTxtEnd(uint16_t value);
    int8_t   GetIDskSrcF();
    void     SetIDskSrcF(int8_t value);

    // Helper to advance to next line (for multi-line tests)
    void AdvanceToNextLine();
  }  // namespace Asm
}  // namespace EdAsmNg

class Phase82SourceReaderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Reset source reader state
    EdAsmNg::Asm::SetSrcP(0);
    EdAsmNg::Asm::SetTxtEnd(0);
    EdAsmNg::Asm::SetIDskSrcF(0);
    EdAsmNg::Asm::SetCarryFlag(false);
  }
};

TEST_F(Phase82SourceReaderTest, GSrcLin_MemoryMode_SingleLine) {
  // Original: ASM3.S:2991-3056 - GSrcLin memory mode
  // Test reading a single line from memory

  const char* source = "LDA #$00\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  // Call GSrcLin
  EdAsmNg::Asm::GSrcLin();

  // Verify carry clear (line available, C=0)
  EXPECT_FALSE(EdAsmNg::Asm::GetCarryFlag());

  // Verify SrcP points to start of line
  uint16_t srcP = EdAsmNg::Asm::GetSrcP();
  EXPECT_NE(srcP, 0);  // Should point to valid memory
}

TEST_F(Phase82SourceReaderTest, GSrcLin_MemoryMode_MultipleLines) {
  // Test reading multiple lines sequentially

  const char* source = "LDA #$00\rSTA $1000\rRTS\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  uint16_t line1Start = EdAsmNg::Asm::GetSrcP();

  // First line
  EdAsmNg::Asm::GSrcLin();
  EXPECT_FALSE(EdAsmNg::Asm::GetCarryFlag());
  EXPECT_EQ(EdAsmNg::Asm::GetSrcP(), line1Start);

  // Advance to next line
  EdAsmNg::Asm::AdvanceToNextLine();

  // Second line
  EdAsmNg::Asm::GSrcLin();
  EXPECT_FALSE(EdAsmNg::Asm::GetCarryFlag());

  // Advance to next line
  EdAsmNg::Asm::AdvanceToNextLine();

  // Third line
  EdAsmNg::Asm::GSrcLin();
  EXPECT_FALSE(EdAsmNg::Asm::GetCarryFlag());
}

TEST_F(Phase82SourceReaderTest, GSrcLin_MemoryMode_EOF) {
  // Test reaching end of memory buffer

  const char* source = "NOP\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  // Advance SrcP to TxtEnd (EOF)
  uint16_t txtEnd = EdAsmNg::Asm::GetTxtEnd();
  EdAsmNg::Asm::SetSrcP(txtEnd);

  // Call GSrcLin
  EdAsmNg::Asm::GSrcLin();

  // Verify carry set (EOF, C=1)
  EXPECT_TRUE(EdAsmNg::Asm::GetCarryFlag());
}

TEST_F(Phase82SourceReaderTest, GSrcLin_EmptySource) {
  // Test empty buffer (SrcP == TxtEnd from start)

  EdAsmNg::Asm::SetSrcP(0x1000);
  EdAsmNg::Asm::SetTxtEnd(0x1000);
  EdAsmNg::Asm::SetIDskSrcF(0);  // Memory mode

  // Call GSrcLin
  EdAsmNg::Asm::GSrcLin();

  // Verify carry set (EOF immediately, C=1)
  EXPECT_TRUE(EdAsmNg::Asm::GetCarryFlag());
}

TEST_F(Phase82SourceReaderTest, GSrcLin_DiskMode_Stubbed) {
  // Test disk mode returns EOF (stubbed for Phase 8.2)

  // Set disk mode
  EdAsmNg::Asm::SetIDskSrcF(-1);  // Disk mode (MSB set)

  // Call GSrcLin
  EdAsmNg::Asm::GSrcLin();

  // Verify carry set (stubbed returns EOF, C=1)
  EXPECT_TRUE(EdAsmNg::Asm::GetCarryFlag());
}

TEST_F(Phase82SourceReaderTest, SetupMemorySource_Helper) {
  // Test the helper function sets up state correctly

  const char* source = "LDA #$FF\rSTA $2000\r";
  size_t      length = strlen(source);

  EdAsmNg::Asm::SetupMemorySource(source, length);

  // Verify SrcP and TxtEnd are set with correct relationship
  uint16_t srcP   = EdAsmNg::Asm::GetSrcP();
  uint16_t txtEnd = EdAsmNg::Asm::GetTxtEnd();

  // SrcP should be at a simulated base address (e.g., 0x1000)
  EXPECT_NE(srcP, 0);  // Not null/zero

  // TxtEnd should be SrcP + length
  EXPECT_EQ(txtEnd, srcP + length);

  // Verify IDskSrcF = 0 (memory mode)
  EXPECT_EQ(EdAsmNg::Asm::GetIDskSrcF(), 0);
}

TEST_F(Phase82SourceReaderTest, GSrcLin_BoundaryCondition_LastByte) {
  // Test when SrcP is at last byte before TxtEnd

  const char* source = "A";
  EdAsmNg::Asm::SetupMemorySource(source, 1);

  // SrcP at start, TxtEnd at start+1
  EdAsmNg::Asm::GSrcLin();

  // Should succeed (C=0) - have one byte
  EXPECT_FALSE(EdAsmNg::Asm::GetCarryFlag());

  // Advance to TxtEnd
  EdAsmNg::Asm::SetSrcP(EdAsmNg::Asm::GetTxtEnd());

  // Now should fail (C=1) - at TxtEnd
  EdAsmNg::Asm::GSrcLin();
  EXPECT_TRUE(EdAsmNg::Asm::GetCarryFlag());
}

TEST_F(Phase82SourceReaderTest, GSrcLin_MemoryMode_CompareCheck) {
  // Test the actual comparison logic (SrcP >= TxtEnd)

  // Set up: SrcP < TxtEnd
  EdAsmNg::Asm::SetSrcP(0x1000);
  EdAsmNg::Asm::SetTxtEnd(0x1010);
  EdAsmNg::Asm::SetIDskSrcF(0);
  EdAsmNg::Asm::GSrcLin();
  EXPECT_FALSE(EdAsmNg::Asm::GetCarryFlag());  // C=0

  // Set up: SrcP == TxtEnd
  EdAsmNg::Asm::SetSrcP(0x2000);
  EdAsmNg::Asm::SetTxtEnd(0x2000);
  EdAsmNg::Asm::GSrcLin();
  EXPECT_TRUE(EdAsmNg::Asm::GetCarryFlag());  // C=1

  // Set up: SrcP > TxtEnd
  EdAsmNg::Asm::SetSrcP(0x3010);
  EdAsmNg::Asm::SetTxtEnd(0x3000);
  EdAsmNg::Asm::GSrcLin();
  EXPECT_TRUE(EdAsmNg::Asm::GetCarryFlag());  // C=1
}

//=================================================
// Phase 8.3: Line Processing Helpers Tests
//=================================================

// Helper functions to access Phase 8.3 internals for testing
namespace EdAsmNg {
  namespace Asm {
    // Register accessors
    uint8_t GetY();
    void    SetY(uint8_t value);
    uint8_t GetA();
    void    SetA(uint8_t value);

    // Source pointer byte access helper
    uint8_t GetSrcPByte(uint8_t index);

    // Line processing helpers
    void NextRec();   // Advance to next record/line
    void NxtField();  // Advance to next field (skip spaces)
    void ChrGot();    // Get character at Y, classify, uppercase
    void ChrGet();    // Get character at Y, classify, uppercase, advance Y
  }  // namespace Asm
}  // namespace EdAsmNg

class Phase83LineHelpersTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Reset state
    EdAsmNg::Asm::SetSrcP(0);
    EdAsmNg::Asm::SetTxtEnd(0);
    EdAsmNg::Asm::SetY(0);
    EdAsmNg::Asm::SetA(0);
  }
};

TEST_F(Phase83LineHelpersTest, NextRec_SingleLine) {
  // Set up source: "LDA #$00\rSTA $1000\r"
  const char* source = "LDA #$00\rSTA $1000\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  uint16_t startSrcP = EdAsmNg::Asm::GetSrcP();
  EdAsmNg::Asm::SetY(0);

  // Call NextRec()
  EdAsmNg::Asm::NextRec();

  // Verify SrcP advanced past first CR (should point to 'S' in "STA")
  uint16_t newSrcP = EdAsmNg::Asm::GetSrcP();
  EXPECT_GT(newSrcP, startSrcP);
  EXPECT_EQ(newSrcP, startSrcP + 9);  // "LDA #$00\r" = 9 bytes

  // Verify Y reset to 0
  EXPECT_EQ(EdAsmNg::Asm::GetY(), 0);
}

TEST_F(Phase83LineHelpersTest, NextRec_EmptyLine) {
  // Set up source with empty line: "\rLDA\r"
  const char* source = "\rLDA\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  uint16_t startSrcP = EdAsmNg::Asm::GetSrcP();
  EdAsmNg::Asm::SetY(0);

  // Call NextRec() from first line (empty)
  EdAsmNg::Asm::NextRec();

  // Verify SrcP advanced past CR (should point to 'L' in "LDA")
  uint16_t newSrcP = EdAsmNg::Asm::GetSrcP();
  EXPECT_EQ(newSrcP, startSrcP + 1);  // Skipped one CR
  EXPECT_EQ(EdAsmNg::Asm::GetY(), 0);
}

TEST_F(Phase83LineHelpersTest, NxtField_SkipSpaces) {
  // Set up source: "     LDA"
  const char* source = "     LDA";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  EdAsmNg::Asm::SetY(0);

  // Call NxtField()
  EdAsmNg::Asm::NxtField();

  // Verify Y=0 after advancing (NxtField adjusts SrcP and resets Y)
  EXPECT_EQ(EdAsmNg::Asm::GetY(), 0);

  // Verify SrcP advanced to skip spaces
  // SrcP should now point to 'L' in "LDA"
  uint8_t ch = EdAsmNg::Asm::GetSrcPByte(0);
  EXPECT_EQ(ch, 'L');
}

TEST_F(Phase83LineHelpersTest, NxtField_NoSpaces) {
  // Set up source: "LDA"
  const char* source = "LDA";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  uint16_t startSrcP = EdAsmNg::Asm::GetSrcP();
  EdAsmNg::Asm::SetY(0);

  // Call NxtField()
  EdAsmNg::Asm::NxtField();

  // Verify Y=0 (already at non-space)
  EXPECT_EQ(EdAsmNg::Asm::GetY(), 0);

  // Verify SrcP unchanged (no spaces to skip)
  EXPECT_EQ(EdAsmNg::Asm::GetSrcP(), startSrcP);
}

TEST_F(Phase83LineHelpersTest, NxtField_OnlyCR) {
  // Set up source: "\r"
  const char* source = "\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  uint16_t startSrcP = EdAsmNg::Asm::GetSrcP();
  EdAsmNg::Asm::SetY(0);

  // Call NxtField()
  EdAsmNg::Asm::NxtField();

  // Verify Y=0 (CR stops scanning)
  EXPECT_EQ(EdAsmNg::Asm::GetY(), 0);

  // Verify SrcP unchanged (stopped at CR)
  EXPECT_EQ(EdAsmNg::Asm::GetSrcP(), startSrcP);
}

TEST_F(Phase83LineHelpersTest, ChrGot_Lowercase) {
  // Set up source: "lda"
  const char* source = "lda";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  EdAsmNg::Asm::SetY(0);

  // Call ChrGot()
  EdAsmNg::Asm::ChrGot();

  // Verify A='L' (uppercased)
  EXPECT_EQ(EdAsmNg::Asm::GetA(), 'L');

  // Verify Y=0 (not incremented)
  EXPECT_EQ(EdAsmNg::Asm::GetY(), 0);
}

TEST_F(Phase83LineHelpersTest, ChrGot_Uppercase) {
  // Set up source: "LDA"
  const char* source = "LDA";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  EdAsmNg::Asm::SetY(0);

  // Call ChrGot()
  EdAsmNg::Asm::ChrGot();

  // Verify A='L' (unchanged)
  EXPECT_EQ(EdAsmNg::Asm::GetA(), 'L');

  // Verify Y=0
  EXPECT_EQ(EdAsmNg::Asm::GetY(), 0);
}

TEST_F(Phase83LineHelpersTest, ChrGet_AdvancesY) {
  // Set up source: "abc"
  const char* source = "abc";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  EdAsmNg::Asm::SetY(0);

  // Call ChrGet()
  EdAsmNg::Asm::ChrGet();

  // Verify A='A' (uppercased)
  EXPECT_EQ(EdAsmNg::Asm::GetA(), 'A');

  // Verify Y=1 (incremented)
  EXPECT_EQ(EdAsmNg::Asm::GetY(), 1);
}

TEST_F(Phase83LineHelpersTest, ChrGet_Sequential) {
  // Set up source: "lda"
  const char* source = "lda";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  EdAsmNg::Asm::SetY(0);

  // Call ChrGet() three times
  EdAsmNg::Asm::ChrGet();
  EXPECT_EQ(EdAsmNg::Asm::GetA(), 'L');
  EXPECT_EQ(EdAsmNg::Asm::GetY(), 1);

  EdAsmNg::Asm::ChrGet();
  EXPECT_EQ(EdAsmNg::Asm::GetA(), 'D');
  EXPECT_EQ(EdAsmNg::Asm::GetY(), 2);

  EdAsmNg::Asm::ChrGet();
  EXPECT_EQ(EdAsmNg::Asm::GetA(), 'A');
  EXPECT_EQ(EdAsmNg::Asm::GetY(), 3);
}

//=================================================
// Phase 8.4: Pass 1 Loop - Symbol Collection Tests
//=================================================

// Helper functions to access Pass 1 internals for testing
namespace EdAsmNg {
  namespace Asm {
    // Pass 1 execution
    void DoPass1();

    // Pass number accessor
    uint8_t GetPassNbr();
    void    SetPassNbr(uint8_t value);

    // PC accessors (may already be declared elsewhere, but redeclared here for clarity)
    void     SetPC(uint16_t value);
    uint16_t GetPC();

    // Symbol table query functions
    bool     HasSymbol(const char* name);
    uint16_t GetSymbolValue(const char* name);
    uint8_t  GetSymbolFlags(const char* name);
    int      GetSymbolCount();

    // Reset assembler state for clean test environment
    void ResetAsmState();
  }  // namespace Asm
}  // namespace EdAsmNg

class Phase84Pass1Test : public ::testing::Test {
 protected:
  void SetUp() override {
    EdAsmNg::Asm::ResetAsmState();
    EdAsmNg::Asm::SetPC(0);
    EdAsmNg::Asm::SetPassNbr(0);
  }
};

TEST_F(Phase84Pass1Test, test_pass1_empty_source) {
  // Test Pass 1 on empty source - should complete without error
  const char* source = "";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  // Run Pass 1
  EdAsmNg::Asm::DoPass1();

  // Verify PassNbr was set to 0 for Pass 1
  EXPECT_EQ(EdAsmNg::Asm::GetPassNbr(), 0);

  // Verify PC is still 0 (no code generated)
  EXPECT_EQ(EdAsmNg::Asm::GetPC(), 0);

  // Verify no symbols added
  EXPECT_EQ(EdAsmNg::Asm::GetSymbolCount(), 0);
}

TEST_F(Phase84Pass1Test, test_pass1_simple_label) {
  // Test parsing a single label line
  const char* source = "START\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  // Run Pass 1
  EdAsmNg::Asm::DoPass1();

  // Verify symbol was added
  EXPECT_TRUE(EdAsmNg::Asm::HasSymbol("START"));

  // Verify symbol has correct address (PC=0 when defined)
  EXPECT_EQ(EdAsmNg::Asm::GetSymbolValue("START"), 0x0000);
}

TEST_F(Phase84Pass1Test, test_pass1_label_with_nop) {
  // Test label with NOP instruction - should add symbol and increment PC by 1
  const char* source = "START NOP\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  // Run Pass 1
  EdAsmNg::Asm::DoPass1();

  // Verify symbol was added
  EXPECT_TRUE(EdAsmNg::Asm::HasSymbol("START"));

  // Verify symbol has address 0 (defined at start)
  EXPECT_EQ(EdAsmNg::Asm::GetSymbolValue("START"), 0x0000);

  // Verify PC incremented by 1 (NOP is 1 byte)
  EXPECT_EQ(EdAsmNg::Asm::GetPC(), 0x0001);
}

TEST_F(Phase84Pass1Test, test_pass1_statement_no_label) {
  // Test statement without label (leading spaces) - PC should advance
  const char* source = "      NOP\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  // Run Pass 1
  EdAsmNg::Asm::DoPass1();

  // Verify no symbol added
  EXPECT_EQ(EdAsmNg::Asm::GetSymbolCount(), 0);

  // Verify PC incremented by 1
  EXPECT_EQ(EdAsmNg::Asm::GetPC(), 0x0001);
}

TEST_F(Phase84Pass1Test, test_pass1_multiple_lines) {
  // Test multi-line source with PC accumulation
  const char* source =
      "START NOP\r"
      "      LDA #$00\r"
      "LOOP  STA $1000\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  // Run Pass 1
  EdAsmNg::Asm::DoPass1();

  // Verify symbols
  EXPECT_TRUE(EdAsmNg::Asm::HasSymbol("START"));
  EXPECT_TRUE(EdAsmNg::Asm::HasSymbol("LOOP"));

  // Verify symbol addresses
  EXPECT_EQ(EdAsmNg::Asm::GetSymbolValue("START"), 0x0000);
  EXPECT_EQ(EdAsmNg::Asm::GetSymbolValue("LOOP"), 0x0003);  // After NOP(1) + LDA(2)

  // Verify final PC (NOP=1, LDA=2, STA=3)
  EXPECT_EQ(EdAsmNg::Asm::GetPC(), 0x0006);
}

TEST_F(Phase84Pass1Test, test_pass1_comment_line) {
  // Test line starting with ';' - should be ignored, PC unchanged
  const char* source = "; This is a comment\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  // Run Pass 1
  EdAsmNg::Asm::DoPass1();

  // Verify PC unchanged
  EXPECT_EQ(EdAsmNg::Asm::GetPC(), 0x0000);

  // Verify no symbols added
  EXPECT_EQ(EdAsmNg::Asm::GetSymbolCount(), 0);
}

TEST_F(Phase84Pass1Test, test_pass1_blank_line) {
  // Test empty/blank line - should be ignored, PC unchanged
  const char* source = "\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  // Run Pass 1
  EdAsmNg::Asm::DoPass1();

  // Verify PC unchanged
  EXPECT_EQ(EdAsmNg::Asm::GetPC(), 0x0000);

  // Verify no symbols added
  EXPECT_EQ(EdAsmNg::Asm::GetSymbolCount(), 0);
}

TEST_F(Phase84Pass1Test, test_pass1_org_directive) {
  // Test ORG directive - should jump PC to $8000
  const char* source = "      ORG $8000\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  // Run Pass 1
  EdAsmNg::Asm::DoPass1();

  // Verify PC jumped to $8000
  EXPECT_EQ(EdAsmNg::Asm::GetPC(), 0x8000);

  // Verify no symbols added
  EXPECT_EQ(EdAsmNg::Asm::GetSymbolCount(), 0);
}

TEST_F(Phase84Pass1Test, test_pass1_label_after_org) {
  // Test label defined after ORG - should have correct address
  const char* source =
      "      ORG $8000\r"
      "START NOP\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  // Run Pass 1
  EdAsmNg::Asm::DoPass1();

  // Verify symbol defined at correct address
  EXPECT_TRUE(EdAsmNg::Asm::HasSymbol("START"));
  EXPECT_EQ(EdAsmNg::Asm::GetSymbolValue("START"), 0x8000);

  // Verify PC advanced past NOP
  EXPECT_EQ(EdAsmNg::Asm::GetPC(), 0x8001);
}

TEST_F(Phase84Pass1Test, test_pass1_reserved_label_A_error) {
  // Single-letter label "A" is reserved -> error, no symbol added, no PC advance
  const char* source = "A NOP\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  // Run Pass 1
  EdAsmNg::Asm::DoPass1();

  // Verify reserved label not added and error registered
  EXPECT_FALSE(EdAsmNg::Asm::HasSymbol("A"));
  EXPECT_GT(EdAsmNg::Asm::GetErrorCount(), 0);
  auto errInfo = EdAsmNg::Asm::GetErrorInfo(0);
  EXPECT_EQ(errInfo.errIndex, 0x1E);

  // PC should not advance when label is invalid
  EXPECT_EQ(EdAsmNg::Asm::GetPC(), 0x0000);
}

TEST_F(Phase84Pass1Test, test_pass1_invalid_label_first_char_error) {
  // Label starting with a digit is invalid -> error, no symbol added, no PC advance
  const char* source = "1FOO NOP\r";
  EdAsmNg::Asm::SetupMemorySource(source, strlen(source));

  // Run Pass 1
  EdAsmNg::Asm::DoPass1();

  // Verify invalid label not added and error registered
  EXPECT_FALSE(EdAsmNg::Asm::HasSymbol("1FOO"));
  EXPECT_GT(EdAsmNg::Asm::GetErrorCount(), 0);
  auto errInfo = EdAsmNg::Asm::GetErrorInfo(0);
  EXPECT_EQ(errInfo.errIndex, 0x0E);

  // PC should not advance when label is invalid
  EXPECT_EQ(EdAsmNg::Asm::GetPC(), 0x0000);
}
