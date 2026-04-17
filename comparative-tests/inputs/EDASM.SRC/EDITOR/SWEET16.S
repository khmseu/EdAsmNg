Name          : SWEET16.S
End of file   :   16,627
This file was generated using the DiskBrowser utility with minimal editing.
It is meant for viewing purposes only.
;---------------------------------------------------------;
; EDITOR/SWEET16.S - Sweet16 Virtual Machine Interpreter
;
; Sweet16 is a 16-bit virtual machine interpreter originally
; written by Steve Wozniak for the Apple II. It provides an
; elegant way to perform 16-bit operations on the 8-bit 6502
; processor with compact code.
;
; DISASSEMBLY CREDITS:
;  Initial source was disassembled with The Flaming Bird
;  Disassembler. The Sweet16 code was disassembled with
;  the Merlin8 disassembler. Final re-assembly must be
;  done with the EdAsm assembler to produce a binary
;  image which will be compared against the original.
;    (c) Phoenix corp. 1992,93  -  All rights reserved
;
; IMPLEMENTATION NOTES:
; - Sweet16 code executes as if on a 16-bit processor
; - Has sixteen 16-bit little-endian registers (R0-R15)
; - Registers stored in zero page ($00-$1F, 32 bytes total)
; - Sweet16 programs are typically much more compact than
;   equivalent 6502 code, at the cost of slower execution
;
; REGISTER ASSIGNMENTS:
;   R0  - Accumulator (general 16-bit arithmetic)
;   R1-R11 - General purpose registers
;   R12 - Subroutine return address stack pointer
;   R13 - Comparison result register (for branch testing)
;   R14 - Status register (carry in LSB of R14H)
;   R15 - Program counter (Sweet16 instruction pointer)
;
; STATUS FLAGS:
;   Carry: Stored in bit 0 of R14+1 (R14H)
;   Negative: Determined by MSB of "prior result" register
;   Zero: Determined when "prior result" register is $0000
;
; ENTRY POINT: NEWSW16 ($D000)
; Called with JSR NEWSW16 from 6502 code. The return address
; on the stack becomes the Sweet16 program counter. Sweet16
; code follows immediately after the JSR instruction.
;
; EXIT: Use RTN instruction to return to 6502 code.
;
; X LABEL REFERENCES:
; External labels (Xabcd) in this file refer to local labels
; (Labcd) in the EDITOR module or EDASM.SYSTEM (EI) module.
;---------------------------------------------------------;
            ORG    $D000
;=================================================
; Sweet16 Interpreter Main Entry Point
;
; This code sets up the Sweet16 virtual machine state and
; begins executing Sweet16 instructions. It preserves the
; 6502 registers and sets up R15 as the Sweet16 PC.
;
NEWSW16     EQU    *               ;Sweet16 entry point (called with JSR)
            STA    Z58             ;Preserve 6502 A register
            STX    Z59             ;Preserve 6502 X register
            STY    Z5A             ;Preserve 6502 Y register
            PHP                    ;Push processor status
            PLA                    ;Pull it to A
            STA    Z5B             ;Save 6502 flags
            CLD                    ;Ensure decimal mode is off (Sweet16 uses binary)
            PLA                    ;Pull return address low byte
            STA    Reg15           ;Store in R15 (Sweet16 PC low)
            PLA                    ;Pull return address high byte
            STA    Reg15+1         ;Store in R15 (Sweet16 PC high)
                                    ; Now R15 points to Sweet16 code after JSR
;
; Instruction Fetch and Decode Loop
;
LD011       INC    Reg15           ;Increment SW16 PC to next instruction byte
            BNE    LD017           ;If no page crossing, continue
            INC    Reg15+1         ;Handle page crossing
;
LD017       LDY    #0              ;Y=0 for indirect indexed addressing
SW16BK      EQU    *-1             ;Breakpoint location (can patch for debugging)
            LDA    (Reg15),Y       ;Fetch the instruction byte
            AND    #$0F            ;Mask out register number (low nibble)
            ASL                    ;Double for 2-byte register addressing
            TAX                    ;X now indexes into zero page register (R0-R15)
            LSR                    ;Restore register number
            EOR    (Reg15),Y       ;XOR with full opcode byte to isolate operation
                                    ; This gives us the opcode in high nibble
            BEQ    ToBR            ;If zero, it's a non-register op or CPIM
