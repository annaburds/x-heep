// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Register Package auto-generated by `reggen` containing data structure

package dma_reg_pkg;

  // Address widths within the block
  parameter int BlockAw = 4;

  ////////////////////////////
  // Typedefs for registers //
  ////////////////////////////

  typedef struct packed {logic [31:0] q;} dma_reg2hw_ptr_in_reg_t;

  typedef struct packed {logic [31:0] q;} dma_reg2hw_ptr_out_reg_t;

  typedef struct packed {logic [31:0] q;} dma_reg2hw_dma_start_reg_t;

  typedef struct packed {logic [31:0] q;} dma_reg2hw_done_reg_t;

  typedef struct packed {
    logic [31:0] d;
    logic        de;
  } dma_hw2reg_dma_start_reg_t;

  typedef struct packed {
    logic [31:0] d;
    logic        de;
  } dma_hw2reg_done_reg_t;

  // Register -> HW type
  typedef struct packed {
    dma_reg2hw_ptr_in_reg_t ptr_in;  // [127:96]
    dma_reg2hw_ptr_out_reg_t ptr_out;  // [95:64]
    dma_reg2hw_dma_start_reg_t dma_start;  // [63:32]
    dma_reg2hw_done_reg_t done;  // [31:0]
  } dma_reg2hw_t;

  // HW -> register type
  typedef struct packed {
    dma_hw2reg_dma_start_reg_t dma_start;  // [65:33]
    dma_hw2reg_done_reg_t done;  // [32:0]
  } dma_hw2reg_t;

  // Register offsets
  parameter logic [BlockAw-1:0] DMA_PTR_IN_OFFSET = 4'h0;
  parameter logic [BlockAw-1:0] DMA_PTR_OUT_OFFSET = 4'h4;
  parameter logic [BlockAw-1:0] DMA_DMA_START_OFFSET = 4'h8;
  parameter logic [BlockAw-1:0] DMA_DONE_OFFSET = 4'hc;

  // Register index
  typedef enum int {
    DMA_PTR_IN,
    DMA_PTR_OUT,
    DMA_DMA_START,
    DMA_DONE
  } dma_id_e;

  // Register width information to check illegal writes
  parameter logic [3:0] DMA_PERMIT[4] = '{
      4'b1111,  // index[0] DMA_PTR_IN
      4'b1111,  // index[1] DMA_PTR_OUT
      4'b1111,  // index[2] DMA_DMA_START
      4'b1111  // index[3] DMA_DONE
  };

endpackage
