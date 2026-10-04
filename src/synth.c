#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/wait.h>

#include "synth.h"
#include "audio.h"

static SynthState g_synth;
static const char *SYNTH_WAV_PATH = "/dev/tabos_synth.wav";

void synth_init(void) {
    memset(&g_synth, 0, sizeof(g_synth));

    /* Pulse 1: Melodi/lead */
    g_synth.pulse1.enabled = 1;
    g_synth.pulse1.freq = 440.0f; /* A4 */
    g_synth.pulse1.volume = 12;
    g_synth.pulse1.duty = DUTY_50_0;
    g_synth.pulse1.phase = 0.0f;

    /* Pulse 2: Harmoni/detune */
    g_synth.pulse2.enabled = 0;
    g_synth.pulse2.freq = 440.0f;
    g_synth.pulse2.volume = 8;
    g_synth.pulse2.duty = DUTY_25_0;
    g_synth.pulse2.phase = 0.0f;

    /* Triangle: Dyp bass */
    g_synth.triangle.enabled = 0;
    g_synth.triangle.freq = 220.0f; /* A3 */
    g_synth.triangle.phase = 0.0f;

    /* Noise: Lo-fi metallisk perkussjon */
    g_synth.noise.enabled = 0;
    g_synth.noise.volume = 10;
    g_synth.noise.lfsr = 0x7FFF; /* 15-bit shift register initiert til alle 1-ere */
    g_synth.noise.shift_rate = 8000.0f;
    g_synth.noise.phase = 0.0f;
    g_synth.noise.last_sample = 0;

    /* Filter & Envelope */
    g_synth.filter.lpf_enabled = 0;
    g_synth.filter.cutoff = 1.0f;
    g_synth.filter.lpf_state = 0.0f;
    g_synth.filter.bitcrush = 0;
    g_synth.filter.attack_time = 0.01f;
    g_synth.filter.decay_time = 0.25f;
    g_synth.filter.sustain_lvl = 0.6f;
    g_synth.filter.release_time = 0.08f;
}

SynthState *synth_get_state(void) {
    return &g_synth;
}

/*
 * mix_nes_buffer:
 * Genererer 'length' samples med rå signed 16-bit PCM (44100 Hz).
 * Akkumulerer fase for hver kanal, beregner bølgeform, summerer
 * og beskytter mot digital klipping (soft/hard limiter).
 */
void mix_nes_buffer(int16_t *buffer, size_t length) {
    const float inv_rate = 1.0f / (float)SYNTH_SAMPLE_RATE;

    /* Duty cycle terskler */
    static const float duty_thresholds[4] = { 0.125f, 0.250f, 0.500f, 0.750f };

    for (size_t i = 0; i < length; i++) {
        int32_t mix = 0;

        /* 1. Pulse / Firkantbølge 1 */
        if (g_synth.pulse1.enabled && g_synth.pulse1.freq > 10.0f) {
            float thresh = duty_thresholds[g_synth.pulse1.duty & 3];
            int16_t amp = (int16_t)(g_synth.pulse1.volume * 1100);
            mix += (g_synth.pulse1.phase < thresh) ? amp : -amp;

            g_synth.pulse1.phase += g_synth.pulse1.freq * inv_rate;
            if (g_synth.pulse1.phase >= 1.0f) g_synth.pulse1.phase -= 1.0f;
        }

        /* 2. Pulse / Firkantbølge 2 */
        if (g_synth.pulse2.enabled && g_synth.pulse2.freq > 10.0f) {
            float thresh = duty_thresholds[g_synth.pulse2.duty & 3];
            int16_t amp = (int16_t)(g_synth.pulse2.volume * 1100);
            mix += (g_synth.pulse2.phase < thresh) ? amp : -amp;

            g_synth.pulse2.phase += g_synth.pulse2.freq * inv_rate;
            if (g_synth.pulse2.phase >= 1.0f) g_synth.pulse2.phase -= 1.0f;
        }

        /* 3. Trekantbølge (Triangle) - Fast amplitude på NES */
        if (g_synth.triangle.enabled && g_synth.triangle.freq > 10.0f) {
            float tri_val;
            if (g_synth.triangle.phase < 0.5f) {
                tri_val = -1.0f + (4.0f * g_synth.triangle.phase);
            } else {
                tri_val = 3.0f - (4.0f * g_synth.triangle.phase);
            }
            mix += (int32_t)(tri_val * 14000.0f);

            g_synth.triangle.phase += g_synth.triangle.freq * inv_rate;
            if (g_synth.triangle.phase >= 1.0f) g_synth.triangle.phase -= 1.0f;
        }

        /* 4. Lo-Fi Støy (15-bit LFSR) */
        if (g_synth.noise.enabled) {
            g_synth.noise.phase += g_synth.noise.shift_rate * inv_rate;
            if (g_synth.noise.phase >= 1.0f) {
                g_synth.noise.phase -= 1.0f;
                /* NES LFSR: feedback = bit0 ^ bit1 */
                uint16_t b0 = g_synth.noise.lfsr & 1;
                uint16_t b1 = (g_synth.noise.lfsr >> 1) & 1;
                uint16_t feedback = b0 ^ b1;
                g_synth.noise.lfsr = (g_synth.noise.lfsr >> 1) | (feedback << 14);

                int16_t n_amp = (int16_t)(g_synth.noise.volume * 1000);
                g_synth.noise.last_sample = (g_synth.noise.lfsr & 1) ? n_amp : -n_amp;
            }
            mix += g_synth.noise.last_sample;
        }

        /* Enkelt 1-pols IIR Lavpassfilter (LPF) */
        if (g_synth.filter.lpf_enabled && g_synth.filter.cutoff < 0.98f) {
            float alpha = g_synth.filter.cutoff * g_synth.filter.cutoff;
            if (alpha < 0.005f) alpha = 0.005f;
            g_synth.filter.lpf_state += alpha * ((float)mix - g_synth.filter.lpf_state);
            mix = (int32_t)g_synth.filter.lpf_state;
        }

        /* Bitcrusher (8-bit eller 4-bit lo-fi kvantisering) */
        if (g_synth.filter.bitcrush == 8) {
            mix = (mix / 256) * 256;
        } else if (g_synth.filter.bitcrush == 4) {
            mix = (mix / 4096) * 4096;
        }

        /* Digital clipping beskyttelse (Hard limiter) */
        if (mix > 32700) mix = 32700;
        if (mix < -32700) mix = -32700;

        buffer[i] = (int16_t)mix;
    }
}

