module serial_link_xheep_wrapper
  //import obi_pkg::*;
  //import reg_pkg::*;
  //import axi_pkg::*;
 #(
  parameter type axi_req_t  = logic,
  parameter type axi_rsp_t  = logic,
  parameter type aw_chan_t  = logic,
  parameter type ar_chan_t  = logic,
  parameter type r_chan_t   = logic,
  parameter type w_chan_t   = logic,
  parameter type b_chan_t   = logic,
  parameter type cfg_req_t  = logic,
  parameter type cfg_rsp_t  = logic,
  parameter type obi_req_t  = logic,
  parameter type obi_resp_t = logic,
  //parameter int NumChannels = 1,
  parameter int NumChannels = 32,
  parameter int NumLanes = 8,//8,
  parameter int MaxClkDiv = 32
) (
  input  logic                      clk_i,
  input  logic                      fast_clock,
  input  logic                      rst_ni,
  input  logic                      clk_reg_i,
  input  logic                      rst_reg_ni,
  input  logic                      testmode_i,

  input  obi_req_t                  obi_req_i,
  output obi_resp_t                 obi_rsp_i,
  output obi_req_t                  obi_req_o,
  input  obi_resp_t                 obi_rsp_o,

  input  cfg_req_t                  cfg_req_i,
  output cfg_rsp_t                  cfg_rsp_o,
  input  logic [NumChannels-1:0]    ddr_rcv_clk_i,
  output logic [NumChannels-1:0]    ddr_rcv_clk_o,
  input  logic [NumChannels-1:0][NumLanes-1:0] ddr_i,
  output logic [NumChannels-1:0][NumLanes-1:0] ddr_o

);

  //logic clk_serial_link;
  logic rst_serial_link_n;

  logic clk_ena;
  logic reset_n;

  axi_req_t                  fast_sl_req_i,fast_sl_req_O,axi_in_req_i,axi_out_req_o;
  axi_rsp_t                  fast_sl_rsp_i,fast_sl_rsp_O,axi_in_rsp_o,axi_out_rsp_i;
  cfg_req_t                  fast_cfg_req_i;
  cfg_rsp_t                  fast_cfg_rsp_o;

  

