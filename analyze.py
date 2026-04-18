import os
import re


def get_supported_mnemonics(cpp_path):
    mnemonics = set()
    with open(cpp_path, "r") as f:
        content = f.read()
        # Look for mnemonic == "NAME"
        matches = re.findall(r'mnemonic\s*==\s*"([^"]+)"', content)
        mnemonics.update(matches)
    return mnemonics


def parse_ei_source(start_file, base_dir):
    tokens = set()
    visited = set()

    def process_file(file_path):
        if file_path in visited:
            return
        visited.add(file_path)
        if not os.path.exists(file_path):
            return

        with open(file_path, "r") as f:
            for line in f:
                # Remove comments
                line = line.split(";")[0].strip()
                if not line:
                    continue

                # Check for include
                inc_match = re.match(r"^\s*INCLUDE\s+(\S+)", line, re.I)
                if inc_match:
                    inc_file = inc_match.group(1)
                    # Normalize path
                    inc_path = os.path.join(os.path.dirname(file_path), inc_file)
                    process_file(inc_path)
                    continue

                # Token extraction: Label? Mnemonic/Directive
                # Labels start at col 0 or are followed by colon
                parts = line.split()
                if not parts:
                    continue

                # Simple heuristic: if first part doesn't look like a mnemonic, it's a label.
                # Or if it has a colon.
                mnemonic = ""
                if line[0].isspace():
                    mnemonic = parts[0]
                else:
                    if len(parts) > 1:
                        mnemonic = parts[1]

                if mnemonic:
                    # Clean mnemonic: remove trailing colon if present (though usually on labels)
                    mnemonic = mnemonic.rstrip(":").upper()
                    tokens.add(mnemonic)

    process_file(start_file)
    return tokens


cpp_path = "src/lib/asm/asm.cpp"
ei_start = "comparative-tests/inputs/EDASM.SRC/EI.S"
base_dir = os.path.dirname(ei_start)

supported = get_supported_mnemonics(cpp_path)
# Add some known directives that aren't mnemonics but are supported
supported.update(
    ["ORG", "EQU", "DS", "DB", "DW", "INCLUDE", "HEX", "ASC", "MSB", "LST", "REV"]
)

ei_tokens = parse_ei_source(ei_start, base_dir)

unsupported = sorted([t for t in ei_tokens if t not in supported])

print(f"Supported mnemonics found: {len(supported)}")
print(f"EI tokens found: {len(ei_tokens)}")
print("\nUnsupported / Missing tokens in EI.S:")
for t in unsupported:
    print(f"  {t}")
