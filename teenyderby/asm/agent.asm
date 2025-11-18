.const RAND 0x8010

.const DLY_AMT 2

!move
lod rA, [RAND]
mod rA, 4
cmp rA, 0
je !move_up

cmp rA, 1
je !move_down

cmp rA, 2
je !move_left

cmp rA, 3
je !move_right


!move_up
str [0x9000], rZ
DLY DLY_AMT
jmp !move

!move_down
str [0x9001], rZ
DLY DLY_AMT
jmp !move

!move_left
str [0x9002], rZ
DLY DLY_AMT
jmp !move

!move_right
str [0x9003], rZ
DLY DLY_AMT
jmp !move