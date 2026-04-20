import re
import sys


def parse_lst(filename):
    mapping = {}
    # Captures 4-digit hex address, any content, then the line number.
    # The line number is expected to be preceded by some whitespace.
    pattern = re.compile(r"^([0-9A-F]{4}):.*\s+(\d+)\s+")
    try:
        with open(filename, "r") as f:
            for line_idx, line in enumerate(f):
                match = pattern.match(line)
                if match:
                    addr = match.group(1)
                    line_num = int(match.group(2))
                    if line_num not in mapping:
                        mapping[line_num] = addr
    except FileNotFoundError:
        print(f"Error: {filename} not found")
        return None
    return mapping


def get_lines(filename):
    with open(filename, "r") as f:
        return f.readlines()


def main():
    map1 = parse_lst("comparative-tests/edasm-outputs/EI/EI.LST")
    map2 = parse_lst("/tmp/ei.lst")

    if map1 is None or map2 is None:
        return

    common_lines = sorted(set(map1.keys()) & set(map2.keys()))
    diffs = []
    for ln in common_lines:
        if map1[ln] != map2[ln]:
            diffs.append(ln)

    if not diffs:
        print("No differences found in addresses.")
        return

    print(f"Found {len(diffs)} differences.")
    print("First 20 differing line numbers (Line: EI.LST_addr vs /tmp/ei.lst_addr):")
    for ln in diffs[:20]:
        print(f"{ln}: {map1[ln]} vs {map2[ln]}")

    first_diff = diffs[0]
    print(f"\nSurrounding lines for first difference (Line {first_diff}):")

    lines1 = get_lines("comparative-tests/edasm-outputs/EI/EI.LST")
    lines2 = get_lines("/tmp/ei.lst")

    def print_context(lines, target_line_num, label):
        print(f"--- {label} ---")
        indices = []
        # Pattern to find the line in the file
        pattern = re.compile(r"^([0-9A-F]{4}):.*\s+(" + str(target_line_num) + r")\s+")
        for i, line in enumerate(lines):
            if pattern.match(line):
                indices.append(i)

        if indices:
            target_idx = indices[0]
            start = max(0, target_idx - 4)
            end = min(len(lines), target_idx + 5)
            for i in range(start, end):
                prefix = ">>>" if i == target_idx else "   "
                print(f"{prefix}{lines[i].rstrip()}")
        else:
            print(f"Could not find line number {target_line_num} in file.")

    print_context(lines1, first_diff, "comparative-tests/edasm-outputs/EI/EI.LST")
    print_context(lines2, first_diff, "/tmp/ei.lst")


if __name__ == "__main__":
    main()
