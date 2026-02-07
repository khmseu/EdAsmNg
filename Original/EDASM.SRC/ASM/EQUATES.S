Name          : EQUATES.S
End of file   :    7,281
This file was generated using the DiskBrowser utility with minimal editing.
It is meant for viewing purposes only.

**************************************************
* ASM/EQUATES.S - Zero Page and Symbol Definitions for Assembler
*
* This file defines the zero page memory locations, flags, and
* constants used by the EDASM.ASM assembler module. The assembler
* makes extensive use of zero page ($00-$FF) for fast access to
* variables and pointers during the assembly process.
*
* NAMING CONVENTIONS:
* Suffix meanings for symbol names:
*   B - Buffer (memory area for data storage)
*   F - Flag (boolean state variable, often $80/$00 or bit flags)
*   P - Pointer (16-bit address, uses 2 bytes)
*   S - String (can be Pascal, C-string, or other format)
*   T - Table (array of data)
*   Idx - Index (offset into array/table)
*
* ZERO PAGE MEMORY USAGE:
* The contents of $60-$F1 are saved on entry to the assembler
* and restored on exit, allowing the assembler to use these
* locations as temporary workspace without affecting other
* modules. Some locations are "recycled" - they have different
* meanings in different parts of the code to maximize the use
* of limited zero page space.
*
* ASSEMBLY PASSES:
* Pass 1 ($00): Build symbol table, determine addresses
* Pass 2 ($01): Generate object code, resolve references
* Pass 3 ($02): Print symbol table (if requested)
**************************************************
*
* File Control Table (FCT) Indices
* These indices are used to access different file handles in
* the file control tables. Each file type (OBJ, source, include,
* macro, listing) has a dedicated slot.
*
ObjFile     EQU    0               ;Object file (BIN/REL/SYS output) FCT index
ChnFile     EQU    2               ;Chain file (main source file) FCT index
InclFile    EQU    4               ;Include file (INCLUDE directive) FCT index
MacFile     EQU    6               ;Macro library file FCT index
LstFile     EQU    8               ;Listing file (LST output) FCT index
*
* Symbol Table Flag Bits (referenced on page 231 of documentation)
* Each symbol in the symbol table has a flag byte describing its
* characteristics. Multiple flags can be combined using OR.
*
* Example: A symbol can be both 'relative' ($20) and 'entry' ($08)
*          giving a combined flag value of $28.
*
undefined   equ    $80             ;Symbol referenced but not yet defined (forward ref)
unrefd      equ    $40             ;Symbol defined but never referenced (unused label)
relative    equ    $20             ;Symbol has relocatable address (not absolute)
external    equ    $10             ;Symbol defined in another module (EXTRN directive)
entry       equ    $08             ;Symbol exported to other modules (ENTRY directive)
macro       equ    $04             ;Symbol is a macro name
nosuchlabel equ    $02             ;Symbol lookup failed (error condition)
fwdrefd     equ    $01             ;Symbol forward referenced (used before defined)
*
* ZERO PAGE MEMORY LOCATIONS ($60-$F1)
* NB: The contents of $60-$F1 are saved by the Assembler on entry
* and restored on exit, so these locations can be freely used as
* workspace without affecting other modules.
*
* Many of these locations are "recycled" - they have different
* meanings when used in different parts of the code. For example,
* ByteCnt and SavIndX both use $70, but in different contexts.
*
*=================================================
* $60-$6F: General Assembly Control Variables
*=================================================
*
Z60         EQU    $60             ;Generic zero page location (multipurpose)
BCDNbr      EQU    $60             ;Source file line numbers in BCD format ($60-$62, 3 bytes)
StrtSymT    EQU    $63             ;Start address of symbol table (2 bytes: $63-$64)
EndSymT     EQU    $65             ;Current end address of symbol table (2 bytes: $65-$66)
                                    ; Symbol table grows upward from StrtSymT to EndSymT
