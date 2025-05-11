#include <stdio.h>
#include <stdlib.h>

#include "x-heep.h"
#include "core_v_mini_mcu.h"
#include "serial_link_single_channel_regs.h"
#include "serial_link_regs.h"
#include "csr.h"
#include "dma.h"
#include "dma_regs.h"
#include "fast_intr_ctrl.h"
//#include "timer_sdk.h"


#define DMA_DATA_LARGE 4
#define TEST_DATA_LARGE 30

static uint32_t to_be_sent_4B[TEST_DATA_LARGE] __attribute__((aligned(4))) = {0};
static uint32_t copied_data_4B[TEST_DATA_LARGE] __attribute__((aligned(4))) = {0};


void WRITE_SL_CONFIG(void);
void SL_CPU_TRANS(uint32_t *src, uint32_t *dst, uint32_t large);
void SL_DMA_TRANS(uint32_t *src, uint32_t *dst, uint32_t large);
void wait_for_interrupt(void);
void dma_intr_handler_trans_done(uint8_t channel){}

int main(int argc, char *argv[]){
    printf("AAAAAAAA:\n");
    volatile int32_t *addr_p = 0x50000040;
    volatile int32_t *addr_p_external = 0xF0010000;
    volatile int32_t *addr_p_recreg = 0x51000000;
    unsigned int cycles1,cycles2,cycles3;
    WRITE_SL_CONFIG();
    
    for (int i = 0; i < TEST_DATA_LARGE; i++) {
        to_be_sent_4B[i] = i+1;
        printf("to_be_sent_4B %x\n", to_be_sent_4B[i]);
    }
    

    uint32_t chunks = TEST_DATA_LARGE / DMA_DATA_LARGE;
    uint32_t remainder = TEST_DATA_LARGE % DMA_DATA_LARGE;
    for (uint32_t i = 0; i < chunks; i++) {
        SL_CPU_TRANS(to_be_sent_4B + i * DMA_DATA_LARGE, copied_data_4B + i * DMA_DATA_LARGE, DMA_DATA_LARGE);
        //SL_DMA_TRANS(to_be_sent_4B + i * DMA_DATA_LARGE, copied_data_4B + i * DMA_DATA_LARGE, DMA_DATA_LARGE);
    }
    if (remainder > 0) {
        //SL_CPU_TRANS(to_be_sent_4B + chunks * DMA_DATA_LARGE, copied_data_4B + chunks * DMA_DATA_LARGE, remainder);
        SL_DMA_TRANS(to_be_sent_4B + chunks * DMA_DATA_LARGE, copied_data_4B + chunks * DMA_DATA_LARGE, remainder);
    }

    printf("data saved:\n");
    for (int i = 0; i < TEST_DATA_LARGE; i++) {
        printf("%x\n", copied_data_4B[i]);
    }

    printf("DONE\n");  
    return EXIT_SUCCESS;
}

void __attribute__ ((optimize("00"))) SL_CPU_TRANS(uint32_t *src, uint32_t *dst, uint32_t large){
    volatile int32_t *addr_p_external = 0xF0010000;
    volatile int32_t *addr_p_recreg = 0x51000000;
    printf("CPU is sending..\n");
    for (int i = 0; i < large; i++) {
        *addr_p_external = *(src + i);
    }
    printf("done.\n\r");

    printf("CPU is receiving..\n");
    for (int i = 0; i < large; i++) {
        *(dst + i) = *addr_p_recreg;
    }
    printf("done.\n\r");
}

