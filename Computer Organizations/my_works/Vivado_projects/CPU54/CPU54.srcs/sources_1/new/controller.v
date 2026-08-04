`timescale 1ns / 1ps

//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/07/31 09:41:06
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

// 单周期54条指令CPU的组合逻辑控制器。
// MUX编码遵循数据通路，并保持CPU31原有选择值不变。
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

    input i_div,
    input i_divu,
    input i_mult,
    input i_multu,
    input i_bgez,
    input i_jalr,
    input i_lbu,
    input i_lhu,
    input i_lb,
    input i_lh,
    input i_sb,
    input i_sh,
    input i_break,
    input i_syscall,
    input i_eret,
    input i_mfhi,
    input i_mflo,
    input i_mthi,
    input i_mtlo,
    input i_mfc0,
    input i_mtc0,
    input i_clz,
    input i_teq,

    input alu_zero,
    input alu_negative,
    input [31:0] status,

    output M0,
    output M1,
    output M2,
    output [1:0] M3,
    output [1:0] M4,
    output M5,
    output [1:0] M6,
    output [1:0] M7,
    output M8,
    output [2:0] M9,
    output [1:0] M10,
    output [1:0] M11,
    output [1:0] M12,
    output [1:0] M13,

    output rf_w,

    output dm_ena,
    output dm_r,
    output dm_w,

    output HI_w,
    output LO_w,
    output div_signed,
    output mul_signed,

    output mfc0,
    output mtc0,
    output exception,
    output eret,
    output [4:0] cause,

    output [3:0] aluc
    );

    wire teq_taken;

    // MUX0：选择PC+4或分支目标地址。
    assign M0 = (i_beq & alu_zero) |
                (i_bne & ~alu_zero) |
                (i_bgez & ~alu_negative);

    // MUX1：选择Rs或零扩展后的移位量。
    assign M1 = i_sll | i_srl | i_sra |
                i_sllv | i_srlv | i_srav;

    // MUX2：选择Rs[4:0]或sa。
    assign M2 = i_sll | i_srl | i_sra;

    // MUX3：00为MUX8，01为Rs，10为CP0异常入口，11为CP0读出的EPC。
    // 已受理的异常优先于其他所有PC来源。
    assign M3[1] = exception | i_eret;
    assign M3[0] = ~exception & (i_jr | i_jalr | i_eret);

    // MUX4：00为Rt，01为立即数，10为零。
    assign M4[1] = i_bgez;
    assign M4[0] = i_addi | i_addiu | i_andi | i_ori | i_xori |
                   i_lw | i_sw | i_slti | i_sltiu | i_lui |
                   i_lbu | i_lhu | i_lb | i_lh | i_sb | i_sh;

    // MUX5：选择16位有符号扩展或无符号扩展。
    assign M5 = i_andi | i_ori | i_xori | i_lui;

    // MUX6：00为ALU结果，01为字装载结果，10为字节或半字装载结果。
    assign M6[1] = i_lbu | i_lhu | i_lb | i_lh;
    assign M6[0] = i_lw;

    // MUX7：00为Z，01为~Z，10为~N。
    assign M7[1] = i_bgez;
    assign M7[0] = i_bne;

    // MUX8：选择普通分支路径或跳转目标地址。
    assign M8 = i_j | i_jal;

    // MUX9：000为MUX6，001为链接地址，010为CLZ结果，011为HI，
    //       100为LO，101为CP0读出数据。
    assign M9[2] = i_mflo | i_mfc0;
    assign M9[1] = i_clz | i_mfhi;
    assign M9[0] = i_jal | i_jalr | i_mfhi | i_mfc0;

    // MUX10：00为Rs，01为DIV余数，10为MUL高32位。
    assign M10[1] = i_mult | i_multu;
    assign M10[0] = i_div | i_divu;

    // MUX11：00为Rs，01为DIV商，10为MUL低32位。
    assign M11[1] = i_mult | i_multu;
    assign M11[0] = i_div | i_divu;

    // MUX12：00为LB结果，01为LBU结果，10为LH结果，11为LHU结果。
    assign M12[1] = i_lh | i_lhu;
    assign M12[0] = i_lbu | i_lhu;

    // MUX13：00为SW写入数据，01为SB写入数据，10为SH写入数据。
    assign M13[1] = i_sh;
    assign M13[0] = i_sb;

    assign rf_w = i_add | i_addu | i_sub | i_subu |
                  i_and | i_or | i_xor | i_nor |
                  i_slt | i_sltu |
                  i_sll | i_srl | i_sra |
                  i_sllv | i_srlv | i_srav |
                  i_addi | i_addiu |
                  i_andi | i_ori | i_xori |
                  i_lw | i_slti | i_sltiu | i_lui | i_jal |
                  i_jalr | i_lbu | i_lhu | i_lb | i_lh |
                  i_mfhi | i_mflo | i_mfc0 | i_clz;

    assign dm_r = i_lw | i_lbu | i_lhu | i_lb | i_lh;
    assign dm_w = i_sw | i_sb | i_sh;
    assign dm_ena = dm_r | dm_w;

    assign HI_w = i_div | i_divu | i_mult | i_multu | i_mthi;
    assign LO_w = i_div | i_divu | i_mult | i_multu | i_mtlo;
    assign div_signed = i_div;
    assign mul_signed = i_mult;

    assign mfc0 = i_mfc0;
    assign mtc0 = i_mtc0;
    assign eret = i_eret;

    // 本实验中，Status[0]为全局使能位。
    // Status[8]、Status[9]或Status[10]置位时，分别屏蔽syscall、break或teq异常。
    assign teq_taken = i_teq & alu_zero;
    assign exception = status[0] &
                       ((i_syscall & ~status[8]) |
                        (i_break & ~status[9]) |
                        (teq_taken & ~status[10]));

    // 异常编码：syscall为8，break为9，teq为13。
    assign cause[4] = 1'b0;
    assign cause[3] = i_syscall | i_break | teq_taken;
    assign cause[2] = teq_taken;
    assign cause[1] = 1'b0;
    assign cause[0] = i_break | teq_taken;

    assign aluc[3] = i_lui |
                     i_sltu | i_sltiu |
                     i_slt | i_slti |
                     i_sra | i_srav |
                     i_srl | i_srlv |
                     i_sll | i_sllv;

    assign aluc[2] = i_and | i_andi |
                     i_or | i_ori |
                     i_xor | i_xori |
                     i_nor |
                     i_sra | i_srav |
                     i_srl | i_srlv |
                     i_sll | i_sllv;

    assign aluc[1] = i_add | i_addi |
                     i_lw | i_sw |
                     i_lbu | i_lhu | i_lb | i_lh | i_sb | i_sh |
                     i_sub |
                     i_xor | i_xori |
                     i_nor |
                     i_sltu | i_sltiu |
                     i_slt | i_slti |
                     i_sll | i_sllv;

    assign aluc[0] = i_subu |
                     i_beq | i_bne | i_bgez | i_teq |
                     i_sub |
                     i_or | i_ori |
                     i_nor |
                     i_slt | i_slti |
                     i_srl | i_srlv;

endmodule
