; ---------------------------
; DEAD AGENT - Demonstration of death mechanics
; This agent intentionally drives into a wall at full speed to die
; ---------------------------

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
.const DIR_0           0x9010 
.const DIR_45          0x9011 
.const DIR_90          0x9012 
.const DIR_135         0x9013 
.const DIR_180         0x9014 
.const DIR_225         0x9015 
.const DIR_270         0x9016 
.const DIR_315         0x9017 
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
.const SELF_IS_DEAD    0x9036

.const FULL_THROTTLE   100

; Start of program
jmp !init

!init
    ; Pick a random direction to drive into a wall
    lod rA, [RAND]
    mod rA, 4
    
    ; Choose cardinal direction (0, 90, 180, or 270)
    cmp rA, 0
    je !set_dir_0
    
    cmp rA, 1
    je !set_dir_90
    
    cmp rA, 2
    je !set_dir_180
    
    ; else direction 270
    jmp !set_dir_270

!set_dir_0
    str [DIR_0], rZ
    jmp !suicide_run

!set_dir_90
    str [DIR_90], rZ
    jmp !suicide_run

!set_dir_180
    str [DIR_180], rZ
    jmp !suicide_run

!set_dir_270
    str [DIR_270], rZ
    ; fall through to suicide_run

!suicide_run
    ; Check if we're dead yet
    lod rB, [SELF_IS_DEAD]
    cmp rB, 1
    je !dead_loop
    
    ; Still alive, keep ramming the wall at full throttle
    set rC, FULL_THROTTLE
    str [THROTTLE], rC
    
    jmp !suicide_run

!dead_loop
    ; We're dead, just loop forever doing nothing
    set rC, 0
    str [THROTTLE], rC
    jmp !dead_loop
