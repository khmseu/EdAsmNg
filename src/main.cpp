#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "EdAsmNg/asm.hpp"

void print_usage(const char* prog_name) {
  std::cerr << "Usage: " << prog_name
            << " <input.asm> [--listing <listing.lst>] [--object <output.obj>]\n";
}

std::string read_file(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Cannot open file: " + filename);
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

void write_binary(const std::string& filename, const std::vector<uint8_t>& data) {
  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Cannot write file: " + filename);
  }
  file.write(reinterpret_cast<const char*>(data.data()), data.size());
}

void write_text(const std::string& filename, const std::string& text) {
  std::ofstream file(filename);
  if (!file) {
    throw std::runtime_error("Cannot write file: " + filename);
  }
  file << text;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  std::string input_file;
  std::string listing_file;
  std::string object_file;

  // Parse arguments
  input_file = argv[1];
  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "--listing") == 0 && i + 1 < argc) {
      listing_file = argv[++i];
    } else if (strcmp(argv[i], "--object") == 0 && i + 1 < argc) {
      object_file = argv[++i];
    } else {
      std::cerr << "Unknown option: " << argv[i] << "\n";
      print_usage(argv[0]);
      return 1;
    }
  }

  try {
    // Read source file
    std::string source = read_file(input_file);

    // Replace Unix LF with CR (ProDOS text format expected by assembler)
    for (char& c : source) {
      if (c == '\n') c = '\r';
    }

    // Initialize assembler
    EdAsmNg::Asm::ResetErrorState();
    EdAsmNg::Asm::ResetAsmState();
    EdAsmNg::Asm::SetPC(0);
    EdAsmNg::Asm::EnableTestObjMemory(true);
    EdAsmNg::Asm::ClearTestObjMemory();

    // Load source
    EdAsmNg::Asm::SetupMemorySource(source.c_str(), source.length());

    // Run three-pass assembly
    std::cout << "Pass 1...\n";
    EdAsmNg::Asm::SetPassNbr(0);
    EdAsmNg::Asm::DoPass1();

    std::cout << "Pass 2...\n";
    EdAsmNg::Asm::RewindSource();
    EdAsmNg::Asm::SetPassNbr(1);
    EdAsmNg::Asm::SetGenF(0);  // Enable code generation (clear suspension flag)
    EdAsmNg::Asm::DoPass2();

    std::cout << "Pass 3...\n";
    EdAsmNg::Asm::RewindSource();
    EdAsmNg::Asm::SetPassNbr(2);
    EdAsmNg::Asm::DoPass3();

    // Get results
    uint16_t objpc  = EdAsmNg::Asm::GetObjPC();
    uint16_t pc     = EdAsmNg::Asm::GetPC();
    uint16_t curadr = EdAsmNg::Asm::GetCurAdr();

    std::cout << "Assembly complete:\n";
    std::cout << "  PC: $" << std::hex << pc << "\n";
    std::cout << "  ObjPC: $" << std::hex << objpc << "\n";
    std::cout << "  CurAdr: $" << std::hex << curadr << "\n";

    // Write object file if requested
    if (!object_file.empty() && objpc > 0) {
      // Find the actual code range (first and last non-zero bytes)
      // Original EDASM writes only the code region, not the full memory
      uint16_t first_addr = 0xFFFF;
      uint16_t last_addr  = 0;

      for (uint16_t addr = 0; addr < objpc; addr++) {
        if (EdAsmNg::Asm::GetTestObjMemory(addr) != 0) {
          if (first_addr == 0xFFFF) first_addr = addr;
          last_addr = addr;
        }
      }

      if (first_addr != 0xFFFF) {
        // Write only the actual code region
        std::vector<uint8_t> obj_data;
        for (uint16_t addr = first_addr; addr <= last_addr; addr++) {
          obj_data.push_back(EdAsmNg::Asm::GetTestObjMemory(addr));
        }
        write_binary(object_file, obj_data);
        std::cout << "Wrote " << obj_data.size() << " bytes to " << object_file << " (range $"
                  << std::hex << first_addr << "-$" << last_addr << ")\n";
      } else {
        std::cout << "No code generated, object file not written\n";
      }
    }

    // Write listing file if requested (placeholder - not yet implemented)
    if (!listing_file.empty()) {
      std::ostringstream listing;
      listing << "EdAsmNg Listing (placeholder)\n";
      listing << "Source: " << input_file << "\n";
      listing << "PC: $" << std::hex << pc << "\n";
      listing << "ObjPC: $" << std::hex << objpc << "\n";
      listing << "\n(Full listing generation not yet implemented)\n";
      write_text(listing_file, listing.str());
      std::cout << "Wrote listing to " << listing_file << "\n";
    }

    return 0;

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
