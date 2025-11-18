!move_up_init
set rA, rZ
!move_up
	str [0x9000], rZ
	inc rA
	cmp rA, 2
	jge !move_down_init
	jmp !move_up

!move_down_init
set rA, rZ
!move_down
	str [0x9001], rZ
	inc rA
	cmp rA, 2
	jge !move_up_init
	jmp !move_down