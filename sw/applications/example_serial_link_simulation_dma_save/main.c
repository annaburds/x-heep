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



#define TEST_DATA_LARGE 1



static uint32_t copied_data_4B[TEST_DATA_LARGE] __attribute__((aligned(4))) = {0};

int32_t NUM_TO_CHECK = 429496729;
int32_t NUM_TO_BE_CHECKED;
void WRITE_SL(void);
void dma_intr_handler_trans_done(uint8_t channel){
}

int main(int argc, char *argv[])
{

    volatile int32_t *addr_p = 0x50000040;
    volatile int32_t *addr_p_external = 0xF0010000;
    volatile int32_t *addr_p_recreg = 0x51000000;

    unsigned int cycles1,cycles2,cycles3;
    WRITE_SL();





        volatile static dma_config_flags_t res;
        volatile static dma_target_t tgt_src;
        volatile static dma_target_t tgt_dst;
        volatile static dma_trans_t trans;

        // The DMA is initialized (i.e. Any current transaction is cleaned.)
        //static uint32_t test_data_large[TEST_DATA_LARGE] __attribute__((aligned(4))) = {0};
        //uint32_t *test_addr_4B_PTR = &test_data_large[0];
        dma_init(NULL);
        tgt_src.ptr = (uint32_t *)     0x51000000;
        tgt_src.inc_du = 0;
        tgt_src.size_du = TEST_DATA_LARGE;
        tgt_src.trig = DMA_TRIG_MEMORY;
        tgt_src.type = DMA_DATA_TYPE_WORD;



        tgt_dst.ptr = (uint32_t *)copied_data_4B;
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


        CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
        CSR_WRITE(CSR_REG_MCYCLE, 0);
        res |= dma_validate_transaction(&trans, false, false);
        res |= dma_load_transaction(&trans);
        res |= dma_launch(&trans);


        while(!dma_is_ready(0)) {
            CSR_CLEAR_BITS(CSR_REG_MSTATUS, 0x8);
                    if (!dma_is_ready(0)) {
                        wait_for_interrupt();
                    }
            CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
        }  
        CSR_READ(CSR_REG_MCYCLE, &cycles1);
        printf("DMA reading takes  %d cycles\n\r", cycles2);
        printf("read from dma %x .\n\r", *copied_data_4B);


    printf("DONE\n");  
    return EXIT_SUCCESS;
}

void __attribute__ ((optimize("00"))) WRITE_SL(void){

    volatile int32_t *addr_p = 0x50000040;
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
    printf("sending full axi package through external SL 32 bits takes  %d cycles\n\r", cycles1);
        
    *addr_p_external = NUM_TO_CHECK;

}

void __attribute__ ((optimize("00"))) DMA_READ(void){

    //volatile int32_t *addr_p = 0x50000040;
    //volatile int32_t *addr_p_external = 0xF0010000;
    //volatile int32_t *addr_p_recreg = 0x51000000;
    //
    //volatile static dma_config_flags_t res;
    //volatile static dma_target_t tgt_src;
    //volatile static dma_target_t tgt_dst;
    //volatile static dma_trans_t trans;
    //// The DMA is initialized (i.e. Any current transaction is cleaned.)
    ////static uint32_t test_data_large[TEST_DATA_LARGE] __attribute__((aligned(4))) = {0};
    ////uint32_t *test_addr_4B_PTR = &test_data_large[0];
    //dma_init(NULL);
    //static volatile uint32_t pippo = 123; 
    //tgt_src.ptr = (uint32_t *)     0x51000000;
    //tgt_src.inc_du = 0;
    //tgt_src.size_du = TEST_DATA_LARGE;
    //tgt_src.trig = DMA_TRIG_MEMORY;
    //tgt_src.type = DMA_DATA_TYPE_WORD;
    //tgt_dst.ptr = (uint32_t *)copied_data_4B;
    //tgt_dst.inc_du = 1;
    //tgt_dst.size_du = TEST_DATA_LARGE;
    //tgt_dst.trig = DMA_TRIG_MEMORY;
    //tgt_dst.type = DMA_DATA_TYPE_WORD;
    //trans.src = &tgt_src;
    //trans.dst = &tgt_dst;
    //trans.mode = DMA_TRANS_MODE_SINGLE;
    //trans.win_du = 0;
    //trans.sign_ext = 0;
    //trans.end = DMA_TRANS_END_INTR;
    //res |= dma_validate_transaction(&trans, false, false);
    //res |= dma_load_transaction(&trans);
    //printf("lalalal %d\n",res);
    //res |= dma_launch(&trans);
    //printf("%d\n",res);
    //while(!dma_is_ready(0)) {
    //    CSR_CLEAR_BITS(CSR_REG_MSTATUS, 0x8);
    //            if (!dma_is_ready(0)) {
    //                wait_for_interrupt();
    //            }
    //    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    //}  
    //printf("read from dma %d .\n\r", *(tgt_dst.ptr));
}


void __attribute__ ((optimize("00"))) READ_SL(void){
    volatile int32_t *addr_p_external = 0xF0010000;// bus serial link from mcu_cfg.hjson
    while(1){
    if (*addr_p_external ==NUM_TO_CHECK){
        
        break;
    }}
}

void __attribute__ ((optimize("00"))) REG_CONFIG(void){
    volatile int32_t *addr_p_reg =(int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_SINGLE_CHANNEL_CTRL_REG_OFFSET); 
    *addr_p_reg = (*addr_p_reg)| 0x00000001; // clock enable
    *addr_p_reg = (*addr_p_reg)& 0x11111101; // rst on
    *addr_p_reg = (*addr_p_reg)| 0x00000002; // rst oFF
}


void __attribute__ ((optimize("00"))) REG_CONFIG_MULTI(void){
    volatile int32_t *addr_p_reg =(int32_t *)(SERIAL_LINK_START_ADDRESS + SERIAL_LINK_CTRL_REG_OFFSET); 
    *addr_p_reg = (*addr_p_reg)| 0x00000001; // clock enable
    *addr_p_reg = (*addr_p_reg)& 0x11111101; // rst on
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

    *addr_p_reg_ext = (*addr_p_reg_ext)& 0x11111101; // rst on
    *addr_p_reg_ext = (*addr_p_reg_ext)| 0x00000002; // rst oFF
    // /*  AXI ISOLATE                   */ 
    // all channels are isolated by default 
    int32_t *addr_p_reg_ISOLATE_IN_ext =(int32_t *)(EXT_PERIPHERAL_START_ADDRESS + 0x04000 + SERIAL_LINK_SINGLE_CHANNEL_CTRL_REG_OFFSET); 
    *addr_p_reg_ISOLATE_IN_ext &= ~(1<<8);
    int32_t *addr_p_reg_ISOLATE_OUT_ext =(int32_t *)(EXT_PERIPHERAL_START_ADDRESS + 0x04000 + SERIAL_LINK_SINGLE_CHANNEL_CTRL_REG_OFFSET);
    *addr_p_reg_ISOLATE_OUT_ext &= ~(1<<9);
    }