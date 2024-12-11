module fifo_serial_link_wrapper #(
    //parameter type         obi_req_t  = logic,
    //parameter type         obi_rsp_t = logic,

    // Parameters of Axi Slave Bus Interface S00_AXI
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 32,
    parameter FIFO_DEPTH = 8

) (
    //input   obi_req_t                       writer_req,
    //output  obi_rsp_t                       writer_rsp,
    //input   obi_req_t                       reader_req,
    //output  obi_rsp_t                       reader_rsp,

    input logic testmode_i,

    input  logic                  reader_req_i,
    output logic                  reader_gnt_o,
    output logic                  reader_rvalid_o,
    input  logic [ADDR_WIDTH-1:0] reader_addr_i,    //
    input  logic                  reader_we_i,
    input  logic [           3:0] reader_be_i,
    output logic [DATA_WIDTH-1:0] reader_rdata_o,
    input  logic [DATA_WIDTH-1:0] reader_wdata_i,

    // request from the serial link
    input  logic                  writer_req_i,     // req.w_valid
    output logic                  writer_gnt_o,
    output logic                  writer_rvalid_o,  // rsp.b_valid
    input  logic [ADDR_WIDTH-1:0] writer_addr_i,    // req.ar.addr
    input  logic                  writer_we_i,      // 1
    input  logic [           3:0] writer_be_i,      // 0
    output logic [DATA_WIDTH-1:0] writer_rdata_o,   // rsp.r.data
    input  logic [DATA_WIDTH-1:0] writer_wdata_i,   // req.w.data

    output logic fifo_empty_o,
    output logic fifo_full_o,

    input logic clk_i,
    input logic rst_ni
);

  logic push, pop, full, empty;
  logic [DATA_WIDTH-1:0] reader_rdata_n;

  assign writer_rdata_o = 0;
  // assign writer_rvalid_o = 1;

  assign writer_gnt_o = '0;  //~full;
  assign reader_gnt_o = ~empty;

  assign push = (~full) & writer_req_i & (writer_we_i);
  assign pop = (~empty) & reader_req_i & (~reader_we_i);

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      writer_rvalid_o <= 0;
      reader_rvalid_o <= 0;
      reader_rdata_o  <= 0;
    end else begin
      writer_rvalid_o <= push;
      reader_rvalid_o <= pop;
      reader_rdata_o  <= reader_rdata_n;
    end
  end

  //   assign fast_sl_rsp_O.ar_ready = 1;
  // assign fast_sl_rsp_O.aw_ready = 1;
  // assign fast_sl_rsp_O.w_ready = 1;


  // fifo_v3 #(
  //     .DATA_WIDTH(DATA_WIDTH),
  //     .DEPTH(FIFO_DEPTH)
  // ) fifo_i (
  //     .clk_i     (clk_i),           // Clock
  //     .rst_ni    (rst_ni),          // Asynchronous reset active low
  //     .flush_i   ('0),               // flush the queue
  //     .testmode_i('0),                // test_mode to bypass clock gating
  //     // status flags
  //     .full_o    (full),            // queue is full
  //     .empty_o   (empty),           // queue is empty
  //     .usage_o   (),                // fill pointer
  //     // as long as the queue is not full we can push new data
  //     .data_i    (writer_wdata_i),  // data to push into the queue
  //     .push_i    (push),            // data is valid and can be pushed to the queue
  //     // as long as the queue is not empty we can pop new elements
  //     .data_o    (reader_rdata_n),  // output data
  //     .pop_i     (pop)              // pop head from queue
  // );


  enum logic [2:0] {
    IDLE,
    DATA1,
    PUSH1,
    DATA2,
    PUSH2,
    WAIT1,
    WAIT2
  }
      CS, NS;

  always_ff @(posedge clk_i or negedge rst_ni) begin : FSM
    if (!rst_ni) begin
      CS <= IDLE;
    end else begin
      CS <= NS;
    end
  end

  logic [3:0] count;
  always_ff @(posedge clk_i or negedge rst_ni) begin : COUNTER
    if (!rst_ni) begin
      count <= 4'b0;
    end else begin
      if (CS == PUSH1 || CS == PUSH2) count <= 0;
      else count <= count + 1;
    end
  end

  always_comb begin
    case (CS)
      IDLE: NS = (count == 4'b1111) ? DATA1 : IDLE;
      DATA1: NS = (count == 4'b1111) ? PUSH1 : DATA1;
      PUSH1: NS = DATA2;
      DATA2: NS = (count == 4'b1111) ? PUSH2 : DATA2;
      PUSH2: NS = WAIT1;
      WAIT1: NS = (pop == 1) ? WAIT2 : WAIT1;
      WAIT2: NS = (pop == 1) ? IDLE : WAIT2;
      default: NS = IDLE;
    endcase
  end

  logic push_fsm;
  logic [31:0] wdata_fsm;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      push_fsm  <= 0;
      wdata_fsm <= 32'b0;
    end else begin
      push_fsm <= (NS == PUSH1 || NS == PUSH2) ? 1 : 0;
      wdata_fsm <= ((CS == IDLE && NS == DATA1) || (CS == PUSH1 && NS == DATA2)) ? (wdata_fsm + 1) : wdata_fsm;
    end
  end


  logic push_fifo;
  logic [31:0] wdata_fifo;

  assign push_fifo  = (testmode_i == 1) ? push_fsm : push;
  assign wdata_fifo = (testmode_i == 1) ? wdata_fsm : writer_wdata_i;

  fifo_v3 #(
      .DATA_WIDTH(DATA_WIDTH),
      .DEPTH(FIFO_DEPTH)
  ) fifo_i (
      .clk_i     (clk_i),           // Clock
      .rst_ni    (rst_ni),          // Asynchronous reset active low
      .flush_i   ('0),              // flush the queue
      .testmode_i('0),              // test_mode to bypass clock gating
      // status flags
      .full_o    (full),            // queue is full
      .empty_o   (empty),           // queue is empty
      .usage_o   (),                // fill pointer
      // as long as the queue is not full we can push new data
      .data_i    (wdata_fifo),      // data to push into the queue
      .push_i    (push_fifo),       // data is valid and can be pushed to the queue
      // as long as the queue is not empty we can pop new elements
      .data_o    (reader_rdata_n),  // output data
      .pop_i     (pop)              // pop head from queue
  );

  assign fifo_empty_o = empty;
  assign fifo_full_o  = full;

endmodule


