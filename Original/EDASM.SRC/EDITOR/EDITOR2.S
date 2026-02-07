Name          : EDITOR2.S
End of file   :    7,116
This file was generated using the DiskBrowser utility with minimal editing.
It is meant for viewing purposes only.

;=================================================
; EDITOR/EDITOR2.S - Editor Module Part 2: System Commands
;
; This is the second part of the EDASM.ED editor module.
; It contains system-level commands and module loading routines.
;
; COMMANDS IMPLEMENTED IN THIS FILE:
; - EXIT: Exit to another .SYSTEM program (default: BASIC.SYSTEM)
; - SWAP: Swap between dual text buffers
; - Various initialization and system interface routines
;
; MODULE LOADING:
; This file handles loading and transferring control to other
; .SYSTEM programs. When exiting, it:
; 1. Sets up the load address ($2000 for .SYSTEM files)
; 2. Opens and reads the target file
; 3. Initializes video card and I/O hooks
; 4. Transfers control to the new program
;
; EXTERNAL REFERENCES (X-labels):
; XB937 → LB937: Open and read EDASM module
; XBA8A: Read parameter block (data buffer address)
; XBA8C: Read parameter block (length)
; XBD80: Pathname buffer
; XBB1A: Default BASIC.SYSTEM pathname
; X2000: Standard .SYSTEM load address
;
; SWEET16 USAGE:
; This file uses Sweet16 instructions (SET, LDI, BNZ, RTN)
; for efficient 16-bit pointer operations.
;=================================================
;
; EXIT Command Handler
; Exits EdAsm and loads another .SYSTEM program.
; Defaults to BASIC.SYSTEM if no pathname specified.
;
; The command interpreter's OpenNRd routine expects
; a pointer to the pathname to be stored in R4.
;
LDCC8       SET    R4,XBD80        ;Point R4 to pathname buffer at $BD80
            SET    R10,XBD80       ;Also set R10 to pathname buffer
            LDI    R10             ;Load indirect: get first byte (length)
            BNZ    LDCD4           ;If non-zero, user typed a pathname
            SET    R4,XBB1A        ;No pathname: default to BI's (BASIC.SYSTEM) name
LDCD4       RTN                    ;Return to Sweet16 caller
;
; Setup Read Parameters and Load .SYSTEM File
; Configures ProDOS read parameter block and loads file at $2000
;
            LDA    #>X2000         ;High byte of load address
            STA    XBA8A           ;Store in read parameter block (data buffer)
            LDA    #<X2000         ;Low byte of load address
            STA    XBA8A+1         ;Store in read parameter block
            LDA    #$00            ;Read length low byte (0)
            STA    XBA8C           ; (will read up to buffer size)
            LDA    #$98            ;Read length high byte ($9800 = 38KB max)
            STA    XBA8C+1
            LDA    #SYStype        ;File type = SYS ($FF)
            JSR    XB937           ;Open and read file into memory (→ LB937)
            BCS    LDD09           ;If carry set, error occurred
            JSR    LA7AB           ;Initialize video card, setup I/O hooks
            LDA    #$00
            STA    BITMAP+$13
            STA    BITMAP+$15
            STA    BITMAP+$14
            STA    BITMAP+$16
            LDA    #$01
            STA    BITMAP+$17
            JMP    XB92E           ;Go execute and don't come back
;
LDD09       PHA                    ;Errcode
            JSR    LA788           ;Reset I/O hooks
            PLA
            JMP    XB602           ;Report err
;=================================================
; Checks keyboard for a ctrl-S, ctrl-C and spacebar
; keypress
; Z=1,C=0 -> ctrl-S
; Z=1,C=1 -> ctrl-C
; Z=0,C=0 -> any other key
;
KeyPress    LDA    KBD
            BPL    LDD2C
            CMP    #CTRLS+$80
            BEQ    LDD2C           ;Z=1
            BIT    KBDSTROBE
            CMP    #CTRLC+$80
            BNE    LDD23
            SEC
            RTS
