#include "hardware_init.h"
#include "xuartps_hw.h"
#include "xil_io.h"
#include "xil_printf.h"

XGpio BTNInst;
XScuGic INTCInst;
volatile bool NEW_IMAGE_READY = false;

extern "C" void BTN_Intr_Handler(void *InstancePtr) {
    XGpio *GpioPtr = (XGpio *)InstancePtr;
    NEW_IMAGE_READY = true;
    XGpio_InterruptClear(GpioPtr, BTN_INT);
}

extern "C" int InterruptSystemSetup(XScuGic *XScuGicInstancePtr) {
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, XScuGicInstancePtr);
    Xil_ExceptionEnable();
    return XST_SUCCESS;
}

extern "C" int IntcInitFunction(u16 DeviceId, XGpio *GpioInstancePtr) {
    XScuGic_Config *IntcConfig = XScuGic_LookupConfig(DeviceId);
    XScuGic_CfgInitialize(&INTCInst, IntcConfig, IntcConfig->CpuBaseAddress);
    InterruptSystemSetup(&INTCInst);
    XScuGic_Connect(&INTCInst, INTC_GPIO_INTERRUPT_ID, (Xil_ExceptionHandler)BTN_Intr_Handler, (void *)GpioInstancePtr);
    XGpio_InterruptEnable(GpioInstancePtr, BTN_INT);
    XGpio_InterruptGlobalEnable(GpioInstancePtr);
    XScuGic_Enable(&INTCInst, INTC_GPIO_INTERRUPT_ID);
    return XST_SUCCESS;
}

int hardware_init() {
    XGpio_Initialize(&BTNInst, BTN_DEVICE_ID);
    XGpio_SetDataDirection(&BTNInst, 1, 0xFF);
    XGpio_SetDataDirection(&BTNInst, 2, 0xFF);
    IntcInitFunction(XPAR_PS7_SCUGIC_0_DEVICE_ID, &BTNInst);
    return 0;
}

u32 read_lfsr() {
    return Xil_In32(LFSR_REG_VAL) & 0xFF;
}

void process_uart_name_input(char* player_name, int& name_len, volatile bool& screen_needs_update) {
    if (XUartPs_IsReceiveData(STDOUT_BASEADDRESS)) {
        char c = XUartPs_ReadReg(STDOUT_BASEADDRESS, XUARTPS_FIFO_OFFSET);

        bool skip_standard_input = false;
        static int clear_idx = 0;
        const char* cmd = "clear";

        // Command Detection
        if (c == cmd[clear_idx]) {
            clear_idx++;
            skip_standard_input = true;

            if (clear_idx == 5) {
                player_name[0] = '\0';
                name_len = 0;
                clear_idx = 0;
                screen_needs_update = true;
                xil_printf("\r\nName Cleared!\r\n");
            }
        } else {
            clear_idx = 0;
        }

        // Standard Input Logic
        if (!skip_standard_input) {
            if (c == 0x08 || c == 0x7F) {
                if (name_len > 0) {
                    player_name[--name_len] = '\0';
                    screen_needs_update = true;
                }
            } else if (c >= 32 && c <= 126 && name_len < 31) {
                player_name[name_len++] = c;
                player_name[name_len] = '\0';
                screen_needs_update = true;
            }
        }
    }
}
