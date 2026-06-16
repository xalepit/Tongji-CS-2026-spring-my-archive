`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/04/07 16:01:08
// Design Name: 
// Module Name: DIVU_tb
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


module DIVU_tb();
    reg [31:0] dividend;
    reg [31:0] divisor;
    reg start;
    reg clock;
    reg reset;
    wire [31:0] q;
    wire [31:0] r;
    wire busy;
    
    reg [31:0] dividend_set [0:4];
    reg [31:0] divisor_set [0:3];
    integer i;
    integer j;
    
    DIVU uut(
        .dividend(dividend),
        .divisor(divisor),
        .start(start),
        .clock(clock),
        .reset(reset),
        .q(q),
        .r(r),
        .busy(busy)
    );
    
    always #1 clock = ~clock;
    
    initial begin
        clock = 0;
        reset = 1;
        start = 0;
        dividend = 0;
        divisor = 0;
    
        dividend_set[0] = 32'h00000000;
        dividend_set[1] = 32'hffffffff;
        dividend_set[2] = 32'haaaaaaaa;
        dividend_set[3] = 32'h55555555;
        dividend_set[4] = 32'h7fffffff;
    
        divisor_set[0] = 32'hffffffff;
        divisor_set[1] = 32'haaaaaaaa;
        divisor_set[2] = 32'h55555555;
        divisor_set[3] = 32'h7fffffff;
    
        #25 reset = 0;
    
        for (i = 0; i < 5; i = i + 1) begin
            for (j = 0; j < 4; j = j + 1) begin
                dividend = dividend_set[i];
                divisor = divisor_set[j];
                start = 1;
                #20 start = 0;
                wait(busy == 0);
                #20;
            end
        end
    
        #20;
        $finish;
    end

endmodule