;
LDD23       CMP    #SPACE+$80      ;Single-step?
            BNE    LDD2C
LDD27       LDA    KBD             ;Wait for another
            BPL    LDD27           ; key press
LDD2C       CLC
            RTS
;=================================================
; Chk for a ctrl-C
;
LDD2E       JSR    KeyPress
            BCS    LDD34           ;ctrl-C
            RTS
LDD34       JMP    LDCA9
;=================================================
LDD37       JSR    LA614           ;Get char fr user's input key rtn
            CMP    #$60
            BCC    LDD40
            AND    #$5F            ;To ucase
LDD40       RTS
;=================================================
; Init tab table
;
LDD41       EQU    *               ;ENTRY
            LDA    #$00
            STA    SwapMode
            STA    XBE00           ;Curr PN
            LDY    #10
LDD4A       LDA    DefTabsT,Y      ;Only 3 are set
            CPY    #3
            BCC    LDD53
            LDA    #0              ;Zero the rest
LDD53       STA    UserTabT,Y      
            DEY
            BPL    LDD4A
;
            LDA    #SPACE
            STA    TabChar
            RTS
;=================================================
; PR# cmd
; Parsed parms are in $BD00 buf
;
LDD5E       RTN
            LDY    #1
            LDX    XBD00           ;Get Slot #
            STX    Z59
            BEQ    LDD8C           ;slot 0
            CPX    #8
            BCS    LDD99           ;slot > 7
;
            LDA    #%00000010
LDD6E       DEX
            BEQ    LDD74
            ASL
            BNE    LDD6E
LDD74       BIT    SLTBYT
            BEQ    LDD99           ;Invalid
;
            LDA    XBD00,Y
            STA    DevCtlS,Y
LDD7F       INY
            CPY    #33
            BEQ    LDD8C
            LDA    XBD00,Y
            STA    DevCtlS,Y
            BNE    LDD7F
;
LDD8C       LDA    #0              ;Set delimiter
            STA    DevCtlS,Y       ; to mark EOS
            LDX    Z59
            STX    DevCtlS
            JMP    XB1CB
LDD99       JMP    LD53E
;=================================================
; Tabs cmd
; Input
;  Reg12-ptr to $BD00 buf
;
LDD9C       SET    R9,UserTabT
            LD     R9
            ST     R2
            SET    R8,10           ;Up to 10 tab cols
            LD     R8
            ST     R10
            SUB    R0              ;zero ACC
;
LDDA7       STI    R9              ;Zero the table first
            DCR    R8
            BNZ    LDDA7
;
LDDAB       LDD    R12             ;Get tab posn (stored as a 16-bit val)
            BM     LDDB3           ;If $FFFF, EO list
            STI    R2              ;Save to UserTabT
            LDI    R12
            DCR    R10
            BNZ    LDDAB
LDDB3       RTN
;
            LDA    XBD80           ;Is a tab char passed?
            BEQ    LDDBB           ;No
            STA    TabChar
LDDBB       JMP    XB1CB
;
DefTabsT    DB     $10             ;16,22,36
            DB     $16
            DB     $24
;=================================================
; TYPE cmd
; The command interpreter's ChkAttrF ($B883)
; rtn expects a ptr to pathname to be stored
; at Reg4
;
LDDC1       SET    R4,XBD80        ;Set ptr to PN
            RTN
            LDA    #TXTtype
            JSR    XB883           ;Chk file's properties
            BCC    LDDCF
LDDCC       JMP    XB602           ;Not found, show errmsg
;
LDDCF       BNE    LDDCC           ;Wrong filetype -> err
;
            LDX    #>XA900         ;1024-byte I/O buf
            LDY    #<XA900
            JSR    XB8CE           ;Open the TEXT file
            LDA    XBA87           ;Open Ref #
            STA    XBAA1           ;NL ref #
            JSR    L9F34           ;Set NewLine mode
            BNE    LDDCC
            LDA    XBA87
            STA    L9F43           ;read ref #
