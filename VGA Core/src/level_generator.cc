#include "xil_io.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "level_generator.h"

// Hardware Threshold Addresses
#define AUDIO_DET_BASE_ADDR   0x43C10000
#define AUDIO_REG_LIVE_VAL    (AUDIO_DET_BASE_ADDR)
#define AUDIO_REG_FLAG        (AUDIO_DET_BASE_ADDR + 8)

// Shared Memory for Map Generation
#define GENERATED_MAP_PTR     ((volatile char*)0xFFFF2000)
#define MAP_SIZE              150

void generate_level_pre_game(u32* audio_data_start, int total_samples) {
    int map_index = 0;
    int sample_counter = 0;

    // Guardrails for playability
    int consecutive_spikes = 0;
    int buffer_counter = 0;
    bool spike_detected_in_tile = false;

    // --- CONFIGURATION ---
    const int SAMPLES_PER_TILE = 1000;
    const int MAX_CONSECUTIVE   = 3;
    const int MANDATORY_BUFFER  = 5;
    const u32 THRESHOLD_VAL     = 16776700;

    // --- SAFETY BUFFER ---
    const int START_SAFETY_TILES = 25; // ~0.4 seconds of flat ground
    const int SAFETY_SAMPLE_OFFSET = START_SAFETY_TILES * SAMPLES_PER_TILE;

    xil_printf("--- STARTING PRE-GENERATION ---\r\n");


    // This gives the player time to react before the first rhythm spike
//    for (map_index = 0; map_index < START_SAFETY_TILES && map_index < MAP_SIZE; map_index++) {
//        GENERATED_MAP_PTR[map_index] = '_';
//    }


    u32* current_audio_ptr = audio_data_start + SAFETY_SAMPLE_OFFSET;


    for (int i = 0; i < (total_samples - SAFETY_SAMPLE_OFFSET) && map_index < MAP_SIZE; i++) {
        u32 raw_data = current_audio_ptr[i];


        u16 left_16 = (u16)(raw_data & 0xFFFF);
        u32 left_24 = (u32)left_16 << 8;


        Xil_Out32(AUDIO_DET_BASE_ADDR, left_24);

        if (Xil_In32(AUDIO_REG_FLAG) == 1 && Xil_In32(AUDIO_REG_LIVE_VAL) > THRESHOLD_VAL) {
            spike_detected_in_tile = true;
        }

        sample_counter++;
        if (sample_counter >= SAMPLES_PER_TILE) {
            if (spike_detected_in_tile && consecutive_spikes < MAX_CONSECUTIVE && buffer_counter == 0) {
                GENERATED_MAP_PTR[map_index] = '^';
                consecutive_spikes++;
                if (consecutive_spikes >= MAX_CONSECUTIVE) buffer_counter = MANDATORY_BUFFER;
            } else {
                GENERATED_MAP_PTR[map_index] = '_';
                if (consecutive_spikes > 0) buffer_counter = MANDATORY_BUFFER;
                consecutive_spikes = 0;
                if (buffer_counter > 0) buffer_counter--;
            }

            // Reset tile variables
            spike_detected_in_tile = false;
            sample_counter = 0;
            map_index++;
        }
    }

    if (map_index < MAP_SIZE) {
        GENERATED_MAP_PTR[map_index] = '\0';
    } else {
        GENERATED_MAP_PTR[MAP_SIZE - 1] = '\0';
    }

    //Flush cache so VGA and Core 1 see the new map
    Xil_DCacheFlushRange((UINTPTR)0xFFFF2000, MAP_SIZE);

    xil_printf("--- GENERATION COMPLETE (%d tiles) ---\r\n", map_index);
}