core2axi #(
    //.AXI4_WDATA_WIDTH(AXI_DATA_WIDTH),
    //.AXI4_RDATA_WIDTH(AXI_DATA_WIDTH)
  ) obi2axi_bridge_virtual_obi_i (
    .clk_i,
    .rst_ni,
    .data_req_i(obi_req_i.req),
    .data_gnt_o(obi_rsp_i.gnt),
    .data_rvalid_o(obi_rsp_i.rvalid),
    .data_addr_i(obi_req_i.addr),
    .data_we_i(obi_req_i.we),
    .data_be_i(obi_req_i.be),
    .data_rdata_o(obi_rsp_i.rdata),
    .data_wdata_i(obi_req_i.wdata),

    .aw_id_o(axi_in_req_i.aw.id),
    .aw_addr_o(axi_in_req_i.aw.addr),
    .aw_len_o(axi_in_req_i.aw.len),
    .aw_size_o(axi_in_req_i.aw.size),
    .aw_burst_o(axi_in_req_i.aw.burst),
    .aw_lock_o(axi_in_req_i.aw.lock),
    .aw_cache_o(axi_in_req_i.aw.cache),
    .aw_prot_o(axi_in_req_i.aw.prot),
    .aw_region_o(axi_in_req_i.aw.region),
    .aw_user_o(axi_in_req_i.aw.user),
    .aw_qos_o(axi_in_req_i.aw.qos),
    .aw_valid_o(axi_in_req_i.aw_valid),
    .aw_ready_i(axi_in_rsp_o.aw_ready),

    .w_data_o(axi_in_req_i.w.data),
    .w_strb_o(axi_in_req_i.w.strb),
    .w_last_o(axi_in_req_i.w.last),
    .w_user_o(axi_in_req_i.w.user),
    .w_valid_o(axi_in_req_i.w_valid),
    .w_ready_i(axi_in_rsp_o.w_ready),

    .b_id_i(axi_in_rsp_o.b.id),
    .b_resp_i(axi_in_rsp_o.b.resp),
    .b_valid_i(axi_in_rsp_o.b_valid),
    .b_user_i(axi_in_rsp_o.b.user),
    .b_ready_o(axi_in_req_i.b_ready),

    .ar_id_o(axi_in_req_i.ar.id),
    .ar_addr_o(axi_in_req_i.ar.addr),
    .ar_len_o(axi_in_req_i.ar.len),
    .ar_size_o(axi_in_req_i.ar.size),
    .ar_burst_o(axi_in_req_i.ar.burst),
    .ar_lock_o(axi_in_req_i.ar.lock),
    .ar_cache_o(axi_in_req_i.ar.cache),
    .ar_prot_o(axi_in_req_i.ar.prot),
    .ar_region_o(axi_in_req_i.ar.region),
    .ar_user_o(axi_in_req_i.ar.user),
    .ar_qos_o(axi_in_req_i.ar.qos),
    .ar_valid_o(axi_in_req_i.ar_valid),
    .ar_ready_i(axi_in_rsp_o.ar_ready),

    .r_id_i(axi_in_rsp_o.r.id),
    .r_data_i(axi_in_rsp_o.r.data),
    .r_resp_i(axi_in_rsp_o.r.resp),
    .r_last_i(axi_in_rsp_o.r.last),
    .r_user_i(axi_in_rsp_o.r.user), 
    .r_valid_i('1),
    .r_ready_o(axi_in_req_i.r_ready)
  );




  axi2obi #(
    //.C_S00_AXI_DATA_WIDTH(AXI_DATA_WIDTH),
    //.C_S00_AXI_ADDR_WIDTH(AXI_ADDR_WIDTH)
  ) axi2obi_bridge_virtual_r_obi_i (
    .gnt_i('1),

    .data_req_i(obi_req_i.req),
    .data_gnt_o(obi_rsp_i.gnt),
    .data_rvalid_o(obi_rsp_i.rvalid),
    .data_addr_i(obi_req_i.addr),
    .data_we_i(obi_req_i.we),
    .data_be_i(obi_req_i.be),
    .data_rdata_o(obi_rsp_i.rdata),
    .data_wdata_i(obi_req_i.wdata),

    //.data_req_i(obi_req_o.req),
    //.data_gnt_o(obi_rsp_o.gnt),
    //.data_rvalid_o(obi_rsp_o.rvalid),
    //.data_addr_i(obi_req_o.addr),
    //.data_we_i(obi_req_o.we),
    //.data_be_i(obi_req_o.be),
    //.data_rdata_o(obi_rsp_o.rdata),
    //.data_wdata_i(obi_req_o.wdata),

    //.data_req_i(axi_sl_slave_req.req),
    //.data_gnt_o(axi_sl_slave_resp.gnt),
    //.data_rvalid_o(axi_sl_slave_resp.rvalid),
    //.data_addr_i(axi_sl_slave_req.addr),
    //.data_we_i(axi_sl_slave_req.we),
    //.data_be_i(axi_sl_slave_req.be),
    //.data_rdata_o(axi_sl_slave_resp.rdata),
    //.data_wdata_i(axi_sl_slave_req.wdata),

    .s00_axi_aclk(clk_i),
    .s00_axi_aresetn(rst_ni),

    .s00_axi_araddr(axi_out_req_o.ar.addr),
    .s00_axi_arvalid(axi_out_req_o.ar_valid),
    .s00_axi_arready(axi_out_rsp_i.ar_ready),
    .s00_axi_arprot(axi_out_req_o.ar.prot),

    .s00_axi_rdata(axi_out_rsp_i.r.data),
    .s00_axi_rresp(axi_out_rsp_i.r.resp),
    .s00_axi_rvalid(axi_out_rsp_i.r_valid),
    .s00_axi_rready(axi_out_req_o.r_ready),

    .s00_axi_awaddr(axi_out_req_o.aw.addr),
    .s00_axi_awvalid(axi_out_req_o.aw_valid),
    .s00_axi_awready(axi_out_rsp_i.aw_ready),
    .s00_axi_awprot(axi_out_req_o.aw.prot),

    .s00_axi_wdata(axi_out_req_o.w.data),
    .s00_axi_wvalid(axi_out_req_o.w_valid),
    .s00_axi_wready(axi_out_rsp_i.w_ready),
    .s00_axi_wstrb(axi_out_req_o.w.strb),

    .s00_axi_bresp(axi_out_rsp_i.b.resp),
    .s00_axi_bvalid(axi_out_rsp_i.b_valid),
    .s00_axi_bready(axi_out_req_o.b_ready)
  );



  tc_clk_mux2 i_tc_reset_mux (
    .clk0_i (reset_n),
    .clk1_i (rst_ni),
    .clk_sel_i (testmode_i),
    .clk_o (rst_serial_link_n)
  );

  axi_cdc #(
      .axi_req_t        ( axi_req_t   ),
      .axi_resp_t       ( axi_rsp_t   ),
      .aw_chan_t        ( aw_chan_t   ),
      .w_chan_t         ( w_chan_t    ),
      .b_chan_t         ( b_chan_t    ),
      .ar_chan_t        ( ar_chan_t   ),
      .r_chan_t         ( r_chan_t    ),
  /// Depth of the FIFO crossing the clock domain, given as 2**LOG_DEPTH.
      .LogDepth(2)
)axi_cdc_i (
  // slave side - clocked by `src_clk_i`
  .src_clk_i(clk_i),
  .src_rst_ni(rst_ni),
  .src_req_i(axi_in_req_i),
  .src_resp_o(axi_in_rsp_o),
  // master side - clocked by `dst_clk_i`
  .dst_clk_i(fast_clock),
  .dst_rst_ni(rst_ni),
  .dst_req_o(fast_sl_req_i),
  .dst_resp_i(fast_sl_rsp_i)
);






