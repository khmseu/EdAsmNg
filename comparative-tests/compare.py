#!/usr/bin/env python3
"""Compare EDASM (via ProDOS8Emu) and EdAsmNg assembly outputs.

For each .src/.asm file under inputs/, assemble with both implementations,
then compare both object files byte-by-byte and normalized listing files.

Usage:
    python3 comparative-tests/compare.py [input.src ...]

If no inputs are given, all files in comparative-tests/inputs/ are tested.
Pass --max-instructions N to raise the emulator budget (default: 2000000).
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess  # nosec B404
import sys
import tempfile
from pathlib import Path

# Import listing normalizer from same directory
sys.path.insert(0, str(Path(__file__).resolve().parent))
from normalize_listing import normalize_listing  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parent.parent
PRODOS_EMU = Path("/bigdata/KAI/projects/ProDOS8Emu")
RUN_EDASM = PRODOS_EMU / "tools" / "run_edasm_job.py"
PRODOS_LOG = PRODOS_EMU / "prodos8emu_console_output.log"
EDASMNG_BIN = REPO_ROOT / "build" / "src" / "EdAsmNg_app"
INPUTS_DIR = REPO_ROOT / "comparative-tests" / "inputs"
EDASM_OUTPUTS_DIR = REPO_ROOT / "comparative-tests" / "edasm-outputs"
EDASMNG_OUTPUTS_DIR = REPO_ROOT / "comparative-tests" / "edasmng-outputs"
EDASM_RUN_TIMEOUT_SEC = 600
EDASM_MAX_ATTEMPTS = 2
EDASMNG_RUN_TIMEOUT_SEC = 600
EDASMNG_MAX_ATTEMPTS = 2

def to_prodos_name(stem: str, max_stem_len: int = 11) -> str:
    """Sanitize a stem to a ProDOS-legal filename component: [A-Z][A-Z0-9.]{,14}.

    max_stem_len limits the result so that appending a 4-char extension
    (dot + 3 chars, e.g. .OBJ) stays within the 15-character ProDOS limit.
    """
    upper = stem.upper()
    # Replace any character not in [A-Z0-9.] with '.'
    sanitized = re.sub(r'[^A-Z0-9.]', '.', upper)
    # Collapse runs of dots into one
    sanitized = re.sub(r'\.{2,}', '.', sanitized)
    # Strip leading/trailing dots (first character must be A-Z)
    sanitized = sanitized.strip('.')
    # Ensure the result starts with a letter
    if not sanitized or not sanitized[0].isalpha():
        sanitized = 'X' + sanitized
    return sanitized[:max_stem_len]


GREEN = "\033[32m"
RED = "\033[31m"
YELLOW = "\033[33m"
RESET = "\033[0m"


def print_recursive_listing(root: Path, indent: str = "  ") -> None:
    """Recursively print files under root for debugging missing outputs."""
    if not root.exists():
        print(f"{indent}(directory does not exist: {root})")
        return

    print(f"{indent}Recursive listing for: {root}")
    for dirpath, dirnames, filenames in os.walk(root):
        current = Path(dirpath)
        rel = current.relative_to(root)
        rel_text = "." if str(rel) == "." else str(rel)
        print(f"{indent}{rel_text}/")

        for dirname in sorted(dirnames):
            print(f"{indent}  {dirname}/")
        for filename in sorted(filenames):
            file_path = current / filename
            size = file_path.stat().st_size
            print(f"{indent}  {filename} ({size} bytes)")


def print_prodos_logfile(indent: str = "  ") -> None:
    """Print emulator console logfile to help debug missing outputs."""
    print(f"{indent}ProDOS8Emu logfile: {PRODOS_LOG}")
    if not PRODOS_LOG.exists():
        print(f"{indent}(logfile not found)")
        return

    try:
        content = PRODOS_LOG.read_text(errors="replace")
    except OSError as exc:
        print(f"{indent}(failed to read logfile: {exc})")
        return

    if not content.strip():
        print(f"{indent}(logfile is empty)")
        return

    print(f"{indent}--- logfile begin ---")
    print(content.rstrip())
    print(f"{indent}--- logfile end ---")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument(
        "inputs",
        nargs="*",
        metavar="input.src",
        help="Source file(s) to test. Defaults to all files in comparative-tests/inputs/.",
    )
    p.add_argument(
        "--max-instructions",
        type=int,
        default=2_000_000,
        help="Emulator instruction budget for original EDASM (default: 2000000).",
    )
    p.add_argument(
        "--skip-edasm",
        action="store_true",
        help="Skip original EDASM run; only assemble with EdAsmNg.",
    )
    p.add_argument(
        "--no-build",
        action="store_true",
        help="Skip rebuilding EdAsmNg before running.",
    )
    p.add_argument(
        "--debug",
        action="store_true",
        help="Pass --debug to ProDOS8Emu EDASM runner.",
    )
    p.add_argument(
        "--compare-listing",
        action="store_true",
        help="Deprecated: listing comparison is always enabled.",
    )
    return p.parse_args(argv)


def build_edasmng() -> bool:
    print("Building EdAsmNg...")
    result = subprocess.run(  # nosec B603
        ["cmake", "--build", str(REPO_ROOT / "build"), "--target", "EdAsmNg_app", "-j"],
        capture_output=True,
        text=True,
        cwd=str(REPO_ROOT),
    )
    if result.returncode != 0:
        print(f"{RED}Build failed:{RESET}\n{result.stderr}", file=sys.stderr)
        return False
    return True


def run_original_edasm(
    src_path: Path,
    work_dir: Path,
    max_instructions: int,
    debug: bool = False,
) -> Path | None:
    """Run source through original EDASM emulator. Returns path to OBJ file or None on failure."""
    prodos_stem = to_prodos_name(src_path.stem)
    obj_name = f"{prodos_stem}.OBJ"
    lst_name = f"{prodos_stem}.LST"

    # The source file must also have a ProDOS-legal name on the EDASM volume.
    # Create a renamed copy so run_edasm_job imports it under the sanitized name.
    src_ext = src_path.suffix.lstrip('.').upper() or 'ASM'
    prodos_src_name = f"{prodos_stem}.{src_ext}"
    src_copy = work_dir.parent / prodos_src_name
    shutil.copy2(str(src_path), str(src_copy))

    cmd = [
        sys.executable,
        str(RUN_EDASM),
        "--input", str(src_copy),
        "--listing", lst_name,
        "--output", obj_name,
        "--work-dir", str(work_dir),
        "--max-instructions", str(max_instructions),
    ]
    if debug:
        cmd.append("--debug")

    try:
        result = subprocess.run(  # nosec B603
            cmd,
            capture_output=True,
            text=True,
            cwd=str(PRODOS_EMU),
            timeout=EDASM_RUN_TIMEOUT_SEC,
        )
    except subprocess.TimeoutExpired:
        print(f"  {YELLOW}EDASM TIMED OUT ({EDASM_RUN_TIMEOUT_SEC}s){RESET}")
        print_prodos_logfile()
        print_recursive_listing(work_dir)
        return None

    if result.returncode != 0:
        print(f"  {YELLOW}EDASM FAILED (exit {result.returncode}){RESET}")
        if result.stdout.strip():
            print("  stdout:", result.stdout.strip()[:400])
        if result.stderr.strip():
            print("  stderr:", result.stderr.strip()[:400])
        print_prodos_logfile()
        print_recursive_listing(work_dir)
        return None

    obj_path = work_dir / "volumes" / "OUT" / obj_name
    if not obj_path.exists():
        # look one level up just in case
        alt = work_dir / obj_name
        if alt.exists():
            obj_path = alt
        else:
            print(f"  {YELLOW}EDASM ran but OBJ not found at {obj_path}{RESET}")
            print_prodos_logfile()
            print_recursive_listing(work_dir )
            return None
    return obj_path


def run_edasmng(src_path: Path, out_dir: Path) -> Path | None:
    """Run source through EdAsmNg. Returns path to OBJ file or None on failure."""
    stem = src_path.stem.upper()
    obj_path = out_dir / f"{stem}.OBJ"
    lst_path = out_dir / f"{stem}.LST"

    cmd = [
        str(EDASMNG_BIN),
        str(src_path),
        "--object", str(obj_path),
        "--listing", str(lst_path),
    ]
    try:
        result = subprocess.run(  # nosec B603
            cmd,
            capture_output=True,
            text=True,
            timeout=EDASMNG_RUN_TIMEOUT_SEC,
        )
    except subprocess.TimeoutExpired:
        print(f"  {YELLOW}EdAsmNg TIMED OUT ({EDASMNG_RUN_TIMEOUT_SEC}s){RESET}")
        print_prodos_logfile()
        print_recursive_listing(out_dir)
        return None

    if result.returncode != 0:
        print(f"  {YELLOW}EdAsmNg FAILED (exit {result.returncode}){RESET}")
        if result.stderr.strip():
            print("  stderr:", result.stderr.strip()[:400])
        print_prodos_logfile()
        print_recursive_listing(out_dir)
        return None
    if not obj_path.exists():
        print(f"  {YELLOW}EdAsmNg ran but OBJ not found{RESET}")
        print_prodos_logfile()
        print_recursive_listing(out_dir)
        return None
    return obj_path


def hexdump(data: bytes, indent: str = "    ") -> str:
    lines = []
    for i in range(0, len(data), 16):
        chunk = data[i:i + 16]
        hex_part = " ".join(f"{b:02X}" for b in chunk)
        lines.append(f"{indent}{i:04X}: {hex_part}")
    return "\n".join(lines)


def compare(ref_path: Path, ng_path: Path, label: str) -> bool:
    ref = ref_path.read_bytes()
    ng = ng_path.read_bytes()
    if ref == ng:
        print(f"  {GREEN}MATCH{RESET}  {label} ({len(ref)} bytes)")
        return True
    print(f"  {RED}MISMATCH{RESET}  {label}")
    print(f"    Reference ({len(ref)} bytes):")
    print(hexdump(ref))
    print(f"    EdAsmNg   ({len(ng)} bytes):")
    print(hexdump(ng))
    # Show first differing byte
    for i, (a, b) in enumerate(zip(ref, ng)):
        if a != b:
            print(f"    First difference at offset {i}: ref={a:02X} ng={b:02X}")
            break
    if len(ref) != len(ng):
        print(f"    Length difference: ref={len(ref)} ng={len(ng)}")
    return False


def compare_listings(edasm_lst: Path | None, ng_lst: Path | None, label: str) -> str:
    """Compare normalized listing files. Returns 'match', 'diff', or 'skip'."""
    if edasm_lst is None or not edasm_lst.exists():
        print(f"  {YELLOW}LST SKIP{RESET}  {label} (EDASM listing not found)")
        return 'skip'
    if ng_lst is None or not ng_lst.exists():
        print(f"  {YELLOW}LST SKIP{RESET}  {label} (EdAsmNg listing not found)")
        return 'skip'

    edasm_text = edasm_lst.read_text(errors='replace')
    ng_text = ng_lst.read_text(errors='replace')

    edasm_norm = normalize_listing(edasm_text)
    ng_norm = normalize_listing(ng_text)

    if edasm_norm == ng_norm:
        print(f"  {GREEN}LST MATCH{RESET}  {label}")
        return 'match'

    print(f"  {RED}LST DIFF{RESET}  {label}")
    edasm_lines = edasm_norm.splitlines()
    ng_lines = ng_norm.splitlines()
    max_show = 10
    shown = 0
    for i, (el, nl) in enumerate(zip(edasm_lines, ng_lines)):
        if el != nl:
            print(f"    Line {i + 1} differs:")
            print(f"      EDASM:   {el!r}")
            print(f"      EdAsmNg: {nl!r}")
            shown += 1
            if shown >= max_show:
                print(f"    ... (truncated at {max_show} differences)")
                break
    if len(edasm_lines) != len(ng_lines):
        print(f"    Line count: EDASM={len(edasm_lines)} EdAsmNg={len(ng_lines)}")
    return 'diff'


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    if args.compare_listing:
        print(f"{YELLOW}Note:{RESET} --compare-listing is deprecated; listing comparison is always enabled.")

    if not EDASMNG_BIN.exists() and not args.no_build:
        print(f"{YELLOW}EdAsmNg binary not found, will build.{RESET}")

    if not args.no_build:
        if not build_edasmng():
            return 1
    elif not EDASMNG_BIN.exists():
        print(f"{RED}Error: EdAsmNg binary not found at {EDASMNG_BIN}{RESET}", file=sys.stderr)
        return 1

    # Gather input files
    if args.inputs:
        sources = [Path(p).resolve() for p in args.inputs]
    else:
        sources = sorted(INPUTS_DIR.glob("*.src")) + sorted(INPUTS_DIR.glob("*.asm"))

    if not sources:
        print("No input files found.", file=sys.stderr)
        return 1

    passed = 0
    failed = 0
    skipped = 0
    listing_passed = 0
    listing_failed = 0
    listing_skipped = 0

    for src in sources:
        print(f"\n{'='*60}")
        print(f"Source: {src.name}")

        with tempfile.TemporaryDirectory(prefix="edasmng_compare_") as tmp:
            tmp_path = Path(tmp)
            ng_out = tmp_path / "ng"
            ng_out.mkdir()

            # Run EdAsmNg with retry in case the process hangs/intermittently fails.
            ng_obj = None
            ng_out_used = None
            for attempt in range(1, EDASMNG_MAX_ATTEMPTS + 1):
                ng_out_attempt = tmp_path / f"ng_{attempt}"
                ng_out_attempt.mkdir()
                ng_obj = run_edasmng(src, ng_out_attempt)
                if ng_obj is not None:
                    ng_out_used = ng_out_attempt
                    break
                if attempt < EDASMNG_MAX_ATTEMPTS:
                    print(f"  {YELLOW}Retrying EdAsmNg ({attempt + 1}/{EDASMNG_MAX_ATTEMPTS})...{RESET}")

            if args.skip_edasm:
                if ng_obj:
                    print(f"  EdAsmNg: {ng_obj.stat().st_size} bytes")
                    print(hexdump(ng_obj.read_bytes()))
                    passed += 1
                else:
                    failed += 1
                continue

            # Run original EDASM with a retry because emulator runs can be flaky.
            edasm_obj = None
            edasm_work_used = None
            for attempt in range(1, EDASM_MAX_ATTEMPTS + 1):
                edasm_work = tmp_path / f"edasm_work_{attempt}"
                edasm_obj = run_original_edasm(
                    src,
                    edasm_work,
                    args.max_instructions,
                    debug=args.debug,
                )
                if edasm_obj is not None:
                    edasm_work_used = edasm_work
                    break
                if attempt < EDASM_MAX_ATTEMPTS:
                    print(f"  {YELLOW}Retrying original EDASM ({attempt + 1}/{EDASM_MAX_ATTEMPTS})...{RESET}")

            if edasm_obj is None:
                print(f"  {YELLOW}SKIPPED{RESET} (original EDASM did not produce output)")
                skipped += 1
                continue

            if ng_obj is None:
                print(f"  {RED}FAILED{RESET} (EdAsmNg did not produce output)")
                failed += 1
                continue

            assert ng_out_used is not None
            assert edasm_work_used is not None

            ok = compare(edasm_obj, ng_obj, src.name)
            if ok:
                passed += 1
            else:
                failed += 1

            prodos_stem = to_prodos_name(src.stem)
            edasm_lst = edasm_work_used / "volumes" / "OUT" / f"{prodos_stem}.LST"
            ng_lst = ng_out_used / f"{src.stem.upper()}.LST"
            lst_result = compare_listings(edasm_lst, ng_lst, src.name)
            if lst_result == 'match':
                listing_passed += 1
            elif lst_result == 'diff':
                listing_failed += 1
            else:
                listing_skipped += 1

            # Persist artifacts to the named output directories
            stem_upper = src.stem.upper()
            EDASM_OUTPUTS_DIR.mkdir(parents=True, exist_ok=True)
            EDASMNG_OUTPUTS_DIR.mkdir(parents=True, exist_ok=True)
            shutil.copy2(str(edasm_obj), str(EDASM_OUTPUTS_DIR / f"{stem_upper}.OBJ"))
            if edasm_lst.exists():
                shutil.copy2(str(edasm_lst), str(EDASM_OUTPUTS_DIR / f"{stem_upper}.LST"))
            shutil.copy2(str(ng_obj), str(EDASMNG_OUTPUTS_DIR / f"{stem_upper}.OBJ"))
            if ng_lst.exists():
                shutil.copy2(str(ng_lst), str(EDASMNG_OUTPUTS_DIR / f"{stem_upper}.LST"))

    print(f"\n{'='*60}")
    print(f"Results: {GREEN}{passed} passed{RESET}  {RED}{failed} failed{RESET}  {YELLOW}{skipped} skipped{RESET}")
    print(f"Listing: {GREEN}{listing_passed} matched{RESET}  {RED}{listing_failed} differed{RESET}  {YELLOW}{listing_skipped} skipped{RESET}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
