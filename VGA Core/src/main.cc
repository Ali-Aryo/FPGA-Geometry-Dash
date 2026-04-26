#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <vector>
#include "xil_io.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "sleep.h"
#include "xtime_l.h"
#include "xpseudo_asm.h"
#include "xil_mmu.h"
#include "sd_loader.h"
#include "level_generator.h"
#include "shared_amp.h"
#include "hardware_init.h"
#include "video_driver.h"
#include "game_physics.h"

// --- SHARED MEMORY DEFINITIONS ---
#define GENERATED_MAP_PTR      ((volatile char*)0xFFFF2000)
#define MAP_SIZE               150




// --- STATES ---
enum GameState {
    STATE_MENU,
    STATE_GAMEPLAY,
    STATE_PAUSE,
    STATE_ENDLESS,
    STATE_NAMING,
    STATE_AUDIO_LVL,
	STATE_SPRITE_SELECT
};
GameState current_state = STATE_MENU;

// --- PLAYER GLOBALS ---
char player_name[32] = "PLAYER1";
int name_len = 7;

// --- LEVEL CONFIG ---
const char level_map[] = "____________________^___________^_______^^_______^_________^^^________________________^___________^_______^^_______^_________^^^____________________________^___________^_______^^_______^_________^^^____________";
const int LEVEL_LENGTH = sizeof(level_map) - 1;
const int SCROLL_SPEED = 7;
const u32 LFSR_SPIKE_THRESHOLD = 0x90;
int endless_frames = 0;
int endless_last_score = 0;
int endless_high_score = 0;

//Swaps audio files on the SD card

void load_state_audio(const char* filename) {
    *SHARED_AUDIO_STATE_PTR = 0; // Pause Core 1
    xil_printf("Loading Audio: %s\r\n", filename);


    if (load_file_to_memory(filename, AUDIO_DATA_ADDR, 10000000) != XST_SUCCESS) {
        xil_printf("Failed to load %s\r\n", filename);
    }

    Xil_DCacheFlushRange(AUDIO_DATA_ADDR, 10000000);
    *SHARED_AUDIO_STATE_PTR = 2; // Trigger Core 1 Reset
}

