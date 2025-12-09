; ---------------------------
; VULTURE AGENT - Hunts dead agents
; Moves randomly like basic agent, but checks for dead targets and hunts them
; ---------------------------

.const PORT_A_DIR   0x8000
.const PORT_B_DIR   0x8001
.const PORT_A       0x8002
.const PORT_B       0x8003
.const RAND         0x8010
.const RAND_BITS    0x8011

.const THROTTLE        0x9000

.const DIR_0           0x9010 
.const DIR_45          0x9011 
.const DIR_90          0x9012 
.const DIR_135         0x9013 
.const DIR_180         0x9014 
.const DIR_225         0x9015 
.const DIR_270         0x9016 
.const DIR_315         0x9017 

.const SENSOR_TARGET   0x9020
.const SENSOR_REL_X    0x9021
.const SENSOR_REL_Y    0x9022
.const SENSOR_IS_DEAD  0x9028

.const SELF_ID         0x9030

.const DLY_AMT   5
.const FULL_THROTTLE 100

!move
    ; Scan for dead agents sequentially
    lod rA, [RAND]
    mod rA, 16
    str [SENSOR_TARGET], rA
    
    ; Small delay to let sensor read update
    DLY 1
    
    ; Check if target is dead
    lod rB, [SENSOR_IS_DEAD]
    cmp rB, 1
    je !found_dead
    
    ; Not dead - move randomly
    lod rA, [RAND]
    mod rA, 100
    str [THROTTLE], rA
    
    ; Pick random direction
    lod rA, [RAND]
    mod rA, 8
    
    cmp rA, 0
    je !d0
    cmp rA, 1
    je !d1
    cmp rA, 2
    je !d2
    cmp rA, 3
    je !d3
    cmp rA, 4
    je !d4
    cmp rA, 5
    je !d5
    cmp rA, 6
    je !d6
    jmp !d7

!d0
    str [DIR_0], rZ
    jmp !wait

!d1
    str [DIR_45], rZ
    jmp !wait

!d2
    str [DIR_90], rZ
    jmp !wait

!d3
    str [DIR_135], rZ
    jmp !wait

!d4
    str [DIR_180], rZ
    jmp !wait

!d5
    str [DIR_225], rZ
    jmp !wait

!d6
    str [DIR_270], rZ
    jmp !wait

!d7
    str [DIR_315], rZ
    jmp !wait

!wait
    DLY DLY_AMT
    jmp !move

!found_dead
    ; Dead agent found! Get relative position and chase
    lod rC, [SENSOR_REL_X]
    lod rD, [SENSOR_REL_Y]
    
    ; Get absolute values for comparison
    set rE, rC
    cmp rE, 0
    jge !abs_x_ok
    neg rE
    
!abs_x_ok
    set rB, rD
    cmp rB, 0
    jge !abs_y_ok
    neg rB
    
!abs_y_ok
    ; Compare |X| vs |Y| to pick best direction
    cmp rE, rB
    jg !move_x
    
    ; Move Y direction
    cmp rD, 0
    jl !go_north
    str [DIR_90], rZ
    jmp !chase_throttle
    
!go_north
    str [DIR_270], rZ
    jmp !chase_throttle

!move_x
    ; Move X direction
    cmp rC, 0
    jl !go_west
    str [DIR_0], rZ
    jmp !chase_throttle
    
!go_west
    str [DIR_180], rZ
    
!chase_throttle
    set rA, FULL_THROTTLE
    str [THROTTLE], rA
    DLY 2
    
    ; Recheck if still dead and still on target
    lod rB, [SENSOR_IS_DEAD]
    cmp rB, 1
    je !found_dead
    
    ; Dead agent gone, resume normal movement
    jmp !move
