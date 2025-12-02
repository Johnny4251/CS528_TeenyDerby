.const RAND      0x8010
.const DLY_AMT   20

.const THROTTLE  0x9000

.const DERBY_SELF_X_ADDR  0x9034
.const DERBY_SELF_Y_ADDR  0x9035

.const DIR_0     0x9010 
.const DIR_90    0x9012 
.const DIR_180   0x9014 
.const DIR_270   0x9016 

.const MAX_ACCEL 100

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
    lod rA, [DERBY_SELF_X_ADDR]

    cmp rA, 600        
    jge !next_dir      

    set rB, MAX_ACCEL
    str [THROTTLE], rB
    ;DLY DLY_AMT
    jmp !loop


!go_down
    str [DIR_90], rZ
    lod rA, [DERBY_SELF_Y_ADDR]

    cmp rA, 500        
    jge !next_dir

    set rB, MAX_ACCEL
    str [THROTTLE], rB
    ;DLY DLY_AMT
    jmp !loop


!go_left
    str [DIR_180], rZ
    lod rA, [DERBY_SELF_X_ADDR]

    cmp rA, 200        
    jle !next_dir

    set rB, MAX_ACCEL
    str [THROTTLE], rB
    ;DLY DLY_AMT
    jmp !loop


!go_up
    str [DIR_270], rZ
    lod rA, [DERBY_SELF_Y_ADDR]

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
