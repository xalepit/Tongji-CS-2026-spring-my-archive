.text
main:
	addi $2, $0, 6 #被乘数
	addi $3, $0, -7 #乘数
	
    	add $10, $0, $0 #部分积A
    	add $11, $3, $0 #Q
    	add $12, $0, $0 #Q附加位
    	addi $13, $0, 32

booth_loop:
    	beq $13, $0, booth_end

  	andi $14, $11, 1
	beq $14, $12, booth_shift

    	beq $14, $0, booth_add
    	sub $10, $10, $2
    	j booth_shift

booth_add:
    	add $10, $10, $2

booth_shift:
    	andi $15, $10, 1
    	sll $15, $15, 31

    	andi $12, $11, 1
    	srl $11, $11, 1
    	or $11, $11, $15
    	sra  $10, $10, 1

    	addi $13, $13, -1
    	j booth_loop

booth_end: