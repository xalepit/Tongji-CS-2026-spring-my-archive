`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/05/18 19:06:14
// Design Name: 
// Module Name: sccomp_board
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

module sccomp_board(
    input clk_in,
    input reset,
    output [7:0] o_seg,
    output [7:0] o_sel
    );

    wire [31:0] inst;
    wire [31:0] pc;

    reg [25:0] div_cnt;
    reg cpu_clk;

    always @(posedge clk_in or posedge reset) begin
        if (reset) begin
            div_cnt <= 26'b0;
            cpu_clk <= 1'b0;
        end
        else begin
            if (div_cnt == 26'd999_999) begin
                div_cnt <= 26'b0;
                cpu_clk <= ~cpu_clk;
            end
            else begin
                div_cnt <= div_cnt + 1'b1;
            end
        end
    end

    sccomp_dataflow sccomp(
        .clk_in(cpu_clk),
        .reset(reset),
        .inst(inst),
        .pc(pc)
    );

    seg7x16 seg7(
        .clk(clk_in),
        .reset(reset),
        .cs(1'b1),
        .i_data(pc),
        .o_seg(o_seg),
        .o_sel(o_sel)
    );

endmodule
