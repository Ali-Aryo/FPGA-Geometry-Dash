#ifndef HARDWARE_INIT_H
#define HARDWARE_INIT_H

#include "xil_types.h"
#include "xparameters.h"
#include "xscugic.h"
#include "xgpio.h"
#include "xil_exception.h"

// --- HARDWARE GLOBALS ---
extern XGpio BTNInst;
extern XScuGic INTCInst;
extern volatile bool NEW_IMAGE_READY;

// --- CONSTANTS ---
#define BTN_DEVICE_ID          XPAR_AXI_GPIO_1_DEVICE_ID
#define INTC_GPIO_INTERRUPT_ID XPAR_FABRIC_AXI_GPIO_1_IP2INTC_IRPT_INTR
#define BTN_INT                XGPIO_IR_CH1_MASK

#define LFSR_BASE_ADDR         0x43C20000
#define LFSR_REG_VAL           (LFSR_BASE_ADDR + 4)

// --- FUNCTIONS ---
int hardware_init();
u32 read_lfsr();
void process_uart_name_input(char* player_name, int& name_len, volatile bool& screen_needs_update);

extern "C" {
    void BTN_Intr_Handler(void *InstancePtr);
    int InterruptSystemSetup(XScuGic *XScuGicInstancePtr);
    int IntcInitFunction(u16 DeviceId, XGpio *GpioInstancePtr);
}

#endif
