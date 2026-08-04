`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/08/02 14:42:48
// Design Name: 
// Module Name: CLZ
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


module CLZ(
    input [31:0] CLZ_in,
    output reg [31:0] CLZ_out
    );

    integer i;
    reg found_one;

    always @(*) begin
        CLZ_out = 32'b0;
        found_one = 1'b0;
        for (i = 31; i >= 0; i = i - 1) begin
            if (!found_one) begin
                if (CLZ_in[i])
                    found_one = 1'b1;
                else
                    CLZ_out = CLZ_out + 1'b1;
            end
        end
    end

endmodule
