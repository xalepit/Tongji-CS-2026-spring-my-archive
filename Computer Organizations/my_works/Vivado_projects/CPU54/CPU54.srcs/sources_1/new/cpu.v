`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/07/31 09:38:41
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
    wire branch_condition;

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

    wire i_div;
    wire i_divu;
    wire i_mult;
    wire i_multu;
    wire i_bgez;
    wire i_jalr;
    wire i_lbu;
    wire i_lhu;
    wire i_lb;
    wire i_lh;
    wire i_sb;
    wire i_sh;
    wire i_break;
    wire i_syscall;
    wire i_eret;
    wire i_mfhi;
    wire i_mflo;
    wire i_mthi;
    wire i_mtlo;
    wire i_mfc0;
    wire i_mtc0;
    wire i_clz;
    wire i_teq;

    wire M0;
    wire M1;
    wire M2;
    wire [1:0] M3;
    wire [1:0] M4;
    wire M5;
    wire [1:0] M6;
    wire [1:0] M7;
    wire M8;
    wire [2:0] M9;
    wire [1:0] M10;
    wire [1:0] M11;
    wire [1:0] M12;
    wire [1:0] M13;

    wire rf_w_ctrl;
    wire rf_w_effective;
    wire dm_ena_ctrl;
    wire dm_r_ctrl;
    wire dm_w_ctrl;
    wire HI_w_ctrl;
    wire LO_w_ctrl;
    wire div_signed;
    wire mul_signed;
    wire mfc0;
    wire mtc0;
    wire exception;
    wire eret;
    wire [4:0] cause;

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

    wire [31:0] load_byte_signed;
    wire [31:0] load_byte_unsigned;
    wire [31:0] load_half_signed;
    wire [31:0] load_half_unsigned;
    wire [7:0] selected_byte;
    wire [15:0] selected_half;
    wire [31:0] short_load_data;
    wire [31:0] load_or_alu_data;

    wire [31:0] store_byte_data;
    wire [31:0] store_half_data;

    wire [31:0] div_r;
    wire [31:0] div_q;
    wire [31:0] mul_hi;
    wire [31:0] mul_lo;
    wire [31:0] hi_in;
    wire [31:0] lo_in;
    wire [31:0] hi_out;
    wire [31:0] lo_out;
    wire div_by_zero;
    wire HI_w_effective;
    wire LO_w_effective;

    wire [31:0] clz_out;

    wire [31:0] cp0_rdata;
    wire [31:0] cp0_status;
    wire [31:0] cp0_exc_addr;
    wire cp0_timer_int;

    wire write_rt;

    assign pc = pc_cur;

    assign pc_plus4 = pc_cur + 32'd4;
    assign pc_branch = pc_plus4 + {{14{imm16[15]}}, imm16, 2'b00};
    assign pc_jump = {pc_plus4[31:28], index, 2'b00};

    assign branch_condition = (M7 == 2'b00) ? alu_zero :
                              (M7 == 2'b01) ? ~alu_zero :
                              (M7 == 2'b10) ? ~alu_negative :
                                             1'b0;
    assign pc_branch_or_plus4 = (M0 && branch_condition) ? pc_branch : pc_plus4;
    assign pc_jump_or_normal = M8 ? pc_jump : pc_branch_or_plus4;

    // 异常入口优先级由控制器编码保证，高于ERET和寄存器跳转。
    assign pc_next = (M3 == 2'b00) ? pc_jump_or_normal :
                     (M3 == 2'b01) ? Rs_data_out :
                     (M3 == 2'b10) ? cp0_exc_addr :
                                     cp0_rdata;

    assign imm_sign_ext = {{16{imm16[15]}}, imm16};
    assign imm_zero_ext = {16'b0, imm16};
    assign imm_ext = M5 ? imm_zero_ext : imm_sign_ext;

    assign shift_num = {27'b0, M2 ? sa : Rs_data_out[4:0]};
    assign alu_a = M1 ? shift_num : Rs_data_out;
    assign alu_b = (M4 == 2'b00) ? Rt_data_out :
                   (M4 == 2'b01) ? imm_ext :
                                   32'b0;

    assign selected_byte = (alu_r[1:0] == 2'b00) ? dm_rdata[7:0] :
                           (alu_r[1:0] == 2'b01) ? dm_rdata[15:8] :
                           (alu_r[1:0] == 2'b10) ? dm_rdata[23:16] :
                                                  dm_rdata[31:24];
    assign selected_half = alu_r[1] ? dm_rdata[31:16] : dm_rdata[15:0];

    assign load_byte_signed = {{24{selected_byte[7]}}, selected_byte};
    assign load_byte_unsigned = {24'b0, selected_byte};
    assign load_half_signed = {{16{selected_half[15]}}, selected_half};
    assign load_half_unsigned = {16'b0, selected_half};

    assign short_load_data = (M12 == 2'b00) ? load_byte_signed :
                             (M12 == 2'b01) ? load_byte_unsigned :
                             (M12 == 2'b10) ? load_half_signed :
                                             load_half_unsigned;
    assign load_or_alu_data = (M6 == 2'b00) ? alu_r :
                              (M6 == 2'b01) ? dm_rdata :
                              (M6 == 2'b10) ? short_load_data :
                                              32'b0;

    assign write_rt = i_addi | i_addiu |
                      i_andi | i_ori | i_xori |
                      i_lw | i_lbu | i_lhu | i_lb | i_lh |
                      i_slti | i_sltiu | i_lui | i_mfc0;

    assign Rdc = i_jal ? 5'd31 :
                 write_rt ? rt :
                 rd;
    assign Rsc = rs;
    assign Rtc = rt;

    assign Rd_data_in = (M9 == 3'b000) ? load_or_alu_data :
                        (M9 == 3'b001) ? pc_plus4 :
                        (M9 == 3'b010) ? clz_out :
                        (M9 == 3'b011) ? hi_out :
                        (M9 == 3'b100) ? lo_out :
                        (M9 == 3'b101) ? cp0_rdata :
                                         32'b0;

    assign dm_addr = alu_r;

    // SB和SH先与原存储字合并，再沿用原有DMEM整字写接口。
    assign store_byte_data = (alu_r[1:0] == 2'b00) ? {dm_rdata[31:8], Rt_data_out[7:0]} :
                             (alu_r[1:0] == 2'b01) ? {dm_rdata[31:16], Rt_data_out[7:0], dm_rdata[7:0]} :
                             (alu_r[1:0] == 2'b10) ? {dm_rdata[31:24], Rt_data_out[7:0], dm_rdata[15:0]} :
                                                    {Rt_data_out[7:0], dm_rdata[23:0]};
    assign store_half_data = alu_r[1] ? {Rt_data_out[15:0], dm_rdata[15:0]} :
                                               {dm_rdata[31:16], Rt_data_out[15:0]};
    assign dm_wdata = (M13 == 2'b00) ? Rt_data_out :
                      (M13 == 2'b01) ? store_byte_data :
                      (M13 == 2'b10) ? store_half_data :
                                      32'b0;

    assign rf_w_effective = rf_w_ctrl & ~exception;
    assign dm_ena = dm_ena_ctrl & ~exception;
    assign dm_r = dm_r_ctrl & ~exception;
    assign dm_w = dm_w_ctrl & ~exception;

    assign hi_in = (M10 == 2'b00) ? Rs_data_out :
                   (M10 == 2'b01) ? div_r :
                   (M10 == 2'b10) ? mul_hi :
                                    32'b0;
    assign lo_in = (M11 == 2'b00) ? Rs_data_out :
                   (M11 == 2'b01) ? div_q :
                   (M11 == 2'b10) ? mul_lo :
                                    32'b0;

    assign div_by_zero = (i_div | i_divu) && (Rt_data_out == 32'b0);
    assign HI_w_effective = HI_w_ctrl & ~div_by_zero & ~exception;
    assign LO_w_effective = LO_w_ctrl & ~div_by_zero & ~exception;

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
        .i_jal(i_jal),
        .i_div(i_div),
        .i_divu(i_divu),
        .i_mult(i_mult),
        .i_multu(i_multu),
        .i_bgez(i_bgez),
        .i_jalr(i_jalr),
        .i_lbu(i_lbu),
        .i_lhu(i_lhu),
        .i_lb(i_lb),
        .i_lh(i_lh),
        .i_sb(i_sb),
        .i_sh(i_sh),
        .i_break(i_break),
        .i_syscall(i_syscall),
        .i_eret(i_eret),
        .i_mfhi(i_mfhi),
        .i_mflo(i_mflo),
        .i_mthi(i_mthi),
        .i_mtlo(i_mtlo),
        .i_mfc0(i_mfc0),
        .i_mtc0(i_mtc0),
        .i_clz(i_clz),
        .i_teq(i_teq)
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
        .i_div(i_div),
        .i_divu(i_divu),
        .i_mult(i_mult),
        .i_multu(i_multu),
        .i_bgez(i_bgez),
        .i_jalr(i_jalr),
        .i_lbu(i_lbu),
        .i_lhu(i_lhu),
        .i_lb(i_lb),
        .i_lh(i_lh),
        .i_sb(i_sb),
        .i_sh(i_sh),
        .i_break(i_break),
        .i_syscall(i_syscall),
        .i_eret(i_eret),
        .i_mfhi(i_mfhi),
        .i_mflo(i_mflo),
        .i_mthi(i_mthi),
        .i_mtlo(i_mtlo),
        .i_mfc0(i_mfc0),
        .i_mtc0(i_mtc0),
        .i_clz(i_clz),
        .i_teq(i_teq),
        .alu_zero(alu_zero),
        .alu_negative(alu_negative),
        .status(cp0_status),
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
        .M10(M10),
        .M11(M11),
        .M12(M12),
        .M13(M13),
        .rf_w(rf_w_ctrl),
        .dm_ena(dm_ena_ctrl),
        .dm_r(dm_r_ctrl),
        .dm_w(dm_w_ctrl),
        .HI_w(HI_w_ctrl),
        .LO_w(LO_w_ctrl),
        .div_signed(div_signed),
        .mul_signed(mul_signed),
        .mfc0(mfc0),
        .mtc0(mtc0),
        .exception(exception),
        .eret(eret),
        .cause(cause),
        .aluc(aluc)
    );

    regfile cpu_ref(
        .rf_clk(clk),
        .rf_rst(rst),
        .rf_w(rf_w_effective),
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

    DIV div_inst(
        .A(Rs_data_out),
        .B(Rt_data_out),
        .div_signed(div_signed),
        .R(div_r),
        .Q(div_q)
    );

    MUL mul_inst(
        .A(Rs_data_out),
        .B(Rt_data_out),
        .mul_signed(mul_signed),
        .HI(mul_hi),
        .LO(mul_lo)
    );

    HI_LO hi_lo_inst(
        .clk(clk),
        .rst(rst),
        .HI_in(hi_in),
        .LO_in(lo_in),
        .HI_w(HI_w_effective),
        .LO_w(LO_w_effective),
        .HI_out(hi_out),
        .LO_out(lo_out)
    );

    CLZ clz_inst(
        .CLZ_in(Rs_data_out),
        .CLZ_out(clz_out)
    );

    CP0 cp0_inst(
        .clk(clk),
        .rst(rst),
        .mfc0(mfc0),
        .mtc0(mtc0 & ~exception),
        .pc(pc_plus4),
        .Rd(rd),
        .wdata(Rt_data_out),
        .exception(exception),
        .eret(eret),
        .cause(cause),
        .intr(1'b0),
        .rdata(cp0_rdata),
        .status(cp0_status),
        .timer_int(cp0_timer_int),
        .exc_addr(cp0_exc_addr)
    );

endmodule
