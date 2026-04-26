#ifndef VIDEO_DRIVER_H
#define VIDEO_DRIVER_H

#include <vector>

// Screen Constants
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern const int NUM_BYTES_BUFFER;

// Buffer Pointers
extern int* image_buffer_pointer;

// --- SPRITE SELECTION ---
extern int active_sprite;
void draw_sprite_select_screen();

// Core Video Functions
void video_init();
void draw_menu_screen();
void draw_pause_screen();
void draw_full_gameplay_background(int* frame_buffer);
void set_game_colors(int bg_color, int floor_color);

// --- TEXT & UI FUNCTIONS ---
void draw_char(int* frame_buffer, char c, int x, int y, int color, int scale);
void draw_text(int x, int y, const char* str, int color, int scale);
void draw_naming_screen(const char* current_name);

// --- SPRITE & GAME GRAPHICS ---
void erase_prev_sprite(int* frame_buffer, int x, int y_old);
void draw_only_sprite(int* frame_buffer, int x, int y_new);
void update_spike(int* frame_buffer, int x, int y, int scroll_speed, int current_sprite_y);

// --- WORLD RENDERING ---
void draw_normal_world(const char* level_map, int level_length, int scroll_speed);
void draw_endless_world(int scroll_speed, int high_score, int last_score);
void draw_audio_world(const char* level_map, int level_length, int scroll_speed);

#endif