;
; Register Operation Dispatch
; We have a register operation (SET, LD, ST, ADD, SUB, etc.)
;
            STX    Reg14+1         ;Save "prior result" register number in R14H
                                    ; Used for N and Z flag determination
            LSR                    ;Shift opcode right to get index
            LSR                    ; into dispatch table
            LSR                    ; (opcode >> 2) gives us table offset
            TAY                    ;Y = dispatch table index
            LDA    LD07F-2,Y       ;Get high byte of handler address
            STA    Z5C+1           ;Store in jump vector
            LDA    LD05F-2,Y       ;Get low byte of handler address
            STA    Z5C             ;Store in jump vector
            JMP    (Z5C)           ;Jump to operation handler
;
; Non-Register Operation Dispatch
; These operations don't operate on specific registers:
; - CPIM (Compare Immediate)
; - BRL (Branch Long)
; - BSL (Branch Subroutine Long)
; - Branch instructions (BNC, BC, BP, BM, BZ, BNZ, etc.)
;
; On entry: Y=0, X=register*2 (but not used for non-reg ops)
;
ToBR        INC    Reg15           ;Increment SW16 PC for next byte
            BNE    ToBR2           ;If no page crossing, continue
            INC    Reg15+1         ;Handle page crossing
ToBR2       LDA    LD080,X         ;Get high byte of non-reg handler
            STA    Z5C+1           ;Store in jump vector
            LDA    LD060,X         ;Get low byte of non-reg handler
            STA    Z5C             ;Store in jump vector
            LDA    Reg14+1         ;"Prior" result register
            LSR                    ;Prepare 6502 C-bit for BC, BNC
            LDA    Reg14+1
; AND #%0001_1110
            AND    #%00011110
            TAX                    ;(X)=Index to the "prior" result register
            JMP    (Z5C)           ;Go non-reg op rtn
;
; Control is returned to the 6502 and program execution
; continues at the location immediately following the RTN
; instruction. The 6502 registers and status conditions are
; restored to their original contents (prior to entering
; SWEET 16 mode).
;
SW16RTN     LDA    Z5B             ;Restore the contents of the 6502 regs
            PHA
            LDA    Z58
            LDX    Z59
            LDY    Z5A
            PLP
LD05C       JMP    (Reg15)         ;Return to 6502 code via SW16 PC
;
LD05F       DB     >SW16SET        ;1X
LD060       DB     >SW16RTN        ;0
            DB     >SW16LD         ;2X
            DB     >SW16BR         ;1
            DB     >SW16ST         ;3X
            DB     >SW16BNC        ;2
            DB     >SW16LDat       ;4X
            DB     >SW16BC         ;3
            DB     >SW16STat       ;5X
            DB     >SW16BP         ;4
            DB     >SW16LDDat      ;6X
            DB     >SW16BM         ;5
            DB     >SW16STDat      ;7X
            DB     >SW16BZ         ;6
            DB     >SW16POP        ;8X
            DB     >SW16BNZ        ;7
            DB     >SW16STPat      ;9X
            DB     >SW16BM1        ;8
            DB     >SW16ADD        ;AX
            DB     >SW16BNM1       ;9
            DB     >SW16SUB        ;BX
            DB     >SW16BK         ;A
            DB     >SW16POPD       ;CX
            DB     >SW16RS         ;B
            DB     >SW16CPR        ;DX
            DB     >SW16BS         ;C
            DB     >SW16INR        ;EX
            DB     >SW16CPIM       ;D
            DB     >SW16DCR        ;FX
            DB     >SW16BRL        ;E
            DB     >LD011          ;Unused
            DB     >SW16BSL        ;F
;
LD07F       DB     <SW16SET        ;1X
LD080       DB     <SW16RTN        ;0
            DB     <SW16LD         ;2X
            DB     <SW16BR         ;1
            DB     <SW16ST         ;3X
            DB     <SW16BNC        ;2
            DB     <SW16LDat       ;4X
            DB     <SW16BC         ;3
            DB     <SW16STat       ;5X
            DB     <SW16BP         ;4
            DB     <SW16LDDat      ;6X
            DB     <SW16BM         ;5
            DB     <SW16STDat      ;7X
            DB     <SW16BZ         ;6
            DB     <SW16POP        ;8X
            DB     <SW16BNZ        ;7
            DB     <SW16STPat      ;9X
            DB     <SW16BM1        ;8
            DB     <SW16ADD        ;AX
            DB     <SW16BNM1       ;9
            DB     <SW16SUB        ;BX
            DB     <SW16BK         ;A
            DB     <SW16POPD       ;CX
            DB     <SW16RS         ;B
            DB     <SW16CPR        ;DX
            DB     <SW16BS         ;C
            DB     <SW16INR        ;EX
            DB     <SW16CPIM       ;D
            DB     <SW16DCR        ;FX
            DB     <SW16BRL        ;E
            DB     <LD011          ;Unused
            DB     <SW16BSL        ;F
