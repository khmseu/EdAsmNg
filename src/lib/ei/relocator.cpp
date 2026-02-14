//---------------------------------------------------------;
// EI/RELOCATOR.S - EDASM.SYSTEM Relocator
//
// This is the relocator/loader for EDASM.SYSTEM.
//
// MEMORY LAYOUT:
// EDASM.SYSTEM is loaded at $2000-$2FFE (length $FFF = 4095 bytes)
// It consists of two parts:
//   Part 1: Relocator ($2000-$23FF, 1024 bytes)
//     - Runs once at startup
//     - Relocates the EI to its final location
//     - Sets up ProDOS memory management
//     - Then transfers control to the EI
//
//   Part 2: EdAsm Interpreter ($2400-$2FFE, 3071 bytes)
//     - Relocated to $B100-$BCFE (length $0BFF)
//     - Remains resident in memory at all times
//     - Coordinates Editor, Assembler, and Linker modules
//
// STARTUP SEQUENCE:
// 1. ProDOS loads EDASM.SYSTEM at $2000 and starts execution
// 2. Relocator initializes stack and zeros it
// 3. Relocator marks ProDOS memory pages as free/used
// 4. Relocator copies EI code from $2400 to $B100
// 5. Relocator transfers control to EI at $B100
// 6. EI initializes and loads the Editor module
//
// PRODOS MEMORY MANAGEMENT:
// The relocator manages the ProDOS memory bitmap to indicate
// which memory pages are available for use. Pages $20-$BF are
// initially marked as free, then the EI marks its own pages
// ($B1-$B8, $BF) as used to prevent ProDOS from overwriting them.
//
// EXTERNAL REFERENCES:
// None - this is the first code to run, so it cannot reference
// other modules.
//---------------------------------------------------------;
#include "EdAsmNg/ei/relocator.hpp"

#include <ncurses.h>

#include <cctype>
#include <cerrno>
#include <ctime>
#include <string>
#include <string_view>

#include "EdAsmNg/ei/interpreter.hpp"
#include "EdAsmNg/ei/os.hpp"
#include "EdAsmNg/ei/screen.hpp"

static bool L20B6();

//
// Relocator Entry Point
// Initialize stack and zero it for clean startup
//
void Relocator::L2000() {
  int A, X, Y;
  A = X = Y = 0;
  // Get the date/time
  GETDATETIME();  // No parms needed

  INIT();

  SETNORM();
  HOME();
  if (!P8DATE) {
    //
    // No clock card
    //
    SendBanr(Banner);
    SetCH(0);
    L21DE(stateL21DE::L21DE);  // enter DD
    enum class stateL2065 { L2065, L206E, L2074, L207D, L2084 };
    for (;;) {
      stateL2065 state = stateL2065::L2065;
      switch (state) {
        case stateL2065::L2065:
          SetCH(3);
          if (L22C1(stateL22C1::L22C1)) {  // enter MMM
            state = stateL2065::L2074;
            break;
          }
        case stateL2065::L206E:
          L21DE(stateL21DE::L21FB);
          state = stateL2065::L2065;
          break;
          //
          // Year
          //
        case stateL2065::L2074:
          SetCH(7);
          if (L2238(stateL2238::L2238)) {  // enter year digit 1
            state = stateL2065::L2084;
            break;
          }
        case stateL2065::L207D:
          if (L22C1(Relocator::stateL22C1::L22AF)) {
            state = stateL2065::L2074;
            break;
          } else {
            state = stateL2065::L206E;
            break;
          }
          //
        case stateL2065::L2084:
          A = RDKEY();
          if (A == BS) {
            if (L2238(Relocator::stateL2238::L2259)) {
              state = stateL2065::L2084;
              break;
            } else {
              state = stateL2065::L207D;
              break;
            }
          }
          if (A == CR) {
            goto L209E;
          } else {
            COUT(BEL);
            state = stateL2065::L2084;
            break;
          }
      }
      break;
    }
    //
  L209E:
    std::tm tm = {};
    tm.tm_mday = Day;                                   // Day is already 1-31
    tm.tm_mon  = L21CE - 1;                             // Convert 1-12 to 0-11
    tm.tm_year = (L21CF < 80) ? (L21CF + 100) : L21CF;  // 26→126 (2026), 85→85 (1985)
    P8DATE     = PackProDOSDate(tm);
  }
  CurrPfxB = GetProDOSPrefix();  // Get prefix

  if (CurrPfxB.empty()) {
    //
    // No current prefix
    //
    CurrPfxB = GetOnlineVolume(LASTDEV);
  }
  //
  // The code below assumes the TxBuf2 area has been
  // setup properly by ProDOS8
  //

  TxBuf2 = ExecutablePath();
  X      = TxBuf2.find_last_of('/');  // Do we have a trailing /?
  if (std::string::size_type(X) != std::string::npos) {
    //
    TxBuf2.erase(X);                   // Got a trailing /, so remove
    if (TxBuf2.empty()) TxBuf2 = "/";  // it by just adjusting len byte
    //
    if (!SetProDOSPrefix(TxBuf2)) {  // Make this the default prefix
      ShowErr(errno);
    }
    //
    // Get the prefix to Edasm's directory
    //
  } else {  // No trailing /
    EdAsmDir = GetProDOSPrefix();
    if (errno) ShowErr(errno);
  }
  EIStart();
}

// =================================================
void Relocator::ShowErr(int A) {
  ErrCode = A;

  SETNORM();
  INIT();

  HOME();
  PRERR();        // prints "ERR=XX"
  FDD3(ErrCode);  // unsupported mon entry point
  MON();          // jump to monitor
}

//
void Relocator::SendBanr(const std::string_view Banner) {
  for (char c : Banner) {
    addch(c);
  }
  refresh();
}

void Relocator::L21DE(stateL21DE state) {
  int A, X, Y;
  A = X = Y = 0;
  while (true) {
    switch (state) {
      case stateL21DE::L21D5:
        COUT(BEL);
      case stateL21DE::L21DA:
        SetCH(0);
        state = stateL21DE::L21DE;
        break;
      case stateL21DE::L21DE:
        A = RDKEY();
        if (A < '0' || A > '3') {
          state = stateL21DE::L21D5;
          break;
        }
        COUT((A));
        L21D0 = Day = TensT[A & 0x0F];
        state       = stateL21DE::L220D;
        break;
        //
      case stateL21DE::L21FB:
        Day = L21D0;
        SetCH(1);
        state = stateL21DE::L220D;
        break;
        //
      case stateL21DE::L2208:
        COUT(BEL);
      case stateL21DE::L220D:
        A = RDKEY();
        if (A == BS) {
          state = stateL21DE::L21DA;
          break;
        }
        if (A < '0' || A > '9') {
          state = stateL21DE::L2208;
          break;
        }
        COUT(A);
        Day += (A & 0x0F);

        if (Day < 1 || Day > 31) {    // at most 31
          state = stateL21DE::L21D5;  // err
          break;
        } else
          return;
    }
  }
}

// Year
bool Relocator::L2238(stateL2238 state) {
  int A, X, Y;
  A = X = Y = 0;
  for (;;) {
    switch (state) {
      case stateL2238::L222F:
        COUT(BEL);
      case stateL2238::L2234:
        SetCH(7);
      case stateL2238::L2238:
        A = RDKEY();
        if (A == BS) return false;
        if (A < '0' || A > '9') {
          state = stateL2238::L222F;
          break;
        }
        COUT(A);
        L21D1 = L21CF += TensT[(A & 0x0F)];
        state = stateL2238::L226B;
        //
      case stateL2238::L2259:
        L21CF = L21D1;
        SetCH(8);
        state = stateL2238::L226B;
        break;
        //
      case stateL2238::L2266:
        COUT(BEL);
      case stateL2238::L226B:
        A = RDKEY();
        if (A == BS) {
          state = stateL2238::L2234;
          break;
        }
        if (A < '0' || A > '9') {
          state = stateL2238::L2266;
          break;
        }
        COUT(A);
        L21CF += (A & 0x0F);
        if (L21CF == 0) {
          state = stateL2238::L222F;
          break;
        }
        return true;
    }
  }
}

//
// Month entry
//
bool Relocator::L22C1(stateL22C1 state) {
  int A, X, Y;
  A = X = Y = 0;
  for (;;) {
    switch (state) {
      case stateL22C1::L22AF:
        SetCH(5);  // 3rd letter of MMM
        state = stateL22C1::L22DF;
        break;
      case stateL22C1::L22B8:
        COUT(BEL);
      case stateL22C1::L22BD:
        SetCH(3);  // 1st letter of MMM
      case stateL22C1::L22C1:
        A = ToUCase();
        if (A == BS) return false;
        if (!PRUCase(A)) {
          state = stateL22C1::L22C1;
          break;
        }
        MMM[0] = (A);
      case stateL22C1::L22D0:
        A = ToUCase();
        if (A == BS) {
          state = stateL22C1::L22BD;
          break;
        }
        if (!PRUCase(A)) {
          state = stateL22C1::L22D0;
          break;
        }
        MMM[1] = (A);
      case stateL22C1::L22DF:
        A = ToUCase();
        if (A == BS) {
          COUT(A);
          state = stateL22C1::L22D0;
          break;
        }
        //
        if (!PRUCase(A)) {
          state = stateL22C1::L22DF;
          break;
        }
        MMM[2] = (A);
        //
        X = L21CE = 0;
      case stateL22C1::L22F9:
        Y = 0;
        L21CE++;
        L228A = X;
      case stateL22C1::L2301:
        A = MMM[Y];
        if (A == Months[X]) {
          X++;
          Y++;
          if (Y < 3) {
            state = stateL22C1::L2301;
            break;
          }
          if (Y == 3) return true;  // match
        } else {
          X = L228A + 3;
          if (X < 48) {  // bug? (should be 36)
            state = stateL22C1::L22F9;
            break;
          }
          //
          SetCH(3);
          state = stateL22C1::L22B8;  // re-enter
          break;
        }
    }
  }
}

//
// Display char (in A) as uppercase letter
//
bool Relocator::PRUCase(int A) {
  if (A < 'A' || A > 'Z') {
    COUT(BEL);
    return false;
  }
  COUT(A);
  return true;
}

//
int Relocator::ToUCase() {
  int A = RDKEY();
  if (A >= 0xE0) A &= 0xDF;  // should be "a" ($E1)
  return (A);
}