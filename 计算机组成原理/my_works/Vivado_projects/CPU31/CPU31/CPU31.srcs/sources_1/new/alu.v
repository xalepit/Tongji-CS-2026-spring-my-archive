`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/05/10 22:19:48
// Design Name: 
// Module Name: alu
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

module alu(
    input [31:0] a,
    input [31:0] b,
    input [3:0] aluc,
    output reg [31:0] r,
    output reg zero,
    output reg carry,
    output reg negative,
    output reg overflow
    );
    wire [31:0] o_addu = a + b;
    wire [31:0] o_add = a + b;
    wire [31:0] o_subu = a - b;
    wire [31:0] o_sub = a - b;
    wire [31:0] o_and = a & b;
    wire [31:0] o_or = a | b;
    wire [31:0] o_xor = a ^ b;
    wire [31:0] o_nor = ~(a | b);
    wire [31:0] o_lui = { b[15:0] , 16'b0};
    wire [31:0] o_slt = ($signed(a) < $signed(b)) ? 1 : 0;
    wire [31:0] o_sltu = (a < b) ? 1 : 0;
    wire [31:0] o_sra = $signed(b) >>> a[4:0];
    wire [31:0] o_srl = b >> a[4:0];
    wire [31:0] o_sll = b << a[4:0];
    wire [32:0] add_tmp = {1'b0, a} + {1'b0, b};
    wire [32:0] sub_tmp = {1'b0, a} - {1'b0, b};
    
    always @(*) begin
        carry = 0;
        negative = 0;
        overflow = 0;
        case (aluc)
            4'b0000: begin
                r = o_addu;
                carry = add_tmp[32];
            end
            4'b0010: begin 
                r = o_add; 
                overflow = (a[31] == b[31]) && (r[31] != a[31]);
            end
            4'b0001: begin
                r = o_subu; 
                carry = sub_tmp[32];
            end
            4'b0011: begin 
                r = o_sub; 
                overflow = (a[31] != b[31]) && (r[31] != a[31]);
            end
            
            4'b0100: r = o_and;
            4'b0101: r = o_or;
            4'b0110: r = o_xor;
            4'b0111: r = o_nor;
            
            4'b1000,4'b1001: r = o_lui;
            4'b1011: r = o_slt;

            4'b1010: begin 
                r = o_sltu; 
                carry = (a < b);
            end
            
            4'b1110,4'b1111: begin
                r = o_sll;
                if (a[4:0] != 0)
                    carry = b[32 - a[4:0]];
                else
                    carry = 0;
            end
            4'b1101: begin 
                r = o_srl;
                if (a[4:0] != 0)
                    carry = b[a[4:0] - 1];
                else
                    carry = 0;
            end
            4'b1100: begin 
                r = o_sra;
                if (a[4:0] != 0)
                    carry = b[a[4:0] - 1];
                else
                    carry = 0;
            end
            default: r = 32'b0;
        endcase
        
        if(aluc == 4'b1011)  zero = ($signed(a) == $signed(b));
        else if (aluc == 4'b1010) zero = (a == b);
        else zero = (r == 0);
        
        if(aluc == 4'b1011) negative = ($signed(a) < $signed(b));
        else negative = r[31];
    end
endmodule
