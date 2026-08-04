`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/08/02 14:43:09
// Design Name: 
// Module Name: MUL
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


module MUL(
    input [31:0] A,
    input [31:0] B,
    input mul_signed,
    output [31:0] HI,
    output [31:0] LO
    );

    wire signed [63:0] signed_product;
    wire [63:0] unsigned_product;
    wire [63:0] product;

    assign signed_product = $signed(A) * $signed(B);
    assign unsigned_product = A * B;
    assign product = mul_signed ? signed_product : unsigned_product;
    assign HI = product[63:32];
    assign LO = product[31:0];

endmodule
