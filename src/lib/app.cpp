#include "EdAsmNg/app.hpp"

#include <string>
#include <string_view>

namespace EdAsmNg {

  std::string greet(std::string_view name) {
    return std::string("Hello, ") + std::string(name) + "!";
  }

}  // namespace EdAsmNg
