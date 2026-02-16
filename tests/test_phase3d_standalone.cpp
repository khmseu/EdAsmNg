// Standalone test for Phase 3d: ChkRng() and ValidateRange()
// This verifies the logic independent of compilation issues in asm.cpp

#include <cstdint>
#include <iostream>

// Simulated global variables (mimicking asm.cpp)
std::uint16_t ValExpr_word = 0;
#define ValExpr    (reinterpret_cast<std::uint8_t*>(&ValExpr_word)[0])
#define ValExpr_hi (reinterpret_cast<std::uint8_t*>(&ValExpr_word)[1])
std::uint8_t LenTIdx    = 0;
std::uint8_t ModWrdL    = 0;
std::uint8_t X          = 0;
std::uint8_t A          = 0;
int          errorCount = 0;

// Mock error registration
void RegAsmEW(std::uint8_t errorToken) {
  std::cout << "Error registered: 0x" << std::hex << (int)errorToken << std::dec << std::endl;
  errorCount++;
}

//=================================================
// Phase 3d: ChkRng() - Check Range
//=================================================
bool ChkRng(std::uint8_t value, std::uint8_t minVal, std::uint8_t maxVal) {
  if (value < minVal) {
    return true;  // Out of range (below minimum)
  }

  if (value > maxVal) {
    return true;  // Out of range (above maximum)
  }

  return false;  // Value is within range
}

//=================================================
// Phase 3d: ValidateRange() - Validate Addressing Mode Range
//=================================================
void ValidateRange() {
  // Check if this is a branch instruction
  A = ModWrdL;
  if ((A & 0x08) != 0) {  // Branch instruction flag (bit 3)
    // Branch instructions use relative addressing
    // Valid range: -128 to +127 (signed 8-bit)
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
    return;
  }

  // Zero page modes: Must be 0-255 (high byte must be 0)
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
  return;
}

//=================================================
// Test Cases
//=================================================

void test_ChkRng() {
  std::cout << "\n=== Testing ChkRng() ===" << std::endl;

  // Test 1: Value in range
  std::cout << "Test 1: ChkRng(0x50, 0x00, 0xFF) = " << ChkRng(0x50, 0x00, 0xFF) << " (expect 0)"
            << std::endl;

  // Test 2: Value at minimum
  std::cout << "Test 2: ChkRng(0x00, 0x00, 0x7F) = " << ChkRng(0x00, 0x00, 0x7F) << " (expect 0)"
            << std::endl;

  // Test 3: Value at maximum
  std::cout << "Test 3: ChkRng(0x7F, 0x00, 0x7F) = " << ChkRng(0x7F, 0x00, 0x7F) << " (expect 0)"
            << std::endl;

  // Test 4: Value above range
  std::cout << "Test 4: ChkRng(0xFF, 0x00, 0x7F) = " << ChkRng(0xFF, 0x00, 0x7F) << " (expect 1)"
            << std::endl;

  // Test 5: Value below range
  std::cout << "Test 5: ChkRng(0x10, 0x20, 0x80) = " << ChkRng(0x10, 0x20, 0x80) << " (expect 1)"
            << std::endl;
}

void test_ValidateRange() {
  std::cout << "\n=== Testing ValidateRange() ===" << std::endl;

  // Test 1: Immediate mode - any value OK
  std::cout << "\nTest 1: Immediate mode (0x1234)" << std::endl;
  errorCount   = 0;
  LenTIdx      = 2;
  ModWrdL      = 0;
  ValExpr_word = 0x1234;
  ValidateRange();
  std::cout << "Errors: " << errorCount << " (expect 0)" << std::endl;

  // Test 2: Zero page - valid range
  std::cout << "\nTest 2: Zero page (0x0080)" << std::endl;
  errorCount   = 0;
  LenTIdx      = 1;
  ModWrdL      = 0;
  ValExpr_word = 0x0080;
  ValidateRange();
  std::cout << "Errors: " << errorCount << " (expect 0)" << std::endl;

  // Test 3: Zero page - out of range
  std::cout << "\nTest 3: Zero page (0x0100)" << std::endl;
  errorCount   = 0;
  LenTIdx      = 1;
  ModWrdL      = 0;
  ValExpr_word = 0x0100;
  ValidateRange();
  std::cout << "Errors: " << errorCount << " (expect 1, error 0x1C)" << std::endl;

  // Test 4: Absolute mode - any value OK
  std::cout << "\nTest 4: Absolute mode (0xFFFF)" << std::endl;
  errorCount   = 0;
  LenTIdx      = 0;
  ModWrdL      = 0;
  ValExpr_word = 0xFFFF;
  ValidateRange();
  std::cout << "Errors: " << errorCount << " (expect 0)" << std::endl;

  // Test 5: Branch - valid positive offset
  std::cout << "\nTest 5: Branch (+29, 0x001D)" << std::endl;
  errorCount   = 0;
  LenTIdx      = 0;
  ModWrdL      = 0x08;  // Branch flag
  ValExpr_word = 0x001D;
  ValidateRange();
  std::cout << "Errors: " << errorCount << " (expect 0)" << std::endl;

  // Test 6: Branch - valid negative offset
  std::cout << "\nTest 6: Branch (-50, 0xFFCE)" << std::endl;
  errorCount   = 0;
  LenTIdx      = 0;
  ModWrdL      = 0x08;  // Branch flag
  ValExpr_word = 0xFFCE;
  ValidateRange();
  std::cout << "Errors: " << errorCount << " (expect 0)" << std::endl;

  // Test 7: Branch - out of range positive
  std::cout << "\nTest 7: Branch (+200, 0x00C8)" << std::endl;
  errorCount   = 0;
  LenTIdx      = 0;
  ModWrdL      = 0x08;  // Branch flag
  ValExpr_word = 0x00C8;
  ValidateRange();
  std::cout << "Errors: " << errorCount << " (expect 1, error 0x26)" << std::endl;

  // Test 8: Branch - out of range negative
  std::cout << "\nTest 8: Branch (-256, 0xFF00)" << std::endl;
  errorCount   = 0;
  LenTIdx      = 0;
  ModWrdL      = 0x08;  // Branch flag
  ValExpr_word = 0xFF00;
  ValidateRange();
  std::cout << "Errors: " << errorCount << " (expect 1, error 0x26)" << std::endl;

  // Test 9: Branch - edge case +127
  std::cout << "\nTest 9: Branch (+127, 0x007F)" << std::endl;
  errorCount   = 0;
  LenTIdx      = 0;
  ModWrdL      = 0x08;  // Branch flag
  ValExpr_word = 0x007F;
  ValidateRange();
  std::cout << "Errors: " << errorCount << " (expect 0)" << std::endl;

  // Test 10: Branch - edge case -128
  std::cout << "\nTest 10: Branch (-128, 0xFF80)" << std::endl;
  errorCount   = 0;
  LenTIdx      = 0;
  ModWrdL      = 0x08;  // Branch flag
  ValExpr_word = 0xFF80;
  ValidateRange();
  std::cout << "Errors: " << errorCount << " (expect 0)" << std::endl;
}

int main() {
  std::cout << "Phase 3d: ChkRng() & ValidateRange() Standalone Tests" << std::endl;
  std::cout << "========================================================" << std::endl;

  test_ChkRng();
  test_ValidateRange();

  std::cout << "\n========================================================" << std::endl;
  std::cout << "All tests completed!" << std::endl;

  return 0;
}
