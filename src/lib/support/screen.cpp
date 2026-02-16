#include "EdAsmNg/ei/screen.hpp"

#include <ncurses.h>

#include <cstdlib>
#include <stdexcept>

void INIT() {
  if (initscr() == nullptr) {
    throw std::runtime_error("ncurses initialization failed");
  }

  static bool teardown_registered = false;
  if (!teardown_registered) {
    std::atexit([] { endwin(); });
    teardown_registered = true;
  }

  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
  }
}

void SETNORM() {
  attron(COLOR_PAIR(1));
}

void HOME() {
  clear();
  move(0, 0);
}

void SetCH(int column) {
  int y = 0;
  int x = 0;
  getyx(stdscr, y, x);
  move(y, column);
}

int GetCH() {
  int y = 0;
  int x = 0;
  getyx(stdscr, y, x);
  (void)y;
  return x;
}

void SetCV(int row) {
  int y = 0;
  int x = 0;
  getyx(stdscr, y, x);
  (void)y;
  move(row, x);
}

int GetCV() {
  int y = 0;
  int x = 0;
  getyx(stdscr, y, x);
  (void)x;
  return y;
}

int RDKEY() {
  const bool was_echo = is_echo();
  noecho();
  int ch = getch();
  if (was_echo) {
    echo();
  }
  switch (ch) {
    case KEY_BACKSPACE:
    case 127:
    case '\b':
      return BS;
    case '\r':
    case '\n':
      return CR;
    default:
      return ch;
  }
}

void COUT(int ch) {
  switch (ch) {
    case BEL:
      beep();
      return;
    case CR:
    case '\n':
      addch('\n');
      return;
    default:
      addch(ch);
      break;
  }
}
