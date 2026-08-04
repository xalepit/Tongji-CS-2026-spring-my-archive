`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/07/31 09:45:18
// Design Name: 
// Module Name: IMEM
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


module IMEM(
    input [10:0] im_addr_in,
    output [31:0] im_inst_out
    );
//    reg [31:0] ram [0:2047];
//    integer i;

//    initial begin
//        for (i = 0; i < 2048; i = i + 1)
//            ram[i] = 32'h0000_0000;

//        $readmemh("D:/CPU54_test/txt/CP0test.hex.txt", ram); // 要写绝对路径
//    end

//    assign im_inst_out = ram[im_addr_in];
    dist_mem_gen_0 imem(
        .a(im_addr_in),  
        .spo(im_inst_out)
    );
endmodule
