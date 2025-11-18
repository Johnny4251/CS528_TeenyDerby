!move_left_init
set rA, rZ
!move_left
	str [0x9002], rZ
	inc rA
	cmp rA, 2
	jge !move_right_init
	jmp !move_left

!move_right_init
set rA, rZ
!move_right
	str [0x9003], rZ
	inc rA
	cmp rA, 2
	jge !move_left_init
	jmp !move_right