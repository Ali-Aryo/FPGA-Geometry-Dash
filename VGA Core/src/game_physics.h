#ifndef GAME_PHYSICS_H
#define GAME_PHYSICS_H

#include <vector>
#include "xil_types.h"

// Sprite Physics Vars
extern int sprite_y;
extern int prev_sprite_y;
extern int vertical_velocity;
extern bool is_jumping;
extern float sprite_angle;

extern const int GROUND_Y;
extern const int GRAVITY;
extern const int JUMP_STRENGTH;

// World Tracking Vars
extern int level_index;
extern int pixel_offset;
extern std::vector<int> endless_spikes;
extern bool is_level_complete;

// Core Functions
void apply_jump_impulse();
void update_physics();
bool check_collisions(int spike_x);

// World Logic Managers
void reset_game_world();
bool update_normal_world(int scroll_speed, const char* level_map, int level_length);
bool update_endless_world(int scroll_speed, u32 lfsr_spike_threshold);

#endif
