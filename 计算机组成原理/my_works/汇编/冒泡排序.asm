.text
main:
	addi $2, $zero, 7
	addi $3, $zero, 9
	addi $4, $zero, 8
	addi $5, $zero, -3
	addi $6, $zero, -6
	
	addi $7, $zero, 0
	addi $9, $zero, 5 #数字个数
	addi $10, $zero, 0 #temp，用于交换
	addi $11, $zero, 0 #用于辅助判断

bubble_sort:
	addi $7, $7, 1
	slt $11, $2, $3
        beq $11, 0, swap_23
done_23:
        slt $11, $3, $4
        beq $11, 0, swap_34
done_34:
        slt $11, $4, $5
        beq $11, 0, swap_45
done_45:
        slt $11, $5, $6
        beq $11, 0, swap_56
done_56:
        beq $7, $9, end
        j bubble_sort
        
swap_23:
	addi $10, $2, 0
	addi $2, $3, 0
	addi $3, $10, 0
	j done_23
	
swap_34:
	addi $10, $3, 0
	addi $3, $4, 0
	addi $4, $10, 0
	j done_34
	
swap_45:
	addi $10, $4, 0
	addi $4, $5, 0
	addi $5, $10, 0
	j done_45
	
swap_56:
	addi $10, $5, 0
	addi $5, $6, 0
	addi $6, $10, 0
	j done_56
end:
	
