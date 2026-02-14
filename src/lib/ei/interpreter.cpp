//=================================================
// EI/EDASMINT.S - EdAsm Interpreter (Command Interpreter)
//
// This is the main EdAsm Interpreter module that coordinates
// between the Editor, Assembler, and Linker modules. It provides:
//
// - Command-line interface and parsing
// - Module loading and switching (ED, ASM, LINK)
// - System initialization and setup
// - ProDOS interface routines
// - Memory management
// - Error handling and messages
// - I/O hook management
//
// MEMORY ORGANIZATION:
// This module is loaded at $2400-$2FFE and relocated to
// $B100-$BCFE (length $0BFF = 3071 bytes). It remains
// resident in memory at all times whether in edit, assembly,
// or link mode.
//
// INITIALIZATION:
// On startup, the EI:
// 1. Marks memory pages $B1-$B8 and $BF as used in ProDOS bitmap
// 2. Sets up RESET vector to handle Apple II RESET key
// 3. Sets up Ctrl-Y handler for user break
// 4. Initializes error message vector
// 5. Loads and starts the Editor module (EDASM.ED)
//
// COMMAND INTERFACE:
// The EI provides a command-line interface where users enter
// commands like ASM, EDIT, LINK, CATALOG, RUN, etc. It parses
// these commands and calls the appropriate routines in the
// Editor, Assembler, or Linker modules.
//
// EXTERNAL REFERENCES:
// This module references routines in the Editor module (EDASM.ED).
// See EI/EXTERNALS.S for X-label to L-label mappings.
//
// SWEET16:
// The EI uses Sweet16 extensively for compact 16-bit operations.
// The Sweet16 interpreter is located at $D000 (Language Card).
//=================================================
#include "EdAsmNg/ei/interpreter.hpp"

#include <cstdint>

#include "EdAsmNg/ei/screen.hpp"

//=================================================
// Character output routines with column tracking
// Original: $B339
//=================================================

//=================================================
// Interpreter Initialization
// Original: EIStart at $B100
//=================================================

namespace {
  // Memory layout constants
  constexpr std::uint16_t X9900 = 0x9900;  // HiMem for Editor

  // Editor state
  std::uint16_t TxtBgn   = 0x0801;  // Text begin (R5)
  std::uint16_t TxtEnd   = 0x0801;  // Text end (R7)
  std::uint16_t HiMem    = X9900;   // High memory limit
  std::uint16_t FreeMem  = 0;       // Free memory available
  std::uint8_t  ExitFlag = 0;       // Exit flag

  //=================================================
  constexpr const char CPYRIGHTS[] =
      "\r\r\r\r\r\r\r"
      "                   PRODOS  EDITOR-ASSEMBLER // "
      "\r\r"
      "                      RELEASE 1.1 "
      "01-MAY-85"
      "\r\r"
      "                         BY JOHN ARKLEY"
      "\r\r\r"
      "                      "
      "COPYRIGHT (C) 1983-85"
      "\r\r"
      "                     "
      "  APPLE COMPUTER INC."
      "\r";
  //=================================================

  //=================================================

  // File I/O constants
  constexpr std::uint16_t XAD00   = 0xAD00;  // 1024-byte I/O buffer address
  constexpr std::uint16_t X9D14   = 0x9D14;  // EdAsm.AutoST filename string address
  constexpr std::uint8_t  TXTtype = 0x04;    // ProDOS TXT file type

  // File check result codes
  constexpr int FILE_NOT_FOUND  = -1;
  constexpr int FILE_TYPE_MATCH = 0;
  constexpr int FILE_WRONG_TYPE = 1;

  // Command buffer
  char SavCmdB = 0;  // Saved command buffer

  // Register for file operations
  std::uint16_t Reg4 = 0;  // General purpose register

