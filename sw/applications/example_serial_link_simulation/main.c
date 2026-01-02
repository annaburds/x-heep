// Copyright 2025 EPFL
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
// Description: Example application to test the Serial Link in simulation. Will count the clock cycles to execute a full write read transaction.



#include "serial_link_single_channel_regs.h" 
#include "serial_link_regs.h"
#include "serial_link.h"
#include "csr.h"


/* By default, printfs are activated for FPGA and disabled for simulation. */
#define PRINTF_IN_FPGA  1
#define PRINTF_IN_SIM   0


#if TARGET_SIM && PRINTF_IN_SIM
        #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#elif PRINTF_IN_FPGA && !TARGET_SIM
    #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#else
    #define PRINTF(...)
#endif


int32_t NUM_TO_CHECK = 525;
int main(int argc, char *argv[])
{

    volatile int32_t *addr_p_external = SL_EXTERNAL_WRITE;
    volatile int32_t *addr_p_recreg = SL_INTERNAL_READ;
    int32_t rcv_data;

    sl_sim_init();
    
    *addr_p_external = NUM_TO_CHECK;
    rcv_data = *addr_p_recreg;
    if (rcv_data != NUM_TO_CHECK){
        PRINTF("Received data (%u) does not match expected (%d)\n", rcv_data, NUM_TO_CHECK);
        return EXIT_FAILURE;
    }

    PRINTF("DONE\n");
    
    return EXIT_SUCCESS;
}

