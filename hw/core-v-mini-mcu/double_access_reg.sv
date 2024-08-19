

// This model implements double access register. Priority is given to write request
module double_access_reg #(
    //parameter type         obi_req_t  = logic,
    //parameter type         obi_rsp_t = logic,
    parameter WordSize = 32,
    parameter AddrSize = 32,

    // Parameters of Axi Slave Bus Interface S00_AXI
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 32

) (

    //input   obi_req_t                       writer_req,
    //output  obi_rsp_t                       writer_rsp,
    //input   obi_req_t                       reader_req,
    //output  obi_rsp_t                       reader_rsp,

    input  logic                            reader_req_i,
    output logic                            reader_gnt_o,
    output logic                            reader_rvalid_o,
    input  logic [ADDR_WIDTH          -1:0] reader_addr_i,
    input  logic                            reader_we_i,
    input  logic [                     3:0] reader_be_i,
    output logic [DATA_WIDTH          -1:0] reader_rdata_o,
    input  logic [DATA_WIDTH          -1:0] reader_wdata_i,

    input  logic                            writer_req_i,
    output logic                            writer_gnt_o,
    output logic                            writer_rvalid_o,
    input  logic [ADDR_WIDTH          -1:0] writer_addr_i,
    input  logic                            writer_we_i,
    input  logic [                     3:0] writer_be_i,
    output logic [DATA_WIDTH          -1:0] writer_rdata_o,
    input  logic [DATA_WIDTH          -1:0] writer_wdata_i,




    input logic clk_i,
    input logic rst_ni


);
  enum logic [2:0] {
    IDLE,  // also empty
    FULL,
    READ
  }
      CS, NS;

  logic [DATA_WIDTH          -1:0] curr_data;
  logic [DATA_WIDTH          -1:0] next_data;





  //(clk) begin
  always @(posedge clk_i or negedge rst_ni) begin : FSM_SEQ
    if (!rst_ni) begin
      CS <= IDLE;
    end else begin
      CS <= NS;
    end
  end


  always @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      curr_data <= '0;
    end else begin
      curr_data <= next_data;
    end
  end


  always_comb begin
    NS = CS;
    next_data = curr_data;

    writer_rdata_o = '0;
    writer_rvalid_o = '0;
    writer_gnt_o = '0;
    reader_gnt_o = '0;
    reader_rvalid_o = '0;
    //data_data_o = '0;

    case (CS)
      // wait for a request to come in from the serial link
      IDLE: begin
        if (writer_req_i) begin
          NS = FULL;
          writer_gnt_o = '1;
          next_data = writer_wdata_i;
        end else begin
          NS = IDLE;
        end
      end
      FULL: begin
        if (reader_req_i == '1 && reader_we_i == '0) begin
          NS = READ;
          reader_gnt_o = '1;
        end else begin
          //next_wdata = writer_wdata_i;
          NS = FULL;
        end
      end
      READ: begin
        reader_rvalid_o = '1;
        reader_rdata_o = curr_data;
        NS = IDLE;

      end
      default: NS = IDLE;
    endcase
  end



endmodule
