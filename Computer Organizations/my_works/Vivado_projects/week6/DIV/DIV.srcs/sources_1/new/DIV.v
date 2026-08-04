`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/04/07 16:38:09
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
    input [31:0] dividend,
    input [31:0] divisor,
    input start,
    input clock,
    input reset,
    output [31:0] q,
    output [31:0] r,
    output reg busy
    );

    reg [4:0] count;
    reg [31:0] reg_q;
    reg [31:0] reg_r;
    reg [31:0] reg_b;
    reg core_r_sign;
    reg q_sign;
    reg r_sign_out;

    wire [31:0] dividend_abs;
    wire [31:0] divisor_abs;
    wire [32:0] sub_add;
    wire [31:0] r_unsigned;

    assign dividend_abs = dividend[31] ? (~dividend + 32'b1) : dividend;
    assign divisor_abs = divisor[31] ? (~divisor + 32'b1) : divisor;

    assign sub_add = core_r_sign ? ({reg_r, reg_q[31]} + {1'b0, reg_b}) :
                                   ({reg_r, reg_q[31]} - {1'b0, reg_b});

    assign r_unsigned = core_r_sign ? (reg_r + reg_b) : reg_r;

    assign q = q_sign ? (~reg_q + 32'b1) : reg_q;
    assign r = r_sign_out ? (~r_unsigned + 32'b1) : r_unsigned;

    always @(posedge clock or posedge reset) begin
        if (reset == 1'b1) begin
            count <= 5'b0;
            reg_q <= 32'b0;
            reg_r <= 32'b0;
            reg_b <= 32'b0;
            core_r_sign <= 1'b0;
            q_sign <= 1'b0;
            r_sign_out <= 1'b0;
            busy <= 1'b0;
        end
        else begin
            if (start) begin
                reg_r <= 32'b0;
                core_r_sign <= 1'b0;
                reg_q <= dividend_abs;
                reg_b <= divisor_abs;
                q_sign <= dividend[31] ^ divisor[31];
                r_sign_out <= dividend[31];
                count <= 5'b0;
                busy <= 1'b1;
            end
            else if (busy) begin
                reg_r <= sub_add[31:0];
                core_r_sign <= sub_add[32];
                reg_q <= {reg_q[30:0], ~sub_add[32]};
                count <= count + 5'b1;
                if (count == 5'd31)
                    busy <= 1'b0;
            end
        end
    end

endmodule
