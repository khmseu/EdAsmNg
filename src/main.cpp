#include "EdAsmNg/app.hpp"

#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
  std::string_view name = "World";
  if (argc > 1 && argv[1] != nullptr && !std::string_view(argv[1]).empty()) {
    name = argv[1];
  }

  std::cout << EdAsmNg::greet(name) << '\n';
  return 0;
}
