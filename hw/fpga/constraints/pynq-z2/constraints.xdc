create_clock -add -name sys_clk_pin -period 8.00 -waveform {0 5} [get_ports {clk_i}];
# Rising edge is at 270 degree, falling edge at 450 (resp. 60) degrees
set edge_list [list [expr $T_FWD_CLK / 4 * 3] [expr $T_FWD_CLK / 4 * 5]]
create_clock -name vir_clk_ddr_in -period $T_FWD_CLK
create_clock -name clk_ddr_in -period $T_FWD_CLK -waveform $edge_list [get_ports ddr_rcv_clk_i]
# The data launching clock with 0 degree clock phase
create_generated_clock -name clk_slow -source clk_i -divide_by $FWD_CLK_DIV \
    [get_pins -hierarchical clk_slow_reg/Q]

# this is the "forwarded clock", we are assuming it is shifted by -90 or +270 degrees (or +90 degrees and inverted)
set edge_list [list [expr 1 + $FWD_CLK_DIV / 2 * 3] [expr 1 + $FWD_CLK_DIV / 2 * 5] [expr 1 + $FWD_CLK_DIV / 2 * 7]]
create_generated_clock -name clk_ddr_out -source clk_i -edges $edge_list \[get_pins -hierarchical ddr_rcv_clk_o_reg/Q]
# Input
set_false_path -setup -rise_from [get_clocks vir_clk_ddr_in] -rise_to [get_clocks clk_ddr_in]
set_false_path -setup -fall_from [get_clocks vir_clk_ddr_in] -fall_to [get_clocks clk_ddr_in]
# Output
set_false_path -setup -rise_from [get_clocks clk_slow] -rise_to [get_clocks clk_ddr_out]
set_false_path -setup -fall_from [get_clocks clk_slow] -fall_to [get_clocks clk_ddr_out]
# Input
set_false_path -hold  -rise_from [get_clocks vir_clk_ddr_in] -fall_to [get_clocks clk_ddr_in]
set_false_path -hold  -fall_from [get_clocks vir_clk_ddr_in] -rise_to [get_clocks clk_ddr_in]
# Output
set_false_path -hold  -rise_from [get_clocks clk_slow] -fall_to [get_clocks clk_ddr_out]
set_false_path -hold  -fall_from [get_clocks clk_slow] -rise_to [get_clocks clk_ddr_out]

set_false_path -from [get_pins my_system_clock_pin] -to [get_ports my_clk_ddr_out_port]

# Window has a margin on both side of 5% of a quarter of the clock period
set MARGIN              [expr $T_FWD_CLK / 4 * 0.05]

# Input delays
set_input_delay -max -clock [get_clocks vir_clk_ddr_in] [expr $MARGIN] [get_ports ddr_i]
set_input_delay -add_delay -min -clock [get_clocks vir_clk_ddr_in] [expr -$MARGIN] [get_ports ddr_i]
set_input_delay -add_delay -max -clock_fall -clock [get_clocks vir_clk_ddr_in] [expr $MARGIN] [get_ports ddr_i]
set_input_delay -add_delay -min -clock_fall -clock [get_clocks vir_clk_ddr_in] [expr -$MARGIN] [get_ports ddr_i]

# Output delays
set_output_delay -max -clock [get_clocks clk_ddr_out] [expr $T_FWD_CLK / 4 + $MARGIN] -reference_pin [get_ports ddr_rcv_clk_o] [get_ports ddr_o]
set_output_delay -add_delay -min -clock [get_clocks clk_ddr_out] [expr $T_FWD_CLK / 4 - $MARGIN] -reference_pin [get_ports ddr_rcv_clk_o] [get_ports ddr_o]
set_output_delay -add_delay -max -clock_fall -clock [get_clocks clk_ddr_out] [expr $T_FWD_CLK / 4 + $MARGIN] -reference_pin [get_ports ddr_rcv_clk_o] [get_ports ddr_o]
set_output_delay -add_delay -min -clock_fall -clock [get_clocks clk_ddr_out] [expr $T_FWD_CLK / 4 - $MARGIN] -reference_pin [get_ports ddr_rcv_clk_o] [get_ports ddr_o]



