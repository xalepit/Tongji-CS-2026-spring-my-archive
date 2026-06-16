`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/05/17 14:38:37
// Design Name: 
// Module Name: cpu_tb
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


module cpu_tb();

    reg clk;
    reg reset;
    wire [31:0] inst;
    wire [31:0] pc;

    sccomp_dataflow sc_inst(
        .clk_in(clk),
        .reset(reset),
        .inst(inst),
        .pc(pc)
    );

    initial begin
        clk = 1'b0;
        reset = 1'b1;
        #200;
        reset = 1'b0;
        #1000000;
        $finish;
    end

    always #50 clk = ~clk;

//    always @(posedge clk) begin
//        if (!reset) begin
//            #10;
//            if (pc == 32'h00400fe0 && inst == 32'h081003f8) begin
//                #200;
//                $finish;
//            end
//        end
//    end

endmodule
