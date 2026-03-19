import obi_pkg::*;

module ebpc_wrapper #(
    parameter FIFO_DEPTH  = 4,
    parameter WORD_LENGTH = 32
) (
    input logic clk_i,
    input logic rst_ni,

    // OBI interface
    input  obi_req_t  obi_req_i,
    output obi_resp_t obi_resp_o,

    // EBPC Encoder interface
    output logic [FIFO_DEPTH*WORD_LENGTH-1:0] bpc_data_o,
    output logic                              bpc_vld_o,
    input  logic                              bpc_rdy_i,

    output logic [FIFO_DEPTH*WORD_LENGTH-1:0] znz_data_o,
    output logic                              znz_vld_o,
    input  logic                              znz_rdy_i,

    output logic idle_o
);

  localparam DATA_W = FIFO_DEPTH * WORD_LENGTH;

  // FIFO signals
  logic [     WORD_LENGTH-1:0] fifo_data_out;
  logic                        fifo_full;
  logic                        fifo_empty;
  logic [$clog2(FIFO_DEPTH):0] fifo_usage;

  // Instantiate fifo_v3
  fifo_v3 #(
      .DATA_WIDTH(WORD_LENGTH),
      .DEPTH(FIFO_DEPTH)
  ) fifo_inst (
      .clk_i     (clk_i),
      .rst_ni    (rst_ni),
      .flush_i   (1'b0),
      .testmode_i(1'b0),
      .full_o    (fifo_full),
      .empty_o   (fifo_empty),
      .usage_o   (fifo_usage),
      .data_i    (obi_req_i.wdata),
      .push_i    (obi_req_i.req && obi_req_i.we && !fifo_full),
      .data_o    (fifo_data_out),
      .pop_i     (0)                                             // We'll use pop later if needed
  );

  // Flatten FIFO contents into DATA_W for ebpc_encoder
  logic [DATA_W-1:0] data_flat;

  // Simple combinational assignment: for simulation we can assume FIFO_DEPTH=DEPTH
  // We'll just replicate fifo_data_out across the bus for now
  genvar i;
  generate
    for (i = 0; i < FIFO_DEPTH; i = i + 1) begin : GEN_FLATTEN
      assign data_flat[(i+1)*WORD_LENGTH-1-:WORD_LENGTH] = fifo_data_out;
    end
  endgenerate

  // EBPC encoder instantiation
  ebpc_encoder u_ebpc_encoder (
      .clk_i     (clk_i),
      .rst_ni    (rst_ni),
      .data_i    (data_flat),
      .last_i    (fifo_full),    // last when FIFO full
      .vld_i     (!fifo_empty),  // valid when FIFO not empty
      .rdy_o     (),             // ignored
      .idle_o    (idle_o),
      .znz_data_o(znz_data_o),
      .znz_vld_o (znz_vld_o),
      .znz_rdy_i (znz_rdy_i),
      .bpc_data_o(bpc_data_o),
      .bpc_vld_o (bpc_vld_o),
      .bpc_rdy_i (bpc_rdy_i)
  );

  // OBI response: keep returning last written data until FIFO is full
  logic [WORD_LENGTH-1:0] last_wdata;
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) last_wdata <= '0;
    else if (obi_req_i.req && obi_req_i.we) last_wdata <= obi_req_i.wdata;
  end

  assign obi_resp_o.gnt    = 1'b1;  // always grant
  assign obi_resp_o.rvalid = 1'b1;
  assign obi_resp_o.rdata  = (!fifo_full) ? last_wdata : fifo_data_out;

endmodule