PassNbr     EQU    $67             ;Current assembly pass: 0=Pass1, 1=Pass2, 2=Pass3
ListingF    EQU    $68             ;Listing flag: $80=LST ON, $00=LST OFF
SubTtlF     EQU    $69             ;Subtitle flag: $00=none, $40=SBTL cmd, $FF=subtitle string
LineCnt     EQU    $6A             ;Number of lines printed on current page
PageNbr     EQU    $6B             ;Current page number (2 bytes: $6B-$6C)
FileNbr     EQU    $6D             ;Current file number in assembly
LogPL       EQU    $6E             ;Logical page length (lines per page)
PhyPL       EQU    $6F             ;Physical page length (actual printer lines)
*
*=================================================
* $70-$7F: Instruction Processing and Pointers
*=================================================
*
SavIndX     EQU    $70             ;Temporary storage for X register
ByteCnt     EQU    $70             ;Number of bytes generated so far (aliases SavIndX)
PrtCol      EQU    $71             ;Current printing column position
EIStack     EQU    $72             ;EdAsm Interpreter's saved stack pointer
CancelF     EQU    $73             ;Cancel flag (user abort via Ctrl-X or other)
NbrErrs     EQU    $74             ;Number of errors encountered (2 bytes: $74-$75)
PrSlot      EQU    $76             ;Printer slot number (0=none, 1-7=slot)
AbortF      EQU    $77             ;Abort flag (fatal error encountered)
SavIndY     EQU    $78             ;Temporary storage for Y register
SrcP        EQU    $79             ;Source pointer - points within current source line (2 bytes)
UnsortedP   EQU    $79             ;Pointer to unsorted auxiliary work array (aliases SrcP)
Src2P       EQU    $7B             ;Copy of source pointer used during code listing (2 bytes)
PC          EQU    $7D             ;Program Counter / position counter (2 bytes: $7D-$7E)
                                    ; Current assembly address during code generation
SortedP     EQU    $7D             ;Pointer to sorted auxiliary array (aliases PC)
ObjPC       EQU    $7F             ;Object code Program Counter (where to store in memory)
SymFBP      EQU    $7F             ;Pointer to symbol's flag byte field (during Pass 1)
CodeLen     EQU    $7F             ;Current length of code image for REL files (stored at BOF)
AuxAryE     EQU    $7F             ;Pointer to last end of sorted array (aliases above)
*
*=================================================
* $80-$8F: File and Symbol Table Management
*=================================================
*
FileLen     EQU    $81             ;Current length of BIN/REL file (2 bytes: $81-$82)
CurrORG     EQU    $83             ;Current origin address from ORG directive (2 bytes)
SymP        EQU    $85             ;Pointer to symbol name (2 bytes: $85-$86)
MnemP       EQU    $85             ;Pointer to mnemonic table entry (aliases SymP)
Delimitr    EQU    $85             ;Delimiter character (aliases SymP)
DTEndCol    EQU    $85             ;End column index of DateTime string (aliases above)
StrType     EQU    $86             ;String type: 0=DCI (inverted last char), -1=ASC
DTCurIdx    EQU    $86             ;Current index into DateTime string (aliases StrType)
MemTop      EQU    $87             ;Top of available memory (2 bytes: $87-$88)
TotLines    EQU    $89             ;Total line count (3 bytes: $89-$8B for large counts)
VidSlot     EQU    $8C             ;Video card slot number
SaveA       EQU    $8D             ;Saved Accumulator value
SaveY       EQU    $8E             ;Saved Y register value
SaveX       EQU    $8F             ;Saved X register value
*=================================================
* $90-$9F: Code Generation and Expression Evaluation
*=================================================
*
DskListF    EQU    $90             ;Disk listing flag: $00=off, $40=partial, $80=lst to file
LstDBIdx    EQU    $91             ;LST data buffer index / number of chars to write
WinLeft     EQU    $92             ;Left edge of 40-column window (for 80-col cards)
WinRight    EQU    $93             ;Right edge of 40-column window
X6502F      EQU    $94             ;65C02 processor flag (vs 6502)
HighMem     EQU    $95             ;High memory address of generated object code (2 bytes)
ExprAccF    EQU    $97             ;Expression's accumulated flag bits
ColCnt      EQU    $97             ;Current print column count (aliases ExprAccF)
SortF       EQU    $98             ;Sort flag for symbol table
NxtToken    EQU    $98             ;Next token type flag (indicates nature of next char)
LstCodeF    EQU    $99             ;Listing control bits for machine code output
SymRefCh    EQU    $99             ;Character printed before symbol's address (aliases above)
GMC         EQU    $9A             ;Generated Machine Code buffer ($9A-$9D, 4 bytes)
                                    ; Holds the bytes of the current instruction being generated
