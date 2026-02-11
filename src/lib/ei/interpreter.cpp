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

#include <iostream>

#include "EdAsmNg/ei/os.hpp"
#include "EdAsmNg/ei/screen.hpp"
