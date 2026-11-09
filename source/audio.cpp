#include "audio.h"
#include "assets.h"
#include <vector>
#include <cstdio>
#include <cstdlib>

namespace audio {

struct SoundBuffer {
    Uint8* data = NULL;
    Uint32 length = 0;
};

struct ActiveVoice {
    Uint8* data = NULL;
    Uint32 length = 0;
    Uint32 position = 0;
};

static bool g_initialized = false;
static SDL_AudioSpec g_obtained_spec;

// Audio data containers
// Level sounds: level 1..4, index 1..8 -> [level-1][index-1]
static SoundBuffer g_level_sounds[4][8];
static SoundBuffer g_buzzer;
static SoundBuffer g_undo;
static SoundBuffer g_music;

// Music state
static bool g_music_playing = false;
static bool g_music_paused = false;
static Uint32 g_music_pos = 0;

// Active SFX voices
static std::vector<ActiveVoice> g_active_voices;

static void audio_callback(void* userdata, Uint8* stream, int len) {
    (void)userdata;
    SDL_memset(stream, 0, len);

    // 1. Mix background music if playing and not paused
    if (g_music_playing && !g_music_paused && g_music.data && g_music.length > 0) {
        int bytes_left_to_mix = len;
        Uint8* dst = stream;

        while (bytes_left_to_mix > 0) {
            Uint32 chunk = g_music.length - g_music_pos;
            if (chunk > (Uint32)bytes_left_to_mix) {
                chunk = (Uint32)bytes_left_to_mix;
            }

            // Mix background music with moderate volume (approx 40%)
            SDL_MixAudio(dst, g_music.data + g_music_pos, chunk, SDL_MIX_MAXVOLUME / 2);
            g_music_pos += chunk;
            dst += chunk;
            bytes_left_to_mix -= chunk;

            // Loop music
            if (g_music_pos >= g_music.length) {
                g_music_pos = 0;
            }
        }
    }

    // 2. Mix active SFX voices
    for (size_t i = 0; i < g_active_voices.size(); ) {
        ActiveVoice& voice = g_active_voices[i];
        Uint32 remaining = voice.length - voice.position;
        Uint32 to_mix = (remaining > (Uint32)len) ? (Uint32)len : remaining;

        SDL_MixAudio(stream, voice.data + voice.position, to_mix, SDL_MIX_MAXVOLUME);
        voice.position += to_mix;

        if (voice.position >= voice.length) {
            g_active_voices.erase(g_active_voices.begin() + i);
        } else {
            ++i;
        }
    }
}

static bool load_wav_file(const char* filename, SoundBuffer& out_buf) {
    std::string path = assets::get_asset_path(filename);
    SDL_AudioSpec wav_spec;
    Uint32 wav_len = 0;
    Uint8* wav_data = NULL;

    if (!SDL_LoadWAV(path.c_str(), &wav_spec, &wav_data, &wav_len)) {
        fprintf(stderr, "Erreur chargement WAV %s (%s) : %s\n", filename, path.c_str(), SDL_GetError());
        return false;
    }

    // Check if format conversion is needed
    if (wav_spec.format == g_obtained_spec.format &&
        wav_spec.channels == g_obtained_spec.channels &&
        wav_spec.freq == g_obtained_spec.freq) {
        out_buf.data = wav_data;
        out_buf.length = wav_len;
        return true;
    }

    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt, wav_spec.format, wav_spec.channels, wav_spec.freq,
                          g_obtained_spec.format, g_obtained_spec.channels, g_obtained_spec.freq) < 0) {
        fprintf(stderr, "Erreur build audio CVT pour %s : %s\n", filename, SDL_GetError());
        SDL_FreeWAV(wav_data);
        return false;
    }

    cvt.len = wav_len;
    cvt.buf = (Uint8*)malloc(wav_len * cvt.len_mult);
    if (!cvt.buf) {
        SDL_FreeWAV(wav_data);
        return false;
    }

    memcpy(cvt.buf, wav_data, wav_len);
    SDL_FreeWAV(wav_data);

    if (SDL_ConvertAudio(&cvt) < 0) {
        fprintf(stderr, "Erreur conversion audio pour %s : %s\n", filename, SDL_GetError());
        free(cvt.buf);
        return false;
    }

    out_buf.data = cvt.buf;
    out_buf.length = (Uint32)(cvt.len * cvt.len_ratio);
    return true;
}

