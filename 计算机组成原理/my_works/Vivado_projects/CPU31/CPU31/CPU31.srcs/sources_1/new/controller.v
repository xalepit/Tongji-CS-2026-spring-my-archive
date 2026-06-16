`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/05/14 23:01:36
// Design Name: 
// Module Name: controller
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


module controller(
    input i_add,
    input i_addu,
    input i_sub,
    input i_subu,
    input i_and,
    input i_or,
    input i_xor,
    input i_nor,
    input i_slt,
    input i_sltu,
    input i_sll,
    input i_srl,
    input i_sra,
    input i_sllv,
    input i_srlv,
    input i_srav,
    input i_jr,

    input i_addi,
    input i_addiu,
    input i_andi,
    input i_ori,
    input i_xori,
    input i_lw,
    input i_sw,
    input i_beq,
    input i_bne,
    input i_slti,
    input i_sltiu,
    input i_lui,

    input i_j,
    input i_jal,

    input alu_zero,

    output M0,
    output M1,
    output M2,
    output M3,
    output M4,
    output M5,
    output M6,
    output M7,
    output M8,
    output M9,

    output rf_w,

    output dm_ena,
    output dm_r,
    output dm_w,

    output [3:0] aluc
    );

    assign M0 = (i_beq & alu_zero) | (i_bne & ~alu_zero);

    assign M1 = i_sll | i_srl | i_sra | i_sllv | i_srlv | i_srav;

    assign M2 = i_sll | i_srl | i_sra;

    assign M3 = i_jr;

    assign M4 = i_addi | i_addiu | i_andi | i_ori | i_xori |
                i_lw | i_sw | i_slti | i_sltiu | i_lui;

    assign M5 = i_andi | i_ori | i_xori | i_lui;

    assign M6 = i_lw;

    assign M7 = i_bne;

    assign M8 = i_j | i_jal;

    assign M9 = i_jal;

    assign rf_w = i_add  | i_addu | i_sub  | i_subu |
                i_and  | i_or   | i_xor  | i_nor  |
                i_slt  | i_sltu |
                i_sll  | i_srl  | i_sra  |
                i_sllv | i_srlv | i_srav |
                i_addi | i_addiu |
                i_andi | i_ori   | i_xori |
                i_lw |
                i_slti | i_sltiu |
                i_lui |
                i_jal;

    assign dm_r = i_lw;

    assign dm_w = i_sw;

    assign dm_ena = i_lw | i_sw;

    assign aluc[3] = i_lui |
                   i_sltu | i_sltiu |
                   i_slt  | i_slti  |
                   i_sra  | i_srav  |
                   i_srl  | i_srlv  |
                   i_sll  | i_sllv;

    assign aluc[2] = i_and | i_andi |
                   i_or  | i_ori  |
                   i_xor | i_xori |
                   i_nor |
                   i_sra | i_srav |
                   i_srl | i_srlv |
                   i_sll | i_sllv;

    assign aluc[1] = i_add | i_addi |
                   i_lw  | i_sw   |
                   i_sub |
                   i_xor | i_xori |
                   i_nor |
                   i_sltu | i_sltiu |
                   i_slt  | i_slti  |
                   i_sll  | i_sllv;

    assign aluc[0] = i_subu |
                   i_beq  | i_bne  |
                   i_sub  |
                   i_or   | i_ori  |
                   i_nor  |
                   i_slt  | i_slti |
                   i_srl  | i_srlv;

endmodule