;
; The SW16 accumulator is loaded from Rn and branch
; conditions set according to the data transferred. The
; Carry is cleared and the contents of Rn are not changed.
; Input
;  Rn
; Output
;  R0 - both bytes
;
SW16LD      LDA    Reg0,X          ;Move Rx to R0 (Acc)
            STA    Reg0
            LDA    Reg0+1,X
            STA    Reg0+1
            JMP    LD011
;
; The SW16 ACC is stored into Rn and branch
; conditions set according to the data transferred.
; The carry is cleared and the contents of the ACC
; are unchanged
; Input
;  R0
; Output
;  Rn - both bytes
;
SW16ST      LDA    Reg0            ;Move R0 (Acc) to Rx
            STA    Reg0,X
            LDA    Reg0+1
            STA    Reg0+1,X
            JMP    LD011
;
; The low byte of ACC is stored into the memory location
; whose ADDRESS resides in Rn. Branch conditions reflect
; the 2-byte ACC contents. After the transfer, Rn is
; incremented by 1.
; Input
;  Rn - ptr to mem location
;  R0 - lobyte
; Output
;  Rn = Rn+1
;  R14H = 0 - carry cleared
;
SW16STat    LDA    Reg0
            STA    (Reg0,X)        ;Store byte indirect
            LDY    #0
            STY    Reg14+1         ;Indicate R0 is result -ve
;
; The contents of Rn are incremented by 1. The carry is
; cleared and other branch conditions reflect the
; incremented value.
;
SW16INR     INC    Reg0,X          ;Increment Rx
            BNE    LD0C3
            INC    Reg0+1,X
LD0C3       JMP    LD011
;
; The lobyte of the SW16 ACC is loaded from the memory
; location whose ADDRESS resides in Rn and ACC hi-byte
; is zeroed. Branch conditions reflect the final contents
; of the ACC which will always be positive and never
; minus 1. The Carry is cleared and after the transfer,
; Rn is  incremented by 1.
; Input
;  Rn - ptr to mem location
; Output
;  R0 - lobyte
;  Rn = Rn+1
;  R14H = 0 - carry cleared
;
SW16LDat    LDA    (Reg0,X)
            STA    Reg0
            LDY    #0              ;Zero the ACC's hi-byte
            STY    Reg0+1
            STY    Reg14+1         ;Indicate R0 is +ve
            INC    Reg0,X
            BNE    LD0D6
            INC    Reg0+1,X
LD0D6       JMP    LD011
;
; Rn is decremented by 1 and the hibyte of ACC is loaded
; from the memory location whose address now resides in Rn.
; Rn is again decremented by 1 and the lobyte of ACC is
; loaded from the corresponding memory location. Branch
; conditions reflect the final ACC contents. The carry
; is cleared. Because Rn is decremented prior to loading
; each of the ACC halves, double-byte stacks may be
; implemented with the STD Rn and POPD Rn ops where Rn
; is the stack pointer.
;
; Input
;  Rn - ptr to mem location
; Output
;  R0 - both bytes
;  Rn = Rn+2
;  R14H = 0 - carry cleared
;
SW16POPD    LDA    Reg0,X
            BNE    LD0DF
            DEC    Reg0+1,X
LD0DF       DEC    Reg0,X
            LDA    (Reg0,X)
            TAY
            LDA    Reg0,X
            BNE    LD0EA
            DEC    Reg0+1,X
LD0EA       DEC    Reg0,X
            LDA    (Reg0,X)
            STA    Reg0
            STY    Reg0+1
            LDY    #0
            STY    Reg14+1
            JMP    LD011
;
; The lobyte of the ACC is loaded from the memory location
; whose address resides in Rn AFTER Rn is decremented by 1,
; and the hibyte is cleared. Branch conditions reflect
; the final 2-byte ACC contents which will always be
; positive and never minus one. The carry is cleared.
; Because Rn is decremented prior to loading the ACC,
; single byte stacks may be implemented with the
; STI Rn and POP Rn ops where Rn is the stack pointer.
; Input
;  Rn - ptr to mem location
; Output
;  R0L - changed
;  R0H = 0
;  Rn = Rn+1
;  R14H = 0 - carry cleared
;
SW16POP     LDY    #0
            LDA    Reg0,X
            JMP    LD106
