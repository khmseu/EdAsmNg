Name          : EQUATES.S
End of file   :    1,580
This file was generated using the DiskBrowser utility with minimal editing.
It is meant for viewing purposes only.

            SYS
            ORG    $2000
;=================================================
; LINKER/EQUATES.S - Zero Page Definitions for Linker Module
;
; The Linker resolves external references between relocatable
; object files (REL) and produces executable binaries (BIN/SYS).
;
; This file defines the zero page memory locations used by the
; linker for managing pointers to various data structures, flags,
; and counters during the linking process.
;
; LINKING PROCESS OVERVIEW:
; 1. Read REL files and build DEFINE (EntTbl) and REFERENCE (XtnTbl) tables
; 2. Match EXTRN references to ENTRY definitions
; 3. Apply relocations using RLD (Relocation Dictionary)
; 4. Generate output file (BIN/SYS/REL)
; 5. Optionally produce LinkMap and cross-reference listings
;=================================================
;
; Zero Page Pointers ($80-$9C)
; These are 16-bit pointers (2 bytes each) to various data structures
;
Z80         EQU    $80             ;General purpose pointer (2 bytes)
Z82         EQU    $82             ;General purpose pointer (2 bytes)
SrcP        EQU    $84             ;Pointer into source file data buffer ($0C00+) (2 bytes)
Z86         EQU    $86             ;Pointer into temp ESD buffer at $9100 (2 bytes)
Z88         EQU    $88             ;General purpose record pointer (2 bytes)
EntRecP     EQU    $8A             ;Pointer into DEF/ENTRY table (EntTbl) (2 bytes)
                                    ; Points to current 24-byte ENTRY record
XtnRecP     EQU    $8C             ;Pointer into REF/EXTRN table (XtnTbl) (2 bytes)
                                    ; Points to current 8-byte EXTERN record
SymP        EQU    $8E             ;Pointer into symbolic name table ($9200-$A8DF) (2 bytes)
                                    ; Points to current 16-byte symbol name record
Z8E         EQU    $8E             ;Recycled / alternate use of SymP
;
; Command Line and Message Pointers ($90-$94)
;
cp          EQU    $90             ;Command line buffer pointer (2 bytes)
MsgP        EQU    $92             ;Message pointer for error/status messages (2 bytes)
;
; Address and File Management ($94-$9E)
;
CurrORG     EQU    $94             ;Starting PC (origin) of current source file (2 bytes)
Z96         EQU    $96             ;Current EOF position of REL object file (2 bytes)
LwrBound    EQU    $98             ;Old offset into current code image (2 bytes)
UprBound    EQU    $9A             ;Current offset into current code image (2 bytes)
                                    ; Note: $9B apparently not used
Z9C         EQU    $9C             ;Code image pointer (2 bytes)
Z9E         EQU    $9E             ;Flag: source pathname preceded by semicolon
XtnNbr      EQU    $9F             ;Number of EXTRN symbols in REL file's ESD (1 byte)
FTypeF      EQU    $A0             ;File type flag for target output file:
                                    ; $01=BIN, $80=REL, $02=SYS
;
; Linker Options and Flags ($A1-$A8)
; These control linker behavior via the OPTION command
;
OptionsF    EQU    $A1             ;TRUE ($80) when OPTION command encountered
OptFlagT    EQU    $A2             ;Base of option flags table ($A2-$A8)
MapFileF    EQU    $A2             ;Generate map file (default: ON)
ERefF       EQU    $A3             ;Show entry references (default: ON)
SortLMF     EQU    $A4             ;Sort link map file (default: ON)
XRefF       EQU    $A5             ;Generate cross-reference (default: ON)
NumSortF    EQU    $A6             ;Numeric sort (vs alphabetic) (default: OFF)
OptDF       EQU    $A7             ;Option D flag (default: OFF)
PageF       EQU    $A8             ;Page/paginate output (default: OFF)
;
; Option Value Storage ($AB)
; Dual-purpose: stores ON/OFF switches and 16-bit numeric values
;
Switch      EQU    $AB             ;ON/OFF switch value: $80=ON, $00=OFF
Value16     EQU    $AB             ;16-bit value from arithmetic expression (2 bytes)
;
; Listing and Table Management ($AD-$C1)
;
LineCnt     EQU    $AD             ;Current line count on page
PageCnt     EQU    $AE             ;Current page number
JJJ         EQU    $AF             ;Loop variable J (2 bytes, used in sorting)
III         EQU    $B1             ;Loop variable I (2 bytes, used in sorting)
StrtIdx     EQU    $B3             ;Starting index for loops (2 bytes)
EndIdx      EQU    $B5             ;Ending index for loops (2 bytes)
Jump        EQU    $B7             ;Gap between elements in shell sort (2 bytes)
NumRecs     EQU    $B9             ;Total number of records in ENTRY table (2 bytes)
J.TH        EQU    $BB             ;Pointer to j-th element (2 bytes)
I.TH        EQU    $BD             ;Pointer to i-th element (2 bytes)
EntRPJ      EQU    $BF             ;ENTRY record pointer J (2 bytes)
EntRPI      EQU    $C1             ;ENTRY record pointer I (2 bytes)
;=================================================