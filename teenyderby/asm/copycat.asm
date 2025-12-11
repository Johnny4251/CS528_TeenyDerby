; ---------------------------
; TEENYAT REGISTERS
; ---------------------------
.const PORT_A_DIR   0x8000
.const PORT_B_DIR   0x8001
.const PORT_A       0x8002
.const PORT_B       0x8003
.const RAND         0x8010
.const RAND_BITS    0x8011

; ---------------------------
; WRITE-ONLY REGISTERS
; ---------------------------
.const THROTTLE        0x9000

.const DIR_BASE        0x9010
.const DIR_0                      0x9010 
.const DIR_45                     0x9011 
.const DIR_90                     0x9012 
.const DIR_135                    0x9013 
.const DIR_180                    0x9014 
.const DIR_225                    0x9015 
.const DIR_270                    0x9016 
.const DIR_315                    0x9017 
.const DIR_MAX         0x9017 

; ---------------------------
; READ/WRITE REGISTERS
; ---------------------------
.const SENSOR_TARGET   0x9020

; ---------------------------
; READ-ONLY SENSOR REGISTERS
; ---------------------------
.const SENSOR_REL_X    0x9021
.const SENSOR_REL_Y    0x9022
.const SENSOR_X        0x9023
.const SENSOR_Y        0x9024
.const SENSOR_SPEED    0x9025
.const SENSOR_DIR      0x9026
.const SENSOR_HEALTH   0x9027
.const SENSOR_IS_DEAD  0x9028

; ---------------------------
; SELF-STATE REGISTERS
; ---------------------------
.const SELF_ID         0x9030
.const SELF_SPEED      0x9031
.const SELF_DIR        0x9032
.const SELF_HEALTH     0x9033
.const SELF_X          0x9034
.const SELF_Y          0x9035

jmp !init

!init
    lod rA, [SELF_ID] 
    lod rB, [RAND]
    mod rB, 8
    cmp rB, rA
    je !init

    str [SENSOR_TARGET], rB  

!loop

    lod rB, [SENSOR_DIR]  

    cmp rB, 0
    je !dir0
    cmp rB, 1
    je !dir1
    cmp rB, 2
    je !dir2
    cmp rB, 3
    je !dir3
    cmp rB, 4
    je !dir4
    cmp rB, 5
    je !dir5
    cmp rB, 6
    je !dir6
    cmp rB, 7
    je !dir7

    jmp !dir0

!dir0
    str [DIR_0], rZ
    jmp !go

!dir1
    str [DIR_45], rZ
    jmp !go

!dir2
    str [DIR_90], rZ
    jmp !go

!dir3
    str [DIR_135], rZ
    jmp !go

!dir4
    str [DIR_180], rZ
    jmp !go

!dir5
    str [DIR_225], rZ
    jmp !go

!dir6
    str [DIR_270], rZ
    jmp !go

!dir7
    str [DIR_315], rZ

!go
    set rC, 100
    ;lod rC, [SENSOR_SPEED]
    str [THROTTLE], rC

    ;DLY DLY_AMT
    jmp !loop
