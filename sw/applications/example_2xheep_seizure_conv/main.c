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
#include "timer_sdk.h"
#include "x-heep.h"
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
#include "mmio.h"
#include <stdio.h>
#include <stdlib.h>
#include "csr.h"
#include "hart.h"
#include "handler.h"
#include "core_v_mini_mcu.h"
#include "power_manager.h"
#include "x-heep.h"
//#include "timer_util.h"

#define GPIO_TOGGLE_WRITE 1
#define GPIO_TOGGLE_READ 8 // BCS IT HAS TO BE INTERRUPT
#define GPIO_INTR  GPIO_TOGGLE_READ +1
// Enable the C level optimizations for performance gains (loop reordering - loop unrolling)
#define CONV_OPTIMIZED
#define CONV_MAX_OPTIMIZED	// this has the most significant gains as it is the most time consuming part
#define BATCH_OPTIMIZED

#define MAX_INT_16 65535
#define MAX_DATA_SIZE 256*128

int16_t intermediate_map[256*128];

int32_t NUM_TO_CHECK = 9;
int32_t NUM_TO_BE_CHECKED;

plic_result_t plic_res;
uint8_t gpio_intr_flag = 0;
uint32_t trigger_count = 0;




void WRITE_SL(int16_t *);
void handler_1()
{
    //printf("handler 1\n");
    gpio_intr_flag = 1;
    //hw_timer_start();
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

    int16_t predict = forward_propagation(input_array, intermediate_map);
    
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

void conv_block(const int32_t block, int16_t * const layer_in, int16_t * const conv1d_out){
    int32_t depth_size[6] = {23, 128, 128, 128, 100, 2};
    int32_t map_size[6] = {1024, 256, 64, 16, 1, 1};
    signed char* filter = conv1d_w[block];
    signed char* bias = conv1d_b[block];

    conv_max1d(layer_in, filter, conv1d_out, bias, map_size[block], depth_size[block], map_size[block+1]);
    
    batch_normalization(conv1d_out, bn[block * 4], bn[block * 4 + 1], bn[block * 4 + 2], bn[block * 4 + 3],
                        conv1d_out, map_size[block+1]);

    relu(conv1d_out, map_size[block+1] * depth_size[block + 1]);
}

int16_t forward_propagation(int16_t *data, int16_t *intermediate) {
    // SL configurations
    REG_CONFIG();
    AXI_ISOLATE();   
    printf("REG CONFIG SL DONE\n");
    
    int32_t fc_depth_size[3] = {128, 100, 2};
    int32_t fc_map_size[3] = {16, 1, 1};
    int16_t* intermediate_map0 = intermediate;
    int16_t* intermediate_map1 = data;

    //  ************  BLOCK 0  ************ //
    int16_t *layer_out = intermediate_map0;
    int16_t *layer_in = intermediate_map1;
    unsigned int cycles1,cycles2,cycles3;
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    conv_block(0, layer_in, layer_out);
    CSR_READ(CSR_REG_MCYCLE, &cycles1);
    printf("BLOCK 0 takes  %d cycles\n\r", cycles1);
    //  ************  BLOCK 1  ************ //
    layer_out = intermediate_map1;
    layer_in = intermediate_map0;
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    conv_block(1, layer_in, layer_out);
    CSR_READ(CSR_REG_MCYCLE, &cycles2);
    printf("BLOCK 1 takes  %d cycles\n\r", cycles2);
    //  ************  BLOCK 2  ************ //
    layer_out = intermediate_map0;
    layer_in = intermediate_map1;
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);
    conv_block(2, layer_in, layer_out);
    CSR_READ(CSR_REG_MCYCLE, &cycles3);
    printf("BLOCK 2 takes  %d cycles\n\r", cycles3);
    WRITE_SL(layer_out);
    //  ************  FC 0  ************ //

    return 0;
}