int main() {
	int print_counter = 0;

    *SHARED_AUDIO_STATE_PTR = 0;
    Xil_SetTlbAttributes(0xFFFF0000, 0x14de2);

    hardware_init();

    //  SD Asset Loading
    if (init_sd_card() == XST_SUCCESS) {
//        load_file_to_memory("MENU.DAT", 0x018D2000, 5242880);
//        load_file_to_memory("PAUSE.DAT", 0x020BB000, 5242880);
        load_file_to_memory("Menu.wav", AUDIO_DATA_ADDR, 10000000);
        Xil_DCacheFlushRange(AUDIO_DATA_ADDR, 10000000);
        xil_printf("SD Assets Loaded Successfully.\r\n");
    } else {
        xil_printf("SD Initialization failed!\r\n");
    }

    video_init();

    //  Wake up Core 1
    Xil_Out32(ARM1_STARTADR, ARM1_BASEADDR);
    __asm__("dmb");
    __asm__("sev");

    // Frame Limiter Setup
    XTime tCurrent;
    const XTime TICKS_PER_FRAME = COUNTS_PER_SECOND / 60;
    XTime tNextFrame;
    XTime_GetTime(&tNextFrame);
    tNextFrame += TICKS_PER_FRAME;

    if (init_sd_card() == 0) {
        // Pass the DDR address where the music lives
        generate_level_pre_game((u32*)AUDIO_DATA_ADDR, 1324900);

        // signal Core 1 to start playing music
        *SHARED_AUDIO_STATE_PTR = 1;
    }



    while(1) {
    	u32 btn_val = XGpio_DiscreteRead(&BTNInst, 1);
    	        u32 sw_val = XGpio_DiscreteRead(&BTNInst, 2);

    	        // --- 1. DETERMINE NEXT STATE & INVALID STATE CHECK ---
    	        GameState next_state;

    	        u32 mode_switches = sw_val & 0x3E;

    	        bool multiple_modes_active = (mode_switches != 0) && ((mode_switches & (mode_switches - 1)) != 0);

    	        if (multiple_modes_active) {
    	            next_state = STATE_MENU;
    	        }
    	        else if (sw_val & 0x01) next_state = STATE_PAUSE;
    	        else if (sw_val & 0x20) next_state = STATE_SPRITE_SELECT;
    	        else if (sw_val & 0x10) next_state = STATE_NAMING;
    	        else if (sw_val & 0x08) next_state = STATE_AUDIO_LVL;
    	        else if (sw_val & 0x04) next_state = STATE_ENDLESS;
    	        else if (sw_val & 0x02) next_state = STATE_GAMEPLAY;
    	        else next_state = STATE_MENU;

    	        // --- 2. CONTINUOUS AUDIO POLLING ---
    	        if (*SHARED_AUDIO_STATE_PTR != 2) {
    	            if (next_state == STATE_PAUSE) {
    	                // Force audio to stop while in the pause menu
    	                *SHARED_AUDIO_STATE_PTR = 0;
    	            }
    	            else if (sw_val & 0x3E) {
    	                // If gameplay switches are up, ensure we are in PLAY mode
    	                *SHARED_AUDIO_STATE_PTR = 1;
    	            }
    	            else {
    	                // If switches are down, ensure we are in IDLE mode
    	                *SHARED_AUDIO_STATE_PTR = 1;
    	            }
    	        }

    	        // --- 3. STATE TRANSITION LOGIC ---
    	        if (next_state != current_state) {
    	            GameState prev_state = current_state;
    	            current_state = next_state;

    	            // Load Audio and Set Color Themes
    	            if (next_state == STATE_MENU || next_state == STATE_NAMING || next_state == STATE_SPRITE_SELECT) {
    	                load_state_audio("MainMenu.WAV");
    	            }
    	            else if (next_state == STATE_AUDIO_LVL) {
    	                load_state_audio("Audio.WAV");
    	                generate_level_pre_game((u32*)AUDIO_DATA_ADDR, 932715);
    	                set_game_colors(0x002200, 0x004522);
    	            }
    	            else if(next_state == STATE_GAMEPLAY) {
    	                load_state_audio("Level1.WAV");
    	                set_game_colors(0x000033, 0x003245);
    	            }
    	            else if(next_state == STATE_ENDLESS) {
    	                load_state_audio("Endless.WAV");
    	                set_game_colors(0x330000, 0x002500);
    	            }

    	            // ==========================================
    	            // --- SMART RESET LOGIC ---
    	            // ==========================================
    	            static GameState last_played_mode = STATE_MENU;
    	            bool should_reset = true;

    	            if (current_state == STATE_PAUSE) {
    	                should_reset = false;
    	            }
    	            else if (prev_state == STATE_PAUSE && current_state == last_played_mode) {
    	                should_reset = false;
    	            }

    	            if (should_reset) {
    	                // Save the Endless score if abandoning the mode
    	                if (last_played_mode == STATE_ENDLESS && endless_frames > 0) {
    	                    endless_last_score = endless_frames / 60;
    	                    if (endless_last_score > endless_high_score) {
    	                        endless_high_score = endless_last_score;
    	                    }
    	                }
    	                reset_game_world();
    	                endless_frames = 0;
    	            }

    	            // Track what mode we are playing so we can safely unpause later
    	            if (current_state == STATE_GAMEPLAY || current_state == STATE_ENDLESS || current_state == STATE_AUDIO_LVL) {
    	                last_played_mode = current_state;
    	            }
    	            // ==========================================

    	            // Draw Static Backgrounds
    	            if (current_state == STATE_MENU) draw_menu_screen();
    	            else if (current_state == STATE_PAUSE) draw_pause_screen();
    	            else if (current_state == STATE_NAMING) draw_naming_screen(player_name);
    	            else if (current_state == STATE_SPRITE_SELECT) draw_sprite_select_screen();
    	            else draw_full_gameplay_background(image_buffer_pointer);

    	            Xil_DCacheFlush();

    	            // Give SD card/Audio time to settle
    	            XTime_GetTime(&tNextFrame);
    	            tNextFrame += TICKS_PER_FRAME;
    	            usleep(50000);
    	        }

        // --- STATE EXECUTION ---
    	        if (current_state == STATE_NAMING) {
    	                    bool screen_needs_update = false;
    	                    process_uart_name_input(player_name, name_len, screen_needs_update);
    	                    if (screen_needs_update) {
    	                        draw_naming_screen(player_name);
    	                        Xil_DCacheFlush();
    	                    }
    	                }

    	                // --- 2. SPRITE SELECTION ---
    	                else if (current_state == STATE_SPRITE_SELECT) {
    	                    static bool jump_was_pressed = false;
    	                    bool jump_is_pressed = (btn_val & 0x01); // Mask to specifically check button 0

    	                    // Simple debounce so it cycles exactly once per click
    	                    if (jump_is_pressed && !jump_was_pressed) {

    	                        active_sprite = (active_sprite + 1) % 6;
    	                        jump_was_pressed = true;


    	                        draw_sprite_select_screen();
    	                        Xil_DCacheFlush();
    	                    }
    	                    else if (!jump_is_pressed) {
    	                        jump_was_pressed = false;
    	                    }
    	                }

    	                // --- 3. GAMEPLAY MODES ---
    	                else if (current_state == STATE_GAMEPLAY || current_state == STATE_ENDLESS || current_state == STATE_AUDIO_LVL) {

    	                    if ((btn_val == 1) && !is_jumping) {
    	                        if ((current_state == STATE_GAMEPLAY || current_state == STATE_AUDIO_LVL) && is_level_complete) {
    	                            reset_game_world();
    	                            if (current_state == STATE_AUDIO_LVL) {
    	                                *SHARED_AUDIO_STATE_PTR = 2; // Restart music for audio mode
    	                            }
    	                            draw_full_gameplay_background(image_buffer_pointer);
    	                        } else {
    	                            apply_jump_impulse();
    	                        }
    	                    }

    	                    if (is_jumping) {
    	                        update_physics();
    	                        erase_prev_sprite(image_buffer_pointer, 200, prev_sprite_y);
    	                        draw_only_sprite(image_buffer_pointer, 200, sprite_y);
    	                    }

    	                    bool collision_detected = false;

    	                    if (current_state == STATE_GAMEPLAY) {
    	                        collision_detected = update_normal_world(SCROLL_SPEED, level_map, LEVEL_LENGTH);
    	                        draw_normal_world(level_map, LEVEL_LENGTH, SCROLL_SPEED);
    	                        if (is_level_complete) draw_text(192, 400, "LEVEL COMPLETE", 0x00FF00, 8);
    	                    }
    	                    else if (current_state == STATE_ENDLESS) {
    	                        endless_frames++;
    	                        collision_detected = update_endless_world(SCROLL_SPEED, LFSR_SPIKE_THRESHOLD);
    	                        draw_endless_world(SCROLL_SPEED, endless_high_score, endless_last_score);
    	                    }
    	                    else if (current_state == STATE_AUDIO_LVL) {
    	                        static int debug_print_timer = 0;
    	                        static int start_delay_frames = 180;
    	                        static bool audio_started = false;

    	                        if (!audio_started) {
    	                            if (start_delay_frames > 0) {
    	                                start_delay_frames--;
    	                                *SHARED_AUDIO_STATE_PTR = 0;
    	                            } else {
    	                                *SHARED_AUDIO_STATE_PTR = 1;
    	                                audio_started = true;
    	                            }
    	                        }

    	                        if (debug_print_timer++ >= 60) {
    	                            debug_print_timer = 0;
    	                            xil_printf("\r\n[Pos: %d] Map: ", level_index);
    	                            for (int i = 0; i < 30; i++) {
    	                                if (level_index + i < MAP_SIZE) {
    	                                    outbyte(GENERATED_MAP_PTR[level_index + i]);
    	                                }
    	                            }
    	                            xil_printf("\r\n");
    	                        }

    	                        collision_detected = update_normal_world(SCROLL_SPEED, (const char*)GENERATED_MAP_PTR, MAP_SIZE);
    	                        draw_audio_world((const char*)GENERATED_MAP_PTR, MAP_SIZE, SCROLL_SPEED);

    	                        if(print_counter == 0){
    	                            xil_printf("Full Map: %s\r\n", (const char*)GENERATED_MAP_PTR);
    	                            print_counter++;
    	                        }

    	                        if (is_level_complete) {
    	                            draw_text(192, 400, "LEVEL COMPLETE", 0x00FF00, 8);
    	                        }
    	                    }

    	                    draw_text(20, 20, player_name, 0xFFFFFF, 4);

    	                    if (collision_detected) {
    	                        // Save High Score before resetting
    	                        if (current_state == STATE_ENDLESS) {
    	                            endless_last_score = endless_frames / 60;
    	                            if (endless_last_score > endless_high_score) {
    	                                endless_high_score = endless_last_score;
    	                            }
    	                        }

    	                        reset_game_world();
    	                        endless_frames = 0;

    	                        *SHARED_AUDIO_STATE_PTR = 2;
    	                        usleep(1000);

    	                        draw_full_gameplay_background(image_buffer_pointer);
    	                    }

    	                    Xil_DCacheFlush();
    	                }

    	                // --- FRAME LIMITER ---
    	                do {
    	                    XTime_GetTime(&tCurrent);
    	                } while (tCurrent < tNextFrame);
    	                tNextFrame += TICKS_PER_FRAME;

    	            }

    	            return 0;
    	        }