/*
 * synth_render_wav:
 * Rask generering av en komplett 44.1 kHz 16-bit mono WAV-fil
 * med ADSR volum-konvolutt og valgfri pitch-sweep.
 */
void synth_render_wav(const char *path, float duration_sec, float pitch_start, float pitch_end) {
    if (duration_sec <= 0.05f) duration_sec = 0.05f;
    if (duration_sec > 2.0f)   duration_sec = 2.0f;

    size_t total_samples = (size_t)(duration_sec * SYNTH_SAMPLE_RATE);
    int16_t *buf = (int16_t *)malloc(total_samples * sizeof(int16_t));
    if (!buf) return;

    /* Lagre startfrekvenser */
    float orig_p1 = g_synth.pulse1.freq;
    float orig_p2 = g_synth.pulse2.freq;
    float orig_tri = g_synth.triangle.freq;

    /* Generer i blokker for å kunne modulere pitch og envelope */
    const size_t BLOCK_SIZE = 128;
    for (size_t offset = 0; offset < total_samples; offset += BLOCK_SIZE) {
        size_t chunk = BLOCK_SIZE;
        if (offset + chunk > total_samples) chunk = total_samples - offset;

        float t = (float)offset / (float)total_samples;

        /* Pitch sweep */
        if (pitch_start > 0.0f && pitch_end > 0.0f) {
            float cur_pitch = pitch_start + (pitch_end - pitch_start) * t;
            if (g_synth.pulse1.enabled) g_synth.pulse1.freq = cur_pitch;
            if (g_synth.pulse2.enabled) g_synth.pulse2.freq = cur_pitch * 0.995f;
            if (g_synth.triangle.enabled) g_synth.triangle.freq = cur_pitch * 0.5f;
        }

        mix_nes_buffer(&buf[offset], chunk);

        /* ADSR Volum-konvolutt */
        float note_time = (float)offset / (float)SYNTH_SAMPLE_RATE;
        float env = 1.0f;

        float att = g_synth.filter.attack_time;
        float dec = g_synth.filter.decay_time;
        float sus = g_synth.filter.sustain_lvl;
        float rel = g_synth.filter.release_time;
        float sus_time = duration_sec - att - dec - rel;
        if (sus_time < 0.0f) sus_time = 0.0f;

        if (note_time < att) {
            env = note_time / (att > 0.001f ? att : 0.001f);
        } else if (note_time < (att + dec)) {
            float dt = note_time - att;
            env = 1.0f - (1.0f - sus) * (dt / (dec > 0.001f ? dec : 0.001f));
        } else if (note_time < (att + dec + sus_time)) {
            env = sus;
        } else {
            float rt = note_time - (att + dec + sus_time);
            env = sus * (1.0f - (rt / (rel > 0.001f ? rel : 0.001f)));
            if (env < 0.0f) env = 0.0f;
        }

        for (size_t s = 0; s < chunk; s++) {
            buf[offset + s] = (int16_t)(buf[offset + s] * env);
        }
    }

    /* Gjenopprett frekvenser */
    g_synth.pulse1.freq = orig_p1;
    g_synth.pulse2.freq = orig_p2;
    g_synth.triangle.freq = orig_tri;

    /* Skriv WAV-header og PCM-data */
    FILE *f = fopen(path, "wb");
    if (f) {
        uint32_t data_bytes = (uint32_t)(total_samples * 2);
        uint32_t riff_size = 36 + data_bytes;
        uint32_t sample_rate = SYNTH_SAMPLE_RATE;
        uint32_t byte_rate = sample_rate * 2; /* 16-bit mono = 2 bytes/sample */
        uint16_t block_align = 2;
        uint16_t bits_per_sample = 16;
        uint16_t num_channels = 1;
        uint16_t audio_format = 1; /* PCM */
        uint32_t subchunk1_size = 16;

        fwrite("RIFF", 1, 4, f);
        fwrite(&riff_size, 4, 1, f);
        fwrite("WAVE", 1, 4, f);

        fwrite("fmt ", 1, 4, f);
        fwrite(&subchunk1_size, 4, 1, f);
        fwrite(&audio_format, 2, 1, f);
        fwrite(&num_channels, 2, 1, f);
        fwrite(&sample_rate, 4, 1, f);
        fwrite(&byte_rate, 4, 1, f);
        fwrite(&block_align, 2, 1, f);
        fwrite(&bits_per_sample, 2, 1, f);

        fwrite("data", 1, 4, f);
        fwrite(&data_bytes, 4, 1, f);
        fwrite(buf, 2, total_samples, f);
        fclose(f);
    }

    free(buf);
}