// parameter "large" should equal to or less than FIFO size (default 8)
void __attribute__ ((optimize("00"))) SL_DMA_TRANS(uint32_t *src, uint32_t *dst, uint32_t large){
    volatile static dma_config_flags_t res;
    volatile static dma_target_t tgt_src;
    volatile static dma_target_t tgt_dst;
    volatile static dma_trans_t trans;


        dma_init(NULL);
        tgt_src.ptr = (uint32_t *)src;
        tgt_src.inc_d1_du = 1;
        //tgt_src.size_du = large;
        tgt_src.trig = DMA_TRIG_MEMORY;
        tgt_src.type = DMA_DATA_TYPE_WORD;

        tgt_dst.ptr = (uint32_t *)0xF0010000;
        tgt_dst.inc_d1_du = 0;
        //tgt_dst.size_du = large;
        tgt_dst.trig = DMA_TRIG_MEMORY;
        tgt_dst.type = DMA_DATA_TYPE_WORD;

        trans.src = &tgt_src;
        trans.dst = &tgt_dst;
        trans.size_d1_du = large;
        trans.mode = DMA_TRANS_MODE_SINGLE;
        trans.win_du = 0;
        trans.sign_ext = 0;
        trans.end = DMA_TRANS_END_INTR;

        res |= dma_validate_transaction(&trans, false, false);
        res |= dma_load_transaction(&trans);
        res |= dma_launch(&trans);
        printf("DMA launched: send.\n");
        
        if(!dma_is_ready(0)) {
            CSR_CLEAR_BITS(CSR_REG_MSTATUS, 0x8);
                    if (!dma_is_ready(0)) {
                        wait_for_interrupt();
                    }
            CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
        }

        printf("done.\n\r");


        dma_init(NULL);
        tgt_src.ptr = (uint32_t *)0x51000000;
        tgt_src.inc_d1_du = 0;
        //tgt_src.size_d1_du = large;
        tgt_src.trig = DMA_TRIG_MEMORY;
        tgt_src.type = DMA_DATA_TYPE_WORD;

        tgt_dst.ptr = (uint32_t *)dst;
        tgt_dst.inc_d1_du = 1;
        //tgt_dst.size_d1_du = large;
        tgt_dst.trig = DMA_TRIG_MEMORY;
        tgt_dst.type = DMA_DATA_TYPE_WORD;

        trans.src = &tgt_src;
        trans.dst = &tgt_dst;
        trans.mode = DMA_TRANS_MODE_SINGLE;
        trans.size_d1_du = large;
        trans.win_du = 0;
        trans.sign_ext = 0;
        trans.end = DMA_TRANS_END_INTR;

        res |= dma_validate_transaction(&trans, false, false);
        res |= dma_load_transaction(&trans);
        res |= dma_launch(&trans);
        printf("DMA launched: save.\n");

        if(!dma_is_ready(0)) {
            CSR_CLEAR_BITS(CSR_REG_MSTATUS, 0x8);
                    if (!dma_is_ready(0)) {
                        wait_for_interrupt();
                    }
            CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
        }
        printf("done.\n\r");
}

void __attribute__ ((optimize("00"))) WRITE_SL_CONFIG(void){

    int32_t NUM_TO_CHECK = 429496729;
    // volatile int32_t *addr_p = 0x50000040;
    volatile int32_t *addr_p_external = 0xF0010000;
    volatile int32_t *addr_p_recreg = 0x51000000;

    REG_CONFIG();
    AXI_ISOLATE();
    EXTERNAL_BUS_SL_CONFIG();

    unsigned int cycles1;
    CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
    CSR_WRITE(CSR_REG_MCYCLE, 0);
        *addr_p_external = NUM_TO_CHECK;

    while(1){
    if (*addr_p_recreg ==NUM_TO_CHECK){
        CSR_READ(CSR_REG_MCYCLE, &cycles1);
        break;
       }
    }
    printf("Sending one axi package through external SL 32 bits takes %d cycles\n\r", cycles1);
}


void __attribute__ ((optimize("00"))) REG_CONFIG(void){
    volatile int32_t *addr_p_reg =(int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_CTRL_REG_OFFSET); 
    *addr_p_reg = (*addr_p_reg)| 0x00000001; // clock enable
    *addr_p_reg = (*addr_p_reg)& 0xFFFFFFFD; // rst on
    *addr_p_reg = (*addr_p_reg)| 0x00000002; // rst oFF
}


void __attribute__ ((optimize("00"))) REG_CONFIG_MULTI(void){
    volatile int32_t *addr_p_reg =(int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_CTRL_REG_OFFSET); 
    *addr_p_reg = (*addr_p_reg)| 0x00000001; // clock enable
    *addr_p_reg = (*addr_p_reg)& 0xFFFFFFFD; // rst on
    *addr_p_reg = (*addr_p_reg)| 0x00000002; // rst oFF
}

