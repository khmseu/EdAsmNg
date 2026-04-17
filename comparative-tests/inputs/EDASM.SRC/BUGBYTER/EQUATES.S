Name          : EQUATES.S
End of file   :    2,654
This file was generated using the DiskBrowser utility with minimal editing.
It is meant for viewing purposes only.

;---------------------------------------------------------;
; BUGBYTER/EQUATES.S - Constants and Memory Definitions
;
; BugByter (BB) is a 6502 debugger/monitor for the Apple II.
; It provides real-time tracing, breakpoints, register display,
; and disassembly capabilities.
;
; ORIGINAL DISASSEMBLY CREDITS:
;     Disassembled with The Flaming Bird Disassembler
;    (c) Phoenix corp. 1992,93  -  All rights reserved
; References:
;              Neil Parker's 6502 instruction set
;                      Apple II Monitor
;          Appendix C pg 203 of WorkBench Manual
;
; MEMORY USAGE:
; - BB assembled at $0800 but can be relocated anywhere from
;   $0800-$95FF as long as sufficient memory is available
; - Uses 32 bytes of hardware stack ($0100-$011F)
; - Input buffer at $0200-$027F (128 bytes)
;---------------------------------------------------------;
;
; Apple II Monitor Zero Page Locations (used by BB)
;
WNDLFT      EQU    $20             ;Window left edge (text window definition)
WNDWDTH     EQU    $21             ;Window width (characters per line)
WNDTOP      EQU    $22             ;Window top edge (first row)
WNDBTM      EQU    $23             ;Window bottom edge (last row + 1)
CH          EQU    $24             ;Cursor horizontal position (column)
CV          EQU    $25             ;Cursor vertical position (row)
BASL        EQU    $28             ;Base address low for current text line
BASH        EQU    $29             ;Base address high for current text line
CSWL        EQU    $36             ;Character output vector low byte
Z2F         EQU    $2F             ;General purpose zero page location
;
; BugByter Zero Page Working Storage ($3A-$49)
;
            DSECT
            ORG    $3A
MONPC       EQU    *               ;Monitor Program Counter / BugByter PC
PZWS        EQU    *               ;Zero page workspace start
Z3APTR      DS     2               ;$3A-$3B: General pointer
ScrLoc      DS     2               ;$3C-$3D: Screen location pointer
Z3E         DS     2               ;$3E-$3F: General pointer
Z40         DS     1               ;$40: General purpose byte
Z41         DS     1               ;$41: General purpose byte
Z40PTR      EQU    Z40             ;$40-$41: Alternate pointer interpretation
Z42         DS     1               ;$42: General purpose byte
Z43         DS     1               ;$43: General purpose byte
Z44         DS     1               ;$44: General purpose byte
Z43PTR      EQU    Z43             ;$43-$44: Alternate pointer interpretation
;
; Saved CPU Registers ($45-$49)
; BugByter preserves the user program's register state here
;
CPURegs     EQU    *               ;Base of saved CPU registers
HACCUM      DS     1               ;$45: Saved Accumulator
HXREG       DS     1               ;$46: Saved X register
HYREG       DS     1               ;$47: Saved Y register
HFLAGS      DS     1               ;$48: Saved Processor Status (flags)
HSTAKP      DS     1               ;$49: Saved Stack Pointer
Z46         EQU    HXREG           ;$46: Alternate reference
            DEND
;
; Apple II Keyboard Control Codes
; Note: High bit set ($80) for Apple II keyboard input
;
CTRLB       EQU    $82             ;^B - Breakpoint command
CTRLC       EQU    $83             ;^C - Continue execution
CTRLD       EQU    $84             ;^D - Disassemble
BEL         EQU    $87             ;Bell/beep character
CURSL       EQU    $88             ;^H - Cursor left (backspace)
TAB         EQU    $89             ;^I - Tab character
CR          EQU    $8D             ;^M - Carriage return (Enter)
CTRLN       EQU    $8E             ;^N - Next instruction
CURSR       EQU    $95             ;^U - Cursor right
CTRLX       EQU    $98             ;^X - Exit/cancel
ESC         EQU    $9B             ;Escape key
SPACE       EQU    $A0             ;Space character (with high bit)
DELCH       EQU    $FF             ;Delete character
;
; Apple II Hardware Stack and System Vectors
;
STACK       EQU    $0100           ;Hardware stack page ($0100-$01FF)
BBStack     EQU    $011F           ;BugByter's stack limit (uses $0100-$011F, 32 bytes)
INBUF       EQU    $200            ;Input buffer for BI commands ($200-$27F, 128 bytes)
DOSWrm      EQU    $03D0           ;ProDOS warm start vector (2 bytes)
BRKV        EQU    $03F0           ;BRK interrupt vector (2 bytes)
SOFTEV      EQU    $03F2           ;Soft reset vector (2 bytes)
PWREDUP     EQU    $03F4           ;Power-up byte (checksum for warm start detect)
USRADR      EQU    $03F8           ;Ctrl-Y handler address (2 bytes)
;
; Apple II Text Screen Memory Map ($0400-$07FF)
; The Apple II text screen uses a peculiar interleaved memory
; layout. Each group of 8 lines is stored in a different area.
; Lines 0,8,16 are together, then lines 1,9,17, etc.
; This DSECT defines the screen line base addresses.
;
            DSECT  
            ORG    $400