  // Forward declarations for stub functions
  void LB741();  // Load EdAsm.Ed
  void LB3A4();  // Unknown function
  void XDD41();  // Init tab table etc
  void LB85D();  // Empty text buffer
  void LB836();  // Set current prefix to EdAsm's directory
  void LB866();  // Set prefix to current prefix
  void LB9D6();  // Read and execute commands from autostart file
  void LB9CA();  // Error handler - print error message
  void LB1CB();  // Reset I/O hooks

  // ProDOS I/O functions
  int NewLine(std::uint8_t refNum, std::uint8_t mask,
              std::uint8_t newlineChar);  // Set newline mode

  // File operation stubs
  int  CkAttrF(std::uint8_t fileType);       // Check file attributes
  void OpenDFile(std::uint16_t bufferAddr);  // Open data file

  // File reference numbers and parameters
  std::uint8_t OpenRN    = 0xA0;  // Open file reference number
  std::uint8_t NewLineRN = 0xA0;  // Newline reference number
  std::uint8_t RdExeRN   = 0xA0;  // Read/execute reference number
  std::uint8_t ExecMode  = 0;     // Execution mode flag

  // Error handling
  void PrErrMsg(std::uint8_t errorCode);  // Print error message and return to mainloop

}  // namespace

//=================================================
//
// Interpreter Initialization
//
void EIStart() {
  LB741();  // Load EdAsm.Ed

  ExitFlag = 0;

  LB3A4();

  XDD41();  // Init tab table etc
  LB85D();  // Empty text buffer
  ClearScr();

  for (const char* p = CPYRIGHTS; *p != '\0'; ++p) {
    PrChar(*p);  // Display copyrights
  }
  //
  // Check for the presence of the EdAsm.AutoST file
  //
  SavCmdB = CR;

  LB836();  // Set curr pfx to EdAsm's dir

  Reg4 = X9D14;  // Is EdAsm.AutoST present?

  int result = CkAttrF(TXTtype);

  if (result != FILE_NOT_FOUND         // Found
      && result == FILE_TYPE_MATCH) {  // and correct type
    OpenDFile(XAD00);                  // 1024-byte I/O buffer
                                       // Open it
    LB866();                           // Set prefix to curr pfx
    LB9D6();                           // Proceed to read and exec its cmds
    RdExeRN = NewLineRN = OpenRN;
    int result          = NewLine(NewLineRN, 0x7F, 0x0D);
    if (result != 0) {   // Handle error (ProDOS carry flag set)
      PrErrMsg(result);  // Print ProDOS error
      return;
    }
    ExecMode = 0x80;  // Flag we are in exec mode
    LB1CB();          // Reset I/O hooks and continue
  } else {
    LB866();
  }
}

//=================================================
// Error messages and error handling
//=================================================
namespace {
  // Error message strings
  const char MSG_FILE_TOO_LARGE[]     = "FILE TOO LARGE\r";
  const char MSG_FILE_TYPE_MISMATCH[] = "FILE TYPE MISMATCH\r";
  const char MSG_WRITE_PROTECTED[]    = "WRITE PROTECTED\r";
  const char MSG_FILE_LOCKED[]        = "FILE LOCKED\r";
  const char MSG_BAD_PATH_FILE[]      = "BAD PATH/FILE NAME\r";
  const char MSG_FILE_SIZE_MISMATCH[] = "FILE SIZE MISMATCH\r";
  const char MSG_DISK_FULL[]          = "DISK FULL\r";
  const char MSG_DISK_IO_FAILURE[]    = "DISK I/O FAILURE\r";
  const char MSG_FILE_NOT_FOUND[]     = "FILE NOT FOUND\r";
  const char MSG_DIR_NOT_FOUND[]      = "DIRECTORY NOT FOUND\r";
  const char MSG_PATH_NOT_FOUND[]     = "PATH NOT FOUND\r";
  const char MSG_DUPLICATE_FILE[]     = "DUPLICATE FILE NAME\r";
  const char MSG_DIR_FULL[]           = "DIRECTORY FULL\r";
  const char MSG_ASM_NOT_ONLINE[]     = "ASSEMBLER NOT ONLINE\r";
  const char MSG_RE_MOUNT[]           = "PLEASE RE-MOUNT ";           // High bit set on last char
  const char MSG_PRESS_RETURN[]       = "PRESS RETURN TO CONTINUE ";  // High bit set
  const char MSG_PRODOS_ERROR[]       = "PRODOS ERROR=$";

