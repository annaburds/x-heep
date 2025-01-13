# Semester Project: Chip2chip

## Software applications

### 1. example_serial_link_simulation_dma
A complete simulation program that demonstrates bidirectional data transfer through Serial Link. It includes:
- Data transmission and reception in a single program
- Support for both CPU and DMA transfer modes
- Initial configuration testing with cycle count measurement
- Parameters:
    - DMA_DATA_LARGE: maximum transfer chunk size
    - TEST_DATA_LARGE: total test data size


### 2. example_serial_link_dma_sender
A dedicated sender program for emulation testing that sends test data through Serial Link

### 3. example_serial_link_dma_receiver
A dedicated receiver program for emulation testing that receives data through the receive FIFO of Serial Link.


## Serial Link X-HEEP Wrapper Parameters


### AXI Interface Package Selection
The module supports two AXI interface type packages:
- `core_v_mini_mcu_pkg`: Contains standard, unmodified AXI interface types
- `serial_link_minimum_axi_pkg`: Contains optimized, minimal AXI interface types for serial link implementation

#### Configuration Parameters
- `AddrWidth`: Address width
- `*_CH_SIZE`: Size parameters for AXI channels (AW, W, B, AR, R)
- `FIFO_DEPTH`: Receiver FIFO depth

## Adding ILA (Integrated Logic Analyzer) to Vivado Design

### 1. Create ILA Block Design
1. In Vivado, create a new Block Design
2. Add ILA IP core from the IP Catalog
3. Configure ILA Settings:
   * Set Monitor Type to "Native"
   * For Serial Link example, set Number of Probes to 4 for monitoring:
     - ddr_rcv_clk_i
     - ddr_rcv_clk_o
     - ddr_i[3:0]
     - ddr_o[3:0]
   * Set Sample Data Depth: determines how many samples can be captured in a single trigger event
     - Larger depth allows capturing longer signal traces but uses more FPGA resources
     - Common values: 1024, 2048, 4096, 8192
   * Configure probe widths according to signals:
     - Probe 0: 1-bit for ddr_rcv_clk_i
     - Probe 1: 1-bit for ddr_rcv_clk_o
     - Probe 2: 4-bit for ddr_i[3:0]
     - Probe 3: 4-bit for ddr_o[3:0]

### 2. External Port Connection
1. Select the ILA block in the block design
2. Right-click and choose "Make External"
3. Set meaningful net names for better signal identification during debugging
   * Example: "sl_ddr_clk_in", "sl_ddr_data_out", etc.

### 3. Create HDL Wrapper
1. In the Sources window, right-click on the block design
2. Select "Create HDL Wrapper"
3. Choose "Let Vivado manage wrapper and auto-update"

### 4. Instantiate ILA Wrapper
Add the ILA wrapper instance in your design where monitoring is needed:
```verilog
debug_ila_wrapper debug_ila_wrapper_i (
    .clk_0(clk_i),
    .ddr_i(ddr_i),
    .ddr_o(ddr_o),
    .ddr_rcv_clk_i(ddr_rcv_clk_i),
    .ddr_rcv_clk_o(ddr_rcv_clk_o) 
);
```

### 5. Generate Bitstream
1. Run Synthesis and Implementation
2. Generate Bitstream with debug probes included
3. Save the debug probes file (.ltx)
4. Program the FPGA with the generated bitstream

### 6. Debug Session
1. Open Hardware Manager in Vivado
2. Connect to your FPGA board
3. Program the device with the generated bitstream
4. Right-click the ILA core and select "Set Up Debug"
5. Configure trigger conditions as needed
6. Start capturing signals

### Tips
* Choose Sample Data Depth based on your debugging needs and available FPGA resources
* Use meaningful signal names when making ports external
* Consider adding more probes than initially needed to avoid recompilation
* Monitor resource utilization when adding multiple ILA cores
* If unable to capture signals during debugging, try reducing the JTAG clock frequency when opening the hardware target

## Compile on the Server and Emulate on FPGA

### 1. Program the FPGA
1. Download bitstream from server
2. Local setup:
   - Install Vivado Lab Edition
   - Add PYNQ-Z2 board files to `<Vivado Install Path>/data/boards/board_files`
   - Install cable driver from `<Vivado Install Path>/data/xicom/cable_drivers/lin64/install_script/install_drivers`
3. Program FPGA using Vivado Hardware Manager

### 2. Program the Flash
1. Mount server directory to local machine
2. Compile software on server
3. Program flash from local machine using provided script
4. Unmount server when done

For detailed instructions and script configurations, please refer to `Work_with_FPGA_on_server.md`.
