#include "adventures_with_ip.h"
#include "audio.h"
#include "xil_printf.h"
#include "xil_cache.h"

#define AUDIO_DATA_ADDR       0x03000000
#define SHARED_AUDIO_STATE_PTR (volatile u32*)0xFFFF0000

void play_sd_audio() {
    // 1. Skip 44-byte WAV header
    u32* audio_ptr = (u32*)(AUDIO_DATA_ADDR + 44);
    static int sample_idx = 0;

    // 2. Handle Reset Signal (State 2)
    if (*SHARED_AUDIO_STATE_PTR == 2) {
        sample_idx = 0;
        Xil_Out32(I2S_DATA_TX_L_REG, 0);
        Xil_Out32(I2S_DATA_TX_R_REG, 0);
        *SHARED_AUDIO_STATE_PTR = 1;
        return;
    }

    const int MAX_SAMPLES = 932715;

    if (*SHARED_AUDIO_STATE_PTR == 1) {
        if (Xil_In32(I2S_STATUS_REG) & 0x01) {
            u32 raw_data = audio_ptr[sample_idx];

            u16 left_16  = (u16)(raw_data & 0xFFFF);
            u16 right_16 = (u16)((raw_data >> 16) & 0xFFFF);

            Xil_Out32(I2S_DATA_TX_L_REG, (u32)left_16 << 8);
            Xil_Out32(I2S_DATA_TX_R_REG, (u32)right_16 << 8);

            sample_idx++;
            if (sample_idx >= MAX_SAMPLES) {
                sample_idx = 0;
            }
        }
    }
    else if (*SHARED_AUDIO_STATE_PTR == 0) {
        Xil_Out32(I2S_DATA_TX_L_REG, 0);
        Xil_Out32(I2S_DATA_TX_R_REG, 0);
    }
}
