Name          : EXTERNALS.S
End of file   :      884
This file was generated using the DiskBrowser utility with minimal editing.
It is meant for viewing purposes only.

**************************************************
* ASM/EXTERNALS.S - External Memory Locations for Assembler Module
*
* This file defines memory buffers and data structures used by the
* EDASM.ASM assembler module. These locations are external to the
* assembler's main code but referenced throughout.
*
* NOTE: Labels starting with 'X' (e.g., X0800, X6E00) are external
* references that may correspond to local labels (e.g., L0800, L6E00)
* in other modules. This naming convention helps track cross-module
* dependencies.
*
* Memory Layout Overview:
* $0800-$08FF : Entry point / workspace
* $6E00-$6FFF : Macro definition buffer (when listing to file)
* $7200-$727F : Macro expansion buffer (128 bytes)
* $7280-$72FF : Macro string parameter storage (128 bytes)
* $7300-$73FF : Online/listing data buffer (256 bytes)
* $7400-$77FF : Listing/macro file buffer (1024 bytes)
* $A100-$A4FF : Object file buffer (1024 bytes)
* $A500-$A8FF : Source file buffer (1024 bytes)
* $A900-$ACFF : Include file buffer (1024 bytes) - defined in COMMONEQUS.S
* $BC00-$BCFF : Symbol table header (hash table / linked list pointers)
* $BD00-$BD7F : Object code data buffer (128 bytes)
* $BD80-$BDFF : Assembler parameter buffer from Editor (128 bytes)
**************************************************
*
* Workspace and Entry Points
*
X0800       EQU    $0800           ;Entry point address / workspace start
*
* Macro Processing Buffers
*
X6E00       EQU    $6E00           ;1024-byte buffer for MACRO definition file (when LST to file)
MacExpBuf   EQU    $7200           ;Macro expansion buffer ($7200-$727F, 128 bytes)
                                    ; Used to expand macro lines during assembly
MacStrBuf   EQU    $7280           ;Macro string parameter buffer ($7280-$72FF, 128 bytes)
                                    ; Used to store string parameters passed to macros
*
* Listing and Data Buffers
*
OLDataB     EQU    $7300           ;Online data buffer ($7300-$73FF, 256 bytes)
LstDBuf     EQU    $7300           ;Circular data buffer for LST file (aliases OLDataB)
                                    ; 256-byte circular buffer for listing output
X7400       EQU    $7400           ;1024-byte buffer for LST/MACRO file ($7400-$77FF)
*
* Inter-Module Communication Buffers
*
ObjDataB    EQU    $BD00           ;Object code data buffer ($BD00-$BD7F, 128 bytes)
                                    ; Holds generated machine code before writing to OBJ file
AsmParmB    EQU    $BD80           ;Assembler parameter buffer ($BD80-$BDFF, 128 bytes)
                                    ; Used by Editor to pass parameters to Assembler
*
* ProDOS File I/O Buffers (1024 bytes each, required by ProDOS)
*
XA100       EQU    $A100           ;Object file buffer ($A100-$A4FF, 1024 bytes)
                                    ; ProDOS I/O buffer for writing BIN/REL/SYS output files
XA500       EQU    $A500           ;Source file buffer ($A500-$A8FF, 1024 bytes)
                                    ; ProDOS I/O buffer for reading .S source files
*XA900 EQU $A900                   ;Include file buffer ($A900-$ACFF, 1024 bytes)
                                    ; Defined in COMMONEQUS.S - for reading INCLUDE'd files
*
* Symbol Table Data Structures
*
HeaderT     EQU    $BC00           ;Symbol table header array ($BC00-$BCFF, 256 bytes)
                                    ; Hash table of pointers to first node of each
                                    ; singly-linked list chain. 256 entry points
                                    ; provide O(1) lookup for symbol table.
*=================================================