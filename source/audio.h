#ifndef AUDIO_H
#define AUDIO_H

#include "common.h"

namespace audio {

bool init();
void quit();

bool load_sounds();

// Play sound associated with a specific level (level 1..4, sound 1..8)
void play_game_sound(int level, int sound_id);

// UI SFX
void play_buzzer();
void play_undo();

// Background ambient music
void start_music();
void stop_music();
void toggle_pause();
bool is_paused();

// Stop any currently playing sound effects
void stop_all_sfx();

} // namespace audio

#endif // AUDIO_H