static pid_t g_last_audio_pid = -1;

static void play_wav_async(const char *path) {
    /* Stopp forrige tone umiddelbart slik at CPU ikke overbelastes og lyden ikke hakker */
    if (g_last_audio_pid > 0) {
        kill(g_last_audio_pid, SIGKILL);
        waitpid(g_last_audio_pid, NULL, WNOHANG);
        g_last_audio_pid = -1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execl("/system/bin/stagefright", "stagefright", "-a", "-o", path, (char *)NULL);
        _exit(0);
    } else if (pid > 0) {
        g_last_audio_pid = pid;
    }
}

void synth_play_note(float freq, float duration_sec) {
    g_synth.pulse1.freq = freq;
    g_synth.pulse2.freq = freq * 1.003f; /* Liten analog-lignende unison detune */
    g_synth.triangle.freq = freq * 0.5f; /* 1 oktav dypere for fyldig bass */

    synth_render_wav(SYNTH_WAV_PATH, duration_sec, 0.0f, 0.0f);
    play_wav_async(SYNTH_WAV_PATH);
}

void synth_play_preset(int preset_id) {
    switch (preset_id) {
        case 1: /* Laser blaster (Klassisk 8-bit sci-fi) */
            g_synth.pulse1.enabled = 1;
            g_synth.pulse1.duty = DUTY_25_0;
            g_synth.pulse1.volume = 14;
            g_synth.pulse2.enabled = 0;
            g_synth.triangle.enabled = 0;
            g_synth.noise.enabled = 1;
            g_synth.noise.volume = 8;
            g_synth.noise.shift_rate = 14000.0f;
            g_synth.filter.attack_time = 0.005f;
            g_synth.filter.decay_time = 0.18f;
            g_synth.filter.sustain_lvl = 0.0f;
            g_synth.filter.release_time = 0.02f;
            synth_render_wav(SYNTH_WAV_PATH, 0.22f, 1600.0f, 150.0f);
            break;

        case 2: /* Super Mario Coin / Powerup */
            g_synth.pulse1.enabled = 1;
            g_synth.pulse1.duty = DUTY_50_0;
            g_synth.pulse1.volume = 13;
            g_synth.pulse2.enabled = 0;
            g_synth.triangle.enabled = 0;
            g_synth.noise.enabled = 0;
            g_synth.filter.attack_time = 0.01f;
            g_synth.filter.decay_time = 0.20f;
            g_synth.filter.sustain_lvl = 0.3f;
            g_synth.filter.release_time = 0.05f;
            synth_render_wav(SYNTH_WAV_PATH, 0.28f, 987.77f, 1318.51f); /* B5 -> E6 */
            break;

        case 3: /* 8-bit Eksplosjon / Snare */
            g_synth.pulse1.enabled = 1;
            g_synth.pulse1.duty = DUTY_12_5;
            g_synth.pulse1.volume = 12;
            g_synth.triangle.enabled = 1;
            g_synth.triangle.freq = 80.0f;
            g_synth.noise.enabled = 1;
            g_synth.noise.volume = 15;
            g_synth.noise.shift_rate = 5000.0f;
            g_synth.filter.attack_time = 0.002f;
            g_synth.filter.decay_time = 0.35f;
            g_synth.filter.sustain_lvl = 0.0f;
            g_synth.filter.release_time = 0.05f;
            synth_render_wav(SYNTH_WAV_PATH, 0.40f, 250.0f, 40.0f);
            break;

        case 4: /* 1-UP / Powerup fanfare */
            g_synth.pulse1.enabled = 1;
            g_synth.pulse1.duty = DUTY_50_0;
            g_synth.pulse1.volume = 13;
            g_synth.triangle.enabled = 1;
            g_synth.triangle.freq = 330.0f;
            g_synth.noise.enabled = 0;
            g_synth.filter.attack_time = 0.01f;
            g_synth.filter.decay_time = 0.35f;
            g_synth.filter.sustain_lvl = 0.5f;
            g_synth.filter.release_time = 0.05f;
            synth_render_wav(SYNTH_WAV_PATH, 0.45f, 330.0f, 990.0f);
            break;

        case 5: /* Chiptune Bass Slap */
            g_synth.pulse1.enabled = 1;
            g_synth.pulse1.duty = DUTY_75_0;
            g_synth.pulse1.volume = 12;
            g_synth.triangle.enabled = 1;
            g_synth.triangle.freq = 110.0f; /* A2 */
            g_synth.noise.enabled = 0;
            g_synth.filter.attack_time = 0.005f;
            g_synth.filter.decay_time = 0.22f;
            g_synth.filter.sustain_lvl = 0.1f;
            g_synth.filter.release_time = 0.04f;
            synth_render_wav(SYNTH_WAV_PATH, 0.28f, 220.0f, 110.0f);
            break;

        default:
            synth_play_note(440.0f, 0.25f);
            return;
    }

    play_wav_async(SYNTH_WAV_PATH);
}

