.const RAND 0x8010
.const MOVE_FWD     0x9000
.const MOVE_BWD     0x9001
.const TURN_LEFT    0x9002
.const TURN_RIGHT   0x9003
.const FACE_DIR     0x9004 

.const DLY_AMT 2
!move
    lod rA, [RAND]
    mod rA, 5           ; 0–4 → pick between 4 moves + facing cmd

    cmp rA, 0
    je !do_up

    cmp rA, 1
    je !do_down

    cmp rA, 2
    je !do_left

    cmp rA, 3
    je !do_right

    ; rA == 4 → choose random facing direction 0–7
    ; fallthrough
!do_face
    lod rB, [RAND]
    mod rB, 8           ; pick direction 0–7
    str [FACE_DIR], rB
    DLY DLY_AMT
    jmp !move

!do_up
    str [MOVE_FWD], rZ
    DLY DLY_AMT
    jmp !move

!do_down
    str [MOVE_BWD], rZ
    DLY DLY_AMT
    jmp !move

!do_left
    str [TURN_LEFT], rZ
    DLY DLY_AMT
    jmp !move

!do_right
    str [TURN_RIGHT], rZ
    DLY DLY_AMT
    jmp !move