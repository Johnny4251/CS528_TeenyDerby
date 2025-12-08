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

; ---------------------------
; SELF-STATE REGISTERS
; ---------------------------
.const SELF_ID         0x9030
.const SELF_SPEED      0x9031
.const SELF_DIR        0x9032
.const SELF_HEALTH     0x9033
.const SELF_X          0x9034
.const SELF_Y          0x9035
 

.const MAX_ACCEL 75
.const DLY_AMT   20

set rC, 0       

!loop
    cmp rC, 0
    je !go_right

    cmp rC, 1
    je !go_down

    cmp rC, 2
    je !go_left

    ; else: 3
    jmp !go_up

!go_right
    str [DIR_0], rZ
    lod rA, [SELF_X]

    cmp rA, 500        
    jge !next_dir      

    set rB, MAX_ACCEL
    str [THROTTLE], rB
    ;DLY DLY_AMT
    jmp !loop


!go_down
    str [DIR_90], rZ
    lod rA, [SELF_Y]

    cmp rA, 500        
    jge !next_dir

    set rB, MAX_ACCEL
    str [THROTTLE], rB
    ;DLY DLY_AMT
    jmp !loop


!go_left
    str [DIR_180], rZ
    lod rA, [SELF_X]

    cmp rA, 150        
    jle !next_dir

    set rB, MAX_ACCEL
    str [THROTTLE], rB
    ;DLY DLY_AMT
    jmp !loop


!go_up
    str [DIR_270], rZ
    lod rA, [SELF_Y]

    cmp rA, 200        
    jle !next_dir

    set rB, MAX_ACCEL
    str [THROTTLE], rB
    ;DLY DLY_AMT
    jmp !loop


!next_dir
    inc rC
    mod rC, 4           
    set rB, 0
    str [THROTTLE], rB  
    ;DLY DLY_AMT
    jmp !loop