static void free_sound_buffer(SoundBuffer& buf) {
    if (buf.data) {
        free(buf.data);
        buf.data = NULL;
        buf.length = 0;
    }
}

bool init() {
    if (g_initialized) return true;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Impossible d'initialiser SDL audio : %s\n", SDL_GetError());
        return false;
    }

    SDL_AudioSpec desired;
    SDL_memset(&desired, 0, sizeof(desired));
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = 1;      // Mono
    desired.samples = 1024;    // Low latency
    desired.callback = audio_callback;
    desired.userdata = NULL;

    if (SDL_OpenAudio(&desired, &g_obtained_spec) < 0) {
        fprintf(stderr, "Impossible d'ouvrir le périphérique audio SDL : %s\n", SDL_GetError());
        return false;
    }

    // Start playback thread (unpause)
    SDL_PauseAudio(0);
    g_initialized = true;
    return true;
}

bool load_sounds() {
    if (!g_initialized) {
        if (!init()) return false;
    }

    // 1. UI SFX
    load_wav_file("buzzer.wav", g_buzzer);
    load_wav_file("undo.wav", g_undo);
    load_wav_file("bg_music.wav", g_music);

    // 2. 32 Game Sounds for Levels 1 to 4
    for (int lvl = 1; lvl <= 4; ++lvl) {
        for (int snd = 1; snd <= 8; ++snd) {
            char fname[64];
            snprintf(fname, sizeof(fname), "sound_%d_%d.wav", lvl, snd);
            load_wav_file(fname, g_level_sounds[lvl - 1][snd - 1]);
        }
    }

    return true;
}

void play_game_sound(int level, int sound_id) {
    if (!g_initialized) return;
    if (level < 1 || level > 5 || sound_id < 1 || sound_id > 8) return;

    // For level 5, pick from level 1, 3 or 4 sets
    int actual_lvl = (level <= 4) ? level : 1;

    SoundBuffer& sb = g_level_sounds[actual_lvl - 1][sound_id - 1];
    if (!sb.data || sb.length == 0) return;

    SDL_LockAudio();
    ActiveVoice v;
    v.data = sb.data;
    v.length = sb.length;
    v.position = 0;
    g_active_voices.push_back(v);
    SDL_UnlockAudio();
}

void play_buzzer() {
    if (!g_initialized || !g_buzzer.data) return;
    SDL_LockAudio();
    ActiveVoice v;
    v.data = g_buzzer.data;
    v.length = g_buzzer.length;
    v.position = 0;
    g_active_voices.push_back(v);
    SDL_UnlockAudio();
}

void play_undo() {
    if (!g_initialized || !g_undo.data) return;
    SDL_LockAudio();
    ActiveVoice v;
    v.data = g_undo.data;
    v.length = g_undo.length;
    v.position = 0;
    g_active_voices.push_back(v);
    SDL_UnlockAudio();
}

void start_music() {
    SDL_LockAudio();
    g_music_playing = true;
    g_music_paused = false;
    g_music_pos = 0;
    SDL_UnlockAudio();
}

void stop_music() {
    SDL_LockAudio();
    g_music_playing = false;
    g_music_paused = false;
    g_music_pos = 0;
    SDL_UnlockAudio();
}

void toggle_pause() {
    SDL_LockAudio();
    g_music_paused = !g_music_paused;
    SDL_UnlockAudio();
}

bool is_paused() {
    return g_music_paused;
}

void stop_all_sfx() {
    SDL_LockAudio();
    g_active_voices.clear();
    SDL_UnlockAudio();
}

void quit() {
    if (!g_initialized) return;

    SDL_PauseAudio(1);
    SDL_CloseAudio();

    free_sound_buffer(g_buzzer);
    free_sound_buffer(g_undo);
    free_sound_buffer(g_music);

    for (int lvl = 0; lvl < 4; ++lvl) {
        for (int snd = 0; snd < 8; ++snd) {
            free_sound_buffer(g_level_sounds[lvl][snd]);
        }
    }

    g_active_voices.clear();
    g_initialized = false;
}

} // namespace audio
