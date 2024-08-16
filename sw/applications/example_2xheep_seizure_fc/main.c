// Porting to X-Heep : Francesco Poluzzi
/*
 *  Copyright (c) [2024] [Embedded Systems Laboratory (ESL), EPFL]
 *
 *  Licensed under the Apache License, Version 2.0 (the License);
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an AS IS BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


//////////////////////////////////////////////////////
// Main Author:     Alireza Amirshahi               //
// Optimizations:   Dimitrios Samakovlis            //
/////////////////////////////////////////////////////



#include "main.h"
#include <stdio.h>
#include <stdlib.h>

#include "core_v_mini_mcu.h"
#include "serial_link_single_channel_regs.h"
#include "csr.h"
#include "gpio.h"
#include "pad_control.h"
#include "pad_control_regs.h"
#include "rv_plic_regs.h"
#include "rv_plic.h"
#include <limits.h>
#include "ams_regs.h"
#include "x-heep.h"
#include "iffifo_regs.h"
#include "power_manager.h"
#include "mmio.h"
#include "handler.h"
#include "csr.h"
#include "hart.h"

#include "rv_plic.h"

#include "dma.h"
#include "dma_regs.h"
#include "fast_intr_ctrl.h"
#include "timer_sdk.h"



#define TEST_DATA_LARGE 256



static uint32_t copied_data_4B[TEST_DATA_LARGE] __attribute__((aligned(4))) = {0}

// Enable the C level optimizations for performance gains (loop reordering - loop unrolling)
#define CONV_OPTIMIZED
#define CONV_MAX_OPTIMIZED	// this has the most significant gains as it is the most time consuming part
#define BATCH_OPTIMIZED

#define MAX_DATA_SIZE 256*128

#define GPIO_TOGGLE_WRITE 1
#define GPIO_TOGGLE_READ 8 // BCS IT HAS TO BE INTERRUPT
#define GPIO_INTR GPIO_TOGGLE_READ + 1
#define MAX_INT_16 65535
//int16_t intermediate_map [256*128];

int32_t NUM_TO_CHECK = 9;
int16_t intermediate_map[256*128];
int32_t NUM_TO_BE_CHECKED;

plic_result_t plic_res;
volatile uint8_t gpio_intr_flag = 0;
uint32_t trigger_count = 0;

void WRITE_SL(void);
void handler_1()
{
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    gpio_intr_flag = 1;

}
void dma_intr_handler_trans_done(uint8_t channel)

{

   // DO HERE WHATEVER YOU WANT

}
    
// ===========================> Functions Prototype <===============================
void conv1d(const int16_t *data, const signed char *filter, int16_t *map_out, const signed char *bias, int32_t filter_size,
            int32_t input_len, int32_t input_depth, int32_t output_len, int32_t n_filter, int32_t strides, int32_t relu);

void conv_max1d(const int16_t *data, const signed char *filter, int16_t *map_out, const signed char *bias,
                int32_t input_len, int32_t input_depth, int32_t output_len);

void batch_normalization(const int16_t *data, const signed char *gamma, const signed char *beta, const signed char *mean, const signed char *var,
                         int16_t *map_out, int32_t input_len);

int16_t forward_propagation(int16_t *data, int16_t *intermediate);

// =================================================================================

int main()
{
    #ifdef PRINT_CYCLES
        timer_init();
        timer_start();
    #endif
// =================================================================================

    REG_CONFIG();
    AXI_ISOLATE();

    volatile int32_t *addr_p_external = 0x50000040; // for testing purposes with commented core2axi part
    gpio_intr_flag = 0;
    uint32_t time = 0;
    

    pad_control_t pad_control;
    pad_control.base_addr = mmio_region_from_addr((uintptr_t)PAD_CONTROL_START_ADDRESS);
    plic_Init();
    plic_irq_set_priority(GPIO_INTR, 1);
    plic_irq_set_enabled(GPIO_INTR, kPlicToggleEnabled); // Enable interrupt on processor side
    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    const uint32_t mask = 1 << 11;
    CSR_SET_BITS(CSR_REG_MIE, mask);

    gpio_result_t gpio_res;
    gpio_cfg_t cfg_in = {
        .pin = GPIO_TOGGLE_READ,
        .mode = GpioModeIn,
        .en_input_sampling = true,
        .en_intr = true,
        .intr_type = GpioIntrEdgeRising
        };
    gpio_res = gpio_config(cfg_in);

    gpio_assign_irq_handler(GPIO_INTR, &handler_1);
    volatile int read_value = 0;
    volatile int read_flag = 0;
    unsigned int cycles1,cycles2;
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    while (1)
    {   
        printf("Waiting for a trigger on GPIO %d\n\r", GPIO_TOGGLE_READ);

        while (gpio_intr_flag==0 ){}
        for (size_t i = 0; i < MAX_DATA_SIZE; i++) { 
        static dma_config_flags_t res;
        static dma_target_t tgt_src;
        static dma_target_t tgt_dst;
        static dma_trans_t trans;



        tgt_src.ptr = (uint8_t *)     0x50000040;
        tgt_src.inc_du = 0;
        tgt_src.size_du = TEST_DATA_LARGE;
        tgt_src.trig = DMA_TRIG_MEMORY;
        tgt_src.type = DMA_DATA_TYPE_WORD;



        tgt_dst.ptr = (uint8_t *)copied_data_4B;
        tgt_dst.inc_du = 1;
        tgt_dst.size_du = TEST_DATA_LARGE;
        tgt_dst.trig = DMA_TRIG_MEMORY;
        tgt_dst.type = DMA_DATA_TYPE_WORD;



        trans.src = &tgt_src;
        trans.dst = &tgt_dst;
        trans.mode = DMA_TRANS_MODE_SINGLE;
        trans.win_du = 0;
        trans.sign_ext = 0;
        trans.end = DMA_TRANS_END_INTR;



        res |= dma_validate_transaction(&trans, false, false);
        res |= dma_load_transaction(&trans);
        res |= dma_launch(&trans);



        while(!dma_is_ready(0)) {

        // here the CPU will be asleep waiting for the DMA to finish. Instead of this you can do anything else!
                    CSR_CLEAR_BITS(CSR_REG_MSTATUS, 0x8);
                    if (!dma_is_ready(0)) {
                        wait_for_interrupt();
                    }
                    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
        // Change this section to decide what the CPU does while asleep.
        }
        CSR_READ(CSR_REG_MCYCLE, &cycles1);    
        printf("first write finished with  %d cycles.\n\r", cycles1);
        gpio_intr_flag=0;

        }
    }

    printf("Transaction completed started FC\n");

// =================================================================================

    int16_t predict = forward_propagation(input_array, intermediate_map);

    #ifdef PRINT_OUTPUT
    printf("Prediction : %d", predict);
    if (predict == 0)
        printf(" (Normal)\n");
    else
        printf(" (Seizure)\n");
    #endif

    #ifdef PRINT_CYCLES
        uint32_t cycles=timer_stop();
        printf("Cycles: %d\n",cycles);
    #endif
    
    return 0;
}


void conv1d(const int16_t * const data, const signed char * const filter, int16_t *map_out, const signed char * const bias, const int32_t filter_size,
            const int32_t input_len, const int32_t input_depth, const int32_t output_len, const int32_t n_filter, const int32_t strides,
            const int32_t relu) {
    register int32_t sum;
    int32_t mult;
    #ifndef CONV_OPTIMIZED
    int32_t pad = 0;
    
    for (int32_t w_n = 0; w_n < n_filter; w_n++) {
        for (int32_t start_index = 0; start_index < input_len; start_index += strides) {
            sum = 0;
            for (int32_t w_i = 0; w_i < filter_size; w_i++) {
                for (int32_t w_j = 0; w_j < input_depth; w_j++) {
                    if (start_index + w_i - pad < input_len && start_index + w_i - pad > -1) {
                        mult = MUL(mem3d(filter, filter_size, input_depth, w_n, w_j, w_i),
                                   mem2d(data, input_len, w_j, start_index + w_i - pad), NUM_FRACTION_CNV_FC);
			            sum += mult;
                    }
                }
            }
    #else
    for (int32_t start_index = 0; start_index < input_len; start_index += strides) {
	    register const signed char *filter_address = filter;		                                // Ali's optimization for faster addressing of filter elements
	    for (int32_t w_n = 0; w_n < n_filter; w_n++)    {
            sum = 0;
	        for (int32_t w_j = 0; w_j < input_depth; w_j++) {
                register const int16_t *data_address = &data[input_len * w_j + start_index];		// similar to Ali's optimization for faster addressing of data elements
                for (register int32_t w_i = 0; w_i < filter_size; w_i++) {
                    mult = MUL_CONV(*(filter_address++), *(data_address++), NUM_FRACTION_CNV_FC);
                    sum += mult;
                }
            }
        
    #endif
            sum += bias[w_n];

            if (relu && sum < 0)
                sum = 0; // Relu

            if (sum > (MAX_INT_16) )    {
                #ifdef PRINT_OVERFLOW
                printf("Overflow %d\n", sum);
                #endif
                sum = (MAX_INT_16) -1;
            }
            else if (sum < -(MAX_INT_16)){
                #ifdef PRINT_OVERFLOW
                printf("Overflow %d\n", sum);
                #endif
                sum = -(MAX_INT_16) +1;
            }
            mem2d(map_out, output_len, w_n, start_index / strides) = (int16_t) sum;

        }
    }
}


void conv_max1d(const int16_t * const data, const signed char * const filter, int16_t *map_out, const signed char * const bias,
                const int32_t input_len, const int32_t input_depth, const int32_t output_len) {
    register int32_t sum;
    register int32_t mult;
    
    #ifndef CONV_MAX_OPTIMIZED
    for (int32_t w_n = 0; w_n < 128; w_n++) {
        int32_t maximum = NEG_INF;
        for (int32_t start_index = 0; start_index < input_len; start_index++) {
            sum = 0;
            for (int32_t w_i = 0; w_i < 3; w_i++) {
                for (int32_t w_j = 0; w_j < input_depth; w_j++) {
                    if (start_index + w_i - 1 < input_len && start_index + w_i - 1 > -1) {
                        mult = MUL(mem3d(filter, 3, input_depth, w_n, w_j, w_i),
                                   mem2d(data, input_len, w_j, start_index + w_i - 1), NUM_FRACTION_CNV_FC);
			sum += mult;
                    }
                }
            }
             sum += bias[w_n];
	     
            if (sum > (MAX_INT_16) ){
                    sum = (MAX_INT_16) -1;
            }else if (sum < -(MAX_INT_16)){
                sum = -(MAX_INT_16) +1;
            }
            if (sum > maximum) {
                maximum = sum;
            }
            if (start_index % 4 == 3) {
                mem2d(map_out, output_len, w_n, start_index / 4) = (int16_t) maximum;
                maximum = NEG_INF;
            }
        }
    }
    #else
    
    int32_t maximum[128];
    
        for (int32_t w_n = 0; w_n < 128; w_n++) {
        maximum[w_n] = NEG_INF;
    }
    
    
    //loop 0 - unrolled
    register const signed char *filter_address = filter;
        for (int32_t w_n = 0; w_n < 128; w_n++) {
        sum = 0;
	    for (int32_t w_j = 0; w_j < input_depth; w_j++) {
            register const int16_t *data_address = &data[input_len * w_j];		// reset data address every time we go to a deeper layer
            filter_address++;
            // 1 to 3 since 1st is out of bound (padding)
            for (register int32_t w_i = 1; w_i < 3; w_i++) {
                mult = MUL_CONV(*(filter_address++), *(data_address++), NUM_FRACTION_CNV_FC);
                sum += mult;
            }
        }
        sum += bias[w_n];
            
        if (sum > (MAX_INT_16) ){
            #ifdef PRINT_OVERFLOW
            printf("Overflow %d\n", sum);
            #endif
            sum = (MAX_INT_16) -1;
        }
        else if (sum < -(MAX_INT_16)){
            #ifdef PRINT_OVERFLOW
            printf("Overflow %d\n", sum);
            #endif
            sum = -(MAX_INT_16) +1;
        }
        if (sum > maximum[w_n]) {
            maximum[w_n] = sum;
        }
    }
    
        for (int32_t start_index = 1; start_index < input_len - 1; start_index ++) {
        filter_address = filter;						// reset filter address every time we change position
        register int32_t offset = start_index - 1;						// precalculate this addition
        for (int32_t w_n = 0; w_n < 128; w_n++) {
            sum = 0;
            for (int32_t w_j = 0; w_j < input_depth; w_j++) {
                register const int16_t *data_address = &data[input_len * w_j + offset];		// reset data address every time we go to a deeper layer
                for (register int32_t w_i = 0; w_i < 3; w_i++) {
                    mult = MUL_CONV(*(filter_address++), *(data_address++), NUM_FRACTION_CNV_FC);
                    sum += mult;
                }
            }
            sum += bias[w_n];
            
            
            if (sum > (MAX_INT_16) ){
                sum = (MAX_INT_16) -1;
            }
            else if (sum < -(MAX_INT_16)){
                sum = -(MAX_INT_16) +1;
            }
            if (sum > maximum[w_n]) {
                maximum[w_n] = sum;
            }
            if (start_index % 4 == 3) {
                mem2d(map_out, output_len, w_n, start_index / 4) = (int16_t) maximum[w_n];
                maximum[w_n] = NEG_INF;
            }
        }
    }
    
    //last loop unrolled
        filter_address = filter;
        for (int32_t w_n = 0; w_n < 128; w_n++) {
        sum = 0;
	    for (int32_t w_j = 0; w_j < input_depth; w_j++) {
            register const int16_t *data_address = &data[input_len * w_j + input_len - 2];		// reset data address every time we go to a deeper layer
            // 0 to 2 since 3rd is out of bound (padding)
            for (register int32_t w_i = 0; w_i < 2; w_i++) {
                mult = MUL_CONV(*(filter_address++), *(data_address++), NUM_FRACTION_CNV_FC);
                sum += mult;
            }
            filter_address++;
        }
        sum += bias[w_n];
            
        if (sum > (MAX_INT_16) ){
            #ifdef PRINT_OVERFLOW
            printf("Overflow %d\n", sum);
            #endif
            sum = (MAX_INT_16) -1;
        }
        else if (sum < -(MAX_INT_16)){
            #ifdef PRINT_OVERFLOW
            printf("Overflow %d\n", sum);
            #endif
            sum = -(MAX_INT_16) +1;
        }
        if (sum > maximum[w_n]) {
            maximum[w_n] = sum;
        }
        if ((input_len - 1) % 4 == 3) {
            mem2d(map_out, output_len, w_n, (input_len - 1) / 4) = (int16_t) maximum[w_n];
            maximum[w_n] = NEG_INF;
        }
    }
    #endif
}

void batch_normalization(const int16_t *data, const signed char *gamma, const signed char *beta, const signed char *mean, const signed char *var,
                         int16_t *map_out, const int32_t input_len) {
#ifndef BATCH_OPTIMIZED
    for (int32_t start_index = 0; start_index < input_len; start_index++) {
        for (int32_t w_j = 0; w_j < 128; w_j++) {
            int16_t normalized = mem2d(data, input_len, w_j, start_index) -
            ((int16_t) mean[w_j] << (NUM_FRACTION_DATA - NUM_FRACTION_BN));
            int16_t standardized = MUL(normalized, var[w_j], NUM_FRACTION_BN);
            int16_t new_standardized = MUL(standardized, gamma[w_j], NUM_FRACTION_BN);
            mem2d(map_out, input_len, w_j, start_index) =
                    (int16_t) (new_standardized + ((int16_t) beta[w_j] << (NUM_FRACTION_DATA - NUM_FRACTION_BN)));
        }
    }
#else
    const int16_t *data_address = data;
    int16_t *map_out_address = map_out;
    for (int32_t w_j = 0; w_j < 128; w_j++) {
	for (int32_t start_index = 0; start_index < input_len; start_index++) {
            int16_t normalized = *(data_address++) -
		((int16_t) mean[w_j] << (NUM_FRACTION_DATA - NUM_FRACTION_BN));
            int16_t standardized = MUL(normalized, var[w_j], NUM_FRACTION_BN);
            int16_t new_standardized = MUL(standardized, gamma[w_j], NUM_FRACTION_BN);
            *(map_out_address++) =
                    (int16_t) (new_standardized + ((int16_t) beta[w_j] << (NUM_FRACTION_DATA - NUM_FRACTION_BN)));
        }
    }
#endif
}

void relu(int16_t * const data, int32_t input_len) {
    for (int32_t i = 0; i < input_len; i++)
        data[i] = (data[i] < 0) ? 0 : data[i];
}


int16_t forward_propagation(int16_t *data, int16_t *intermediate) {
    int32_t fc_depth_size[3] = {128, 100, 2};
    int32_t fc_map_size[3] = {16, 1, 1};
    int16_t* intermediate_map0 = intermediate;
    int16_t* intermediate_map1 = data;
    //heep_kResults[kResultsIdx++] = 11;

    //  ************  BLOCK 0  ************ //
    int16_t *layer_out = intermediate_map0;
    int16_t *layer_in = intermediate_map1;
    //  ************  Serial Link received data  ************ //

    conv1d(layer_in, dense_w[0], layer_out, dense_b[0], fc_map_size[0],fc_map_size[0],
           fc_depth_size[0], fc_map_size[1], fc_depth_size[1], fc_map_size[0], 1);
    
    #ifdef PRINT_FC0_OUT
    printf("\nFC 0 out:\n");
    for(int i=0; i< fc_depth_size[1]*fc_map_size[0]; i++)
	    printf("%d ", layer_out[i]);
    printf("\n");
    #endif
    
    //  ************  FC 1  ************ //
    //layer_out = intermediate_map0;
    //layer_in = intermediate_map1;
    conv1d(layer_in, dense_w[1], layer_out, dense_b[1], fc_map_size[1],fc_map_size[1],
           fc_depth_size[1], fc_map_size[2], fc_depth_size[2], fc_map_size[1], 0);
    
    #ifdef PRINT_FC1_OUT
    printf("\n\nFC 1 out:\n");
    for(int i=0; i< fc_depth_size[2]*fc_map_size[1]; i++)
	    printf("%d ", layer_out[i]);
    printf("\n");
    #endif

    if (layer_out[0] > layer_out[1])
        return 0;
    else
        return 1;
}
