Name          : EXTERNALS.S
End of file   :    3,020
This file was generated using the DiskBrowser utility with minimal editing.
It is meant for viewing purposes only.

;=================================================
; EDITOR/EXTERNALS.S - External Entry Points for EDASM.SYSTEM
;
; This file defines external labels for EDASM.SYSTEM (the EI module)
; entry points and data areas. These are the published API that
; the Editor module uses to interact with the system.
;
; IMPORTANT: Label Naming Convention
; -----------------------------------
; Labels beginning with 'X' (e.g., XB198, XB883) are EXTERNAL
; REFERENCES to code in the EDASM.SYSTEM (EI) module. These
; labels correspond to LOCAL LABELS in that module:
;
;   Editor Reference    EI Module Label
;   ----------------   -----------------
;   XB198         -->   LB198  (warm start)
;   XB1AB         -->   LB1AB  (EI main loop)
;   XB883         -->   LB883  (check file attributes)
;   etc.
;
; The 'X' prefix indicates "eXternal to this module" while
; the 'L' prefix in the actual code indicates "Local label".
; This naming convention helps track cross-module dependencies.
;
; ORGANIZATION:
; - Entry points ($B198-$B9B0): Callable routines
; - Data areas ($B40A-$BADA): Shared data and buffers
;=================================================
;
; EDASM.SYSTEM Entry Points (Callable Routines)
; These are the main functions provided by the EI module that
; the Editor can call via JSR.
;
XB198       EQU    $B198           ;Warm start - restart EdAsm without reload (→ LB198)
XB1AB       EQU    $B1AB           ;EI main loop - command interpreter (→ LB1AB)
XB1CB       EQU    $B1CB           ;Reset I/O hooks to defaults (→ LB1CB)
XB211       EQU    $B211           ;Command parser - parse user input (→ LB211)
XB274       EQU    $B274           ;Get non-blank character from input (→ LB274)
XB286       EQU    $B286           ;Command table pointer (→ LB286)
XB289       EQU    $B289           ;Do an RTS (simple return) (→ LB289)
XB28E       EQU    $B28E           ;Print command syntax error (→ LB28E)
XB290       EQU    $B290           ;Print error message (→ LB290)
XB339       EQU    $B339           ;Output carriage return (CR) (→ LB339)
XB33D       EQU    $B33D           ;Beep (sound bell/error tone) (→ LB33D)
XB343       EQU    $B343           ;Output character in A register (→ LB343)
XB36B       EQU    $B36B           ;Call Monitor's VTAB (vertical tab to row) (→ LB36B)
XB375       EQU    $B375           ;Call Monitor's BELL1 (bell/beep) (→ LB375)
XB37F       EQU    $B37F           ;Call Monitor's PRBLNK (print blanks) (→ LB37F)
XB389       EQU    $B389           ;Call Monitor's PRNTAX (print A,X as hex) (→ LB389)
XB393       EQU    $B393           ;Call Monitor's SETKBD & SETVID (reset I/O) (→ LB393)
XB3D9       EQU    $B3D9           ;Output char in A through EI's CSW hook (→ LB3D9)
XB5F8       EQU    $B5F8           ;Fatal error handler - abort program (→ LB5F8)
XB5FF       EQU    $B5FF           ;Close file and fall through to error (→ LB5FF)
XB602       EQU    $B602           ;Print error message with code (→ LB602)
XB620       EQU    $B620           ;Print "ER:" prefix and beep (→ LB620)
XB659       EQU    $B659           ;Print error message and error code (→ LB659)
XB6EA       EQU    $B6EA           ;Multi-buffer error message handler (→ LB6EA)
XB7AB       EQU    $B7AB           ;Load EDASM.ED module (Editor) (→ LB7AB)
XB836       EQU    $B836           ;Set ProDOS prefix to EDASM's boot dir (→ LB836)
XB844       EQU    $B844           ;Load EDASM.ED or EDASM.ASM module (→ LB844)
XB86F       EQU    $B86F           ;Print invalid pathname error (→ LB86F)
XB883       EQU    $B883           ;Check file's attributes (GET_FILE_INFO) (→ LB883)
XB8B2       EQU    $B8B2           ;Copy filename to two buffers (→ LB8B2)
XB8CE       EQU    $B8CE           ;Open file for reading (→ LB8CE)
XB8EF       EQU    $B8EF           ;Close file and proceed to error handler (→ LB8EF)
XB8FE       EQU    $B8FE           ;Close file and return to caller (→ LB8FE)
XB904       EQU    $B904           ;Set file info (SET_FILE_INFO) (→ LB904)
XB90B       EQU    $B90B           ;Kill/destroy file (DESTROY) (→ LB90B)
XB912       EQU    $B912           ;Rename file (RENAME) (→ LB912)
XB919       EQU    $B919           ;Create file (CREATE) (→ LB919)
XB920       EQU    $B920           ;Write to file (WRITE) (→ LB920)
XB927       EQU    $B927           ;Set end-of-file position (SET_EOF) (→ LB927)
XB92E       EQU    $B92E           ;Transfer control to another .SYSTEM program (→ LB92E)
XB937       EQU    $B937           ;Open and read an EDASM module (→ LB937)
XB966       EQU    $B966           ;Check file's size (GET_EOF) (→ LB966)
XB99B       EQU    $B99B           ;Convert A to printable hex code (→ LB99B)
XB9B0       EQU    $B9B0           ;EXEC command handler (→ LB9B0)
;=================================================
; EDASM.SYSTEM Data Areas (Shared Memory Locations)
; These memory locations contain data that is shared between
; the EI module and other modules (Editor, Assembler, Linker).
; The Editor can read/write these locations for inter-module
; communication.
;
; ERROR MESSAGE STRINGS ($B40A-$B5BE)
; Text of error messages are stored in this range.
; These are Pascal-style strings (length byte followed by text).
;
XB40A       EQU    $B40A           ;Error message text table start
XB420       EQU    $B420           ;Error message strings in range
XB427       EQU    $B427           ; $B40A through $B5BE contain
XB432       EQU    $B432           ; ProDOS error messages and
XB43E       EQU    $B43E           ; EdAsm-specific error messages
XB448       EQU    $B448           ; that can be displayed to the user
XB45B       EQU    $B45B           ; via the error handling routines
XB50D       EQU    $B50D           ; (XB602, XB620, XB659, etc.)
XB521       EQU    $B521           ;"File locked" message
;
; SWAP COMMAND DATA ($B9EE, $BA33)
; Used by the SWAP command to manage split-buffer editing mode
;
XB9EE       EQU    $B9EE           ;SWAP command data area 1
XBA33       EQU    $BA33           ;SWAP command data area 2
;
; SWEET16 AND MEMORY MANAGEMENT ($BA78-$BA80)
;
XBA78       EQU    $BA78           ;Sweet16 interpreter data area
XBA80       EQU    $BA80           ;Pointer to free memory start (2 bytes)
                                    ; Points to first available byte after program
;
; PRODOS FILE I/O PARAMETER BLOCKS
; These are ProDOS MLI parameter blocks used for file operations.
; They are set up before calling ProDOS MLI ($BF00).
;
XBA87       EQU    $BA87           ;OPEN parameter block: ref num (returned)
XBA8A       EQU    $BA8A           ;READ parameter block: ref num
XBA8C       EQU    $BA8C           ;READ parameter block: request length
XBA8E       EQU    $BA8E           ;READ parameter block: actual length (returned)
XBA91       EQU    $BA91           ;WRITE parameter block: ref num
XBA92       EQU    $BA92           ;WRITE parameter block: data buffer pointer
XBA94       EQU    $BA94           ;WRITE parameter block: write length
XBAA0       EQU    $BAA0           ;NEWLINE parameter block start
XBAA1       EQU    $BAA1           ;NEWLINE parameter block: ref num
XBAA4       EQU    $BAA4           ;CAT/CATALOG command read parameter block
XBAA5       EQU    $BAA5           ;CAT read parameter block: ref num
XBAA8       EQU    $BAA8           ;CAT read parameter block: read length
XBAAC       EQU    $BAAC           ;Directory block read buffer pointer
XBAAD       EQU    $BAAD           ;Directory read parameter block: ref num
XBAB5       EQU    $BAB5           ;SET_EOF parameter block: ref num
XBAB6       EQU    $XBAB6           ;SET_EOF parameter block: new EOF (3 bytes)
;
; FILE INFO BUFFERS ($BABC-$BAE8)
; These buffers hold file information returned by GET_FILE_INFO
; and used by SET_FILE_INFO ProDOS calls.
;
XBABC       EQU    $BABC           ;File info buffer area start
XBAC8       EQU    $BAC8           ;Pathname pointer (2 bytes)
XBACA       EQU    $BACA           ;File access attributes byte
XBACB       EQU    $BACB           ;File type byte
XBACC       EQU    $BACC           ;Auxiliary type (2 bytes)
XBACE       EQU    $BACE           ;Storage type byte
XBACF       EQU    $BACF           ;Blocks used (2 bytes)
XBAD1       EQU    $BAD1           ;Modification date (2 bytes)
XBADA       EQU    $BADA           ;SET_FILE_INFO parameter block start
XBADC       EQU    $BADC           ;SET_FILE_INFO: access byte
XBADD       EQU    $BADD           ;SET_FILE_INFO: file type
XBADE       EQU    $BADE           ;SET_FILE_INFO: aux type (2 bytes)
XBAE3       EQU    $BAE3           ;File info continuation
XBAE8       EQU    $BAE8           ;File info buffer end
XBAEA       EQU    $BAEA           ;File info continuation
XBAEB       EQU    $BAEB           ;File info continuation
XBAEC       EQU    $BAEC           ;File info continuation
XBAEE       EQU    $BAEE           ;File info continuation
XBAF4       EQU    $BAF4           ;Pathname pointer for file operations (2 bytes)
XBAF7       EQU    $BAF7           ;File info continuation
XBAFB       EQU    $BAFB           ;File info continuation
XBAFD       EQU    $BAFD           ;File info continuation
XBAFE       EQU    $BAFE           ;File info continuation
XBB1A       EQU    $BB1A           ;Extended file info area
XBB80       EQU    $BB80           ;Extended buffer area
;=================================================
; End of EDITOR/EXTERNALS.S
;
; These external references allow the Editor module to call
; functions in the EDASM.SYSTEM (EI) module and access shared
; data structures. The X-prefix convention helps maintain clear
; separation between modules while enabling inter-module
; communication through a well-defined API.
;=================================================