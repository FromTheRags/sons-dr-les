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
static SDL_AudioStream* g_audio_stream = NULL;
static SDL_AudioSpec g_target_spec;

// Audio data containers
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

static void SDLCALL audio_stream_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) {
    (void)userdata;
    (void)total_amount;
    if (additional_amount <= 0) return;

    std::vector<Uint8> mix_buf(additional_amount, 0);

    // 1. Mix background music
    if (g_music_playing && !g_music_paused && g_music.data && g_music.length > 0) {
        int bytes_left = additional_amount;
        Uint8* dst = mix_buf.data();

        while (bytes_left > 0) {
            Uint32 chunk = g_music.length - g_music_pos;
            if (chunk > (Uint32)bytes_left) {
                chunk = (Uint32)bytes_left;
            }

            SDL_MixAudio(dst, g_music.data + g_music_pos, g_target_spec.format, chunk, 0.40f);
            g_music_pos += chunk;
            dst += chunk;
            bytes_left -= chunk;

            if (g_music_pos >= g_music.length) {
                g_music_pos = 0;
            }
        }
    }

    // 2. Mix active SFX voices
    for (size_t i = 0; i < g_active_voices.size(); ) {
        ActiveVoice& voice = g_active_voices[i];
        Uint32 chunk = voice.length - voice.position;
        if (chunk > (Uint32)additional_amount) {
            chunk = (Uint32)additional_amount;
        }

        SDL_MixAudio(mix_buf.data(), voice.data + voice.position, g_target_spec.format, chunk, 1.0f);
        voice.position += chunk;

        if (voice.position >= voice.length) {
            g_active_voices.erase(g_active_voices.begin() + i);
        } else {
            ++i;
        }
    }

    SDL_PutAudioStreamData(stream, mix_buf.data(), additional_amount);
}

static bool load_and_convert_wav(const char* name, SoundBuffer* out_buf) {
    if (!out_buf) return false;
    std::string path = assets::get_asset_path(name);
    if (path.empty()) return false;

    SDL_AudioSpec wav_spec;
    Uint8* wav_data = NULL;
    Uint32 wav_len = 0;

    if (!SDL_LoadWAV(path.c_str(), &wav_spec, &wav_data, &wav_len)) {
        fprintf(stderr, "Erreur SDL_LoadWAV (%s) : %s\n", path.c_str(), SDL_GetError());
        return false;
    }

    // Convert to target spec if needed
    if (wav_spec.format == g_target_spec.format &&
        wav_spec.channels == g_target_spec.channels &&
        wav_spec.freq == g_target_spec.freq) {
        out_buf->data = (Uint8*)malloc(wav_len);
        if (out_buf->data) {
            memcpy(out_buf->data, wav_data, wav_len);
            out_buf->length = wav_len;
        }
    } else {
        SDL_AudioStream* cvt = SDL_CreateAudioStream(&wav_spec, &g_target_spec);
        if (cvt) {
            SDL_PutAudioStreamData(cvt, wav_data, (int)wav_len);
            SDL_FlushAudioStream(cvt);
            int available = SDL_GetAudioStreamAvailable(cvt);
            if (available > 0) {
                out_buf->data = (Uint8*)malloc(available);
                if (out_buf->data) {
                    SDL_GetAudioStreamData(cvt, out_buf->data, available);
                    out_buf->length = (Uint32)available;
                }
            }
            SDL_DestroyAudioStream(cvt);
        }
    }

    SDL_free(wav_data);
    return out_buf->data != NULL;
}

static void free_sound_buffer(SoundBuffer* buf) {
    if (buf && buf->data) {
        free(buf->data);
        buf->data = NULL;
        buf->length = 0;
    }
}

bool init() {
    if (g_initialized) return true;

    g_target_spec.channels = 2;
    g_target_spec.format = SDL_AUDIO_S16LE;
    g_target_spec.freq = 44100;

    g_audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &g_target_spec, audio_stream_callback, NULL);
    if (!g_audio_stream) {
        fprintf(stderr, "Échec de SDL_OpenAudioDeviceStream: %s\n", SDL_GetError());
        return false;
    }

    SDL_ResumeAudioStreamDevice(g_audio_stream);
    g_initialized = true;
    return true;
}

void quit() {
    if (!g_initialized) return;

    if (g_audio_stream) {
        SDL_PauseAudioStreamDevice(g_audio_stream);
        SDL_DestroyAudioStream(g_audio_stream);
        g_audio_stream = NULL;
    }

    stop_all_sfx();
    stop_music();

    // Free all loaded sound buffers
    for (int lvl = 0; lvl < 4; ++lvl) {
        for (int s = 0; s < 8; ++s) {
            free_sound_buffer(&g_level_sounds[lvl][s]);
        }
    }
    free_sound_buffer(&g_buzzer);
    free_sound_buffer(&g_undo);
    free_sound_buffer(&g_music);

    g_initialized = false;
}

bool load_sounds() {
    // 1. Level sounds: 4 levels x 8 sounds
    for (int lvl = 1; lvl <= 4; ++lvl) {
        for (int s = 1; s <= 8; ++s) {
            char fname[64];
            snprintf(fname, sizeof(fname), "sound_%d_%d.wav", lvl, s);
            load_and_convert_wav(fname, &g_level_sounds[lvl - 1][s - 1]);
        }
    }

    // 2. UI sounds
    load_and_convert_wav("buzzer.wav", &g_buzzer);
    load_and_convert_wav("undo.wav", &g_undo);

    // 3. Background music loop
    load_and_convert_wav("bg_music.wav", &g_music);

    return true;
}

static void play_buffer(const SoundBuffer& buf) {
    if (!g_initialized || !buf.data || buf.length == 0 || !g_audio_stream) return;

    SDL_LockAudioStream(g_audio_stream);
    ActiveVoice v;
    v.data = buf.data;
    v.length = buf.length;
    v.position = 0;
    g_active_voices.push_back(v);
    SDL_UnlockAudioStream(g_audio_stream);
}

void play_game_sound(int level, int sound_id) {
    if (level < 1 || level > 4 || sound_id < 1 || sound_id > 8) return;
    play_buffer(g_level_sounds[level - 1][sound_id - 1]);
}

void play_buzzer() {
    play_buffer(g_buzzer);
}

void play_undo() {
    play_buffer(g_undo);
}

void start_music() {
    if (g_audio_stream) SDL_LockAudioStream(g_audio_stream);
    g_music_playing = true;
    g_music_paused = false;
    g_music_pos = 0;
    if (g_audio_stream) SDL_UnlockAudioStream(g_audio_stream);
}

void stop_music() {
    if (g_audio_stream) SDL_LockAudioStream(g_audio_stream);
    g_music_playing = false;
    g_music_paused = false;
    g_music_pos = 0;
    if (g_audio_stream) SDL_UnlockAudioStream(g_audio_stream);
}

void toggle_pause() {
    if (g_audio_stream) SDL_LockAudioStream(g_audio_stream);
    g_music_paused = !g_music_paused;
    if (g_audio_stream) SDL_UnlockAudioStream(g_audio_stream);
}

bool is_paused() {
    return g_music_paused;
}

void stop_all_sfx() {
    if (g_audio_stream) SDL_LockAudioStream(g_audio_stream);
    g_active_voices.clear();
    if (g_audio_stream) SDL_UnlockAudioStream(g_audio_stream);
}

} // namespace audio