;=================================================
LD100       EQU    *               ;ENTRY?
            LDA    ROMIN2
            JMP    $E000           ;AppleSoft
;=================================================
LD106       BNE    LD10A
            DEC    Reg0+1,X
LD10A       DEC    Reg0,X
            LDA    (Reg0,X)
            STA    Reg0
            STY    Reg0+1          ;zero hi-byte
            LDY    #0
            STY    Reg14+1
            JMP    LD011
;
; The lobyte of the ACC is loaded from memory location
; whose address resides in Rn, and Rn is then incremented
; by 1. Its hibyte is loaded from the memory location whose
; address resides in the incremented Rn, and Rn is again
; incremented by 1. Branch conditions reflect the contents
; of final ACC. The carry is cleared.
; Input
;  Rn - ptr to mem location
; Output
;  R0 - both bytes changed
;  Rn = Rn+2
;  R14H = 0 - carry cleared
;
SW16LDDat   LDA    (Reg0,X)
            STA    Reg0
            INC    Reg0,X
            BNE    LD123
            INC    Reg0+1,X
LD123       LDA    (Reg0,X)
            STA    Reg0+1
            LDY    #0
            STY    Reg14+1
            INC    Reg0,X
            BNE    LD131
            INC    Reg0+1,X
LD131       JMP    LD011
;
; The lobyte of ACC is stored into memory location
; whose address resides in Rn, and Rn is the incremented
; by 1. The hibyte of ACC is stored into the memory
; location whose address resides in the incremented
; Rn, and Rn is again incremented by 1. Branch conditions
; reflect the contents of ACC which are unchanged.
; The carry is cleared.
; Input
;  Rn - ptr to mem location
;  R0 - both bytes used
; Output
;  Rn = Rn+2
;  R14H = 0 - carry cleared
;
SW16STDat   LDA    Reg0
            STA    (Reg0,X)
            LDY    #0
            STY    Reg14+1
            INC    Reg0,X
            BNE    LD142
            INC    Reg0+1,X
LD142       LDA    Reg0+1
            STA    (Reg0,X)
            INC    Reg0,X
            BNE    LD14C
            INC    Reg0+1,X
LD14C       JMP    LD011
;
; The lobyte of ACC is stored into the memory location
; whose address resides in Rn after Rn is decremented by 1.
; Branch conditions will reflect the 2-byte ACC contents
; which are not modified. STP Rn and POP Rn are used
; together to move data blocks beginning at the greatest
; address and working down. Additionally, single-byte
; stacks may be implemented with the STP Rn ops.
; Input
;  R0 - lobyte used
;  Rn - ptr to mem location
; Output
;  Rn = Rn+1
;  R14H = 0 - carry cleared
;
SW16STPat   LDA    Reg0,X
            BNE    LD155
            DEC    Reg0+1,X
LD155       DEC    Reg0,X
            LDA    Reg0
            STA    (Reg0,X)
            LDY    #0
            STY    Reg14+1
            JMP    LD011
;
; The contents of Rn are decremented by 1. The carry is
; cleared and other branch conditions reflect the
; decremented value.
; Input
;  Rn
; Output
;  Rn = Rn-1
;  R14H = set on entry into SW16
;
SW16DCR     LDA    Reg0,X
            BNE    LD168
            DEC    Reg0+1,X
LD168       DEC    Reg0,X
            JMP    LD011
;
; The low order 16 bits of the subtraction are restored in
; the ACC, the 17-th bit becomes the carry and other branch
; conditions reflect the final ACC contents. If the 16-bit
; unsigned ACC contents are greater than or equal to the
; 16-bit unsigned Rn contents, then the carry is set,
; otherwise it is cleared. Rn is not disturbed.
; For SUB instruction, register specification would be
; changed to indicate R0 (the SW16 ACC)
;
; Output
;  R0
;  R14H = lsb indicate carry condition (0/1)
;
SW16SUB     LDY    #0
;
; The ACC (R0) contents are compared to Rn by performing
; the 16 bit binary subtraction ACC-Rn and storing the low
; order 16 difference bits in R13 for subsequent branch
; tests. If the 16 bit unsigned ACC contents are greater
; than or equal to the 16 bit unsigned Rn contents, then
; the carry is set, otherwise it is cleared. No other
; registers, including ACC and Rn, are disturbed.
; Input
;  Rn - both bytes used; unchanged
;  R0 - both bytes used; unchanged
; Output
;  R13 = 0 if ACC == Rn
;  R14H = lsb indicate carry condition (26/27)
;
SW16CPR     SEC                    ;(Y)=13;2 for CPR instr
            LDA    Reg0
            SBC    Reg0,X
