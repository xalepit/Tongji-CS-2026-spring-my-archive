`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/08/02 14:45:06
// Design Name: 
// Module Name: HI_LO
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


module HI_LO(
    input clk,
    input rst,
    input [31:0] HI_in,
    input [31:0] LO_in,
    input HI_w,
    input LO_w,
    output [31:0] HI_out,
    output [31:0] LO_out
    );

    reg [31:0] hi_reg;
    reg [31:0] lo_reg;

    assign HI_out = hi_reg;
    assign LO_out = lo_reg;

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            hi_reg <= 32'b0;
            lo_reg <= 32'b0;
        end
        else begin
            if (HI_w)
                hi_reg <= HI_in;
            if (LO_w)
                lo_reg <= LO_in;
        end
    end

endmodule