  // Error table structure: each entry has error code and message pointer
  struct ErrorEntry {
    std::uint8_t code;
    const char*  message;
  };

  // Error table (matching the original ErrTable at $B5BF)
  const ErrorEntry ErrTable[] = {
      {0x10, MSG_FILE_TOO_LARGE},  {0x11, MSG_FILE_TYPE_MISMATCH}, {0x12, MSG_FILE_SIZE_MISMATCH},
      {0x27, MSG_DISK_IO_FAILURE}, {0x2A, MSG_DISK_IO_FAILURE},    {0x2B, MSG_WRITE_PROTECTED},
      {0x48, MSG_DISK_FULL},       {0x46, MSG_FILE_NOT_FOUND},     {0x40, MSG_BAD_PATH_FILE},
      {0x4B, MSG_BAD_PATH_FILE},   {0x4E, MSG_FILE_LOCKED},        {0x44, MSG_PATH_NOT_FOUND},
      {0x45, MSG_DIR_NOT_FOUND},   {0x47, MSG_DUPLICATE_FILE},     {0x49, MSG_DIR_FULL},
      {0xFC, MSG_ASM_NOT_ONLINE},  {0xFD, MSG_PRESS_RETURN},       {0xFE, MSG_RE_MOUNT},
      {0xFF, MSG_PRODOS_ERROR},
  };

  constexpr int ErrTableSize = sizeof(ErrTable) / sizeof(ErrTable[0]);

  // Error handling variables
  std::uint8_t ErrCode   = 0;        // Current error code
  char         HexStr[3] = "XX";     // Hex string for unknown error codes
  const char*  Reg9      = nullptr;  // Pointer to error message (Z12 in original)

  // Forward declarations for error handling functions
  void        ClsFile2();   // Close file 2
  void        EIWrmStrt();  // EI warm start (return to main loop)
  void        Cnv2Hex(std::uint8_t value, char& hiNibble, char& loNibble);  // Convert byte to hex
  void        LB5FF(std::uint8_t errorCode);       // Entry point with close file 2
  void        PrErrMsg(std::uint8_t errorCode);    // Print error message and return to mainloop
  void        PrErrMsg2(std::uint8_t errorCode);   // Print error message
  void        LB620();                             // Print "ERR: " prefix
  const char* FindErrMsg(std::uint8_t errorCode);  // Find error message in table

  //=================================================
  //
  void LB5FF(std::uint8_t errorCode) {  // ENTRY
    ClsFile2();
    PrErrMsg(errorCode);
  }

  //
  // ($B602) Print Error Message and return to CI mainloop
  //  (A) - error code
  //
  void PrErrMsg(std::uint8_t errorCode) {  // ENTRY
    PrErrMsg2(errorCode);
    EIWrmStrt();
  }

  //=================================================
  // ($B608)
  // Input
  //  (A)=error code
  //
  void PrErrMsg2(std::uint8_t errorCode) {  //<<< from here
    // Search error table for matching error code
    // Original: LDX #$39; loops through table backwards (0x39 = 57 = 19 entries * 3 bytes)
    const char* message = FindErrMsg(errorCode);

    if (message != nullptr) {
      // Found the error message
      Reg9 = message;
      LB620();  // Print "ERR: " and the message
    } else {
      // Error code not in table - show hex code (LB649)
      ErrCode = errorCode;
      Cnv2Hex(ErrCode, HexStr[0], HexStr[1]);
      PrErrMsg2(0xFF);  // Show "PRODOS ERROR=$XX"
    }
  }

  // FindErrMsg: Search error table for error code
  const char* FindErrMsg(std::uint8_t errorCode) {
    for (int i = 0; i < ErrTableSize; i++) {
      if (ErrTable[i].code == errorCode) {
        return ErrTable[i].message;
      }
    }
    return nullptr;  // Not found
  }

