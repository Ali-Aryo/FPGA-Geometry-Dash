#include "video_driver.h"
#include <string.h>
#include "xil_cache.h"
#include "game_physics.h"
#include "font8x8.h"
#include <math.h>
#include <stdio.h>

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 1024;
const int NUM_BYTES_BUFFER = 5242880;

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

int active_sprite = 0;

int * image_buffer_pointer = (int *)0x00900000;
int * image_menu_pointer   = (int *)0x018D2000;
int * image_pause_pointer  = (int *)0x020BB000;

// --- NEW: Global Color States ---
int active_bg_color = 0x000033;
int active_floor_color = 0x000000;

int get_bg_color(int x, int y) {
    int ground_level_y = 914;

    // Sky Area
    if (y < ground_level_y) {
        int secondary_bg = (active_bg_color & 0xFEFEFE) >> 1;
        bool is_gap = false;

        if (x >= 250 && x <= 280) is_gap = true;                   // Main left vertical line
        else if (y >= 400 && y <= 430) is_gap = true;              // Main horizontal line
        else if (x >= 550 && x <= 580 && y < 400) is_gap = true;   // Top-middle vertical line
        else if (y >= 650 && y <= 680 && x > 280) is_gap = true;   // Bottom-right horizontal line

        return is_gap ? secondary_bg : active_bg_color;
    }
    // Floor Line
    else if (y == ground_level_y) {
        return 0xFFFFFF;
    }
    // Floor Blocks Area
    else {
        int secondary_floor = (active_floor_color & 0xFEFEFE) >> 1;
        if (x % 400 > 380) return secondary_floor;
        return active_floor_color;
    }
}

void set_game_colors(int bg_color, int floor_color) {
    active_bg_color = bg_color;
    active_floor_color = floor_color;
}

void video_init() {
    memcpy(image_buffer_pointer, image_menu_pointer, NUM_BYTES_BUFFER);
    Xil_DCacheFlush();
}

void draw_menu_screen() {
    // 1. Save current state so we don't mess up the game if we exit the menu
    int temp_bg = active_bg_color;
    int temp_floor = active_floor_color;
    int temp_sprite = active_sprite;
    float temp_angle = sprite_angle;

    // 2. Set Menu Colors
    active_bg_color = 0xFFFF00;
    active_floor_color = 0xFF0000;

    // 3. Draw Geometric Background
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            image_buffer_pointer[(y * SCREEN_WIDTH) + x] = get_bg_color(x, y);
        }
    }

    // 4. Draw Centered Title Text (Yellow)
    draw_text(208, 100, "FPGA GEOMETRY DASH", 0x00FFFF, 6);

    // 5. Draw gradient Yellow Play Button (Triangle)
    int play_w = 60;
        int play_h = 60;
        int play_cx = 640 - (play_w); // Center it
        int play_cy = 240;
        for(int y = play_cy - play_h; y <= play_cy + play_h; y++) {
            for(int x = play_cx; x <= play_cx + play_w * 2; x++) {
                int dx = x - play_cx;
                int dy = y > play_cy ? y - play_cy : play_cy - y;

                if (dx <= (play_w * 2) - (dy * 2)) {
                    int intensity = 255 - (dx * 2);
                    if (intensity < 100) intensity = 100;

                    int color = (intensity << 16) | (intensity << 8) | 0xFF; // RGB Yellow
                    image_buffer_pointer[(y * SCREEN_WIDTH) + x] = color;
                }
            }
        }

    // 6. Draw Switch Instructions
    draw_text(450, 340, "SWITCH MAP:", 0x000000, 4);
    draw_text(450, 400, "SW 1: PAUSE", 0x000000, 3);
    draw_text(450, 450, "SW 2: LEVEL 1", 0x000000, 3);
    draw_text(450, 500, "SW 3: ENDLESS MODE", 0x000000, 3);
    draw_text(450, 550, "SW 4: AUDIO MODE", 0x000000, 3);
    draw_text(450, 600, "SW 5: NAME SELECT", 0x000000, 3);
    draw_text(450, 650, "SW 6: SPRITE SELECT", 0x000000, 3);

    // 7. Draw Freeze-Frame Sprites
    int jump_xs[] = {100, 300, 500, 700, 900, 1100};
    int jump_ys[] = {850, 780, 720, 720, 780, 850};
    float jump_as[] = {0.0f, 18.0f, 36.0f, 54.0f, 72.0f, 90.0f};

    for (int i = 0; i < 6; i++) {
        active_sprite = i;
        sprite_angle = jump_as[i];
        draw_only_sprite(image_buffer_pointer, jump_xs[i], jump_ys[i]);
    }

    active_bg_color = temp_bg;
    active_floor_color = temp_floor;
    active_sprite = temp_sprite;
    sprite_angle = temp_angle;
}

