`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/04/02 15:04:47
// Design Name: 
// Module Name: MULT_tb
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


module MULT_tb();
    reg clk;
    reg reset;
    reg [31:0] a;
    reg [31:0] b;
    wire [63:0] z;
    
    MULT uut (
        .clk(clk),
        .reset(reset),
        .a(a),
        .b(b),
        .z(z)
    );
    
    initial begin
        clk = 0;
        forever #2 clk = ~clk;
    end
    
    initial begin
        reset = 1;
        a = 32'b0;
        b = 32'b0;
    
        #20;
        reset = 0;
    
        a = 32'h00000000; 
        b = 32'h00000000;
    
        #50;
        a = 32'h00000000; 
        b = 32'hFFFFFFFF;
    
        #50;
        a = 32'hFFFFFFFF; 
        b = 32'h00000000;
    
        #50;
        a = 32'hFFFFFFFF; 
        b = 32'hFFFFFFFF;
    
        #50;
        a = 32'h80000000; 
        b = 32'hAAAAAAAA;
    
        #50;
        a = 32'hAAAAAAAA; 
        b = 32'h80000000;
    
        #50;
        a = 32'h00000001; 
        b = 32'h00000001;
    
        #50;
        a = 32'hFFFFFFFF; 
        b = 32'h00000001;
    
        #50;
        a = 32'h00000001; 
        b = 32'hFFFFFFFF;
    
        #50;
        a = 32'h80000000; 
        b = 32'h80000000;
    
        #50;
        a = 32'h0000002D; 
        b = 32'h00000068;
    
        #50;
        a = 32'h12345678; 
        b = 32'h87654321;
    
        #50;
        a = 32'h55555555; 
        b = 32'hAAAAAAAA;
    
        #50;
        a = 32'h7FFFFFFF; 
        b = 32'h00000002;
    
        #50;
        a = 32'hFFFFFFFE; 
        b = 32'h00000002;
    
        #50;
        a = 32'h00000000;
        b = 32'h00000000;
    end

endmodule