  // LB620: Print "ERR: " prefix and error message
  void LB620() {
    // Print "ERR: "
    PrChar('E');
    PrChar('R');
    PrChar('R');
    RingBell();
    PrChar(':');
    PrChar(SPACE);

    // Print the error message string (LB63C)
    // Original uses indirect indexed addressing: LDA (Z12),Y
    int index = 0;
    while (true) {
      char ch = Reg9[index];
      if (ch & 0x80) {  // High bit set marks end
        break;
      }
      PrChar(ch);
      index++;
      if (ch == CR) {
        break;
      }
    }

    // If this is the hex error message, also print the hex code
    if (Reg9 == MSG_PRODOS_ERROR) {
      PrChar(HexStr[0]);
      PrChar(HexStr[1]);
      PrChar(CR);
    }
  }

  // Stub implementations
  void ClsFile2() {
    // TODO: Close file 2
  }

  void EIWrmStrt() {
    // TODO: Return to command interpreter main loop
  }

  void Cnv2Hex(std::uint8_t value, char& hiNibble, char& loNibble) {
    // Convert byte to hex ASCII characters
    // High nibble
    std::uint8_t hi = (value >> 4) & 0x0F;
    hiNibble        = (hi < 10) ? ('0' + hi) : ('A' + hi - 10);
    // Low nibble
    std::uint8_t lo = value & 0x0F;
    loNibble        = (lo < 10) ? ('0' + lo) : ('A' + lo - 10);
  }

}  // namespace

//=================================================

//=================================================
// Stub implementations (to be implemented later)
//=================================================
namespace {
  void LB741() {
    // TODO: Load EdAsm.Ed editor module
  }

  void LB3A4() {
    // TODO: Implement unknown initialization function
  }

  void XDD41() {
    // TODO: Init tab table etc
  }

  void LB85D() {
    // TODO: Empty text buffer
  }

  void LB836() {
    // TODO: Set current prefix to EdAsm's directory
  }

  void LB866() {
    // TODO: Set prefix to current prefix
  }

  void LB9D6() {
    // TODO: Read and execute commands from autostart file
  }

  void LB9CA() {
    // TODO: Print ProDOS error message
  }

  void LB1CB() {
    // TODO: Reset I/O hooks to defaults and enter main command loop
  }

  int NewLine(std::uint8_t refNum, std::uint8_t mask, std::uint8_t newlineChar) {
    // TODO: ProDOS NEWLINE call ($C9)
    // Sets newline mode for the specified file reference number
    // Returns 0 on success, non-zero on error
    (void)refNum;
    (void)mask;
    (void)newlineChar;
    return 0;  // Success for now
  }

  int CkAttrF(std::uint8_t fileType) {
    // TODO: Check file attributes
    // Returns FILE_NOT_FOUND if not found, FILE_TYPE_MATCH if matches, FILE_WRONG_TYPE if wrong
    // type
    (void)fileType;
    return FILE_NOT_FOUND;  // Default: file not found
  }

  void OpenDFile(std::uint16_t bufferAddr) {
    // TODO: Open data file with specified buffer
    (void)bufferAddr;
  }

  int PrColumn = 0;  // Current column position
}  // namespace

//=================================================
// $B339
//
char PrtCR() {  // ENTRY
  return PrChar(CR);
}

char RingBell() {  // ENTRY
  return PrChar(BEL);
}

char ClearScr() {     // ENTRY
  return PrChar(FF);  // Form Feed
}

char PrChar(int A) {  // ENTRY
  if ((A) == BS) {
    PrColumn--;
    if (PrColumn < 0) {
      PrColumn = 0;  // Reset to 0
    }
  }
  if (A >= (SPACE)) {  // ctrl-chars? // Yes, non-printable
    PrColumn++;
  }
  if ((A) == CR) {
    PrColumn = 0;
  }
  COUT(A);
  return A;
}

//=================================================
