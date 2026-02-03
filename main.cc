// includes
#include <stdio.h>
#include "xil_types.h"
#include "xparameters.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xscugic.h"
#include "xuartps.h" //new added for uart
#include "xil_cache.h"

// consts for uart init
#define UART_DEVICE_ID      XPAR_XUARTPS_0_DEVICE_ID
#define INTC_DEVICE_ID      XPAR_SCUGIC_SINGLE_DEVICE_ID
#define UART_INT_IRQ_ID     XPAR_XUARTPS_1_INTR
//  reolustion and base addr (from vavado hardware)
#define H_RES 1280
#define V_RES 1024
#define BASE_ADDR 0x00900000


XScuGic InterruptController; //vga_tutorial_students
XUartPs Uart_Ps;
volatile bool UPDATE_SCREEN_FLAG = true;
volatile int color_shift = 0;

// colors (0x00RRGGBB)
u32 colors[5] = {
    0x00FF0000, // Red
    0x0000FF00, // Green
    0x000000FF, // Blue
    0x00FFFF00, // Yellow
    0x00FFFFFF  // White
};

//https://github.com/Xilinx/embeddedsw/blob/1bb19ac1ab06ab322ba4340bed372f93ca612a18/XilinxProcessorIPLib/drivers/uartps/src/xuartps.h#L329
void Uart_Interrupt_Handler(void *CallBackRef, u32 Event, unsigned int EventData)
{
    u8 ReceivedChar;
    XUartPs_Recv(&Uart_Ps, &ReceivedChar, 1);

    if (ReceivedChar == 's' || ReceivedChar == 'S') {
        color_shift++;
        UPDATE_SCREEN_FLAG = true;
    }
}

// --- Interrupt Setup Helper ---
int SetupInterruptSystem(XScuGic *IntcInstancePtr, XUartPs *UartInstancePtr, u16 UartIntrId)
{
    int Status;
    XScuGic_Config *IntcConfig;

    // 1. Initialize the GIC (Generic Interrupt Controller)
    IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
    Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress);
    if (Status != XST_SUCCESS) return XST_FAILURE;
    //vga_tutorial_students
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
        (Xil_ExceptionHandler) XScuGic_InterruptHandler, IntcInstancePtr);
    Xil_ExceptionEnable();

    // 2. Connect the UART Handler to the GIC
    // Note: We use the wrapper 'XUartPs_InterruptHandler' provided by Xilinx
    // which calls our custom 'Uart_Interrupt_Handler' defined below
    Status = XScuGic_Connect(IntcInstancePtr, UartIntrId,
          (Xil_ExceptionHandler) XUartPs_InterruptHandler,
          (void *) UartInstancePtr);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    // 3. Enable the Interrupt at the GIC
    XScuGic_Enable(IntcInstancePtr, UartIntrId);

    // 4. Enable the Interrupt at the UART Device (Receive Data Interrupt)
    XUartPs_SetHandler(UartInstancePtr, (XUartPs_Handler)Uart_Interrupt_Handler, UartInstancePtr);
    XUartPs_SetInterruptMask(UartInstancePtr, XUARTPS_IXR_RXOVR | XUARTPS_IXR_RXFULL);

    return XST_SUCCESS;
}


void draw_bars(int shift_amount) {
    u32 * display_ptr = (u32 *)BASE_ADDR; //pointing to mem addr of BASE_ADDR. THis location is inside the boards DDR mem
    int bar_width = H_RES / 5; // vert
   // int bar_height = V_RES / 5;//horz
    for (int y = 0; y < V_RES; y++) {
        for (int x = 0; x < H_RES; x++) {

            // Calculate which bar we are in (0 to 4)
            // Adding 'shift_amount' cycles the colors
           int bar_index = (x / bar_width + shift_amount) % 5; //vert
         //   int bar_index = (y / bar_height + shift_amount) % 5; //horz

            // Write the pixel color to memory
            // Formula: (Row * Width) + Column
            display_ptr[y * H_RES + x] = colors[bar_index];
        }
    }
}


int main()
{
    Xil_DCacheFlush();
    int Status;
    XUartPs_Config *Config;

    // Initialize UART Driver
    Config = XUartPs_LookupConfig(UART_DEVICE_ID);
    Status = XUartPs_CfgInitialize(&Uart_Ps, Config, Config->BaseAddress);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    // Setup Interrupts (Connect UART to GIC)
    Status = SetupInterruptSystem(&InterruptController, &Uart_Ps, UART_INT_IRQ_ID);
    if (Status != XST_SUCCESS) return XST_FAILURE;


    while(1) {
        // Wait until s
        while(UPDATE_SCREEN_FLAG == false){
        }

        UPDATE_SCREEN_FLAG = false;

        draw_bars(color_shift);
        Xil_DCacheFlush();


    }

    return 0;
}
