* SIMPLE TEST FILE FOR EDASM COMPARISON [WS]
* Tests basic assembly operations
;
        ORG   $0800
;
START   LDA   #$00
        STA   $C000
        LDX   #$10
LOOP    DEX
        BNE   LOOP
        RTS
;
DATA    DFB   $01,$02,$03,$04
        DFB   $05,$06,$07,$08
;
MESSAGE ASC   "HELLO WORLD"
        DFB   $00
;
; END