reg_cdc #(
    .req_t(cfg_req_t),
    .rsp_t(cfg_rsp_t)
) reg_cdc_i(
    .src_clk_i(clk_reg_i),
    .src_rst_ni(rst_ni),
    .src_req_i(cfg_req_i),
    .src_rsp_o(cfg_rsp_o),

    .dst_clk_i(fast_clock),
    .dst_rst_ni(rst_ni),
    .dst_req_o(fast_cfg_req_i),
    .dst_rsp_i(fast_cfg_rsp_o)
);


  axi_cdc #(
      .axi_req_t        ( axi_req_t   ),
      .axi_resp_t       ( axi_rsp_t   ),
      .aw_chan_t        ( aw_chan_t   ),
      .w_chan_t         ( w_chan_t    ),
      .b_chan_t         ( b_chan_t    ),
      .ar_chan_t        ( ar_chan_t   ),
      .r_chan_t         ( r_chan_t    ),
  /// Depth of the FIFO crossing the clock domain, given as 2**LOG_DEPTH.
      .LogDepth(2)
)axi_cdc_O (
  // slave side - clocked by `src_clk_i`
  .src_clk_i(fast_clock),
  .src_rst_ni(rst_ni),
  .src_req_i(fast_sl_req_O),
  .src_resp_o(fast_sl_rsp_O),
  // master side - clocked by `dst_clk_i`
  .dst_clk_i(clk_i),
  .dst_rst_ni(rst_ni),
  .dst_req_o(axi_out_req_o),
  .dst_resp_i(axi_out_rsp_i)
);




  if (NumChannels > 1) begin : gen_multi_channel_serial_link
    serial_link #(
      .axi_req_t        ( axi_req_t   ),
      .axi_rsp_t        ( axi_rsp_t   ),
      .aw_chan_t        ( aw_chan_t   ),
      .w_chan_t         ( w_chan_t    ),
      .b_chan_t         ( b_chan_t    ),
      .ar_chan_t        ( ar_chan_t   ),
      .r_chan_t         ( r_chan_t    ),
      .cfg_req_t        ( cfg_req_t   ),
      .cfg_rsp_t        ( cfg_rsp_t   ),
      .hw2reg_t         ( serial_link_reg_pkg::serial_link_hw2reg_t ),
      .reg2hw_t         ( serial_link_reg_pkg::serial_link_reg2hw_t ),
      .NumChannels      ( NumChannels ),
      .NumLanes         ( NumLanes    ),
      .MaxClkDiv        ( MaxClkDiv   )
    ) i_serial_link (
      .clk_i          ( fast_clock             ),
      .rst_ni         ( rst_ni            ),
      .clk_sl_i       ( fast_clock   ),
      .rst_sl_ni      ( rst_serial_link_n ),
      .clk_reg_i      ( fast_clock         ),
      .rst_reg_ni     ( rst_reg_ni        ),
      .testmode_i     ( 1'b0              ),
      .axi_in_req_i   ( fast_sl_req_i      ),
      .axi_in_rsp_o   ( fast_sl_rsp_i     ),//axi_in_rsp_o      ),
      .axi_out_req_o  ( fast_sl_req_O     ),
      .axi_out_rsp_i  ( fast_sl_rsp_O     ),
      .cfg_req_i      ( fast_cfg_req_i         ),
      .cfg_rsp_o      ( fast_cfg_rsp_o    ),//cfg_rsp_o         ),
      .ddr_rcv_clk_i  ( ddr_rcv_clk_i     ),
      .ddr_rcv_clk_o  ( ddr_rcv_clk_o     ),
      .ddr_i          ( ddr_i             ),
      .ddr_o          ( ddr_o             ),
      .isolated_i     (   2'b0            ), //2'b0
      .isolate_o      (                   ), //2'b0
      .clk_ena_o      ( clk_ena           ),
      .reset_no       ( reset_n           )

      

    );
  end else begin : gen_single_channel_serial_link
    serial_link #(
      .axi_req_t        ( axi_req_t   ),
      .axi_rsp_t        ( axi_rsp_t   ),
      .aw_chan_t        ( aw_chan_t   ),
      .w_chan_t         ( w_chan_t    ),
      .b_chan_t         ( b_chan_t    ),
      .ar_chan_t        ( ar_chan_t   ),
      .r_chan_t         ( r_chan_t    ),
      .cfg_req_t        ( cfg_req_t   ),
      .cfg_rsp_t        ( cfg_rsp_t   ),
      .hw2reg_t         ( serial_link_single_channel_reg_pkg::serial_link_single_channel_hw2reg_t ),
      .reg2hw_t         ( serial_link_single_channel_reg_pkg::serial_link_single_channel_reg2hw_t ),
      .NumChannels      ( NumChannels ),
      .NumLanes         ( NumLanes    ),
      .MaxClkDiv        ( MaxClkDiv   )
    ) i_serial_link (
      .clk_i          ( fast_clock        ),
      .rst_ni         ( rst_ni            ),
      .clk_sl_i       ( fast_clock        ),
      .rst_sl_ni      ( rst_serial_link_n ),
      .clk_reg_i      ( fast_clock        ),
      .rst_reg_ni     ( rst_reg_ni        ),
      .testmode_i     ( 1'b0              ),
      .axi_in_req_i   ( fast_sl_req_i     ),
      .axi_in_rsp_o   ( fast_sl_rsp_i     ),//axi_in_rsp_o      ),
      .axi_out_req_o  ( fast_sl_req_O     ),
      .axi_out_rsp_i  ( fast_sl_rsp_O     ),
      .cfg_req_i      ( fast_cfg_req_i    ),
      .cfg_rsp_o      ( fast_cfg_rsp_o    ),//cfg_rsp_o         ),
      .ddr_rcv_clk_i  ( ddr_rcv_clk_i     ),
      .ddr_rcv_clk_o  ( ddr_rcv_clk_o     ),
      .ddr_i          ( ddr_i             ),
      .ddr_o          ( ddr_o             ),
      .isolated_i     ( 2'b0              ),
      .isolate_o      (                   ),
      .clk_ena_o      ( clk_ena           ),
      .reset_no       ( reset_n           )
    );
  end

endmodule
