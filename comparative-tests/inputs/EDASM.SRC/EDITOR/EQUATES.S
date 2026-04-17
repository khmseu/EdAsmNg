Name          : EQUATES.S
End of file   :      448
This file was generated using the DiskBrowser utility with minimal editing.
It is meant for viewing purposes only.

;=================================================
; EDITOR/EQUATES.S - Zero Page Definitions for Editor Module
;
; This file defines zero page memory locations used by the
; EDASM.ED editor module. The editor provides full-screen text
; editing capabilities with file operations and Sweet16 virtual
; machine support.
;
; Unlike the assembler, the editor uses a more limited set of
; zero page locations since it must coexist with user text
; buffers and the Sweet16 interpreter.
;
; MEMORY ORGANIZATION:
; - Zero page ($00-$FF): Editor variables and Sweet16 registers
; - Text buffer: Grows from TxtBgn (R5) to TxtEnd (R7)
; - Sweet16 regs: $00-$1F (32 bytes for 16 registers x 2 bytes)
;=================================================
;
; Zero Page Memory Usage for Editor
;
Z10         EQU    $10             ;Counter / general purpose (R8 in Sweet16)
Z12         EQU    $12             ;Character storage (R9 in Sweet16)
Z4E         EQU    $4E             ;General purpose workspace
Z58         EQU    $58             ;General purpose workspace
Z59         EQU    $59             ;Slot number (peripheral card slot 1-7)
Z5A         EQU    $5A             ;General purpose workspace
Z5B         EQU    $5B             ;General purpose workspace
Z5C         EQU    $5C             ;General purpose workspace
Z5E         EQU    $5E             ;Character being printed (output character buffer)
Z60         EQU    $60             ;Tab expansion flag ($80=expand tabs, $00=literal)
Z62         EQU    $62             ;General purpose workspace
Z63         EQU    $63             ;General purpose workspace
Z64         EQU    $64             ;General purpose workspace
Z65         EQU    $65             ;General purpose workspace
Z66         EQU    $66             ;General purpose workspace
Z67         EQU    $67             ;General purpose workspace
Z7A         EQU    $7A             ;General purpose workspace
Z7B         EQU    $7B             ;General purpose workspace
Z7D         EQU    $7D             ;Pointer for text move operations (see below)
Z7E         EQU    $7E             ;These 6 locations ($7E-$83) form three
Z80         EQU    $80             ; 16-bit pointers used to move text within
Z82         EQU    $82             ; memory efficiently (source, dest, count)