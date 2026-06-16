`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/04/01 08:06:33
// Design Name: 
// Module Name: MULTU
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


module MULTU(
    input clk,
    input reset,
    input [31:0] a,
    input [31:0] b,
    output [63:0] z
    );
    reg [63:0] stored [0:31];
    reg [63:0] sum2 [0:15];
    reg [63:0] sum4 [0:7];
    reg [63:0] sum8 [0:3];
    reg [63:0] sum16 [0:1];
    reg [63:0] temp;
    
    integer i;
    
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            temp <= 64'b0;
            
            for (i = 0; i < 32; i = i + 1)
                stored[i] <=  64'b0;
                
            for (i = 0; i < 16; i = i + 1)
                sum2[i] <=  64'b0;
                
            for (i = 0; i < 8; i = i + 1)
                sum4[i] <=  64'b0;   
                
            for (i = 0; i < 4; i = i + 1)
                sum8[i] <=  64'b0;    
                
            for (i = 0; i < 2; i = i + 1)
                sum16[i] <=  64'b0;   
        end
        else begin
            for (i = 0; i < 32; i = i + 1)
                stored[i] <=  b[i] ? ({32'b0, a} << i) : 64'b0;
                
            for (i = 0; i < 16; i = i + 1)
                sum2[i] <=  stored[2 * i] + stored[2 * i + 1];
                
            for (i = 0; i < 8; i = i + 1)
                sum4[i] <= sum2[2 * i] + sum2[2 * i + 1];   
                
            for (i = 0; i < 4; i = i + 1)
                sum8[i] <= sum4[2 * i] + sum4[2 * i + 1];      
                
            for (i = 0; i < 2; i = i + 1)
                sum16[i] <= sum8[2 * i] + sum8[2 * i + 1];   
                
            temp <= sum16[0] + sum16[1];
        end
    end
    
    assign z = temp;
endmodule


