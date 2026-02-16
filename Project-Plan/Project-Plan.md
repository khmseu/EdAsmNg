# Project Plan

## Precursor

<https://github.com/markpmlim/EdAsm>

[Original](../Original)

## Basic Concept

We want to reimplement this editor-assembler as closely to identical as possible

## Rules

The project will use C++ on Linux. Insofar as the original does direct screen addressing, we can use ncurses.

Apart from that, we will follow the original source as closely as possible, including trying to keep all the original labels, comments, and bugs. We will try to follow the original code structure as closely as possible, and implement the original algorithms as exactly as possible.

## Exceptions

- when the interfaces to linux and ncurses demand something different
- we will include all code in a single executable
- when we have specific labels for lower and higher bytes, these can be replaced by one label for either a 16 bit word, or an address, as appropriate
- the original code uses Xabcd labels for linking to other pieces of the code, and Labcd labels inside separate pieces; these are aliases

## Rules

- if there is some other reason that makes it impossible to follow the old code
- pointers inside the code will never be converted to integers or vice versa
- code pointers (possibly offset by one if they are supposed to be called with RTS) shall be implemented as function pointers
- do not unroll loops

## Example of how to convert code, including comments

```asm
ChrGot2     LDA    (SrcP),Y
            STY    ZPSaveY
            TAY
            BPL    L8227
;
            BRK                    ;source file must be std ASCII
;
```

```c++
  void ChrGot2() {
    A              = SrcP_at(Y);     // LDA (SrcP),Y
    ZPSaveY        = Y;              // STY ZPSaveY
    uint8_t char_y = A;              // TAY
    if ((int8_t)A >= 0) goto L8227;  // BPL L8227
//
    std::abort();  // BRK - source file must be std ASCII
//
```

## Basic Plan

- the original assembler is found in these directories:
  - [COMMONEQUS.S](../Original/EDASM.SRC/COMMONEQUS.S)
  1. [ASM](../Original/EDASM.SRC/ASM)
  2. [EI](../Original/EDASM.SRC/EI)
  3. [LINKER](../Original/EDASM.SRC/LINKER)
  4. [EDITOR](../Original/EDASM.SRC/EDITOR)
  5. [BUGBYTER](../Original/EDASM.SRC/BUGBYTER)
- We will first convert #1, then #2, and so forth
- Target directories:
  - `src/lib/ei` for EI
  - `src/lib/asm` for ASM
  - `src/lib/linker` for LINKER
  - `src/lib/editor` for EDITOR
  - `src/lib/bugbyter` for BUGBYTER
- with include dirs:
  - `src/include/ei`
  - `src/include/asm`
  - `src/include/linker`
  - `src/include/editor`
  - `src/include/bugbyter`
- to start with, the complete code inside one of the original directories (for example ASM) will be converted into a single file (for example `src/lib/asm/asm.cpp`), and we will add the necessary functions to it as we go along, and we will add the necessary includes to it as we go along
- we will also add a support module for any necessary supporting code for which there is no code in the original, for example code interfacing with ncurses for screen operations, and we will put this in `src/lib/support` with include dir `src/include/support`
- we will add a `src/CMakeLists.txt` to build the project, and we will add the necessary files to it as we go along
- we will add a `tests/` directory for unit tests, and we will add the necessary files to it as we go along
- overall orchestration of the project shall be handled by the Atlas agent
