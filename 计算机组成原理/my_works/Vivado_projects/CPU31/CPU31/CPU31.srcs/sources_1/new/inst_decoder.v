`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/05/14 23:02:56
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
    output i_jal
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

endmodule
