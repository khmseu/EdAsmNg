Name          : EQUATES.S
End of file   :      391
This file was generated using the DiskBrowser utility with minimal editing.
It is meant for viewing purposes only.

;=================================================
; EI/EQUATES.S - Zero Page Definitions for EdAsm Interpreter
;
; The EdAsm Interpreter (EI) module is the system core that
; provides the command-line interface and coordinates between
; the Editor, Assembler, and Linker modules.
;
; Since the EI uses Sweet16 extensively, the first 32 bytes of
; zero page ($00-$1F) are reserved for Sweet16 registers (16
; registers x 2 bytes each). This area must be treated with care.
;
; SWEET16 REGISTER USAGE IN ZERO PAGE:
; $00-$01 : R0  (Accumulator)
; $02-$03 : R1
; $04-$05 : R2
; $06-$07 : R3
; ... (etc) ...
; $1E-$1F : R15 (Program Counter)
;=================================================
;
; Zero Page Memory Usage by EdAsm Interpreter
;
Z00         EQU    $00             ;Reg0 / R0 - Sweet16 Accumulator
Z04         EQU    $04             ;Reg2 / R2 - Sweet16 general register
Z12         EQU    $12             ;Reg9 / R9 - Pointer to message / saved character
ErrCode     EQU    $72             ;ProDOS error code (from last MLI call)
BufIndex    EQU    $76             ;Index into input buffer (InBuf at $0200)
MulCmdF     EQU    $77             ;Multiple commands flag: MSB on = multiple cmds
HelpF       EQU    $7C             ;Help menu flag: $80 = display help menu