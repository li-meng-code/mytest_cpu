module lfsr4_gen (
  input      [3:0] s,
  output     [3:0] s_next
);

/* P(x) = x^4 + x + 1
 * tap: bit 1, 0
 */

wire fb = s[3];
wire [3:0] mask  = {1'b0, 1'b0, fb, fb};
wire [3:0] shift = {s[2:0], 1'b0};

assign s_next = (s != 0) ? (shift ^ mask) : 4'b1;

endmodule

module lfsr4 (
  input        clock,
  input        reset,
  input        stall,
  input  [3:0] init,
  output [3:0] out
);

reg  [3:0] lfsr;
wire [3:0] lfsr_next;

assign out = lfsr;

lfsr4_gen u_lfsr4_gen(
  .s      (lfsr),
  .s_next (lfsr_next)
);

always @(posedge clock) begin
  if (reset) begin
    lfsr <= (init != 0) ? init : 4'b1;
  end
  else if (!stall) begin
    lfsr <= (init != 0) ? init : lfsr_next;
  end
  else begin
    lfsr <= lfsr;
  end
end

endmodule
