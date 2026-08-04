`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/05/14 22:57:11
// Design Name: 
// Module Name: cpu
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

module cpu(
    input clk,
    input rst,
    input [31:0] inst,
    input [31:0] dm_rdata,
    output [31:0] pc,
    output [31:0] dm_addr,
    output [31:0] dm_wdata,
    output dm_ena,
    output dm_r,
    output dm_w
    );

    wire [31:0] pc_cur;
    wire [31:0] pc_next;
    wire [31:0] pc_plus4;
    wire [31:0] pc_branch;
    wire [31:0] pc_jump;
    wire [31:0] pc_branch_or_plus4;
    wire [31:0] pc_jump_or_normal;

    wire [5:0] op;
    wire [4:0] rs;
    wire [4:0] rt;
    wire [4:0] rd;
    wire [4:0] sa;
    wire [5:0] func;
    wire [15:0] imm16;
    wire [25:0] index;

    wire i_add;
    wire i_addu;
    wire i_sub;
    wire i_subu;
    wire i_and;
    wire i_or;
    wire i_xor;
    wire i_nor;
    wire i_slt;
    wire i_sltu;
    wire i_sll;
    wire i_srl;
    wire i_sra;
    wire i_sllv;
    wire i_srlv;
    wire i_srav;
    wire i_jr;

    wire i_addi;
    wire i_addiu;
    wire i_andi;
    wire i_ori;
    wire i_xori;
    wire i_lw;
    wire i_sw;
    wire i_beq;
    wire i_bne;
    wire i_slti;
    wire i_sltiu;
    wire i_lui;

    wire i_j;
    wire i_jal;

    wire M0;
    wire M1;
    wire M2;
    wire M3;
    wire M4;
    wire M5;
    wire M6;
    wire M7;
    wire M8;
    wire M9;

    wire rf_w;
    wire [4:0] Rdc;
    wire [4:0] Rsc;
    wire [4:0] Rtc;
    wire [31:0] Rd_data_in;
    wire [31:0] Rs_data_out;
    wire [31:0] Rt_data_out;

    wire [31:0] imm_sign_ext;
    wire [31:0] imm_zero_ext;
    wire [31:0] imm_ext;
    wire [31:0] shift_num;
    wire [31:0] alu_a;
    wire [31:0] alu_b;
    wire [31:0] alu_r;
    wire alu_zero;
    wire alu_carry;
    wire alu_negative;
    wire alu_overflow;
    wire [3:0] aluc;

    wire write_rt;

    assign pc = pc_cur;

    assign pc_plus4 = pc_cur + 32'd4;
    assign pc_branch = pc_plus4 + {{14{imm16[15]}}, imm16, 2'b00};
    assign pc_jump = {pc_plus4[31:28], index, 2'b00};

    assign pc_branch_or_plus4 = M0 ? pc_branch : pc_plus4;
    assign pc_jump_or_normal = M8 ? pc_jump : pc_branch_or_plus4;
    assign pc_next = M3 ? Rs_data_out : pc_jump_or_normal;

    assign imm_sign_ext = {{16{imm16[15]}}, imm16};
    assign imm_zero_ext = {16'b0, imm16};
    assign imm_ext = M5 ? imm_zero_ext : imm_sign_ext;

    assign shift_num = {27'b0, M2 ? sa : Rs_data_out[4:0]};

    assign alu_a = M1 ? shift_num : Rs_data_out;
    assign alu_b = M4 ? imm_ext : Rt_data_out;

    assign write_rt = i_addi | i_addiu |
                      i_andi | i_ori | i_xori |
                      i_lw |
                      i_slti | i_sltiu |
                      i_lui;

    assign Rdc = i_jal ? 5'd31 :
                 write_rt ? rt :
                 rd;

    assign Rsc = rs;
    assign Rtc = rt;

    assign Rd_data_in = M9 ? pc_plus4 :
                      M6 ? dm_rdata :
                      alu_r;

    assign dm_addr = alu_r;
    assign dm_wdata = Rt_data_out;

    PC pc_inst(
        .pc_clk(clk),
        .rst(rst),
        .pc_data_in(pc_next),
        .pc_data_out(pc_cur)
    );

    inst_decoder inst_decoder_inst(
        .inst(inst),

        .op(op),
        .rs(rs),
        .rt(rt),
        .rd(rd),
        .sa(sa),
        .func(func),
        .imm16(imm16),
        .index(index),

        .i_add(i_add),
        .i_addu(i_addu),
        .i_sub(i_sub),
        .i_subu(i_subu),
        .i_and(i_and),
        .i_or(i_or),
        .i_xor(i_xor),
        .i_nor(i_nor),
        .i_slt(i_slt),
        .i_sltu(i_sltu),
        .i_sll(i_sll),
        .i_srl(i_srl),
        .i_sra(i_sra),
        .i_sllv(i_sllv),
        .i_srlv(i_srlv),
        .i_srav(i_srav),
        .i_jr(i_jr),

        .i_addi(i_addi),
        .i_addiu(i_addiu),
        .i_andi(i_andi),
        .i_ori(i_ori),
        .i_xori(i_xori),
        .i_lw(i_lw),
        .i_sw(i_sw),
        .i_beq(i_beq),
        .i_bne(i_bne),
        .i_slti(i_slti),
        .i_sltiu(i_sltiu),
        .i_lui(i_lui),

        .i_j(i_j),
        .i_jal(i_jal)
    );

    controller controller_inst(
        .i_add(i_add),
        .i_addu(i_addu),
        .i_sub(i_sub),
        .i_subu(i_subu),
        .i_and(i_and),
        .i_or(i_or),
        .i_xor(i_xor),
        .i_nor(i_nor),
        .i_slt(i_slt),
        .i_sltu(i_sltu),
        .i_sll(i_sll),
        .i_srl(i_srl),
        .i_sra(i_sra),
        .i_sllv(i_sllv),
        .i_srlv(i_srlv),
        .i_srav(i_srav),
        .i_jr(i_jr),

        .i_addi(i_addi),
        .i_addiu(i_addiu),
        .i_andi(i_andi),
        .i_ori(i_ori),
        .i_xori(i_xori),
        .i_lw(i_lw),
        .i_sw(i_sw),
        .i_beq(i_beq),
        .i_bne(i_bne),
        .i_slti(i_slti),
        .i_sltiu(i_sltiu),
        .i_lui(i_lui),

        .i_j(i_j),
        .i_jal(i_jal),

        .alu_zero(alu_zero),

        .M0(M0),
        .M1(M1),
        .M2(M2),
        .M3(M3),
        .M4(M4),
        .M5(M5),
        .M6(M6),
        .M7(M7),
        .M8(M8),
        .M9(M9),

        .rf_w(rf_w),

        .dm_ena(dm_ena),
        .dm_r(dm_r),
        .dm_w(dm_w),

        .aluc(aluc)
    );

    regfile cpu_ref(
        .rf_clk(clk),
        .rf_rst(rst),
        .rf_w(rf_w),
        .Rdc(Rdc),
        .Rsc(Rsc),
        .Rtc(Rtc),
        .Rd_data_in(Rd_data_in),
        .Rs_data_out(Rs_data_out),
        .Rt_data_out(Rt_data_out)
    );

    alu alu_inst(
        .a(alu_a),
        .b(alu_b),
        .aluc(aluc),
        .r(alu_r),
        .zero(alu_zero),
        .carry(alu_carry),
        .negative(alu_negative),
        .overflow(alu_overflow)
    );

endmodule
