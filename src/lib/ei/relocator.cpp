#include "EdAsmNg/ei/relocator.hpp"

#include <ncurses.h>

#include <string_view>

namespace {

void SETNORM() {
  attron(COLOR_PAIR(1));
}
void HOME() {
  clear();
}
static constexpr std::string_view Banner =
    "\n\n\n  PRODOS  EDITOR-ASSEMBLER //\n\n\n\n\n\n"
    "ENTER THE DATE AND PRESS RETURN\n\nDD-MMM-YY";
void SendBanr(const std::string_view Banner) {
  for (char c : Banner) {
    addch(c);
  }
  refresh();
}
}  // namespace

void L2000() {
  // Relocator Entry Point
  if (initscr() == nullptr) {
    return;
  }

  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
  }
  SETNORM();
  HOME();
  SendBanr(Banner);
  move(3, 4);
  addstr("hello");
  refresh();

  if (has_colors()) {
    attroff(COLOR_PAIR(1));
  }

  endwin();
}