void synth_play_arpeggio(int pattern_id) {
    /* Rask 8-toners chiptune melodi */
    static const float riff_c_major[] = {
        261.63f, 329.63f, 392.00f, 523.25f, 659.25f, 523.25f, 392.00f, 329.63f
    };
    static const float riff_cyber_minor[] = {
        220.00f, 261.63f, 329.63f, 440.00f, 523.25f, 440.00f, 329.63f, 293.66f
    };

    const float *notes = (pattern_id == 2) ? riff_cyber_minor : riff_c_major;
    const size_t note_count = 8;
    const float note_dur = 0.09f; /* 90 ms per tone = kjapt og funky arpeggio */
    const size_t note_samples = (size_t)(note_dur * SYNTH_SAMPLE_RATE);
    const size_t total_samples = note_samples * note_count;

    int16_t *buf = (int16_t *)malloc(total_samples * sizeof(int16_t));
    if (!buf) return;

    for (size_t n = 0; n < note_count; n++) {
        g_synth.pulse1.freq = notes[n];
        g_synth.pulse2.freq = notes[n] * 1.004f;
        g_synth.triangle.freq = notes[n] * 0.5f;

        mix_nes_buffer(&buf[n * note_samples], note_samples);

        /* Hurtig per-note decay for arpeggio-effekt */
        for (size_t s = 0; s < note_samples; s++) {
            float fade = 1.0f - ((float)s / (float)note_samples) * 0.4f;
            buf[n * note_samples + s] = (int16_t)(buf[n * note_samples + s] * fade);
        }
    }

    /* Skriv WAV */
    FILE *f = fopen(SYNTH_WAV_PATH, "wb");
    if (f) {
        uint32_t data_bytes = (uint32_t)(total_samples * 2);
        uint32_t riff_size = 36 + data_bytes;
        uint32_t sample_rate = SYNTH_SAMPLE_RATE;
        uint32_t byte_rate = sample_rate * 2;
        uint16_t block_align = 2;
        uint16_t bits_per_sample = 16;
        uint16_t num_channels = 1;
        uint16_t audio_format = 1;
        uint32_t subchunk1_size = 16;

        fwrite("RIFF", 1, 4, f);
        fwrite(&riff_size, 4, 1, f);
        fwrite("WAVE", 1, 4, f);
        fwrite("fmt ", 1, 4, f);
        fwrite(&subchunk1_size, 4, 1, f);
        fwrite(&audio_format, 2, 1, f);
        fwrite(&num_channels, 2, 1, f);
        fwrite(&sample_rate, 4, 1, f);
        fwrite(&byte_rate, 4, 1, f);
        fwrite(&block_align, 2, 1, f);
        fwrite(&bits_per_sample, 2, 1, f);
        fwrite("data", 1, 4, f);
        fwrite(&data_bytes, 4, 1, f);
        fwrite(buf, 2, total_samples, f);
        fclose(f);
    }

    free(buf);
    play_wav_async(SYNTH_WAV_PATH);
}
