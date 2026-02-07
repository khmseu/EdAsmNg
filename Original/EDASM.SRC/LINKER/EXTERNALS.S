Name          : EXTERNALS.S
End of file   :      826
This file was generated using the DiskBrowser utility with minimal editing.
It is meant for viewing purposes only.

;=================================================
; LINKER/EXTERNALS.S - Memory Layout for Linker Module
;
; The Linker module (EDASM.LINK) links relocatable object files
; (REL) into executable programs (BIN or SYS files). It resolves
; external references, applies relocations, and generates symbol
; cross-reference listings.
;
; MEMORY ORGANIZATION ($0800-$A8DF):
; The linker uses a large contiguous memory area to hold multiple
; data structures simultaneously:
;
; $0800-$0BFF : Source file I/O buffer (1024 bytes)
; $0C00-$3EFF : Source data buffer for REL files (13KB)
; $3F00-$42FF : Object/temp ESD file buffer (1024 bytes)
; $4300-$46FF : LinkMap/temp RLD file buffer (1024 bytes)
; $4700-$70FF : DEFINE table - 24-byte records (10.5KB)
; $7100-$90FF : REFERENCE table - 8-byte records (8KB)
; $9100-$91FF : Temp ESD/REL object data buffer (256 bytes)
; $9200-$A8DF : Symbol name table - 16-byte records (5.7KB)
;
; ESD = External Symbol Dictionary
; RLD = Relocation Dictionary
;=================================================
;
; Linker Memory Buffer Allocations
;
SrcIOAdr    EQU    $0800           ;1024-byte I/O buffer for REL source files
                                    ; ProDOS file operations use this buffer
SrcDBuf     EQU    $0C00           ;Read/Write data buffer for source files
                                    ; Large buffer ($0C00-$3EFF) holds REL file contents
X3F00       EQU    $3F00           ;1024-byte buffer for OBJ/temp ESD files ($3F00-$42FF)
X4300       EQU    $4300           ;1024-byte buffer for LinkMap/temp RLD file ($4300-$46FF)
EntTbl      EQU    $4700           ;DEFINE table ($4700-$70FF)
                                    ; 24-byte records for ENTRY symbols from REL files
                                    ; Each record: symbol name, address, flags, source file
XtnTbl      EQU    $7100           ;REFERENCE table ($7100-$90FF)
                                    ; 8-byte records for EXTRN symbols that need resolution
                                    ; Each record: symbol name pointer, file offset
X9100       EQU    $9100           ;Data buffer for temp ESD/REL object file ($9100-$91FF)
X9200       EQU    $9200           ;1-page data buffer for reading code image
SymTbl      EQU    $9200           ;Symbolic name table ($9200-$A8DF)
                                    ; 16-byte records store unique symbol names
                                    ; referenced by EntTbl and XtnTbl pointers
XA8E0       EQU    $A8E0           ;Upper limit of symbolic name table
                                    ; If SymTbl grows beyond this, linker runs out of memory
;=================================================
; Shared Data Areas
;
CurrPfxB    EQU    $BB80           ;Current ProDOS prefix buffer (set by user in Editor)
                                    ; Contains the active directory pathname