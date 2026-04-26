#ifndef ADVENTURES_WITH_IP_H_
#define ADVENTURES_WITH_IP_H_

#include <stdio.h>
#include <xil_io.h>
#include <xil_printf.h>
#include <xparameters.h>
#include "xgpio.h"
#include "xiicps.h"
#include "audio.h"

#define SHARED_AUDIO_STATE_PTR (volatile u32*)0xFFFF0000

//void audio_stream();
void play_sd_audio();
void audio_playback_loop();
unsigned char gpio_init();

extern XGpio Gpio;

#define BUTTON_SWITCH_ID    XPAR_GPIO_1_DEVICE_ID
#define BUTTON_CHANNEL      1
#define SWITCH_CHANNEL      2

#endif
