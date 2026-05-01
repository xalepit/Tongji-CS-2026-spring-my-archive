.text 
main:
	addi $2, $zero, 0
	addi $3, $zero, 1
	addi $4, $zero 10
	
	addi $5, $zero, 1
	beq $4, $5, fib_n1
	
	addi $5, $zero, 2
	beq $4, $5, fib_n2
	
fib_loop:
	add $6, $2, $3
	addi $5, $5, 1
	beq $4, $5, loop_done
	
	add $2, $3, $zero
	add $3, $6, $zero
	j fib_loop
	
loop_done:
	addi $1, $6, 0
	j end
	
fib_n1:
	addi $1, $2, 0
	j end

fib_n2:
	addi $1, $3, 0
	j end

end:
	
