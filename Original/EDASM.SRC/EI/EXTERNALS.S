Name          : EXTERNALS.S
End of file   :      979
This file was generated using the DiskBrowser utility with minimal editing.
It is meant for viewing purposes only.

*=================================================
* EI/EXTERNALS.S - External References for EdAsm Interpreter
*
* The EdAsm Interpreter (EI) is the core system module that
* coordinates between the Editor, Assembler, and Linker modules.
* It provides the command-line interface and system services.
*
* This file references routines and tables from EDASM.ED (Editor)
* module that the EI needs to call.
*
* LABEL NAMING CONVENTION:
* X-prefixed labels (e.g., X9AA5) are external references to the
* Editor module. In the Editor's code, these correspond to local
* labels (e.g., L9AA5).
*
* X9AA5  in EI module  →  L9AA5  in Editor module
* XA70B  in EI module  →  LA70B  in Editor module
* etc.
*=================================================
*
* Editor Module Entry Points (References from EDASM.ED)
*
X9AA5       EQU    $9AA5           ;Set R12 (Sweet16) to $BD00 (→ L9AA5)
X9C98       EQU    $9C98           ;Output error message routine (→ L9C98)
X9D11       EQU    $9D11           ;Default TAB table data (→ L9D11)
X9D14       EQU    $9D14           ;Edasm.AutoStart routine (→ L9D14)
X9D21       EQU    $9D21           ;JMP table for commands (→ L9D21)
X9E55       EQU    $9E55           ;Convert date/time to ASCII string (→ L9E55)
X9F83       EQU    $9F83           ;Copyright message string (→ L9F83)
XA70B       EQU    $A70B           ;Command line input routine (→ LA70B)
XA75B       EQU    $A75B           ;Check for presence of 80-column card (→ LA75B)
XA788       EQU    $A788           ;Reset I/O hooks to defaults (→ LA788)
XA78A       EQU    $A78A           ;Alternate entry point of $A788 (→ LA78A)
DoHelp      EQU    $A7F1           ;Display help menu (→ LA7F1)
XA872       EQU    $A872           ;Load EDASM.SWAP module (→ LA872)
XA8D1       EQU    $A8D1           ;Volume/subdirectory header entry (→ LA8D1)
NEWSW16     EQU    $D000           ;Modified Sweet16 interpreter resides in LC Bank2
                                    ; at $D000 (Language Card memory)
XD610       EQU    $D610           ;Printer initialization routine (→ LD610)
XDD41       EQU    $DD41           ;Initialize TAB table (→ LDD41)
XDE18       EQU    $DE18           ;Table of commands (ASM, NEW, ONLINE, etc.) (→ LDE18)
XDFA2       EQU    $DFA2           ;Command Sweet16 jump table (→ LDFA2)
*
* Module Load Addresses
* These define where different EdAsm modules are loaded in memory
*
LoadAdrAsm  EQU    $6800           ;EDASM.ASM load address
X7800       EQU    $7800           ;EDASM.ASM sub-module load/execute address
X9900       EQU    $9900           ;EDASM.ED sub-module load/execute address