#pragma once

#include <cstdint>
#include <string>
#include <string_view>

class Relocator {
 public:
  // Relocator Entry Point
  void L2000();

 private:
  // Day input state machine
  enum class stateL21DE { L21D5, L21DA, L21DE, L21FB, L2208, L220D };
  void L21DE(stateL21DE state);

  // Year input state machine
  enum class stateL2238 { L222F, L2234, L2238, L2259, L2266, L226B };
  bool L2238(stateL2238 state);

  // Month input state machine
  enum class stateL22C1 { L22AF, L22B8, L22BD, L22C1, L22D0, L22DF, L22F9, L2301 };
  bool L22C1(stateL22C1 state);

  // Utility methods
  void SendBanr(std::string_view Banner);
  bool PRUCase(int A);
  int  ToUCase();
  void ShowErr(int A);

  // Data members
  int  Day;
  int  L21CE;
  int  L21CF;
  int  L21D0;
  int  L21D1;
  char MMM[3];
  char L228A;

 public:
  int         ErrCode;   // $72
  std::string TxBuf2;    // $0280
  std::string CurrPfxB;  // $BB80
  std::string EdAsmDir;  // $BE79

  // Static constants
  static constexpr std::string_view Banner =
      "\n\n\n  PRODOS  EDITOR-ASSEMBLER //\n\n\n\n\n\n"
      "ENTER THE DATE AND PRESS RETURN\n\nDD-MMM-YY";
  static constexpr const int  TensT[]  = {00, 10, 20, 30, 40, 50, 60, 70, 80, 90};
  static constexpr const char Months[] = "JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";
};
