module axi4lite_uart #(
  addr_width = 32, 
  data_width = 32,
  data_bytes = 32
) (
  /** AR channel */
  input  wire [addr_width-1:0]  araddr,
  input  wire                   arvalid,
  output wire                   arready,

  /** R channel */
  output wire [data_width-1:0]  rdata,
  output wire [1:0]             rresp,
  output wire                   rvalid,
  input  wire                   rready,

  /** AW channel */
  input  wire [addr_width-1:0]  awaddr,
  input  wire                   awvalid,
  output wire                   awready,

  /** W channel */
  input  wire [data_width-1:0]  wdata,
  input  wire [data_bytes-1:0]  wstrb,
  input  wire                   wvalid,
  output wire                   wready,

  /** B channel */
  output wire [1:0]             bresp,
  output wire                   bvalid,
  input  wire                   bready,

  /** Clock & Reset */
  input  wire                   clock,
  input  wire                   reset
);

reg araddr_caught;
reg awaddr_caught;
reg wdata_caught;

reg rdata_sent;

reg [addr_width-1:0] awaddr_reg;
reg [data_width-1:0] wdata_reg;
reg [data_bytes-1:0] wstrb_reg;

assign arready = !araddr_caught;
assign awready = !awaddr_caught;
assign wready  = !wdata_caught;

assign bvalid = awaddr_caught && wdata_caught;
assign rvalid = araddr_caught;

assign rdata = 32'b0;
assign bresp = 2'b0;
assign rresp = 2'b0;

always @(posedge clock) begin
  if (reset) begin
    araddr_caught <= 0;
    awaddr_caught <= 0;
    wdata_caught  <= 0;
    wstrb_reg     <= 0;
    awaddr_reg    <= 0;
    wdata_reg     <= 0;
  end
  else begin
    if (arready && arvalid) begin
      /* so far we don't have anything to ready */
      araddr_caught <= 1;
    end
    else if (awready && awvalid && wready && wvalid) begin
      awaddr_reg    <= awaddr;
      wdata_reg     <= wdata;
      wstrb_reg     <= wstrb;
      awaddr_caught <= 1;
      wdata_caught  <= 1;
    end
    else if (awready && awvalid) begin
      awaddr_reg    <= awaddr;
      awaddr_caught <= 1;
    end
    else if (wready && wvalid) begin
      wdata_reg    <= wdata;
      wstrb_reg    <= wstrb;
      wdata_caught <= 1;
    end
    else if (rvalid && rready) begin
      araddr_caught <= 0;
    end
    else if (bvalid && bready) begin
      awaddr_caught <= 0;
      wdata_caught  <= 0;
      wstrb_reg     <= 0;
    end
  end
end

reg txDone;

always @(posedge clock) begin
  if (reset) begin 
    txDone <= 0;
  end
  else if (!reset) begin
    if (araddr_caught) begin
      $display("[uart warn] reading is not supported!");  
    end
    else if (awaddr_caught && wdata_caught && !txDone) begin
      if (wstrb[0]) begin 
        $write("%c", wdata_reg[7 : 0]);
        // $fflush();
      end
      else $display("[uart error] wrong wstrb: %4b", wstrb);
      txDone <= 1;
    end
    else if (!awaddr_caught || !wdata_caught) begin
      txDone <= 0;
    end
  end
end

endmodule
