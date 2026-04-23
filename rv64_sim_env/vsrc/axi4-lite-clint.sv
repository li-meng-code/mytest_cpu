module axi4lite_clint #(addr_width=32, data_width=32, data_bytes=4)
(
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

/* this can be used as an axi4-lite template */
/* it decouples handshakes and data transmits */

/* mtime definition */
localparam MTIME_BASE  = 32'h02000000;

reg [31:0] mtime_h;
reg [31:0] mtime_l;

reg [31:0] mtime_l_last;

always @(posedge clock) begin
  if (reset) begin
    mtime_h <= 0;
    mtime_l <= 0;
  end
  else begin
    mtime_l <= mtime_l + 1;
    mtime_h <= mtime_h + {31'b0, &mtime_l};
  end
end

/* axi4 write/read logic */
assign bresp = 2'b0;
assign rresp = 2'b0;

reg araddr_caught;
reg awaddr_caught;
reg wdata_caught;

reg rdata_valid_reg;
reg bresp_valid_reg;

reg [addr_width-1:0] awaddr_reg;
reg [addr_width-1:0] araddr_reg;

reg [addr_width-1:0] rdata_reg;

reg [data_width-1:0] wdata_reg;
reg [data_bytes-1:0] wstrb_reg;

assign arready = !araddr_caught;
assign awready = !awaddr_caught;
assign wready  = !wdata_caught;

assign rvalid  = rdata_valid_reg; 
assign bvalid  = bresp_valid_reg;

assign rdata = rdata_reg;

/*********** handshake-read *************/
always @(posedge clock) begin
  if (reset) begin
    araddr_reg      <= 0;
    araddr_caught   <= 0;
    rdata_valid_reg <= 0;
  end
  else begin
    if (arvalid && arready) begin
      araddr_reg    <= araddr;
      araddr_caught <= 1;
    end
    else if (araddr_caught && !rdata_valid_reg) begin /* at the same time, rdata_reg is set */
      rdata_valid_reg <= 1;
    end
    else if (rready && rvalid) begin
      rdata_valid_reg <= 0;
      araddr_caught   <= 0; /* allow to receive next araddr */
    end
  end
end

/*********** handshake-write ************/
always @ (posedge clock) begin
  if (reset) begin
    awaddr_reg      <= 0;
    awaddr_caught   <= 0;

    wdata_reg       <= 0;
    wstrb_reg       <= 0;
    wdata_caught    <= 0;

    bresp_valid_reg <= 0;
  end
  else begin
    if (awready && awvalid && wready && wvalid) begin
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
    else if (awaddr_caught && wdata_caught && !bresp_valid_reg) begin
      bresp_valid_reg <= 1;
    end
    else if (bvalid && bready) begin /* clear status */
      awaddr_caught   <= 0;
      wdata_caught    <= 0;
      bresp_valid_reg <= 0;
    end
  end
end

/************** read data ****************/
reg l_last_updated;
always @(posedge clock) begin
  if (reset) begin
    mtime_l_last   <= 0; 
    rdata_reg      <= 0;
    l_last_updated <= 0;
  end
  else begin
    if (araddr_caught) begin
      if (araddr_reg == MTIME_BASE) begin
        rdata_reg <= mtime_l_last; 
        l_last_updated <= 0;
      end
      else if (araddr_reg == MTIME_BASE + 32'h4) begin
        /* update on reading high bits */
        rdata_reg    <= mtime_h;
        if (!l_last_updated) begin 
          mtime_l_last <= mtime_l; 
          l_last_updated <= 1;
        end
      end
      else begin
        $display("[mtime error] wrong address to read: 0x%08x", araddr_reg);
      end
    end
  end
end

/************** write data ****************/
always @(posedge clock) begin
  if (awaddr_caught && wdata_caught) begin
    $display("[mtime error] not supported write yet.");
  end
end

endmodule