void draw_pause_screen() {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        image_buffer_pointer[i] = (image_buffer_pointer[i] & 0xFEFEFE) >> 1;
    }

    // 2. Draw the Classic | | Pause Symbol
    int center_x = SCREEN_WIDTH / 2;
    int center_y = SCREEN_HEIGHT / 2;

    int bar_width = 40;
    int bar_height = 160;
    int bar_spacing = 30;

    for (int y = center_y - (bar_height / 2); y <= center_y + (bar_height / 2); y++) {
        for (int x = center_x - bar_spacing - bar_width; x <= center_x + bar_spacing + bar_width; x++) {

            if ((x >= center_x - bar_spacing - bar_width && x <= center_x - bar_spacing) ||
                (x >= center_x + bar_spacing && x <= center_x + bar_spacing + bar_width)) {

                image_buffer_pointer[(y * SCREEN_WIDTH) + x] = 0xFFFFFF;
            }
        }
    }
}

void draw_full_gameplay_background(int* frame_buffer) {
    int ground_level_y = 914;
    for (int y = 0; y < ground_level_y; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            frame_buffer[(y * SCREEN_WIDTH) + x] = active_bg_color;
        }
    }
    for (int y = ground_level_y; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            frame_buffer[(y * SCREEN_WIDTH) + x] = active_floor_color;
        }
    }
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        frame_buffer[(ground_level_y * SCREEN_WIDTH) + x] = 0xFFFFFF;
    }
    draw_only_sprite(frame_buffer, 200, sprite_y);
}


void draw_char(int* frame_buffer, char c, int start_x, int start_y, int color, int scale) {
    if (c < 0 || c > 127) return;

    for (int y = 0; y < 8; y++) {
        unsigned char row = font8x8_basic[(int)c][y];
        for (int x = 0; x < 8; x++) {
            if (row & (1 << x)) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        int px = start_x + (x * scale) + sx;
                        int py = start_y + (y * scale) + sy;
                        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                            frame_buffer[(py * SCREEN_WIDTH) + px] = color;
                        }
                    }
                }
            }
        }
    }
}

void draw_text(int x, int y, const char* str, int color, int scale) {
    int current_x = x;
    while (*str) {
        draw_char(image_buffer_pointer, *str, current_x, y, color, scale);
        current_x += (8 * scale);
        str++;
    }
}

void draw_naming_screen(const char* current_name) {
    memset(image_buffer_pointer, 0, NUM_BYTES_BUFFER);

    draw_text(100, 100, "WELCOME CUBER", 0x00FF00, 6);
    draw_text(100, 300, "USE SERIAL TERMINAL TO ENTER YOUR NAME", 0xFFFFFF, 3);
    draw_text(100, 400, "Write CLEAR to input a new one", 0xFFFFFF, 3);

    draw_text(100, 500, "YOUR NAME:", 0x0088FF, 4);
    draw_text(100, 560, current_name, 0xFFFF00, 5);

    Xil_DCacheFlush();
}


void erase_prev_sprite(int* frame_buffer, int x, int y_old) {
    int cx_screen = x + 32;
    int cy_screen = y_old + 32;
    int ground_level_y = 914;

    for (int y = cy_screen - 46; y < cy_screen + 46; y++) {
        for (int x_p = cx_screen - 46; x_p < cx_screen + 46; x_p++) {
            if (x_p >= 0 && x_p < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
                int pixel_index = (y * SCREEN_WIDTH) + x_p;

                if (y < ground_level_y) {
                    frame_buffer[pixel_index] = active_bg_color;
                } else if (y == ground_level_y) {
                    frame_buffer[pixel_index] = 0xFFFFFF;
                } else {
                    frame_buffer[pixel_index] = active_floor_color;
                }
            }
        }
    }
}

