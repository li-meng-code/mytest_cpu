module axi4_lite_sram #(
  parameter int addr_width = 64,
  parameter int data_width = 64,
  parameter int data_bytes = data_width / 8
) (
  /** AR channel */
  input  wire [addr_width-1:0] araddr,
  input  wire                  arvalid,
  output wire                  arready,

  /** R channel */
  output wire [data_width-1:0] rdata,
  output wire [1:0]            rresp,
  output wire                  rvalid,
  input  wire                  rready,

  /** AW channel */
  input  wire [addr_width-1:0] awaddr,
  input  wire                  awvalid,
  output wire                  awready,

  /** W channel */
  input  wire [data_width-1:0] wdata,
  input  wire [data_bytes-1:0] wstrb,
  input  wire                  wvalid,
  output wire                  wready,

  /** B channel */
  output wire [1:0]            bresp,
  output wire                  bvalid,
  input  wire                  bready,

  /** Clock & Reset */
  input  wire                  clock,
  input  wire                  reset
);

  import "DPI-C" pure function longint unsigned sim_mem_read(
    input longint unsigned addr,
    input int len
  );

  import "DPI-C" function void sim_mem_write(
    input longint unsigned addr,
    input longint unsigned wmask,
    input longint unsigned data
  );

  localparam int ADDR_LSB = $clog2(data_bytes);

  logic [addr_width-1:0] araddr_r;
  logic                  araddr_caught_r;
  logic [data_width-1:0] rdata_r;
  logic                  rdata_valid_r;

  logic [addr_width-1:0] awaddr_r;
  logic                  awaddr_caught_r;
  logic [data_width-1:0] wdata_r;
  logic                  wdata_caught_r;
  logic                  bresp_valid_r;
  logic [data_width-1:0] wmask_r;
  logic [data_width-1:0] wmask;

  wire [addr_width-1:0] araddr_aligned = {araddr_r[addr_width-1:ADDR_LSB], {ADDR_LSB{1'b0}}};
  wire [addr_width-1:0] awaddr_aligned = {awaddr_r[addr_width-1:ADDR_LSB], {ADDR_LSB{1'b0}}};

  genvar i;
  generate
    for (i = 0; i < data_bytes; i++) begin : gen_wmask
      assign wmask[i*8 +: 8] = wstrb[i] ? 8'hff : 8'h00;
    end
  endgenerate

  /************************ random delay *************************/
  //`define RANDOM_DELAY
`ifdef RANDOM_DELAY
  logic [3:0] read_delay;
  logic [3:0] write_delay;
  logic [3:0] rlfsr;
  logic [3:0] wlfsr;

  lfsr4 u_lfsr4_read (
    .clock (clock),
    .reset (reset),
    .init  (4'b0),
    .stall (!(arvalid && arready)),
    .out   (rlfsr)
  );

  lfsr4 u_lfsr4_write (
    .clock (clock),
    .reset (reset),
    .init  (4'b0),
    .stall (!(awvalid && awready)),
    .out   (wlfsr)
  );
`endif

  /************************ axi4-lite read ***********************/
  assign rresp   = 2'b00;
  assign rvalid  = rdata_valid_r;
  assign rdata   = rdata_r;
  assign arready = !araddr_caught_r && !rdata_valid_r;  // only allow one outstanding read response

  always @(posedge clock) begin
    if (reset) begin
      araddr_caught_r <= 1'b0;
      rdata_valid_r   <= 1'b0;
      araddr_r        <= '0;
      rdata_r         <= '0;
`ifdef RANDOM_DELAY
      read_delay      <= '0;
`endif
    end
    else begin
      if (arvalid && arready) begin
        araddr_r        <= araddr;
        araddr_caught_r <= 1'b1;
`ifdef RANDOM_DELAY
        read_delay      <= rlfsr;
`endif
      end

      if (araddr_caught_r) begin
`ifdef RANDOM_DELAY
        if (read_delay > 0) begin
          read_delay <= read_delay - 1'b1;
        end
        else begin
`endif
          rdata_r         <= sim_mem_read(araddr_aligned, data_bytes);
          araddr_caught_r <= 1'b0;
          rdata_valid_r   <= 1'b1;
`ifdef RANDOM_DELAY
        end
`endif
      end

      if (rdata_valid_r && rready) begin
        rdata_valid_r <= 1'b0;
      end
    end
  end

  /************************ axi4-lite write ***********************/
  assign bvalid  = bresp_valid_r;
  assign bresp   = 2'b00;
  assign awready = !awaddr_caught_r && !bresp_valid_r;  // only allow one outstanding write response
  assign wready  = !wdata_caught_r && !bresp_valid_r;

  always @(posedge clock) begin
    if (reset) begin
      awaddr_r        <= '0;
      wdata_r         <= '0;
      wmask_r         <= '0;
      awaddr_caught_r <= 1'b0;
      wdata_caught_r  <= 1'b0;
      bresp_valid_r   <= 1'b0;
`ifdef RANDOM_DELAY
      write_delay     <= '0;
`endif
    end
    else begin
      if (awvalid && awready) begin
        awaddr_r        <= awaddr;
        awaddr_caught_r <= 1'b1;
`ifdef RANDOM_DELAY
        write_delay     <= wlfsr;
`endif
      end

      if (wvalid && wready) begin
        wdata_r        <= wdata;
        wmask_r        <= wmask;
        wdata_caught_r <= 1'b1;
      end

      if (awaddr_caught_r && wdata_caught_r) begin
`ifdef RANDOM_DELAY
        if (write_delay > 0) begin
          write_delay <= write_delay - 1'b1;
        end
        else begin
`endif
          sim_mem_write(awaddr_aligned, wmask_r, wdata_r);
          awaddr_caught_r <= 1'b0;
          wdata_caught_r  <= 1'b0;
          bresp_valid_r   <= 1'b1;
`ifdef RANDOM_DELAY
        end
`endif
      end

      if (bresp_valid_r && bready) begin
        bresp_valid_r <= 1'b0;
      end
    end
  end

endmodule