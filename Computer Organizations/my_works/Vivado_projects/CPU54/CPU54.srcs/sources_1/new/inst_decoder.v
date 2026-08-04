`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/07/31 09:40:31
// Design Name: 
// Module Name: inst_decoder
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module inst_decoder(
    input [31:0] inst,
    
    output [5:0] op,
    output [4:0] rs,
    output [4:0] rt,
    output [4:0] rd,
    output [4:0] sa,
    output [5:0] func,
    output [15:0] imm16,
    output [25:0] index,

    output i_add,
    output i_addu,
    output i_sub,
    output i_subu,
    output i_and,
    output i_or,
    output i_xor,
    output i_nor,
    output i_slt,
    output i_sltu,
    output i_sll,
    output i_srl,
    output i_sra,
    output i_sllv,
    output i_srlv,
    output i_srav,
    output i_jr,

    output i_addi,
    output i_addiu,
    output i_andi,
    output i_ori,
    output i_xori,
    output i_lw,
    output i_sw,
    output i_beq,
    output i_bne,
    output i_slti,
    output i_sltiu,
    output i_lui,

    output i_j,
    output i_jal,

    output i_div,
    output i_divu,
    output i_mult,
    output i_multu,
    output i_bgez,
    output i_jalr,
    output i_lbu,
    output i_lhu,
    output i_lb,
    output i_lh,
    output i_sb,
    output i_sh,
    output i_break,
    output i_syscall,
    output i_eret,
    output i_mfhi,
    output i_mflo,
    output i_mthi,
    output i_mtlo,
    output i_mfc0,
    output i_mtc0,
    output i_clz,
    output i_teq
    );

    assign op    = inst[31:26];
    assign rs    = inst[25:21];
    assign rt    = inst[20:16];
    assign rd    = inst[15:11];
    assign sa    = inst[10:6];
    assign func  = inst[5:0];
    assign imm16 = inst[15:0];
    assign index = inst[25:0];

    wire r_type;
    assign r_type = (op == 6'b000000);

    assign i_add  = r_type && (func == 6'b100000);
    assign i_addu = r_type && (func == 6'b100001);
    assign i_sub  = r_type && (func == 6'b100010);
    assign i_subu = r_type && (func == 6'b100011);
    assign i_and  = r_type && (func == 6'b100100);
    assign i_or   = r_type && (func == 6'b100101);
    assign i_xor  = r_type && (func == 6'b100110);
    assign i_nor  = r_type && (func == 6'b100111);
    assign i_slt  = r_type && (func == 6'b101010);
    assign i_sltu = r_type && (func == 6'b101011);

    assign i_sll  = r_type && (rs == 5'b00000) && (func == 6'b000000);
    assign i_srl  = r_type && (rs == 5'b00000) && (func == 6'b000010);
    assign i_sra  = r_type && (rs == 5'b00000) && (func == 6'b000011);

    assign i_sllv = r_type && (sa == 5'b00000) && (func == 6'b000100);
    assign i_srlv = r_type && (sa == 5'b00000) && (func == 6'b000110);
    assign i_srav = r_type && (sa == 5'b00000) && (func == 6'b000111);

    assign i_jr   = r_type && (rt == 5'b00000) && (rd == 5'b00000) &&
                    (sa == 5'b00000) && (func == 6'b001000);

    assign i_jalr = r_type && (rt == 5'b00000) &&
                    (sa == 5'b00000) && (func == 6'b001001);

    assign i_syscall = r_type && (func == 6'b001100);
    assign i_break   = r_type && (func == 6'b001101);

    assign i_mfhi = r_type && (rs == 5'b00000) && (rt == 5'b00000) &&
                    (sa == 5'b00000) && (func == 6'b010000);
    assign i_mthi = r_type && (rt == 5'b00000) && (rd == 5'b00000) &&
                    (sa == 5'b00000) && (func == 6'b010001);
    assign i_mflo = r_type && (rs == 5'b00000) && (rt == 5'b00000) &&
                    (sa == 5'b00000) && (func == 6'b010010);
    assign i_mtlo = r_type && (rt == 5'b00000) && (rd == 5'b00000) &&
                    (sa == 5'b00000) && (func == 6'b010011);

    assign i_mult  = r_type && (rd == 5'b00000) && (sa == 5'b00000) &&
                     (func == 6'b011000);
    assign i_multu = r_type && (rd == 5'b00000) && (sa == 5'b00000) &&
                     (func == 6'b011001);
    assign i_div   = r_type && (rd == 5'b00000) && (sa == 5'b00000) &&
                     (func == 6'b011010);
    assign i_divu  = r_type && (rd == 5'b00000) && (sa == 5'b00000) &&
                     (func == 6'b011011);

    assign i_teq = r_type && (func == 6'b110100);

    assign i_addi  = (op == 6'b001000);
    assign i_addiu = (op == 6'b001001);
    assign i_andi  = (op == 6'b001100);
    assign i_ori   = (op == 6'b001101);
    assign i_xori  = (op == 6'b001110);
    assign i_lui   = (op == 6'b001111);

    assign i_lw    = (op == 6'b100011);
    assign i_sw    = (op == 6'b101011);
    assign i_beq   = (op == 6'b000100);
    assign i_bne   = (op == 6'b000101);
    assign i_slti  = (op == 6'b001010);
    assign i_sltiu = (op == 6'b001011);

    assign i_j     = (op == 6'b000010);
    assign i_jal   = (op == 6'b000011);

    assign i_bgez = (op == 6'b000001) && (rt == 5'b00001);

    assign i_lb  = (op == 6'b100000);
    assign i_lh  = (op == 6'b100001);
    assign i_lbu = (op == 6'b100100);
    assign i_lhu = (op == 6'b100101);
    assign i_sb  = (op == 6'b101000);
    assign i_sh  = (op == 6'b101001);

    assign i_clz = (op == 6'b011100) && (rt == 5'b00000) &&
                   (sa == 5'b00000) && (func == 6'b100000);

    assign i_mfc0 = (op == 6'b010000) && (rs == 5'b00000) &&
                    (inst[10:3] == 8'b00000000) && (inst[2:0] == 3'b000);
    assign i_mtc0 = (op == 6'b010000) && (rs == 5'b00100) &&
                    (inst[10:3] == 8'b00000000) && (inst[2:0] == 3'b000);

    assign i_eret = (op == 6'b010000) && (rs == 5'b10000) &&
                    (rt == 5'b00000) && (rd == 5'b00000) &&
                    (sa == 5'b00000) && (func == 6'b011000);

endmodule
