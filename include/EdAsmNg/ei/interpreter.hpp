#pragma once

//=================================================
// Interpreter Initialization (EI/EDASMINT.S $B100)
//=================================================

// EdAsm Interpreter start - initializes the interpreter system
void EIStart();

//=================================================
// Character output routines (EI/EDASMINT.S $B339)
//=================================================

// Print carriage return
char PrtCR();

// Ring the bell
char RingBell();

// Clear screen
char ClearScr();

// Main character output with column tracking
char PrChar(int A);
