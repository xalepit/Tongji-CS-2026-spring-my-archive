`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/04/02 14:56:17
// Design Name: 
// Module Name: MULT
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


module MULT(
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

    reg sign1;
    reg sign2;
    reg sign3;
    reg sign4;
    reg sign5;

    wire sign;
    wire [31:0] abs_a;
    wire [31:0] abs_b;
    wire [63:0] unsigned_result;

    integer i;

    assign sign = a[31] ^ b[31];
    assign abs_a = a[31] ? (~a + 1'b1) : a;
    assign abs_b = b[31] ? (~b + 1'b1) : b;
    assign unsigned_result = sum16[0] + sum16[1];

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            temp <= 64'b0;
            sign1 <= 1'b0;
            sign2 <= 1'b0;
            sign3 <= 1'b0;
            sign4 <= 1'b0;
            sign5 <= 1'b0;

            for (i = 0; i < 32; i = i + 1)
                stored[i] <= 64'b0;

            for (i = 0; i < 16; i = i + 1)
                sum2[i] <= 64'b0;

            for (i = 0; i < 8; i = i + 1)
                sum4[i] <= 64'b0;

            for (i = 0; i < 4; i = i + 1)
                sum8[i] <= 64'b0;

            for (i = 0; i < 2; i = i + 1)
                sum16[i] <= 64'b0;
        end
        else begin
            sign1 <= sign;
            sign2 <= sign1;
            sign3 <= sign2;
            sign4 <= sign3;
            sign5 <= sign4;

            for (i = 0; i < 32; i = i + 1)
                stored[i] <= abs_b[i] ? ({32'b0, abs_a} << i) : 64'b0;

            for (i = 0; i < 16; i = i + 1)
                sum2[i] <= stored[2 * i] + stored[2 * i + 1];

            for (i = 0; i < 8; i = i + 1)
                sum4[i] <= sum2[2 * i] + sum2[2 * i + 1];

            for (i = 0; i < 4; i = i + 1)
                sum8[i] <= sum4[2 * i] + sum4[2 * i + 1];

            for (i = 0; i < 2; i = i + 1)
                sum16[i] <= sum8[2 * i] + sum8[2 * i + 1];

            temp <= sign5 ? (~unsigned_result + 1'b1) : unsigned_result;
        end
    end

    assign z = temp;

endmodule