IsFwdRef    EQU    $9A             ;Forward reference flag (bit set if forward ref)
NumCols     EQU    $9B             ;Number of print columns: 2, 4, or 6
SymIdx      EQU    $9C             ;Index into symbol record
ERfield     EQU    $9D             ;Expression Result field
SymAddr     EQU    $9D             ;Address associated with symbolic name (2 bytes: $9D-$9E)
ValExpr     EQU    $9F             ;Value of expression (2 bytes: $9F-$A0)
RLDEntP     EQU    $9F             ;Pointer to RLD (Relocation Dictionary) entry
WrkP        EQU    $A1             ;Work pointer to symbol table entry (2 bytes)
JJJ         EQU    $9F             ;Loop variable J (used during sorting algorithms)
III         EQU    $A1             ;Loop variable I (used during sorting algorithms)
*
*=================================================
* $A0-$AF: Instruction Encoding and Loop Control
*=================================================
*
Length      EQU    $A3             ;Instruction length: 1=1 byte, 2=2 bytes, 3=3 bytes
ModWrdL     EQU    $A4             ;Low byte of permitted addressing modes for mnemonic
ModWrdH     EQU    $A5             ;High byte of permitted addressing modes (2 bytes total)
                                    ; These form a bit mask showing valid modes for 65C02/SW16
StrtIdx     EQU    $A3             ;Starting index for FOR loop (aliases Length)
EndIdx      EQU    $A5             ;Ending index for FOR loop (aliases ModWrdH)
LenTIdx     EQU    $A6             ;Index into instruction length table
Filler      EQU    $A7             ;Filler byte for reserved storage (DS directive)
SavLstF     EQU    $A7             ;Saved listing flags (temporary storage)
GMCIdx      EQU    $A7             ;Index into GMC buffer during code generation
RadixCh     EQU    $A7             ;Radix check character during string-to-binary conversion
Jump        EQU    $A7             ;Gap between two elements (shell sort algorithm)
SavFByt     EQU    $A7             ;Saved symbol's flag byte (temporary)
BitsDig     EQU    $A8             ;Bits per digit for bin/octal/hex conversion (1/3/4)
LabelF      EQU    $A9             ;Label field flag: instruction has a label
RecCnt      EQU    $A9             ;Record count for auxiliary array
NumRecs     EQU    $A9             ;Number of records (alias for RecCnt)
SubTIdx     EQU    $AA             ;Offset into opcode sub-table
ZAB         EQU    $AB             ;Generic zero page location
ErrorF      EQU    $AC             ;Error flag: current line is flagged as incorrect
ErrTIdx     EQU    $AC             ;Error info table index (aliases ErrorF)
msbF        EQU    $AD             ;MSB flag (most significant byte)
J.TH        EQU    $AB             ;Offset/pointer to j-th element of aux array
I.TH        EQU    $AD             ;Offset/pointer to i-th element of aux array
EndianF     EQU    $AE             ;Endianness flag: little-endian vs big-endian
Accum       EQU    $AF             ;Main accumulator (2 bytes: $AF-$B0)
NewPC       EQU    $AF             ;New Program Counter (used by DS directive)
SymPJ       EQU    $AF             ;Symbol pointer J (aliases above)
*
*=================================================
* $B0-$BF: Symbol Processing and Code Generation
*=================================================
*
Ret816F     EQU    $B1             ;Return format: -1=16-bit, 0=low 8-bit, 1=high 8-bit
SymPI       EQU    $B1             ;Symbol pointer I (aliases Ret816F)
RepChar     EQU    $B2             ;Repeat character (used by REP directive)
SymNbr      EQU    $B3             ;Number of symbols declared as EXTRN/ENTRY (DEF/REF)
SymLen      EQU    $B3             ;Length of symbolic name (aliases SymNbr)
SavSTS      EQU    $B3             ;Saved start of symbol table (aliases above)
GblAbsF     EQU    $B4             ;Global/Absolute flag: $00=ZDEF/ZREF, $01=DEF/REF
DummyF      EQU    $B5             ;Dummy section flag (DSECT directive)
SavPC       EQU    $B6             ;Saved Program Counter (2 bytes: $B6-$B7)
SavObjPC    EQU    $B8             ;Saved Object PC (2 bytes: $B8-$B9)
CodeImgLen  EQU    $B8             ;Code image length (aliases SavObjPC)
CondAsmF    EQU    $BA             ;Conditional Assembly Flag: $00, $40, or $80
TabTIdx     EQU    $BB             ;Index into Editor's tab table
SymFByte    EQU    $BC             ;Symbol's current flag byte
RelCodeF    EQU    $BD             ;Relocatable code flag
DskSrcF     EQU    $BE             ;Disk source flag: -1=disk file, 0=memory buffer
GenF        EQU    $BF             ;Generation flag: N=1 suppress, V=1 disk, V=0 memory
*=================================================
* $C0-$CF: File I/O and Macro Processing
*=================================================
*
ObjDBIdx    EQU    $C0             ;Object code data buffer index / bytes to write
IDskSrcF    EQU    $C1             ;Include disk source flag: MSB on = from INCLUDE file
MacroF      EQU    $C2             ;Macro status: $00=not using, $40,$06,$04,$80=file opened
MParmCnt    EQU    $C3             ;Macro parameter count (0-9)
MacArg      EQU    $C4             ;Macro argument number (0-9)
ZC5         EQU    $C5             ;(Not used - reserved)
FCTIndex    EQU    $C6             ;File Control Table index: 0, 2, 4, 6, or 8
PathP       EQU    $C7             ;Pathname pointer (2 bytes: $C7-$C8)
SrcPathP    EQU    $C9             ;Current source filename pointer (2 bytes: $C9-$CA)
RelExprF    EQU    $CB             ;Relative expression flag: non-zero = rel addr expr/sub-expr
SavSTE      EQU    $CC             ;Temporary save for high byte of end-of-symbol-table
SavSEF      EQU    $CC             ;Previous sub-expression's RelExprF (aliases above)
NewF        EQU    $CD             ;New file flag: $80 = new file being assembled
Msg2P       EQU    $CE             ;Message pointer (2 bytes: $CE-$CF)
Lower8      EQU    $CE             ;Low 8 bits of 16-bit value (aliases Msg2P)
ParmBIdx    EQU    $CE             ;Index into parameter string passed by EI
OnOffSW     EQU    $CE             ;On/Off switch: $80=ON, $00=OFF
SrcP3       EQU    $CE             ;Pointer to partial source line minus 1
ZCE         EQU    $CE             ;Generic zero page location
ZCF         EQU    $CF             ;Generic zero page location
SymNodeP    EQU    $CE             ;Symbol node pointer (used in symbol table printing)
TotCnt      EQU    $CF             ;Total count of bytes generated
*
*=================================================
* $D0-$DF: Relocation and Symbol Table Management
*=================================================
*
RLDEnd      EQU    $D0             ;Relocation Dictionary end pointer (2 bytes: $D0-$D1)
                                    ; RLD table builds downward from high memory
