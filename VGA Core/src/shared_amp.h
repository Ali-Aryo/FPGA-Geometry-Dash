#ifndef SHARED_AMP_H
#define SHARED_AMP_H

#define sev() __asm__("sev")
#define ARM1_STARTADR 0xFFFFFFF0
#define ARM1_BASEADDR 0x10080000
#define SHARED_AUDIO_STATE_PTR ((volatile unsigned long *)(0xFFFF0000))
#define SHARED_JUMP_TRIGGER_PTR (volatile u32*)0xFFFF0004
#define AUDIO_DATA_ADDR 0x03000000

#endif
