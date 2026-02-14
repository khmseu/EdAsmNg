#pragma once

const char     BEL   = '\a';
const char     BS    = '\b';
const char     CR    = '\r';
constexpr char FF    = '\f';
constexpr char SPACE = ' ';

// Initialize ncurses and color pair 1 (white on black). Throws on failure and
// installs an automatic teardown at process exit.
void INIT();

void SETNORM();
void HOME();

void SetCH(int column);
int  GetCH();
void SetCV(int row);
int  GetCV();

// Wait for a single keypress without echoing it; returns the character code.
int RDKEY();

// Output a single character; BEL triggers beep, others are echoed via addch.
void COUT(int ch);
