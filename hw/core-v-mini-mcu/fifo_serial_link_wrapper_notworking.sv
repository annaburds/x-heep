module fifo_serial_link_wrapper #(
    parameter type axi_req_t   = logic,
    parameter type axi_rsp_t   = logic,

    // AXI data‑path parameters
    parameter int  DATA_WIDTH  = 32,
    parameter int  ADDR_WIDTH  = 32,
    parameter int  FIFO_DEPTH  = 8
)(
    //--------------------------------------------------------------------
    //  Local interface (reader side)
    //--------------------------------------------------------------------
    input  logic                  reader_req_i,
    output logic                  reader_gnt_o,
    output logic                  reader_rvalid_o,
    input  logic [ADDR_WIDTH-1:0] reader_addr_i,
    input  logic                  reader_we_i,
    input  logic [3:0]            reader_be_i,
    output logic [DATA_WIDTH-1:0] reader_rdata_o,
    input  logic [DATA_WIDTH-1:0] reader_wdata_i,

    //--------------------------------------------------------------------
    //  AXI stream coming from the serial link (writer side)
    //--------------------------------------------------------------------
    input  axi_req_t              writer_axi_req,
    output axi_rsp_t              writer_axi_rsp,

    //--------------------------------------------------------------------
    //  Status
    //--------------------------------------------------------------------
    output logic                  fifo_empty_o,
    output logic                  fifo_full_o,

    //--------------------------------------------------------------------
    //  Clock / Reset
    //--------------------------------------------------------------------
    input  logic                  clk_i,
    input  logic                  rst_ni
);

  //------------------------------------------------------------------
  //  Derived local parameters / types
  //------------------------------------------------------------------
  localparam PTR_W = $clog2(FIFO_DEPTH);
  localparam CNT_W = $clog2(FIFO_DEPTH+1);

  //------------------------------------------------------------------
  //  Signals towards the real FIFO
  //------------------------------------------------------------------
  logic push_user, push_refill, push_to_fifo;
  logic pop;
  logic [DATA_WIDTH-1:0] data_to_fifo;
  logic [DATA_WIDTH-1:0] reader_rdata_n;
  logic full, empty;

  //------------------------------------------------------------------
  //  Writer side handshake FSM (unchanged, but gated during REFILL)
  //------------------------------------------------------------------
  enum logic [1:0] { IDLE, WAIT, WREADY, BVALID } state, n_state;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) state <= IDLE;
    else         state <= n_state;
  end

  always_comb begin
    unique case (state)
      IDLE   : n_state = writer_axi_req.aw_valid ? (full ? WAIT : WREADY) : IDLE;
      WAIT   : n_state = full ? WAIT : WREADY;
      WREADY : n_state = writer_axi_req.w_valid ? BVALID : WREADY;
      BVALID : n_state = writer_axi_req.b_ready ? IDLE   : BVALID;
      default: n_state = IDLE;
    endcase
  end

  //------------------------------------------------------------------
  //  Refill control (shadow memory that survives reset)
  //------------------------------------------------------------------
  //  Shadow buffer pointers and counters NEVER reset asynchronously –
  //  they retain their values through rst_ni low (soft reset).
  //------------------------------------------------------------------
  logic [DATA_WIDTH-1:0] shadow_mem [FIFO_DEPTH-1:0];
  logic [PTR_W-1:0]      shadow_wr_ptr;
  logic [PTR_W-1:0]      shadow_rd_ptr;
  logic [CNT_W-1:0]      shadow_count;

  // helper function to wrap pointers
  function automatic logic [PTR_W-1:0] inc_ptr(input logic [PTR_W-1:0] ptr);
    inc_ptr = (ptr == FIFO_DEPTH-1) ? '0 : ptr + 1;
  endfunction

  //------------------------------------------------------------------
  //  Track pushes / pops during normal operation (not during refill)
  //------------------------------------------------------------------
  assign push_user = writer_axi_req.w_valid & writer_axi_rsp.w_ready;  // gated later
  assign pop       = (~empty) & reader_req_i & (~reader_we_i);

  always_ff @(posedge clk_i) begin : shadow_tracker  // **NO async reset here**
    // push path (only normal user pushes – refill pushes come from mem itself)
    if (push_user && !full) begin
      shadow_mem[shadow_wr_ptr] <= writer_axi_req.w.data;
      shadow_wr_ptr             <= inc_ptr(shadow_wr_ptr);
      if (!pop && (shadow_count != FIFO_DEPTH)) shadow_count <= shadow_count + 1;
    end

    // pop path
    if (pop && (shadow_count != 0)) begin
      shadow_rd_ptr <= inc_ptr(shadow_rd_ptr);
      if (!push_user) shadow_count <= shadow_count - 1;
    end
  end

  //------------------------------------------------------------------
  //  Detect reset release (rising edge of rst_ni)
  //------------------------------------------------------------------
  logic rst_ni_d;
  always_ff @(posedge clk_i) rst_ni_d <= rst_ni;
  wire reset_released = rst_ni & ~rst_ni_d; // one‑cycle pulse when reset goes 0 -> 1

  //------------------------------------------------------------------
  //  REFILL FSM – feed back stored data after reset
  //------------------------------------------------------------------
  logic                       refilling;
  logic [PTR_W-1:0]           refill_ptr;
  logic [CNT_W-1:0]           refill_remaining;

  // start refilling as soon as reset is released and we have data
  always_ff @(posedge clk_i) begin
    if (reset_released && (shadow_count != 0)) begin
      refilling        <= 1'b1;
      refill_ptr       <= shadow_rd_ptr;
      refill_remaining <= shadow_count;
    end else if (refilling && push_refill) begin
      refill_ptr       <= inc_ptr(refill_ptr);
      refill_remaining <= refill_remaining - 1;
      if (refill_remaining == 1) refilling <= 1'b0; // last word pushed in this cycle
    end
  end

  // generate push_refill towards the real FIFO
  assign push_refill = refilling & ~full & (refill_remaining != 0);

  //------------------------------------------------------------------
  //  Final multiplexed push/data lines towards fifo_v3
  //------------------------------------------------------------------
  assign push_to_fifo = refilling ? push_refill : push_user;
  assign data_to_fifo = refilling ? shadow_mem[refill_ptr] : writer_axi_req.w.data;

  //------------------------------------------------------------------
  //  Reader side outputs & grants (same as before)
  //------------------------------------------------------------------
  assign reader_gnt_o = ~empty;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      reader_rvalid_o <= 1'b0;
      reader_rdata_o  <= '0;
    end else begin
      reader_rvalid_o <= pop;
      reader_rdata_o  <= reader_rdata_n;
    end
  end

  //------------------------------------------------------------------
  //  AXI response handshake (writer side) – gate aw/w ready during refill
  //------------------------------------------------------------------
  assign writer_axi_rsp.aw_ready = (~refilling) & (state == IDLE);
  assign writer_axi_rsp.w_ready  = (~refilling) & (state == WREADY);
  assign writer_axi_rsp.b_valid  = (state == BVALID);

  //  Fixed, unused AXI read channel signals
  assign writer_axi_rsp.ar_ready = 1'b1;
  assign writer_axi_rsp.r_valid  = 1'b0;
  assign writer_axi_rsp.b.id     = '0;
  assign writer_axi_rsp.b.resp   = '0;
  assign writer_axi_rsp.b.user   = '0;
  assign writer_axi_rsp.r.data   = '0;
  assign writer_axi_rsp.r.id     = '0;
  assign writer_axi_rsp.r.last   = 1'b0;
  assign writer_axi_rsp.r.resp   = '0;
  assign writer_axi_rsp.r.user   = '0;

  //------------------------------------------------------------------
  //  Instantiation of the real FIFO – UNTOUCHED
  //------------------------------------------------------------------
  fifo_v3 #(
      .DATA_WIDTH(DATA_WIDTH),
      .DEPTH      (FIFO_DEPTH)
  ) fifo_i (
      .clk_i     (clk_i),
      .rst_ni    (rst_ni),        //  ***will clear internal memory***
      .flush_i   ('0),            //  never flush from outside
      .testmode_i('0),
      // status flags
      .full_o    (full),
      .empty_o   (empty),
      .usage_o   (),
      // push / pop
      .data_i    (data_to_fifo),
      .push_i    (push_to_fifo),
      .data_o    (reader_rdata_n),
      .pop_i     (pop)
  );

  //------------------------------------------------------------------
  //  Pass‑through flags
  //------------------------------------------------------------------
  assign fifo_empty_o = empty;
  assign fifo_full_o  = full;

endmodule
