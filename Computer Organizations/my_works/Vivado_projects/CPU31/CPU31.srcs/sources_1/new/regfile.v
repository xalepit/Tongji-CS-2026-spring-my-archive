`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/05/10 22:39:44
// Design Name: 
// Module Name: regfile
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

module regfile(
    input rf_clk,   //上升沿有效
    input rf_rst,   //高电平有效
    input rf_w,     //写使能
    input [4:0] Rdc,
    input [4:0] Rsc,
    input [4:0] Rtc,
    input [31:0] Rd_data_in,
    output [31:0] Rs_data_out,
    output [31:0] Rt_data_out
);
   reg [31:0] array_reg [31:0];
   
   integer i;
   
   assign Rs_data_out = (Rsc == 5'b00000) ? 32'b0 : array_reg[Rsc]; //保证读$0时返回0
   assign Rt_data_out = (Rtc == 5'b00000) ? 32'b0 : array_reg[Rtc]; //保证读$0时返回0
   
   always @(posedge rf_clk or posedge rf_rst) begin
       if (rf_rst) begin
            for (i = 0; i < 32; i = i + 1) begin
                array_reg[i] <= 32'b0;
            end
       end
       else begin
            if (rf_w && Rdc != 5'b00000) begin //$0只能返回0，不能被修改
                array_reg[Rdc] <= Rd_data_in;
            end
       end
   end
endmodule