; STA |Reg0,Y
            DB     $99,$00,$00
            LDA    Reg0+1
            SBC    Reg0+1,X
; STA  |Reg0+1,Y
            DB     $99,$01,$00
            TYA
            ADC    #0              ;Propagate the carry as well as indicate
            STA    Reg14+1         ; the "prior" result reg is R0/R13
            JMP    LD011
;
; The contents of Rn are added to the contents of ACC,
; and the low-order 16 bits of the sum restored in ACC.
; The 17-th bit becomes the carry and the other branch
; conditions reflect the final ACC contents.
; For ADD instruction, register specification would be
; changed to indicate R0 (the SW16 ACC)
; Input
;
;  Rn - both bytes used
;  R0 - both bytes used
; Output
;  R0 - changed
;  R14H = lsb indicate carry condition (0/1)
;
SW16ADD     LDA    Reg0
            ADC    Reg0,X
            STA    Reg0
            LDA    Reg0+1
            ADC    Reg0+1,X
            LDY    #0
; STA |Reg0+1,Y
            DB     $99,$01,$00
            TYA                    ;(A)=0
            ADC    #0              ;Propagate the C-bit as well as
            STA    Reg14+1         ; indicate the "prior" result reg is ACC
            JMP    LD011
;
; A branch to the effective address (PC + 2 + d) is taken
; and execution is resumed in SWEET 16 mode. The current
; PC is pushed onto a SWEET 16 subroutine return address
; stack whose pointer is R12, and R12 is incremented by 2.
; The carry is cleared and branch conditions set to
; indicate the current ACC contents.
; Input
;  R15
; Output
;  R12 = R12+2
;  (R12) = Mem location pointed to
;
SW16BS      LDX    #Reg12          ;12;2 (SW16 stack register)
            LDA    Reg15           ;PC
            STA    (Reg0,X)
            INC    Reg12
            BNE    LD1A9
            INC    Reg12+1
LD1A9       LDA    Reg15+1
            STA    (Reg0,X)
            INC    Reg12
            BNE    SW16BR
            INC    Reg12
