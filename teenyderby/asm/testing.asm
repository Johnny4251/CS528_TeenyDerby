.const RAND      0x8010
.const DLY_AMT   25

.const THROTTLE  0x9000

.const DIR_0     0x9010    ; 0°
.const DIR_45    0x9011    ; 45°
.const DIR_90    0x9012    ; 90°
.const DIR_135   0x9013    ; 135°
.const DIR_180   0x9014    ; 180°
.const DIR_225   0x9015    ; 225°
.const DIR_270   0x9016    ; 270°
.const DIR_315   0x9017    ; 315°

.const FULL_THROTTLE 100

!move
    ;cut throttle

    lod rA, [RAND]
    mod rA, 35   
;    set rA, 25
    str [THROTTLE], rA

    lod rA, [RAND]
    mod rA, 8

    cmp rA, 0
    je !dir0

    cmp rA, 1
    je !dir1

    cmp rA, 2
    je !dir2

    cmp rA, 3
    je !dir3

    cmp rA, 4
    je !dir4

    cmp rA, 5
    je !dir5

    cmp rA, 6
    je !dir6

    ; else: 7
    jmp !dir7


!dir0
    str [DIR_0], rZ
    jmp !apply

!dir1
    str [DIR_45], rZ
    jmp !apply

!dir2
    str [DIR_90], rZ
    jmp !apply

!dir3
    str [DIR_135], rZ
    jmp !apply

!dir4
    str [DIR_180], rZ
    jmp !apply

!dir5
    str [DIR_225], rZ
    jmp !apply

!dir6
    str [DIR_270], rZ
    jmp !apply

!dir7
    str [DIR_315], rZ
    ; fall through

!apply
    set rB, FULL_THROTTLE
    str [THROTTLE], rB

    DLY DLY_AMT
    jmp !move
