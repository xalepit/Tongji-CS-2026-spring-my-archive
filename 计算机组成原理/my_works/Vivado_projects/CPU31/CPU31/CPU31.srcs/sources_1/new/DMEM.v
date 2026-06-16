`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/05/11 22:15:49
// Design Name: 
// Module Name: DMEM
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


module DMEM(
    input dm_clk,
    input dm_ena,
    input dm_r,
    input dm_w,
    input [4:0] dm_addr,
    input [31:0] dm_data_in,
    output [31:0] dm_data_out
    );
    reg [31:0] data_mem [31:0];
    assign dm_data_out = (dm_ena && dm_r) ? data_mem[dm_addr] : 32'b0;
    
    always @(posedge dm_clk) begin
        if (dm_ena && dm_w) begin
            data_mem[dm_addr] <= dm_data_in;
        end
    end
endmodule