SLIN00      DS     40              ;$0400: Screen line 0
SLIN08      DS     40              ;$0428: Screen line 8
SLIN16      DS     40              ;$0450: Screen line 16
SHOLA0      DS     8               ;$0478-$047F: Hole (unused bytes between screen sections)
SLIN01      DS     40              ;$0480: Screen line 1
SLIN09      DS     40              ;$04A8: Screen line 9
SLIN17      DS     40              ;$04D0: Screen line 17
SHOLA1      DS     8               ;$04F8-$04FF: Hole
SLIN02      DS     40              ;$0500: Screen line 2
SLIN10      DS     40              ;$0528: Screen line 10
SLIN18      DS     40              ;$0550: Screen line 18
SHOLA2      DS     8               ;$0578-$057F: Hole
SLIN03      DS     40              ;$0580: Screen line 3
SLIN11      DS     40              ;$05A8: Screen line 11
SLIN19      DS     40              ;$05D0: Screen line 19
SHOLA3      DS     8               ;$05F8-$05FF: Hole
SLIN04      DS     40              ;$0600: Screen line 4
SLIN12      DS     40              ;$0628: Screen line 12
SLIN20      DS     40              ;$0650: Screen line 20
SHOLA4      DS     8               ;$0678-$067F: Hole
SLIN05      DS     40              ;$0680: Screen line 5
SLIN13      DS     40              ;$06A8: Screen line 13
SLIN21      DS     40              ;$06D0: Screen line 21
SHOLA5      DS     8               ;$06F8-$06FF: Hole
SLIN06      DS     40              ;$0700: Screen line 6
SLIN14      DS     40              ;$0728: Screen line 14
SLIN22      DS     40              ;$0750: Screen line 22
SHOLA6      DS     8               ;$0778-$077F: Hole
SLIN07      DS     40              ;$0780: Screen line 7
SLIN15      DS     40              ;$07A8: Screen line 15
SLIN23      DS     40              ;$07D0: Screen line 23 (last line)
SHOLA7      DS     8               ;$07F8-$07FF: Hole
            DEND
;
; 6502 Instruction Opcodes (used by disassembler/debugger)
;
iJSR        EQU    $20             ;JSR instruction opcode (Jump to Subroutine)
iRTI        EQU    $40             ;RTI instruction opcode (Return from Interrupt)
iJMP        EQU    $4C             ;JMP instruction opcode (Jump absolute)
iRTS        EQU    $60             ;RTS instruction opcode (Return from Subroutine)
iNOP        EQU    $EA             ;NOP instruction opcode (No Operation)
;
; System Entry Points (ProDOS and Monitor)
;
DOSCMD      EQU    $BE03           ;ProDOS command interpreter entry point
PRINTERR    EQU    $BE0C           ;ProDOS print error routine
PRODOS8     EQU    $BF00           ;ProDOS MLI (Machine Language Interface) entry
;
; Apple II Hardware Soft Switches
;
KBD         EQU    $C000           ;Keyboard data register (bit 7 = key pressed)
KBDSTROBE   EQU    $C010           ;Keyboard strobe (clears keyboard data register)
SPKR        EQU    $C030           ;Speaker toggle (read or write toggles speaker)
TXTCLR      EQU    $C050           ;Text mode off (graphics mode on)
TXTSET      EQU    $C051           ;Text mode on (graphics mode off)
MIXCLR      EQU    $C052           ;Mixed mode off (full screen graphics)
MIXSET      EQU    $C053           ;Mixed mode on (4 lines text, 20 lines graphics)
TXTPAGE1    EQU    $C054           ;Display text page 1
TXTPAGE2    EQU    $C055           ;Display text page 2
LORES       EQU    $C056           ;Low-resolution graphics mode
HIRES       EQU    $C057           ;High-resolution graphics mode
BUTN0       EQU    $C061           ;Paddle button 0 / Open-Apple key (bit 7)
PADDL0      EQU    $C064           ;Paddle 0 analog value
PTRIG       EQU    $C070           ;Paddle trigger (start paddle timer)
;
; Monitor ROM Entry Points
;
MONZ        EQU    $FF69           ;Monitor cold start entry point
;
; BugByter Memory Requirements
; BB uses 32 bytes of hardware stack space ($0100-$011F).
; It is assembled at $0800 but may be relocated to anywhere
; in memory from $0800 to $95FF, as long as sufficient
; contiguous memory is available.
;