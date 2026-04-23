module lfsr8_gen(
  input      [7:0] s,
  output     [7:0] s_next
);

/* P(x) = x^8 + x^6 + x^5 + x^4 + 1 
 * tap: bit 6, 5, 4, 0 
 */

wire fb = s[7];
wire [7:0] mask  = {1'b0, fb, fb, fb, 1'b0, 1'b0, 1'b0, fb};
wire [7:0] shift = {s[6:0], 1'b0};

assign s_next = (s != 0) ? (shift ^ mask) : 8'b1;

endmodule

module lfsr8 (
  input        clock,
  input        reset,
  input        stall,
  input  [7:0] init,
  output [7:0] out
);

reg  [7:0] lfsr;
wire [7:0] lfsr_next;

assign out = lfsr;

lfsr8_gen u_lfsr8_gen(
  .s      (lfsr),
  .s_next (lfsr_next)
);

always @(posedge clock) begin
  if (reset) begin
    lfsr <= (init != 0) ? init : 8'b1;
  end
  else if (!stall) begin
    lfsr <= (init != 0) ? init : lfsr_next;
  end
  else begin
    lfsr <= lfsr;
  end
end

endmodule
