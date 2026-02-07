Name          : COMMONEQUS.S
End of file   :    6,161
This file was generated using the DiskBrowser utility with minimal editing.
It is meant for viewing purposes only.

            CHR    ";"
;=================================================
; COMMONEQUS.S - Common Equates for all EdAsm modules
;
; This file contains shared constants, memory locations, and 
; hardware/software interface definitions used across all EdAsm
; modules (ASM, EDITOR, BUGBYTER, EI, LINKER).
;
; The equates are organized into sections:
; - ASCII control character codes
; - ProDOS file type constants  
; - Apple II zero page locations
; - EdAsm global memory areas
; - Apple II hardware soft switches
; - Monitor ROM entry points
; - Sweet16 virtual machine registers
; - ProDOS MLI parameter block offsets
;=================================================
; ASCII keycodes
; Control characters for keyboard input and text editing
;
CTRLA       EQU    $01             ;^A - Used in editor navigation
CTRLB       EQU    $02             ;^B - Editor command
CTRLC       EQU    $03             ;^C - Copy/interrupt
CTRLD       EQU    $04             ;^D - Delete character
CTRLE       EQU    $05             ;^E - End of line
CTRLF       EQU    $06             ;^F - Find/search
BEL         EQU    $07             ;Bell/beep character
BS          EQU    $08             ;Backspace
TAB         EQU    $09             ;Tab character
FF          EQU    $0C             ;Form feed
CR          EQU    $0D             ;Carriage return (Enter)
CTRLN       EQU    $0E             ;^N - Next line
CTRLO       EQU    $0F             ;^O - Output control
CTRLQ       EQU    $11             ;^Q - Quote/escape next char
CTRLR       EQU    $12             ;^R - Refresh/redraw
CTRLS       EQU    $13             ;^S - Stop output (XOFF)
CTRLT       EQU    $14             ;^T - Tab/transpose
CTRLU       EQU    $15             ;^U - Undo/delete line
CTRLV       EQU    $16             ;^V - Verbose/insert
CTRLW       EQU    $17             ;^W - Word delete
CTRLX       EQU    $18             ;^X - Cancel operation
ESCAPE      EQU    $1B             ;Escape key
SPACE       EQU    $20             ;Space character
UNDERSCORE  EQU    $5F             ;Underscore '_'
DEL         EQU    $7F             ;Delete key (rubout)
;=================================================
; ProDOS File Type Codes
; Used for file operations (create, open, get_file_info)
;
TXTtype     EQU    $04             ;Text file (TXT)
BINtype     EQU    $06             ;Binary file (BIN)
DIRtype     EQU    $0D             ;Directory
RELtype     EQU    $FE             ;Relocatable object file (REL) - EdAsm specific
SYStype     EQU    $FF             ;System file (SYS) - executable
;=================================================
; Apple ][ Standard Zero Page Locations
; These are predefined by the Apple II Monitor ROM
; and used for screen I/O and BASIC operations
;
WNDWDTH     EQU    $21             ;Window width (chars per line, typically 40 or 80)
CH          EQU    $24             ;Cursor horizontal position (column 0-39 or 0-79)
CV          EQU    $25             ;Cursor vertical position (row 0-23)
BASL        EQU    $28             ;Base address low byte for current text line
InvFlg      EQU    $32             ;Inverse/flash mode flag for text display
Prompt      EQU    $33             ;Prompt character (typically '>')
CSWL        EQU    $36             ;Character output hook (CSW = Char out SoftWitch)
KSWL        EQU    $38             ;Character input hook (KSW = Keyboard SoftWitch)
A1          EQU    $3C             ;Aux pointer 1 (2 bytes: $3C-$3D)
A2          EQU    $3E             ;Aux pointer 2 (2 bytes: $3E-$3F)
A4          EQU    $42             ;Aux pointer 4 (2 bytes: $42-$43)
;=================================================
; EdAsm Zero Page Locations
; These zero page locations are referenced by 2 or
; more of EdAsm's modules and are used for inter-module
; communication and shared state management.
;
LoMem       EQU    $0A             ;Low memory boundary = $0801 (page after zero page+stack)
TxtBgn      EQU    $0A             ;Text buffer beginning (alias for Reg5/R5)
TxtEnd      EQU    $0E             ;Text buffer end pointer (alias for Reg7/R7)
HiMem       EQU    $0C             ;High memory boundary = $9900 (not used by ASM module)
StackP      EQU    $49             ;Save area for hardware stack pointer (during context switch)
VideoSlt    EQU    $50             ;Video card slot = $Cs where s=1-3 (for 80-column card)
FileType    EQU    $51             ;Current file type being processed
ExecMode    EQU    $53             ;Execution mode flag
PtrMode     EQU    $54             ;Printer mode: $80=ON, $00=OFF
TabChar     EQU    $5F             ;Tab character setting (set by Editor module)
PrColumn    EQU    $61             ;Current printer column position
UserTabT    EQU    $68             ;User-defined tab table ($68-$71, 10 bytes)
PrintF      EQU    $73             ;Print command flag: -1=Print, 0=List
StackP2     EQU    $73             ;Alt save for hardware stack ptr (on fatal error)
SwapMode    EQU    $74             ;Split-buffer mode: 0=normal, 1=buf1, 2=buf2
CaseMode    EQU    $75             ;Case conversion mode: uppercase/lowercase flag
CmdDelim    EQU    $78             ;Command delimiter/separator character
TruncF      EQU    $79             ;Truncate comments flag: $FF=truncate, $00=keep
;=================================================
; EdAsm Module Load Addresses
; These define where different modules are loaded in memory
;
X2000       EQU    $2000           ;Load & exec address for .SYSTEM files (ProDOS standard)
LoadAdrEd   EQU    $8900           ;Load address for Editor module (EDASM.ED)
;=================================================
; ProDOS 1024-byte I/O Buffers
; EdAsm uses multiple 1K buffers for file operations.
; ProDOS requires aligned 1K buffers for efficient disk I/O.
;
XA900       EQU    $A900           ;I/O buffer 1 (1024 bytes: $A900-$ACFF)
XAD00       EQU    $AD00           ;I/O buffer 2 (1024 bytes: $AD00-$B0FF)
;=================================================
; EdAsm Global Communication Page
; This page is used to pass parameters between EdAsm's
; modules (Editor, Assembler, Linker). The EI (EdAsm
; Interpreter) module must remain resident at all times
; to maintain this shared state.
;
; Memory Layout ($BD00-$BEFC):
; - $BD00-$BD7F: General purpose 128-byte buffer #1
; - $BD80-$BDFF: General purpose 128-byte buffer #2  
; - $BE00-$BE3F: Current pathname buffer (64 bytes)
; - $BE40-$BE61: Device control strings (for 80-col)
; - $BE60-$BE63: Tab table
; - $BE64-$BE73: Date/Time storage
; - $BE79-$BEFB: EdAsm boot directory pathname
; - $BEFC-$BEFF: Error message routine pointer
;
XBD00       EQU    $BD00           ;General purpose 128-byte buffer #1
XBD80       EQU    $BD80           ;General purpose 128-byte buffer #2
XBE00       EQU    $BE00           ;Current pathname buffer ($BE00-$BE3F, 64 bytes)
DevCtlS     EQU    $BE40           ;Device control strings ($BE40-$BE61, init to $C3 if 80-col)
TabTable    EQU    $BE60           ;Tab stop table ($BE60-$BE63)
DateTime    EQU    $BE64           ;Date/Time storage ($BE64-$BE73, 16 bytes)
EdAsmDir    EQU    $BE79           ;EdAsm's boot directory pathname (where EDASM.SYSTEM lives)
PrtError    EQU    $BEFC           ;EdAsm Interpreter error message routine pointer
;=================================================
; Apple ][ Hardware Memory Locations
;
Stack       EQU    $0100           ;Hardware stack page ($0100-$01FF, 256 bytes)
InBuf       EQU    $0200           ;Input buffer ($0200-$027F, 128 bytes) - Monitor ROM
TxBuf2      EQU    $0280           ;Text buffer 2 ($0280-$02FF, 128 bytes)
SOFTEV      EQU    $03F2           ;Soft entry vector (RESET handler, 2 bytes)
PWREDUP     EQU    $03F4           ;Power-up byte (checksum for warm/cold start)
USRADR      EQU    $03F8           ;User vector: Ctrl-Y handler address (2 bytes)
;=================================================
; ProDOS 8 Global Page ($BF00-$BFFF)
; ProDOS reserves page $BF for system use. This includes
; the MLI (Machine Language Interface) entry point and
; system configuration data.
;
PRODOS8     EQU    $BF00           ;ProDOS MLI (Machine Language Interface) entry point
LASTDEV     EQU    $BF30           ;Last device accessed (slot << 4 | drive)
BitMap      EQU    $BF58           ;System bitmap pointer (memory allocation map)
P8DATE      EQU    $BF90           ;ProDOS date (2 bytes, packed format)
P8TIME      EQU    $BF92           ;ProDOS time (2 bytes, packed format)  
MACHID      EQU    $BF98           ;Machine ID byte (identifies Apple II model)
SLTBYT      EQU    $BF99           ;Slot byte ROM map (bit n=1 if slot n has ROM)
CMDADR      EQU    $BF9C           ;Last MLI call return address (for debugging)
MINIVERS    EQU    $BFFC           ;Minimum interpreter version required
IVERSION    EQU    $BFFD           ;Current ProDOS interpreter version
;=================================================
; Apple ][ Hardware Soft Switches
; Memory-mapped I/O locations. Reading or writing these
; addresses controls hardware features (no data is stored).
;
KBD         EQU    $C000           ;Keyboard data (bit 7=1 if key pressed)
CLR80VID    EQU    $C00C           ;Clear 80-column video mode (switch to 40-col)
KBDSTROBE   EQU    $C010           ;Keyboard strobe (read/write clears keyboard data)
BUTN3       EQU    $C060           ;Pushbutton 3 / joystick button 0 (bit 7=pressed)
BUTN2       EQU    $C063           ;Pushbutton 2 / joystick button 1 (bit 7=pressed)
;
; Language Card / Extended Memory Switches (Apple IIe/IIc)
; These control bank-switched RAM in the $D000-$FFFF region.
; The language card provides 16K of additional RAM that can
; replace the ROM in that area. Two banks are available.
;
RDBANK2     EQU    $C080           ;Read RAM bank 2 only; no write enabled
ROMIN2      EQU    $C081           ;Read ROM / Write RAM bank 2 (requires 2 reads)
RDROM2      EQU    $C082           ;Read ROM only; no write enabled  
LCBANK2     EQU    $C083           ;Read/Write RAM bank 2 (requires 2 reads)
;
; Peripheral Card ROM Addresses
;
BASICINT    EQU    $C300           ;Integer BASIC ROM start (slot 3 equivalent)
XC30B       EQU    $C30B           ;Card identification byte (slot 3)
XC30C       EQU    $C30C           ;Card identification byte (slot 3)
;
; High Memory Regions
;
XD000       EQU    $D000           ;LC Bank2 RAM - used by SW16 or Asm's symbol table printer
BASCLD      EQU    $E000           ;AppleSoft BASIC cold start entry point
;=================================================
; Apple ][ Monitor ROM Entry Points ($F000-$FFFF)
; These are standard ROM routines in the Apple II Monitor.
; They provide low-level I/O and utility functions.
; All Apple II models include these at fixed addresses.
;
SWEET16     EQU    $F689           ;Sweet16 interpreter entry (16-bit virtual machine)
                                    ; Original Integer BASIC ROM location
PRNTAX      EQU    $F941           ;Print A register as hex, then X register as hex
PRBLNK      EQU    $F948           ;Print A register worth of blank spaces
INIT        EQU    $FB2F           ;Initialize screen and I/O
BELL1       EQU    $FBDD           ;Sound the bell/beep
VTABZ       EQU    $FC24           ;Vertical tab - position cursor to row in CV
HOME        EQU    $FC58           ;Clear screen and home cursor (top-left)
CLREOL      EQU    $FC9C           ;Clear to end of line from cursor position
WAIT        EQU    $FCA8           ;Wait for (A) 26-usec intervals
RDKEY       EQU    $FD0C           ;Read keyboard, wait for key, return in A
RDCHAR      EQU    $FD35           ;Read character via input vector (KSW)
GETLN       EQU    $FD6A           ;Input line with prompt (result in input buffer)
CROUT       EQU    $FD8E           ;Output carriage return (CR)
COUT        EQU    $FDED           ;Output character in A via user's output vector (CSW)
COUT1       EQU    $FDF0           ;Output character in A directly to video screen
MOVE        EQU    $FE2C           ;Memory move routine (use A1/A2/A4 pointers)
SETNORM     EQU    $FE84           ;Set normal text mode (not inverse/flash)
SETKBD      EQU    $FE89           ;Set keyboard as input device (KSW)
SETVID      EQU    $FE93           ;Set video as output device (CSW)
PRERR       EQU    $FF2D           ;Print error message (beep + "ERR")
MON         EQU    $FF65           ;Enter monitor (save registers first)
MONZ        EQU    $FF69           ;Enter monitor (without saving registers)
IRQV        EQU    $FFFE           ;IRQ/BRK vector address (2 bytes)
;=================================================
; Sweet16 Virtual Machine Registers (Zero Page)
; Sweet16 is a 16-bit virtual machine interpreter written by
; Steve Wozniak for the Apple II. It provides 16 virtual 16-bit
; registers using zero page locations for compact 16-bit operations.
; These registers are used when accessing Sweet16 via 6502 code.
;
; Each register occupies 2 bytes (16 bits) in zero page.
; Registers have special purposes:
;   R0  - Accumulator for 16-bit operations
;   R12 - Subroutine return stack pointer
;   R13 - Comparison result register  
;   R14 - Status register
;   R15 - Program counter
;
Reg0        EQU    $00             ;R0 - Accumulator (2 bytes: $00-$01)
Reg1        EQU    $02             ;R1 - General purpose (2 bytes: $02-$03)
Reg2        EQU    $04             ;R2 - General purpose (2 bytes: $04-$05)
Reg3        EQU    $06             ;R3 - General purpose (2 bytes: $06-$07)
Reg4        EQU    $08             ;R4 - General purpose (2 bytes: $08-$09)
Reg5        EQU    $0A             ;R5 - Text begin ptr (2 bytes: $0A-$0B) = TxtBgn
Reg6        EQU    $0C             ;R6 - High memory ptr (2 bytes: $0C-$0D) = HiMem
Reg7        EQU    $0E             ;R7 - Text end ptr (2 bytes: $0E-$0F) = TxtEnd
Reg8        EQU    $10             ;R8 - General purpose (2 bytes: $10-$11)
Reg9        EQU    $12             ;R9 - General purpose (2 bytes: $12-$13)
Reg10       EQU    $14             ;R10 - General purpose (2 bytes: $14-$15)
Reg11       EQU    $16             ;R11 - General purpose (2 bytes: $16-$17)
Reg12       EQU    $18             ;R12 - Subroutine return stack ptr (2 bytes: $18-$19)
Reg13       EQU    $1A             ;R13 - Comparison result (2 bytes: $1A-$1B)
Reg14       EQU    $1C             ;R14 - Status register (2 bytes: $1C-$1D)
Reg15       EQU    $1E             ;R15 - Program counter (2 bytes: $1E-$1F)
;=================================================
; Sweet16 Register Names (for EdAsm Assembler)
; These labels are used in Sweet16 assembly code within EdAsm.
; For Merlin assembler, Rn are pre-defined symbols.
; Sweet16 instructions use these register numbers (0-15) as operands.
;
R0          EQU    0               ;Accumulator
R1          EQU    1               ;General purpose
R2          EQU    2               ;General purpose
R3          EQU    3               ;General purpose
R4          EQU    4               ;General purpose
R5          EQU    5               ;Text buffer begin (TxtBgn)
R6          EQU    6               ;High memory (HiMem)
R7          EQU    7               ;Text buffer end (TxtEnd)
R8          EQU    8               ;General purpose
R9          EQU    9               ;General purpose
R10         EQU    10              ;General purpose
R11         EQU    11              ;General purpose
R12         EQU    12              ;Subroutine return stack pointer
R13         EQU    13              ;Comparison result
R14         EQU    14              ;Status register  
R15         EQU    15              ;Program counter
;=================================================
; ProDOS MLI Parameter Block Offsets (from ProDOS v1.7)
; 
; ProDOS MLI (Machine Language Interface) calls use parameter
; blocks in memory. These equates define the byte offsets within
; those parameter blocks for different call types.
;
; Usage: Set up a parameter block with these offsets, then
; call PRODOS8 ($BF00) with the command code in A and the
; parameter block address in X (lo) and Y (hi).
;
; Example parameter block layout for OPEN:
;   +0: parameter count (3)
;   +1: pathname pointer (2 bytes)
;   +3: io_buffer pointer (2 bytes)
;   +5: ref_num (1 byte, returned by ProDOS)
;
c.pCnt      EQU    $0              ;Parameter count (first byte of all parm blocks)
c.devNum    EQU    $1              ;Device number (for ON_LINE)
c.refNum    EQU    $1              ;File reference number (for CLOSE, READ, WRITE, etc)
c.intNum    EQU    $1              ;Interrupt number (for ALLOC_INTERRUPT)
c.path      EQU    $1              ;Pathname pointer (2 bytes) (for OPEN, CREATE, etc)
c.isNewl    EQU    $2              ;Newline enable mask (for NEWLINE)
c.dataBuf   EQU    $2              ;Data buffer pointer (2 bytes) (for READ, WRITE)
c.bufAdr    EQU    $2              ;Buffer address pointer (2 bytes) (for OPEN)
c.intAdr    EQU    $2              ;Interrupt handler address (2 bytes)
c.mark      EQU    $2              ;File position marker (3 bytes) (for GET_MARK, SET_MARK)
c.eof       EQU    $2              ;End-of-file position (3 bytes) (for GET_EOF, SET_EOF)
c.attr      EQU    $3              ;File attributes/access bits (for GET_FILE_INFO, etc)
c.newl      EQU    $3              ;Newline character (for NEWLINE)
c.bufPtr    EQU    $3              ;Buffer pointer (2 bytes)
c.newPath   EQU    $3              ;New pathname pointer (2 bytes) (for RENAME)
c.fileId    EQU    $4              ;File ID byte (for GET_FILE_INFO, SET_FILE_INFO)
c.reqCnt    EQU    $4              ;Request count (2 bytes) (for READ, WRITE)
c.blkNum    EQU    $4              ;Block number (2 bytes) (for READ_BLOCK, WRITE_BLOCK)
c.outRef    EQU    $5              ;Output ref num (returned by OPEN)
c.auxId     EQU    $5              ;Auxiliary file ID (2 bytes) (for GET_FILE_INFO, etc)
c.transCnt  EQU    $6              ;Transfer count (2 bytes, returned) (for READ, WRITE)
c.fKind     EQU    $7              ;File kind/type (for GET_FILE_INFO, SET_FILE_INFO)
c.date      EQU    $8              ;Creation date (2 bytes)
c.outBlk    EQU    $8              ;Output block count (2 bytes, returned)
c.time      EQU    $a              ;Creation time (2 bytes)
c.modDate   EQU    $a              ;Modification date (2 bytes)
c.modTime   EQU    $c              ;Modification time (2 bytes)
c.creDate   EQU    $e              ;Creation date (2 bytes, alternate offset)
c.creTime   EQU    $10             ;Creation time (2 bytes, alternate offset)