void __attribute__((optimize("00"))) WRITE_SL(int16_t *data)
{
    gpio_result_t gpio_res; // used to count the number of cycles to be sent from fpga_tx to fpga_rx
    gpio_cfg_t pin_cfg = {
        .pin = GPIO_TOGGLE_WRITE,
        .mode = GpioModeOutPushPull};
    gpio_res = gpio_config(pin_cfg);
    if (gpio_res != GpioOk)
        printf("Gpio initialization failed! res = %d \n", gpio_res);

    volatile int32_t *addr_p = 0x50000040;
    gpio_write(GPIO_TOGGLE_WRITE, false);
    gpio_write(GPIO_TOGGLE_WRITE, true); 
    printf("SL SENDING DATA\n");
    for (size_t i = 0; i < MAX_DATA_SIZE; i++) {
        
        *addr_p = data[i];
        printf("data %d\n",data[i]);
    }
}

void __attribute__((optimize("00"))) REG_CONFIG(void)
{
    volatile int32_t *addr_p_reg = (int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_CTRL_REG_OFFSET);
    *addr_p_reg = (*addr_p_reg) | 0x00000001;
    *addr_p_reg = (*addr_p_reg) & 0x11111101; 
    *addr_p_reg = (*addr_p_reg) | 0x00000002; //needed reset for the credits count in serial link
}

void __attribute__((optimize("00"))) RAW_MODE_EN(void)
{
    int32_t *addr_p_reg_RAW_MODE = (int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_RAW_MODE_EN_REG_OFFSET);
    *addr_p_reg_RAW_MODE = (*addr_p_reg_RAW_MODE) | 0x00000001; // raw mode en

    int32_t *addr_p_RAW_MODE_IN_CH_SEL_REG = (int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_RAW_MODE_IN_CH_SEL_REG_OFFSET);
    //*addr_p_RAW_MODE_IN_CH_SEL_REG = (*addr_p_RAW_MODE_IN_CH_SEL_REG)| 0x00000001; // raw mode select channel

    int32_t *addr_p_RAW_MODE_OUT_CH_MASK_REG = (int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_RAW_MODE_OUT_CH_MASK_REG_OFFSET);
    *addr_p_RAW_MODE_OUT_CH_MASK_REG = (*addr_p_RAW_MODE_OUT_CH_MASK_REG) | 0x00000008; // raw mode mask

    int32_t *addr_p_RAW_MODE_OUT_DATA_FIFO_REG = (int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_RAW_MODE_OUT_DATA_FIFO_REG_OFFSET);
    *addr_p_RAW_MODE_OUT_DATA_FIFO_REG = (*addr_p_RAW_MODE_OUT_DATA_FIFO_REG) | 0x00000001;

    int32_t *addr_p_RAW_MODE_OUT_DATA_FIFO_CTRL_REG = (int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_RAW_MODE_OUT_DATA_FIFO_CTRL_REG_OFFSET);
    *addr_p_RAW_MODE_OUT_DATA_FIFO_CTRL_REG = (*addr_p_RAW_MODE_OUT_DATA_FIFO_CTRL_REG) | 0x00000001;

    int32_t *addr_p_RAW_MODE_OUT_EN_REG = (int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_RAW_MODE_OUT_EN_REG_OFFSET);
    *addr_p_RAW_MODE_OUT_EN_REG = (*addr_p_RAW_MODE_OUT_EN_REG) | 0x00000001;

    int32_t *addr_p_RAW_MODE_IN_DATA_REG = (int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_RAW_MODE_IN_DATA_REG_OFFSET);
    *addr_p_RAW_MODE_IN_DATA_REG = (*addr_p_RAW_MODE_IN_DATA_REG) | 0x00000001;
}

void __attribute__((optimize("00"))) AXI_ISOLATE(void)
{
    int32_t *addr_p_reg_ISOLATE_IN = (int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_CTRL_REG_OFFSET);

    *addr_p_reg_ISOLATE_IN &= ~(1 << 8);
    int32_t *addr_p_reg_ISOLATE_OUT = (int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_CTRL_REG_OFFSET);
    *addr_p_reg_ISOLATE_OUT &= ~(1 << 9); 
}