LDDE9       JSR    L9F3B           ;Read a line into $BD00 buf
            BNE    LDE05           ;Err
            JSR    KeyPress        ;Abort listing?
            BCS    LDE03           ;ctrl-C -> yes
;
            LDY    #0
LDDF5       LDA    XBD00,Y         ;Get a char fr buf
            JSR    XB343           ;Show it
            INY
            CPY    L9F48
            BCC    LDDF5
LDE01       BCS    LDDE9           ;always
;
LDE03       LDA    #$4C            ;EOF err
LDE05       LDX    L9F43
            STX    XBAF7           ;Close Ref #
            JSR    XB8FE           ;Close file
            CMP    #$4C
            BEQ    LDE15
            JMP    XB602           ;Report err
LDE15       JMP    XB1AB           ;Back to EI Main loop
;=================================================
; Editor's Cmd table - used by $B20F
;
            MSB    ON
LDE18       EQU    *               ;ENTRY
            DB     $00
            ASC    "ASM"
            DB     $0A             ;Index into $9D21 table
            ASC    ","             ;comma
            DB     $0C             ;->$9D21
            DB     $00             ;->$9D21 (Chk for a CR)
            DB     $00             ;Not used to index $DFA2
            ASC    "APPEND"
            DB     $10
            DB     $0A
            DB     $00
            DB     $30             ;->$DFA2
            ASC    "Add"
            DB     $10             ;->$9D21
            DB     $00             ;->$9D21
            DB     $02             ;->$DFA2 table
            ASC    "BLOAD"
            DB     $0A             ;->$9D21 table
            ASC    ","
            DB     $22             ;->$9D21 table
            DB     $00
            DB     $24             ;->$DFA2 table
            ASC    "BSAVE"
            DB     $0A
            ASC    ","
            DB     $22
            ASC    ","
            DB     $24
            DB     $00
            DB     $4E
            ASC    "CATALOG"
            DB     $12
            DB     $00
            DB     $50
            ASC    "CAT"
            DB     $12
            DB     $00
            DB     $0E
            ASC    "COLumn"
            DB     $16
            DB     $00
            DB     $44
            ASC    "COpy"
            DB     $02
            ASC    "-"
            DB     $10
            ASC    "TO"
            DB     $02
            DB     $00
            DB     $22
            ASC    "CREATE"
            DB     $0A
            DB     $00
            DB     $5A
            ASC    "Change"
            DB     $04
            DB     $08
            DB     $00
            DB     $28
            ASC    "DELETE"
            DB     $0A
            DB     $00
            DB     $08
            ASC    "Del"
            DB     $04
            DB     $00
            DB     $16
            ASC    "END"
            DB     $00
            DB     $5C
            ASC    "EXIT"
            DB     $12
            DB     $00
            DB     $4C
            ASC    "EXEC"
            DB     $0A
            DB     $00
            DB     $52
            ASC    "Edit"
            DB     $04
            DB     $06
            DB     $00
            DB     $2E
            ASC    "FILE"
            DB     $00
            DB     $34
            ASC    "Find"
            DB     $04
            DB     $06
            DB     $00
            DB     $2A
            ASC    "Insert"
            DB     $02
            DB     $00
            DB     $14
            ASC    "KILL2"
            DB     $00
            DB     $1A
            ASC    "LOaD"
            DB     $0A
            DB     $00
            DB     $10
            ASC    "LOCK"
            DB     $0A
            DB     $00
            DB     $0A
            ASC    "List"
            DB     $04             ;->$9D21
            DB     $00
            DB     $26             ;->$DFA2
            ASC    "MON"
            DB     $00
            DB     $04
            ASC    "NEW"
            DB     $00
            DB     $1C
            ASC    "Online"
            DB     $00             ;Chk for CR
            DB     $4A             ;->$DFA2
            ASC    "PR#"
            DB     $16
            ASC    ","
            DB     $0E
            DB     $00
            DB     $36
            ASC    "PreFiX"
            DB     $12
            DB     $00
            DB     $06
            ASC    "PTRON"
            DB     $00
            DB     $46
            ASC    "PTROFF"
            DB     $00
            DB     $48
            ASC    "Print"
            DB     $04
            DB     $00
            DB     $2C
            ASC    "RENAME"
            DB     $1A             ;->$9D21
            ASC    ","
            DB     $1C             ;->$9D21
            DB     $00
            DB     $20             ;->$DFA2
            ASC    "Replace"
            DB     $04
            DB     $00
            DB     $32
            ASC    "SETDelim"
            DB     $18
            DB     $00
            DB     $38
            ASC    "SETLcase"
            DB     $00
            DB     $3A
            ASC    "SETUcase"
            DB     $00
            DB     $3C
            ASC    "SWAP"
            DB     $00
            DB     $18
            ASC    "SaVE"
            DB     $04
            DB     $12
            DB     $00
            DB     $12
            ASC    "TYPE"
            DB     $0A
            DB     $00
            DB     $58
            ASC    "TRuncOFf"
            DB     $00
            DB     $3E
            ASC    "TRuncON"
            DB     $00
            DB     $40
            ASC    "Tabs"
            DB     $04
            DB     $06
            DB     $00
            DB     $1E
            ASC    "UNLOCK"
            DB     $0A
            DB     $00
            DB     $0C
            ASC    "Where"
            DB     $02
            DB     $00
            DB     $42
            ASC    "XLOAD"
            DB     $0A
            ASC    ","
            DB     $26
            DB     $00
            DB     $54
            ASC    "XSAVE"
            DB     $0A
            ASC    ","
            DB     $1E             ;->$9D21(A$)
            ASC    ","
            DB     $20             ;->$9D21 (L$)
            DB     $00
            DB     $56             ;->$DFA2
            DB     $28             ;What's this
            MSB    OFF
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; These are the entry points for the various
; SW16 routines utilised by the EI ($B659-B6A3)
; The "JMP" takes place at $B6A3 where the
; SW16 PC is set with the contents of R2 register
; R2 was previously set by EI code at $B680-$B68C
;
LDFA2       EQU    *
            DW     LD33C-1         ;0
            DW     LD410-1         ;2
            DW     LD56D-1         ;4
            DW     LA29E-1         ;6
            DW     LD2C4-1         ;8
            DW     LD287-1         ;A
            DW     LD282-1         ;C
            DW     LA077-1         ;E
            DW     LD9B1-1         ;10
            DW     LDA06-1         ;12
            DW     LD656-1         ;14
            DW     LD6BB-1         ;16
            DW     LD76B-1         ;18
            DW     LD504-1         ;1A
            DW     LD521-1         ;1C
            DW     LDD9C-1         ;1E
            DW     LD308-1         ;20
            DW     LD819-1         ;22
            DW     L9BD6-1         ;24
            DW     LD631-1         ;26
            DW     LD8C6-1         ;28
            DW     LD865-1         ;2A
            DW     LD533-1         ;2C
            DW     LDB54-1         ;2E
            DW     LD9A6-1         ;30 - Append
            DW     LD6D6-1         ;32
            DW     LD6EC-1         ;34
            DW     LDD5E-1         ;36
            DW     LD5AD-1         ;38
            DW     LD5CB-1         ;3A
            DW     LD5C2-1         ;3C
            DW     LD582-1         ;3E
            DW     LD58A-1         ;40
            DW     LD58F-1         ;42
            DW     LD547-1         ;44
            DW     LA024-1         ;46
            DW     LA049-1         ;48
            DW     LA309-1         ;4A - Online
            DW     LDCC8-1         ;4C
            DW     L9BF9-1         ;4E
            DW     LA072-1         ;50
            DW     XB9B0-1         ;52
            DW     L9B19-1         ;54
            DW     L9B6E-1         ;56
            DW     LDDC1-1         ;58
            DW     L9F4A-1         ;5A
            DW     $C8D3           ;5C - NOT USED?
;