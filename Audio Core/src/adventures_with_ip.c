
#include "adventures_with_ip.h"

#define SHARED_AUDIO_STATE_PTR (volatile u32*)0xFFFF0000

int main(void)
{

    Xil_DCacheDisable();


    for(volatile int delay = 0; delay < 1000000; delay++);

    IicConfig(XPAR_XIICPS_0_DEVICE_ID);

    AudioPllConfig();
    AudioConfigureJacks();

    xil_printf("Core 1: Audio Hardware Initialized. Monitoring Shared Memory...\r\n");

    int loop_cnt = 0;

    while(1) {
            Xil_DCacheInvalidateRange((UINTPTR)0xFFFF0000, 4);
//            if(loop_cnt == 0){
//            	for(volatile int i = 0; i < 80; i++){
//	                asm("NOP");
//	            }
//            	loop_cnt++;
//            }
            // Call play_sd_audio every iteration.
            // It handles its own internal timing and reset logic.
            play_sd_audio();

            for(volatile int i = 0; i < 55; i++){
                asm("NOP");
            }
        }

    return 0;
}