void draw_only_sprite(int* frame_buffer, int x, int y_new) {
    float rad = sprite_angle * M_PI / 180.0f;
    float cos_a = cosf(rad);
    float sin_a = sinf(rad);

    int cx_screen = x + 32;
    int cy_screen = y_new + 32;

    for (int y = cy_screen - 46; y < cy_screen + 46; y++) {
        for (int x_p = cx_screen - 46; x_p < cx_screen + 46; x_p++) {
            if (x_p >= 0 && x_p < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {

                float dx = (float)(x_p - cx_screen);
                float dy = (float)(y - cy_screen);

                float rx_float = dx * cos_a + dy * sin_a;
                float ry_float = -dx * sin_a + dy * cos_a;

                int rx = (int)(rx_float + 32.5f);
                int ry = (int)(ry_float + 32.5f);

                if (rx >= 0 && rx < 64 && ry >= 0 && ry < 64) {
                    int pixel_index = (y * SCREEN_WIDTH) + x_p;
                    int color = 0x000000;

                    switch(active_sprite) {
                                            case 0: // --- ORIGINAL SMILEY ---
                                                if (rx < 4 || rx > 59 || ry < 4 || ry > 59) color = 0x000000;
                                                else if (rx >= 16 && rx <= 24 && ry >= 16 && ry <= 28) color = 0x000000;
                                                else if (rx >= 39 && rx <= 47 && ry >= 16 && ry <= 28) color = 0x000000;
                                                else if (rx >= 16 && rx <= 47 && ry >= 44 && ry <= 48) color = 0x000000;
                                                else if ((rx >= 16 && rx <= 20 && ry >= 36 && ry <= 44) ||
                                                         (rx >= 43 && rx <= 47 && ry >= 36 && ry <= 44)) color = 0x000000;
                                                else color = 0x0088FF;
                                                break;

                                            case 1: // --- TREASURE CHEST ---
                                                if (rx < 4 || rx > 59 || ry < 4 || ry > 59) color = 0x000000;
                                                else if (ry >= 28 && ry <= 32) color = 0x000000;
                                                else if (rx >= 24 && rx <= 40 && ry >= 20 && ry <= 40) {
                                                    if (rx >= 30 && rx <= 34 && ry >= 28 && ry <= 36) color = 0x000000;
                                                    else color = 0xC0C0C0;
                                                }
                                                else color = 0x8B4513;
                                                break;

                                            case 2: // --- BULLSEYE / TARGET ---
                                                if (rx < 4 || rx > 59 || ry < 4 || ry > 59) color = 0x000000;
                                                else if (rx < 12 || rx > 51 || ry < 12 || ry > 51) color = 0xFFFFFF;
                                                else if (rx < 20 || rx > 43 || ry < 20 || ry > 43) color = 0xFF0000;
                                                else if (rx < 28 || rx > 35 || ry < 28 || ry > 35) color = 0xFFFFFF;
                                                else color = 0xFF0000;
                                                break;

                                            case 3: // --- HYPNOTIC SPIRAL (Diamond Pattern) ---
                                                if (rx < 4 || rx > 59 || ry < 4 || ry > 59) color = 0x000000;
                                                else {
                                                    int dx = rx - 32; if (dx < 0) dx = -dx;
                                                    int dy = ry - 32; if (dy < 0) dy = -dy;
                                                    int d = dx + dy;

                                                    if ((d / 6) % 2 == 0) color = 0xFF00FF;
                                                    else color = 0x00FFFF;
                                                }
                                                break;

                                            case 4: // --- CRACKED STONE ---
                                                if (rx < 4 || rx > 59 || ry < 4 || ry > 59) color = 0x000000;
                                                else {
                                                    int diag1 = rx - ry; if (diag1 < 0) diag1 = -diag1;
                                                    int diag2 = (rx + ry) - 63; if (diag2 < 0) diag2 = -diag2;

                                                    if (diag1 < 3 && rx > 20) color = 0x000000;
                                                    else if (diag2 < 2 && ry < 40) color = 0x000000;
                                                    else if (rx >= 28 && rx <= 32 && ry >= 32) color = 0x000000;
                                                    else color = 0x888888;
                                                }
                                                break;

                                            case 5: // --- RAINBOW STRIPES ---
                                                if (rx < 4 || rx > 59 || ry < 4 || ry > 59) color = 0x000000;
                                                else if (ry < 14) color = 0xFF0000;
                                                else if (ry < 24) color = 0xFF8800;
                                                else if (ry < 34) color = 0xFFFF00;
                                                else if (ry < 44) color = 0x00FF00;
                                                else if (ry < 54) color = 0x0000FF;
                                                else color = 0x8800FF;
                                                break;
                                        }
                    frame_buffer[pixel_index] = color;
                }
            }
        }
    }
}

void update_spike(int* frame_buffer, int x, int y, int scroll_speed, int current_sprite_y) {
    int mid_x = x + 32;

    float rad = sprite_angle * M_PI / 180.0f;
    float cos_a = cosf(rad);
    float sin_a = sinf(rad);

    int cx_sprite = 200 + 32;
    int cy_sprite = current_sprite_y + 32;

    for (int y_p = y; y_p < y + 64; y_p++) {
        int row_from_top = y_p - y;
        int half_width = row_from_top / 2;

        for (int x_p = x; x_p < x + 64 + scroll_speed; x_p++) {
            if (x_p >= 0 && x_p < SCREEN_WIDTH && y_p >= 0 && y_p < SCREEN_HEIGHT) {

                if (x_p >= cx_sprite - 46 && x_p <= cx_sprite + 46 &&
                    y_p >= cy_sprite - 46 && y_p <= cy_sprite + 46) {

                    float dx = (float)(x_p - cx_sprite);
                    float dy = (float)(y_p - cy_sprite);

                    float rx = dx * cos_a + dy * sin_a;
                    float ry = -dx * sin_a + dy * cos_a;

                    if (rx >= -32.0f && rx <= 32.0f && ry >= -32.0f && ry <= 32.0f) {
                        continue;
                    }
                }

                int pixel_index = (y_p * SCREEN_WIDTH) + x_p;

                if (x_p >= x + 64) {
                    frame_buffer[pixel_index] = active_bg_color;
                } else if (x_p >= (mid_x - half_width) && x_p <= (mid_x + half_width)) {
                    frame_buffer[pixel_index] = 0xFF0000;
                } else {
                    frame_buffer[pixel_index] = active_bg_color;
                }
            }
        }
    }
}

// --- WORLD WRAPPERS ---
void draw_normal_world(const char* level_map, int level_length, int scroll_speed) {
	draw_text(1040, 20,"Level 1", 0xFFFFFF, 4);
    for (int i = -1; i < 21; i++) {
        int idx = level_index + i;
        if (idx >= 0 && idx < level_length && level_map[idx] == '^') {
            int spike_x = (i * 64) - pixel_offset;
            update_spike(image_buffer_pointer, spike_x, GROUND_Y, scroll_speed, sprite_y);
        }
    }
}

void draw_endless_world(int scroll_speed, int high_score, int last_score) {
    for (size_t i = 0; i < endless_spikes.size(); i++) {
        update_spike(image_buffer_pointer, endless_spikes[i], GROUND_Y, scroll_speed, sprite_y);
    }

    char score_str[64];
    snprintf(score_str, sizeof(score_str), "HIGH: %ds  LAST: %ds", high_score, last_score);

    int text_scale = 4;
    int text_width = strlen(score_str) * 8 * text_scale;
    int start_x = (SCREEN_WIDTH - text_width) / 2;

    draw_text(start_x, 20, score_str, 0xFFFF00, text_scale);
}

void draw_audio_world(const char* level_map, int level_length, int scroll_speed) {
	draw_text(920, 20,"Audio Level", 0xFFFFFF, 4);
    for (int i = -1; i < 21; i++) {
        int idx = level_index + i;
        if (idx >= 0 && idx < level_length && level_map[idx] == '^') {
            int spike_x = (i * 64) - pixel_offset;
            update_spike(image_buffer_pointer, spike_x, GROUND_Y, scroll_speed, sprite_y);
        }
    }
}

void draw_sprite_select_screen() {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        image_buffer_pointer[i] = 0x220022;
    }

    draw_text(300, 100, "SPRITE SELECTOR", 0xFFFF00, 6);
    draw_text(280, 800, "PRESS JUMP TO CYCLE SPRITES", 0xFFFFFF, 4);

    float temp_angle = sprite_angle;
    sprite_angle = 0.0f;
    draw_only_sprite(image_buffer_pointer, (SCREEN_WIDTH/2) - 32, (SCREEN_HEIGHT/2) - 32);
    sprite_angle = temp_angle;
}
