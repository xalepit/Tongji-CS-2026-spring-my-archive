`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/05/14 21:40:34
// Design Name: 
// Module Name: PC
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


module PC(
    input pc_clk,   //上升沿有效
    input rst,      //高电平有效
    input [31:0] pc_data_in,
    output [31:0] pc_data_out
    );
    reg [31:0] pc_reg = 32'h00400000; //Mars 中 CPU 采用冯诺依曼结构，我们使用它的 default 内存设置，故指令存储起始地址为：0x00400000
    
    assign pc_data_out = pc_reg;
    
    always @(posedge pc_clk or posedge rst) begin
        if (rst) begin
            pc_reg <= 32'h00400000; //起始位置为00400000
        end
        else begin
            pc_reg <= pc_data_in;
        end
     end
endmodule