void __attribute__ ((optimize("00"))) RAW_MODE_EN(void){
    int32_t *addr_p_reg_RAW_MODE =(int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_RAW_MODE_EN_REG_OFFSET); 
    *addr_p_reg_RAW_MODE = (*addr_p_reg_RAW_MODE)| 0x00000001; // raw mode en

    int32_t *addr_p_RAW_MODE_IN_CH_SEL_REG =(int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_RAW_MODE_IN_CH_SEL_REG_OFFSET); 

    int32_t *addr_p_RAW_MODE_OUT_CH_MASK_REG =(int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_RAW_MODE_OUT_CH_MASK_REG_OFFSET); 
    *addr_p_RAW_MODE_OUT_CH_MASK_REG= (*addr_p_RAW_MODE_OUT_CH_MASK_REG)| 0x00000008; // raw mode mask

    int32_t *addr_p_RAW_MODE_OUT_DATA_FIFO_REG =(int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_RAW_MODE_OUT_DATA_FIFO_REG_OFFSET); 
    *addr_p_RAW_MODE_OUT_DATA_FIFO_REG = (*addr_p_RAW_MODE_OUT_DATA_FIFO_REG)| 0x00000001;

    int32_t *addr_p_RAW_MODE_OUT_DATA_FIFO_CTRL_REG =(int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_RAW_MODE_OUT_DATA_FIFO_CTRL_REG_OFFSET); 
    *addr_p_RAW_MODE_OUT_DATA_FIFO_CTRL_REG = (*addr_p_RAW_MODE_OUT_DATA_FIFO_CTRL_REG)| 0x00000001;

    int32_t *addr_p_RAW_MODE_OUT_EN_REG =(int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_RAW_MODE_OUT_EN_REG_OFFSET); 
    *addr_p_RAW_MODE_OUT_EN_REG = (*addr_p_RAW_MODE_OUT_EN_REG)| 0x00000001; 

    int32_t *addr_p_RAW_MODE_IN_DATA_REG =(int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_RAW_MODE_IN_DATA_REG_OFFSET); 
    *addr_p_RAW_MODE_IN_DATA_REG = (*addr_p_RAW_MODE_IN_DATA_REG)| 0x00000001; 
}

void __attribute__ ((optimize("00"))) AXI_ISOLATE(void){
    int32_t *addr_p_reg_ISOLATE_IN =(int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_CTRL_REG_OFFSET); 
    *addr_p_reg_ISOLATE_IN &= ~(1<<8);
    int32_t *addr_p_reg_ISOLATE_OUT =(int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_CTRL_REG_OFFSET);
    *addr_p_reg_ISOLATE_OUT &= ~(1<<9); // axi_out_isolate
    }

void __attribute__ ((optimize("00"))) EXTERNAL_BUS_SL_CONFIG(void){
    // /*                     -------                     */
    // /*  SL TESTHARNESS EXTERNAL BUS X-heep system      */
    // /*  REG CONFIG                   */
    // /*  CTRL register                */
    volatile int32_t *addr_p_reg_ext =(int32_t *)(EXT_PERIPHERAL_START_ADDRESS + 0x04000 + SERIAL_LINK_SINGLE_CHANNEL_CTRL_REG_OFFSET); //0x04000000 
    *addr_p_reg_ext = (*addr_p_reg_ext)| 0x00000001; // ctrl clock enable external

    *addr_p_reg_ext = (*addr_p_reg_ext)& 0xFFFFFFFD; // rst on
    *addr_p_reg_ext = (*addr_p_reg_ext)| 0x00000002; // rst oFF
    // /*  AXI ISOLATE                   */ 
    // all channels are isolated by default 
    int32_t *addr_p_reg_ISOLATE_IN_ext =(int32_t *)(EXT_PERIPHERAL_START_ADDRESS + 0x04000 + SERIAL_LINK_SINGLE_CHANNEL_CTRL_REG_OFFSET); 
    *addr_p_reg_ISOLATE_IN_ext &= ~(1<<8);
    int32_t *addr_p_reg_ISOLATE_OUT_ext =(int32_t *)(EXT_PERIPHERAL_START_ADDRESS + 0x04000 + SERIAL_LINK_SINGLE_CHANNEL_CTRL_REG_OFFSET);
    *addr_p_reg_ISOLATE_OUT_ext &= ~(1<<9);
    }