ZD2         EQU    $D2             ;(Not used - reserved) (2 bytes: $D2-$D3)
SavGenF     EQU    $D4             ;Saved GenF when DSECT (dummy section) is declared
SBufP       EQU    $D5             ;Pointer to SBuf/IBuf data buffer (2 bytes: $D5-$D6)
MsgP        EQU    $D5             ;Message pointer (aliases SBufP)
HashIdx     EQU    $D7             ;Hash table index (for symbol table lookup)
PrvSymP     EQU    $D8             ;Pointer to previous symbol's node (2 bytes: $D8-$D9)
                                    ; Used for linked list traversal
NxtSymP     EQU    $DA             ;Pointer to next symbol's node (2 bytes: $DA-$DB)
NumCycles   EQU    $DD             ;Instruction's number of CPU cycles
NbrWarns    EQU    $DE             ;Number of warnings (2 bytes: $DE-$DF)
*
*=================================================
* $E0-$EF: Listing Control Flags and Miscellaneous
*=================================================
*
* LST Directive Flags ($E0-$E7)
* These flags control what gets included in assembly listings.
* $00 = OFF (feature disabled)
* Non-zero = ON (feature enabled)
*
LstFlags    EQU    $E0             ;Base address of listing flags array
LstCyc      EQU    $E0             ;List CPU cycle times (default: OFF)
LstUnAsm    EQU    $E1             ;List unassembled source (default: ON)
LstExpMac   EQU    $E2             ;List macro expansion lines (default: ON)
LstWarns    EQU    $E3             ;List warning messages (default: ON)
LstGCode    EQU    $E4             ;Generate object code (default: OFF)
LstASym     EQU    $E5             ;List symbols alphabetically (default: ON)
LstVSym     EQU    $E6             ;List symbols by value order (default: OFF)
Lst6Cols    EQU    $E7             ;Use 6-column symbol dump (default: OFF, uses 4-col)
*
* Miscellaneous Flags and Pointers ($E8-$F1)
*
ZE8         EQU    $E8             ;(Initialized but not used)
SW16F       EQU    $E9             ;Sweet16 flag (indicates SW16 virtual machine code)
ZPSaveY     EQU    $EA             ;Zero page save for Y register (2 bytes: $EA-$EB)
RndF        EQU    $EC             ;Random data flag: $80=use random, $00=use filler byte
ErrNbr4     EQU    $EB             ;Error number times 4 (used as table index)
DecimalS    EQU    $ED             ;Decimal string buffer '0000' (4 bytes: $ED-$F0)
                                    ; Holds ASCII decimal digits for output
ZPRetAdr    EQU    $EF             ;Zero page return address (2 bytes: $EF-$F0)
MacPNLen    EQU    $F1             ;Macro library pathname length byte
                                    ; (Prefix/Macro file pathname length)
*=================================================