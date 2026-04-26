#include "game_physics.h"
#include "hardware_init.h"

// --- SPRITE STATE ---
int sprite_y = 850;
int prev_sprite_y = 850;
int vertical_velocity = 0;
bool is_jumping = false;
float sprite_angle = 0.0f;

const int GROUND_Y = 850;
const int GRAVITY = 1;
const int JUMP_STRENGTH = -20;

// --- WORLD STATE ---
int level_index = 0;
int pixel_offset = 0;
bool is_level_complete = false;

std::vector<int> endless_spikes;
int endless_buffer_counter = 0;
int consecutive_spikes = 0;
bool is_in_cluster = false;
const int MANDATORY_BUFFER = 8;


void reset_game_world() {
    level_index = 0;
    pixel_offset = 0;
    is_level_complete = false;

    endless_spikes.clear();
    endless_buffer_counter = MANDATORY_BUFFER;
    consecutive_spikes = 0;
    is_in_cluster = false;

    sprite_y = GROUND_Y;
    vertical_velocity = 0;
    is_jumping = false;
    sprite_angle = 0.0f;
}

void apply_jump_impulse() {
    vertical_velocity = JUMP_STRENGTH;
    is_jumping = true;
}

void update_physics() {
    if (is_jumping) {
        prev_sprite_y = sprite_y;
        sprite_y += vertical_velocity;
        vertical_velocity += GRAVITY;

        sprite_angle += 2.25f;

        if (sprite_y >= GROUND_Y) {
            sprite_y = GROUND_Y;
            is_jumping = false;
            vertical_velocity = 0;

            int nearest_90 = ((int)(sprite_angle + 45.0f) / 90) * 90;
                        sprite_angle = (float)nearest_90;


                        if (sprite_angle >= 360.0f) {
                            sprite_angle -= 360.0f;
                        }
        }
    }
}

bool check_collisions(int spike_x) {
    // 1. Player Hitbox
    int p_left = 200 + 8;
    int p_right = 264 - 8;
    int p_top = sprite_y + 8;
    int p_bottom = sprite_y + 64 - 8;

    // 2. Spike Hitbox
    int s_left = spike_x + 20;
    int s_right = spike_x + 44;
    int s_top = GROUND_Y + 24;
    int s_bottom = GROUND_Y + 64;


    if (p_right > s_left && p_left < s_right && p_bottom > s_top && p_top < s_bottom) {
        return true;
    }
    return false;
}

bool update_normal_world(int scroll_speed, const char* level_map, int level_length) {
    pixel_offset += scroll_speed;

    if (pixel_offset >= 64) {
        pixel_offset -= 64;


        if (!is_level_complete) {
            level_index++;

            if (level_index >= level_length) {
                is_level_complete = true;
            }
        }
    }

    bool collision_detected = false;
    for (int i = -1; i < 21; i++) {
        int idx = level_index + i;


        if (idx >= 0 && idx < level_length && level_map[idx] == '^') {
            int spike_x = (i * 64) - pixel_offset;
            if (check_collisions(spike_x)) {
                collision_detected = true;
            }
        }
    }
    return collision_detected;
}

bool update_endless_world(int scroll_speed, u32 lfsr_spike_threshold) {
    pixel_offset += scroll_speed;

    if (pixel_offset >= 64) {
        pixel_offset -= 64;

        u32 lfsr_raw = read_lfsr();
        bool lfsr_trigger = (lfsr_raw > lfsr_spike_threshold);

        if (endless_buffer_counter >= MANDATORY_BUFFER) {
            if (lfsr_trigger && consecutive_spikes < 3) {
                endless_spikes.push_back(1280);
                consecutive_spikes++;
                is_in_cluster = true;
                if (consecutive_spikes >= 3) {
                    is_in_cluster = false;
                    endless_buffer_counter = 0;
                    consecutive_spikes = 0;
                }
            } else if (is_in_cluster) {
                is_in_cluster = false;
                endless_buffer_counter = 0;
                consecutive_spikes = 0;
            }
        } else {
            endless_buffer_counter++;
        }
    }

    bool collision_detected = false;
    for (size_t i = 0; i < endless_spikes.size(); i++) {
        endless_spikes[i] -= scroll_speed;

        if (check_collisions(endless_spikes[i])) {
            collision_detected = true;
        }

        if (endless_spikes[i] < -64) {
            endless_spikes.erase(endless_spikes.begin() + i);
            i--;
        }
    }
    return collision_detected;
}
