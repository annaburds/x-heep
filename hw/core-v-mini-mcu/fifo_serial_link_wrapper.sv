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

    input  logic                  reader_req_i,
    output logic                  reader_gnt_o,
    output logic                  reader_rvalid_o,
    input  logic [ADDR_WIDTH-1:0] reader_addr_i,    //
    input  logic                  reader_we_i,
    input  logic [           3:0] reader_be_i,
    output logic [DATA_WIDTH-1:0] reader_rdata_o,
    input  logic [DATA_WIDTH-1:0] reader_wdata_i,

    input  logic                  writer_req_i,     // request from the serial link
    output logic                  writer_gnt_o,
    output logic                  writer_rvalid_o,
    input  logic [ADDR_WIDTH-1:0] writer_addr_i,    //
    input  logic                  writer_we_i,
    input  logic [           3:0] writer_be_i,
    output logic [DATA_WIDTH-1:0] writer_rdata_o,
    input  logic [DATA_WIDTH-1:0] writer_wdata_i,

    input logic clk_i,
    input logic rst_ni
);

  logic push, pop, full, empty;
  logic writer_rvalid_n;
  logic reader_rvalid_n;
  logic [DATA_WIDTH-1:0] reader_rdata_n;

  assign writer_rdata_o = 0;
  // assign writer_rvalid_o = 1;

  assign writer_gnt_o = ~full;
  assign reader_gnt_o = ~empty;

  assign push = (~full) & writer_req_i & (writer_we_i);
  assign pop = (~empty) & reader_req_i & (~reader_we_i);

  assign writer_rvalid_n = push;
  assign reader_rvalid_n = pop;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      writer_rvalid_o <= 0;
      reader_rvalid_o <= 0;
      reader_rdata_o  <= 0;
    end else begin
      writer_rvalid_o <= writer_rvalid_n;
      reader_rvalid_o <= reader_rvalid_n;
      reader_rdata_o  <= reader_rdata_n;
    end
  end

  fifo_v3 #(
      .DATA_WIDTH(DATA_WIDTH),
      .DEPTH(FIFO_DEPTH)
  ) fifo_i (
      .clk_i     (clk_i),           // Clock
      .rst_ni    (rst_ni),          // Asynchronous reset active low
      .flush_i   (),                // flush the queue
      .testmode_i(),                // test_mode to bypass clock gating
      // status flags
      .full_o    (full),            // queue is full
      .empty_o   (empty),           // queue is empty
      .usage_o   (),                // fill pointer
      // as long as the queue is not full we can push new data
      .data_i    (writer_wdata_i),  // data to push into the queue
      .push_i    (push),            // data is valid and can be pushed to the queue
      // as long as the queue is not empty we can pop new elements
      .data_o    (reader_rdata_n),  // output data
      .pop_i     (pop)              // pop head from queue
  );


endmodule