;
; An effective address (ea) is calculated by adding the
; signed displacement byte (d) to the PC. The PC contains
; the address of the instruction immediately following the
; BR, or the address of the BR op plus 2. The displacement
; is a signed two's complement value from -128 to +127.
; Branch conditions are not changed.
;
; NOTE: The effective address calculation is identical to
; that for 6502 relative branches. The Hex add & Subtract
; features of the APPLE ][ monitor may be used to
; calculate displacements.
;
SW16BR      CLC
            LDA    (Reg15),Y       ;Get displacement byte
            BPL    LD1B9
            DEY                    ;=$FF (-1)
LD1B9       ADC    Reg15           ;Add to SW16 PC
            STA    Reg15
            TYA
            ADC    Reg15+1
            STA    Reg15+1
            JMP    LD011
;
; A branch is effected only if the carry is set. Branch
; conditions are not changed.
;
SW16BC      BCS    SW16BR
            JMP    LD011
;
; A branch to the effective address is taken only is the
; carry is clear, otherwise execution resumes as normal
; with the next instruction. Branch conditions are
; not changed.
;
SW16BNC     BCC    SW16BR
            JMP    LD011
;
; A branch is effected only if the prior 'result' (or most
; recently transferred data) was positive. Branch
; conditions are not changed
;
SW16BP      LDA    Reg0+1,X        ;Need to chk hi-byte only
            BPL    SW16BR
            JMP    LD011
;
; A branch is effected only if prior 'result' was minus.
; Branch conditions are not changed.
;
SW16BM      LDA    Reg0+1,X        ;Need to chk hi-byte only
            BMI    SW16BR
            JMP    LD011
;
; A Branch is effected only if the prior 'result' was zero.
; Branch conditions are not changed.
;
SW16BZ      LDA    Reg0,X
            ORA    Reg0+1,X
            BEQ    SW16BR
            JMP    LD011
;
; A branch is effected only if the prior 'result' was
; non-zero. Branch conditions are not changed.
;
SW16BNZ     LDA    Reg0,X
            ORA    Reg0+1,X
            BNE    SW16BR
            JMP    LD011
;
; A branch is effected only if the prior 'result' was -1
; ($FFFF Hex). Branch conditions are not changed.
;
SW16BM1     LDA    Reg0,X          ;Get 'prior result' register
            AND    Reg0+1,X
            EOR    #-1
            BEQ    SW16BR
            JMP    LD011
;
; A branch effected only if the prior 'result' was NOT
; -1 ($FFFF). Branch conditions are not changed.
;
SW16BNM1    LDA    Reg0,X
            AND    Reg0+1,X
            EOR    #-1
            BNE    SW16BR
            JMP    LD011
;
; RS terminates execution of a SWEET16 subroutine and
; returns to the SWEET 16 calling program which resumes
; execution (in SWEET 16 mode). R12, which is the SWEET16
; subroutine return stack pointer, is decremented twice.
; Branch conditions are not changed.
;
SW16RS      LDX    #Reg12          ;12;2 for R12 as stack pointer
            LDA    Reg12
            BNE    LD20D
            DEC    Reg12+1
LD20D       DEC    Reg12
            LDA    (Reg0,X)        ;Get byte from SW16 stack
            STA    Reg15+1         ;Modify the PC
            LDA    Reg12
            BNE    LD219
            DEC    Reg12+1
LD219       DEC    Reg12
            LDA    (Reg0,X)
            STA    Reg15
            JMP    LD011
;
; $D222 The 2-byte constant is loaded into Rn (n=0-F) and
; branch conditions set accordingly. Carry is cleared.
; Output
;  Rn
;
SW16SET     LDA    (Reg15),Y       ;Get hi-byte of constant
            STA    Reg0+1,X
            DEY
            LDA    (Reg15),Y       ;Lo-byte of constant
            STA    Reg0,X
            TYA                    ;Y=1
            SEC                    ;Add 2 to SW16 PC
            ADC    Reg15
            STA    Reg15
            BCC    LD235
            INC    Reg15+1
LD235       JMP    LD011
;
; Usage: CPIM constant
; The contents of the ACC is compared against the
; 2-byte constant by performing the 16-bit binary
; subtraction ACC-CONSTANT and storing the low order 16
; difference bits in R13 for subsequent branch tests.
; If the 16-bit unsigned ACC contents are greater
; than or equal to the 16-bit unsigned CONSTANT,
; then the carry is set, otherwise it is cleared.
; No other registers, are disturbed.
;
SW16CPIM    SEC                    ;(Y)=0
            LDX    #Reg13          ;13;2
            LDA    Reg0
            SBC    (Reg15),Y
            STA    Reg0,X
            INC    Reg15
            BNE    LD247
            INC    Reg15+1
LD247       LDA    Reg0+1
            SBC    (Reg15),Y
            STA    Reg0+1,X
            TXA                    ;=26
            ADC    #0
            STA    Reg14+1         ;=27 if there's a C
            JMP    LD011
;
; A branch to the effective address (PC + 3 + d) is taken
; and execution is resumed in SWEET 16 mode. The current PC
; is pushed  onto a SWEET 16 subroutine return address
; stack whose pointer is R12, and R12 is incremented by 2.
; The carry is cleared and branch conditions set to
; indicate the current ACC contents.
;
SW16BSL     LDX    #Reg12          ;12;2 (SW16 stack register)
            LDA    Reg15           ;PC
            STA    (Reg0,X)
            INC    Reg12
            BNE    LD261
            INC    Reg12+1
LD261       LDA    Reg15+1
            STA    (Reg0,X)
            INC    Reg12
            BNE    SW16BRL
            INC    Reg12+1
;
; An effective address (ea) is calculated by adding the
; signed displacement word (d) to the PC. The PC contains
; the address of the instruction immediately following the
; BRL, or the address of the BRL op plus 3. The
; displacement is a signed two's complement value from
; -32768 ($8000) to +32767 ($7FFF).
; Branch conditions are not changed.
;
SW16BRL     CLC
            LDA    (Reg15),Y
            INC    Reg15
            BNE    LD274
            INC    Reg15+1
LD274       ADC    Reg15
            TAX
            LDA    (Reg15),Y
            ADC    Reg15+1
            STA    Reg15+1
            STX    Reg15
            JMP    LD011
;=================================================