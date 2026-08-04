`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/08/02 14:38:37
// Design Name: 
// Module Name: CP0
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


module CP0(
    input clk,
    input rst,
    input mfc0,             // CPU指令MFC0
    input mtc0,             // CPU指令MTC0
    input [31:0] pc,        // 异常时写入EPC的返回地址
    input [4:0] Rd,         // 指定CP0寄存器
    input [31:0] wdata,     // 数据从通用寄存器到CP0
    input exception,
    input eret,             // CPU指令ERET
    input [4:0] cause,
    input intr,
    output [31:0] rdata,    // 数据从CP0到通用寄存器或PC
    output [31:0] status,
    output reg timer_int,
    output [31:0] exc_addr  // 异常起始地址
    );

    reg [31:0] cp0_reg [31:0];
    reg [31:0] saved_status;
    integer i;

    assign rdata = eret ? cp0_reg[14] :
                   mfc0 ? cp0_reg[Rd] :
                          32'b0;
    assign status = cp0_reg[12];

    // PC保存Mars逻辑地址，因此物理偏移0x4对应逻辑地址0x00400004。
    assign exc_addr = 32'h0040_0004;

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            for (i = 0; i < 32; i = i + 1)
                cp0_reg[i] <= 32'b0;
            saved_status <= 32'b0;
            timer_int <= 1'b0;
        end
        else begin
            timer_int <= 1'b0;

            // 异常优先于ERET和MTC0，防止同一周期覆盖EPC、Cause和Status。
            if (exception) begin
                cp0_reg[14] <= pc;
                cp0_reg[13] <= {25'b0, cause, 2'b00};
                saved_status <= cp0_reg[12];
                cp0_reg[12] <= cp0_reg[12] << 5;
            end
            else if (eret) begin
                cp0_reg[12] <= saved_status;
            end
            else if (mtc0) begin
                cp0_reg[Rd] <= wdata;
            end

            // 本实验未实现计时器，保留指导书接口并对外部中断请求给出确定响应。
            if (intr)
                timer_int <= 1'b1;
        end
    end

endmodule
