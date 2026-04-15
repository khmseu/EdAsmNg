# Comparative Fixture Template

Use this when adding a new source under `comparative-tests/inputs/` for EdAsmNg vs original EDASM parity checks.

The goal is practical parity: both assemblers should accept the same source and produce matching object output, plus matching normalized listing output when the source stays within currently safe behavior.

## Discovery Rules

- `comparative-tests/compare.py` auto-discovers `comparative-tests/inputs/*.src` and `comparative-tests/inputs/*.asm` when no input paths are passed.
- If you pass explicit paths, `compare.py` uses only those files.
- EdAsmNg writes outputs using the uppercase source stem.
- The EDASM runner first sanitizes the stem to a ProDOS-safe name before creating `*.OBJ` and `*.LST`.

## ProDOS-Safe Fixture Names

Keep fixture stems conservative so the original EDASM side does not rename them in surprising ways.

- Prefer stems that are already ProDOS-safe: start with a letter, then use only `A-Z`, `0-9`, and optionally `.`.
- Keep the stem to 11 characters or fewer. `compare.py` truncates to 11 so `.OBJ` or `.LST` still fit within the 15-character ProDOS limit.
- Avoid lowercase-only distinctions, punctuation, spaces, and long names. The EDASM side uppercases the stem, replaces unsupported characters with `.`, collapses repeated dots, strips leading and trailing dots, and may prefix `X` if the name would not start with a letter.
- Avoid stems that could collide after uppercasing, sanitizing, or truncation.

Good examples: `branch`, `equexpr`, `fwdjmp`, `input3`, `simpletest`

## Authoring Rules For Parity-Safe Sources

- Stay within behavior already proven by the current green corpus unless you are intentionally exploring a new area.
- Prefer short, self-contained sources with local labels and fixed addresses like `ORG $0800`.
- Prefer uppercase mnemonics, directives, and labels. That matches the existing fixtures and avoids avoidable parser differences.
- Prefer straightforward operands and data. Keep one new behavior per fixture when possible.
- Prefer trailing `;` comments or no comments at all when the goal is clean parity.
- Avoid leading `*` comment lines for new parity fixtures. EDASM can misread them as source statements and emit extra diagnostics such as invalid identifier or undefined opcode chatter. The listing normalizer strips known diagnostic noise, but the cleaner path is to avoid triggering it in the first place.
- Avoid depending on machine-, ROM-, or OS-specific behavior. The parity goal is matching assembly semantics, not host-specific side effects.
- Avoid intentionally invalid source unless the fixture is specifically meant to study diagnostic behavior.

## Known-Safe Areas In The Current Green Corpus

Current green fixtures: `input.src`, `input2.src`, `input3.src`, `branch.src`, `equexpr.src`, `fwdjmp.src`, `simple_test.asm`

These fixtures currently cover parity-safe use of:

- `ORG`, `END`, and `LST ON`
- Simple labels and forward labels
- Implied instructions such as `NOP`, `DEX`, and `RTS`
- Immediate operands such as `LDA #$00` and `LDX #$05`
- Absolute operands such as `STA $C000`
- Branch-relative control flow such as `BNE LOOP`
- Forward absolute `JMP` target resolution exercised by `fwdjmp.src`
- `EQU` symbol definitions used as operands
- `DFB` byte data
- `ASC` string data
- Blank lines and simple multi-line source files already accepted by both assemblers

Treat anything outside that set as new coverage work, not template-safe baseline coverage.

## Minimal Example

Save a new fixture as either `.src` or `.asm` under `comparative-tests/inputs/`.

```asm
        ORG   $0800

START   LDA   #$01
        STA   $C000
        RTS

        END
```

This is a good template because it is short, deterministic, and only uses feature areas already green.

## Verification Commands

Object parity only:

```bash
python3 comparative-tests/compare.py comparative-tests/inputs/yourfixture.src
```

Object parity plus normalized listing comparison:

```bash
python3 comparative-tests/compare.py --compare-listing comparative-tests/inputs/yourfixture.src
```

Useful notes:

- Omit the file path to run the full corpus.
- Add `--no-build` if `build/src/EdAsmNg_app` is already up to date.
- `--compare-listing` reports listing matches and diffs, but object comparison remains the primary pass or fail result.

## Short Checklist

- Name the file with a ProDOS-safe stem, ideally 11 characters or fewer.
- Use `.src` or `.asm` and place it under `comparative-tests/inputs/`.
- Keep the source short and limited to currently green feature areas unless you are intentionally expanding coverage.
- Avoid leading `*` comment lines when you want clean parity without EDASM-only diagnostics.
- Run object parity first.
- Run object plus listing comparison second.
- Only keep the fixture once both assemblers accept it and the outputs match modulo the normalizer's OS-specific cleanup.
