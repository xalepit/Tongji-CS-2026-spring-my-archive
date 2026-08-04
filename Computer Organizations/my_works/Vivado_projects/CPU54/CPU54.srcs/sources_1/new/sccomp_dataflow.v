`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/07/31 09:38:08
// Design Name: 
// Module Name: sccomp_dataflow
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


module sccomp_dataflow(
    input clk_in,   //上升沿有效
    input reset,    //高电平有效
    output [31:0] inst, //指令
    output [31:0] pc    //pc里的地址
    );
    wire clk;
    wire rst;

    wire [31:0] dm_addr;
    wire [31:0] dm_wdata;
    wire [31:0] dm_rdata;
    wire dm_ena;
    wire dm_r;
    wire dm_w;

    wire [31:0] im_offset;
    wire [10:0] im_addr;

    wire [31:0] dm_offset;
    wire [4:0] dm_addr_word;

    assign clk = clk_in;
    assign rst = reset;

    assign im_offset = pc - 32'h0040_0000;
    assign im_addr = im_offset[12:2];

    assign dm_offset = dm_addr - 32'h1001_0000;
    assign dm_addr_word = dm_offset[6:2];

    cpu sccpu(
        .clk(clk),
        .rst(rst),
        .inst(inst),
        .dm_rdata(dm_rdata),
        .pc(pc),
        .dm_addr(dm_addr),
        .dm_wdata(dm_wdata),
        .dm_ena(dm_ena),
        .dm_r(dm_r),
        .dm_w(dm_w)
    );

    IMEM imem(
        .im_addr_in(im_addr),
        .im_inst_out(inst)
    );

    DMEM dmem(
        .dm_clk(clk),
        .dm_ena(dm_ena),
        .dm_r(dm_r),
        .dm_w(dm_w),
        .dm_addr(dm_addr_word),
        .dm_data_in(dm_wdata),
        .dm_data_out(dm_rdata)
    );
endmodule
