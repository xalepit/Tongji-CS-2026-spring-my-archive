`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/08/02 14:43:00
// Design Name: 
// Module Name: DIV
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


module DIV(
    input [31:0] A,
    input [31:0] B,
    input div_signed,
    output reg [31:0] R,
    output reg [31:0] Q
    );

    always @(*) begin
        R = 32'b0;
        Q = 32'b0;
        if (B != 32'b0) begin
            if (div_signed) begin
                Q = $signed(A) / $signed(B);
                R = $signed(A) % $signed(B);
            end
            else begin
                Q = A / B;
                R = A % B;
            end
        end
    end

endmodule
