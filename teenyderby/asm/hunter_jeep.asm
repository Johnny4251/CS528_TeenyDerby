; hunter.asm - aggressive chaser agent
; Picks a random target != self and tries to drive toward it using REL_X/REL_Y

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
.const DIR_0                      0x9010   ; right
.const DIR_45                     0x9011   ; down-right
.const DIR_90                     0x9012   ; down
.const DIR_135                    0x9013   ; down-left
.const DIR_180                    0x9014   ; left
.const DIR_225                    0x9015   ; up-left
.const DIR_270                    0x9016   ; up
.const DIR_315                    0x9017   ; up-right
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

.const FULL_THROTTLE   100
.const EDGE_THR        20      ; threshold for "strong" axis direction
.const DLY_AMT         3

jmp !init

; ===========================
; PICK A RANDOM TARGET != SELF
; ===========================
!pick_target
    lod rA, [SELF_ID]          ; rA = self id

!pick_retry
    lod rB, [RAND]             ; rB = random
    mod rB, 8                  ; assume up to 8 agents
    cmp rB, rA
    je !pick_retry             ; don't target self

    str [SENSOR_TARGET], rB
    ret

!init
    cal !pick_target
    jmp !loop

; ===========================
; MAIN LOOP
; ===========================
!loop
    ; If target is dead, choose a new one
    lod rC, [SENSOR_HEALTH]
    cmp rC, 0
    jle !retarget
    jmp !after_retarget_check

!retarget_cond_done
    cal !pick_target

!after_retarget_check

    ; -----------------------
    ; READ RELATIVE POSITION
    ; -----------------------
    lod rA, [SENSOR_REL_X]     ; rA = rel_x (target - self)
    lod rB, [SENSOR_REL_Y]     ; rB = rel_y

    ; -----------------------
    ; DETERMINE DIRECTION
    ; We do a simple quadrant + axis bias:
    ;   if |rel_x| >> |rel_y|  → prefer left/right
    ;   if |rel_y| >> |rel_x|  → prefer up/down
    ;   otherwise              → diagonal
    ; For simplicity, we use thresholds with signed compares.
    ; -----------------------

    ; Check strong RIGHT (rel_x >= EDGE_THR)
    set rD, EDGE_THR
    cmp rA, rD
    jge !maybe_right           ; rel_x >= +EDGE_THR

    ; Check strong LEFT (rel_x <= -EDGE_THR)
    set rD, EDGE_THR
    mpy rD, -1
    cmp rA, rD
    jle !maybe_left            ; rel_x <= -EDGE_THR

    ; Otherwise, x is small → prioritize vertical
    jmp !vertical_focus

; ---------- RIGHT-SIDE BRANCH ----------
!maybe_right
    ; rel_x large positive: right-ish
    ; decide between right / down-right / up-right via rel_y
    set rD, EDGE_THR
    cmp rB, rD
    jge !dir_down_right        ; y >= +EDGE_THR  → SE (45)

    set rD, EDGE_THR
    mpy rD, -1
    cmp rB, rD
    jle !dir_up_right          ; y <= -EDGE_THR → NE (315)

    ; otherwise mostly horizontal
    jmp !dir_right

; ---------- LEFT-SIDE BRANCH ----------
!maybe_left
    ; rel_x large negative: left-ish
    set rD, EDGE_THR
    cmp rB, rD
    jge !dir_down_left         ; y >= +EDGE_THR → SW (135)

    set rD, EDGE_THR
    mpy rD, -1
    cmp rB, rD
    jle !dir_up_left           ; y <= -EDGE_THR → NW (225)

    ; mostly horizontal left
    jmp !dir_left

; ---------- VERTICAL FOCUS BRANCH ----------
!vertical_focus
    ; Now decide purely up/down based on rel_y
    set rD, EDGE_THR
    cmp rB, rD
    jge !dir_down              ; y >= +EDGE_THR → down

    set rD, EDGE_THR
    mpy rD, -1
    cmp rB, rD
    jle !dir_up                ; y <= -EDGE_THR → up

    ; If we're basically on top of them, just drive right
    jmp !dir_right

; ===========================
; DIR WRITE HELPERS
; ===========================
!dir_right
    str [DIR_0], rZ
    jmp !go

!dir_down_right
    str [DIR_45], rZ
    jmp !go

!dir_down
    str [DIR_90], rZ
    jmp !go

!dir_down_left
    str [DIR_135], rZ
    jmp !go

!dir_left
    str [DIR_180], rZ
    jmp !go

!dir_up_left
    str [DIR_225], rZ
    jmp !go

!dir_up
    str [DIR_270], rZ
    jmp !go

!dir_up_right
    str [DIR_315], rZ
    ; fall through

; ===========================
; APPLY THROTTLE
; ===========================
!go
    ; Simple version: always floor it
    set rC, FULL_THROTTLE
    str [THROTTLE], rC

    ;DLY DLY_AMT
    jmp !loop

; ===========================
; RETARGET LABEL
; ===========================
!retarget
    cal !pick_target
    jmp !loop
