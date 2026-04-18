import os
import re


def find_labels(root_file):
    labels = {}
    processed_files = set()
    total_labels = 0

    def process_file(filepath):
        nonlocal total_labels
        if filepath in processed_files:
            return
        processed_files.add(filepath)

        if not os.path.exists(filepath):
            # Try to resolve relative to the current file's directory if it's not absolute
            return

        try:
            with open(filepath, "r", encoding="latin-1") as f:
                for line_num, line in enumerate(f, 1):
                    original_line = line
                    line = line.strip("\n")
                    if not line:
                        continue

                    # Ignore comments
                    if line.startswith("*") or line.startswith(";"):
                        continue

                    # Handle INCLUDE
                    include_match = re.match(
                        r'^\s+(?:\.INCLUDE|INCLUDE)\s+["\']?([^"\']+)["\']?',
                        line,
                        re.IGNORECASE,
                    )
                    if include_match:
                        inc_file = include_match.group(1)
                        # Assume file is relative to EDASM.SRC directory or current file directory
                        # Based on context EDASM.SRC/EI.S, let's try relative to its dir
                        inc_path = os.path.join(os.path.dirname(filepath), inc_file)
                        process_file(inc_path)
                        continue

                    # Parse label definitions
                    # lines whose first non-space/tab char is alphabetic
                    if line and line[0].isalpha():
                        # token before whitespace/colon is label field
                        match = re.match(r"^([a-zA-Z0-9_.]+)", line)
                        if match:
                            label = match.group(1)
                            total_labels += 1
                            if label not in labels:
                                labels[label] = []
                            labels[label].append(f"{filepath}:{line_num}")

        except Exception as e:
            print(f"Error reading {filepath}: {e}")

    process_file(root_file)
    return labels, total_labels


if __name__ == "__main__":
    root = "comparative-tests/inputs/EDASM.SRC/EI.S"
    labels, total = find_labels(root)

    duplicates = {k: v for k, v in labels.items() if len(v) > 1}

    if not duplicates:
        print(f"none")
        print(f"Total labels counted: {total}")
    else:
        sorted_dups = sorted(duplicates.items(), key=lambda x: len(x[1]), reverse=True)
        for label, occurrences in sorted_dups[:40]:
            print(f"{label}: {', '.join(occurrences)}")
