#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "app.h"
#include "display.h"
#include "input.h"
#include "os.h"
#include "audio.h"
#include "synth.h"

/* Standard frekvenser for oktav 4 (C4 .. C5) */
static const float BASE_NOTE_FREQS[13] = {
    261.63f, /* 0: C   */
    277.18f, /* 1: C#  */
    293.66f, /* 2: D   */
    311.13f, /* 3: D#  */
    329.63f, /* 4: E   */
    349.23f, /* 5: F   */
    369.99f, /* 6: F#  */
    392.00f, /* 7: G   */
    415.30f, /* 8: G#  */
    440.00f, /* 9: A   */
    466.16f, /* 10: A# */
    493.88f, /* 11: B  */
    523.25f  /* 12: C+1 */
};

static const char *NOTE_NAMES[13] = {
    "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B ", "C+"
};

static int g_current_octave = 4; /* 3, 4 eller 5 */
static int g_active_note = -1;
static char g_status_msg[64] = "Klar for retro syntese! Trykk pa en tangent.";
static int16_t g_scope_samples[64];

/* Hjelpefunksjon for å generere et live oscilloskop-bilde */
static void update_oscilloscope(void) {
    mix_nes_buffer(g_scope_samples, 64);
}

static void synth_app_start(void) {
    synth_init();
    g_active_note = -1;
    snprintf(g_status_msg, sizeof(g_status_msg), "NES APU Synthesizer klar (44100 Hz 16-bit PCM)");
    update_oscilloscope();
}

static bool synth_app_update(uint32_t delta_ms) {
    (void)delta_ms;
    return true;
}

static void draw_oscilloscope(int start_x, int start_y, int width, int height) {
    display_draw_box(start_x, start_y, width, height, "OSCILLOSKOP", ANSI_LIGHT_GREEN, ANSI_BLACK, ANSI_DARK_GRAY);

    int mid_y = start_y + (height / 2);
    int plot_w = width - 4;
    if (plot_w > 60) plot_w = 60;

    for (int col = 0; col < plot_w; col++) {
        int idx = (col * 64) / plot_w;
        int16_t val = g_scope_samples[idx];
        /* Skaler val (-32768 .. +32767) til (-2 .. +2) piksler fra midtlinjen */
        int offset = (val * (height - 3)) / 65536;
        int y = mid_y - offset;
        if (y <= start_y) y = start_y + 1;
        if (y >= start_y + height - 1) y = start_y + height - 2;

        display_put_cell(start_x + 2 + col, y, (val >= 0 ? 0xDF : 0xDC), ANSI_LIGHT_GREEN, ANSI_BLACK);
    }
}

static void synth_render_landscape(void) {
    display_clear(ANSI_BLACK);
    SynthState *s = synth_get_state();

    /* Toppbanner */
    display_draw_box(0, 0, 100, 3, "", ANSI_WHITE, ANSI_BLUE, ANSI_YELLOW);
    display_draw_string(2, 1, "tabOS RETRO SYNTHESIZER (Ricoh 2A03 APU & DSP Filter)", ANSI_WHITE, ANSI_BLUE);
    display_draw_button(74, 1, 24, " [X] TILBAKE TIL LAUNCHER ", ANSI_WHITE, ANSI_RED, false);

    /* Oscilloskop (Venstre kolonne: X=2, Y=3, W=38, H=8) */
    draw_oscilloscope(2, 3, 38, 8);

    /* Kontrollpanel & Kanalstatus (Høyre boks: X=42, Y=3, W=56, H=8) */
    display_draw_box(42, 3, 56, 8, "LYDKANALER (NES APU)", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_CYAN);
    
    char p1_str[32], p2_str[32], tri_str[32], noi_str[32];
    const char *duty_str[4] = { "12.5%", "25.0%", "50.0%", "75.0%" };
    snprintf(p1_str, sizeof(p1_str), " [P1: %s %s] ", s->pulse1.enabled ? "PAA" : "AV ", duty_str[s->pulse1.duty & 3]);
    snprintf(p2_str, sizeof(p2_str), " [P2: %s %s] ", s->pulse2.enabled ? "PAA" : "AV ", duty_str[s->pulse2.duty & 3]);
    snprintf(tri_str, sizeof(tri_str), " [TRI: %s] ", s->triangle.enabled ? "PAA" : "AV ");
    snprintf(noi_str, sizeof(noi_str), " [STOY: %s] ", s->noise.enabled ? "PAA" : "AV ");

    display_draw_button(44, 4, 25, p1_str, ANSI_WHITE, s->pulse1.enabled ? ANSI_GREEN : ANSI_DARK_GRAY, false);
    display_draw_button(71, 4, 25, p2_str, ANSI_WHITE, s->pulse2.enabled ? ANSI_GREEN : ANSI_DARK_GRAY, false);
    display_draw_button(44, 7, 25, tri_str, ANSI_WHITE, s->triangle.enabled ? ANSI_CYAN : ANSI_DARK_GRAY, false);
    display_draw_button(71, 7, 25, noi_str, ANSI_WHITE, s->noise.enabled ? ANSI_YELLOW : ANSI_DARK_GRAY, false);

    /* Filter & Envelope Innstillinger (Rad 11 til 16) */
    display_draw_box(2, 11, 48, 6, "DSP FILTER & MODULASJON", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_MAGENTA);
    char lpf_str[32];
    snprintf(lpf_str, sizeof(lpf_str), "LPF Cutoff: %d%% [%s]", (int)(s->filter.cutoff * 100.0f), s->filter.lpf_enabled ? "PAA" : "AV");
    display_draw_string(4, 12, lpf_str, ANSI_WHITE, ANSI_BLACK);
    display_draw_button(4, 14, 10, " [ LPF ] ", ANSI_WHITE, s->filter.lpf_enabled ? ANSI_MAGENTA : ANSI_DARK_GRAY, false);
    display_draw_button(16, 14, 7, " [ - ] ", ANSI_WHITE, ANSI_BLUE, false);
    display_draw_button(25, 14, 7, " [ + ] ", ANSI_WHITE, ANSI_BLUE, false);

    char crush_str[24];
    if (s->filter.bitcrush == 8) snprintf(crush_str, sizeof(crush_str), " [CRUSH 8-BIT] ");
    else if (s->filter.bitcrush == 4) snprintf(crush_str, sizeof(crush_str), " [CRUSH 4-BIT] ");
    else snprintf(crush_str, sizeof(crush_str), " [CRUSH: AV]  ");
    display_draw_button(34, 14, 14, crush_str, ANSI_WHITE, s->filter.bitcrush ? ANSI_YELLOW : ANSI_DARK_GRAY, false);

    /* Lydeffekter & Preset Fanfarer (X=52, Y=11, W=46, H=6) */
    display_draw_box(52, 11, 46, 6, "IKONISKE 8-BIT PRESETS", ANSI_WHITE, ANSI_BLACK, ANSI_YELLOW);
    display_draw_button(54, 12, 12, "[1: LASER] ", ANSI_WHITE, ANSI_RED, false);
    display_draw_button(68, 12, 12, "[2: COIN]  ", ANSI_WHITE, ANSI_YELLOW, false);
    display_draw_button(82, 12, 12, "[3: BOOM]  ", ANSI_WHITE, ANSI_RED, false);
    display_draw_button(54, 14, 12, "[4: 1-UP]  ", ANSI_WHITE, ANSI_GREEN, false);
    display_draw_button(68, 14, 12, "[5: BASS]  ", ANSI_WHITE, ANSI_CYAN, false);
    display_draw_button(82, 14, 12, "[RIFF LOOP]", ANSI_WHITE, ANSI_MAGENTA, false);

    /* Oktavvalg & Volum & Statuslinje */
    char oct_buf[32];
    snprintf(oct_buf, sizeof(oct_buf), "OKTAV: %d", g_current_octave);
    display_draw_string(2, 17, oct_buf, ANSI_LIGHT_CYAN, ANSI_BLACK);
    display_draw_button(12, 17, 8, "[OKT 3]", ANSI_WHITE, (g_current_octave == 3) ? ANSI_CYAN : ANSI_DARK_GRAY, false);
    display_draw_button(21, 17, 8, "[OKT 4]", ANSI_WHITE, (g_current_octave == 4) ? ANSI_CYAN : ANSI_DARK_GRAY, false);
    display_draw_button(30, 17, 8, "[OKT 5]", ANSI_WHITE, (g_current_octave == 5) ? ANSI_CYAN : ANSI_DARK_GRAY, false);

    /* Volumknapper */
    char vol_buf[16];
    snprintf(vol_buf, sizeof(vol_buf), "VOL:%3d%%", audio_get_volume_pct());
    display_draw_string(40, 17, vol_buf, ANSI_LIGHT_GREEN, ANSI_BLACK);
    display_draw_button(51, 17, 5, "[-]", ANSI_WHITE, ANSI_BLUE, false);
    display_draw_button(57, 17, 5, "[+]", ANSI_WHITE, ANSI_BLUE, false);

    display_draw_string(64, 17, g_status_msg, ANSI_YELLOW, ANSI_BLACK);

    /* Piano Tangenter (Rad 19 til 28) */
    display_draw_box(2, 19, 96, 10, "BERORINGSTANGENTER (PIANO KEYBOARD)", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_GREEN);

    /* 13 tangenter: C, C#, D, D#, E, F, F#, G, G#, A, A#, B, C+ */
    for (int i = 0; i < 13; i++) {
        int kx = 4 + (i * 7);
        bool is_sharp = (i == 1 || i == 3 || i == 6 || i == 8 || i == 10);
        bool is_pressed = (g_active_note == i);

        uint8_t fg = is_pressed ? ANSI_BLACK : (is_sharp ? ANSI_WHITE : ANSI_BLACK);
        uint8_t bg = is_pressed ? ANSI_YELLOW : (is_sharp ? ANSI_DARK_GRAY : ANSI_WHITE);

        char key_label[8];
        snprintf(key_label, sizeof(key_label), "%s", NOTE_NAMES[i]);

        /* Tegn tast */
        display_draw_box(kx, 21, 6, 7, "", fg, bg, ANSI_BLACK);
        display_draw_string(kx + 2, 24, key_label, fg, bg);
        if (is_sharp) {
            display_draw_string(kx + 1, 22, "#", ANSI_LIGHT_RED, bg);
        }
    }
}

static void synth_render_portrait(void) {
    display_clear(ANSI_BLACK);
    SynthState *s = synth_get_state();

    /* Toppbanner (60 kolonner x 50 rader) */
    display_draw_box(0, 0, 60, 3, "", ANSI_WHITE, ANSI_BLUE, ANSI_YELLOW);
    display_draw_string(2, 1, "tabOS RETRO SYNTHESIZER", ANSI_WHITE, ANSI_BLUE);
    display_draw_button(44, 1, 14, " [X] LUKK ", ANSI_WHITE, ANSI_RED, false);

    /* Oscilloskop (X=2, Y=3, W=56, H=7) */
    draw_oscilloscope(2, 3, 56, 7);

    /* Kanaler */
    display_draw_box(2, 10, 56, 6, "NES APU KANALER", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_CYAN);
    char p1_str[24], p2_str[24], tri_str[24], noi_str[24];
    const char *duty_str[4] = { "12%", "25%", "50%", "75%" };
    snprintf(p1_str, sizeof(p1_str), "[P1: %s %s]", s->pulse1.enabled ? "PAA" : "AV ", duty_str[s->pulse1.duty & 3]);
    snprintf(p2_str, sizeof(p2_str), "[P2: %s %s]", s->pulse2.enabled ? "PAA" : "AV ", duty_str[s->pulse2.duty & 3]);
    snprintf(tri_str, sizeof(tri_str), "[TRI: %s]", s->triangle.enabled ? "PAA" : "AV ");
    snprintf(noi_str, sizeof(noi_str), "[STOY: %s]", s->noise.enabled ? "PAA" : "AV ");

    display_draw_button(4, 11, 24, p1_str, ANSI_WHITE, s->pulse1.enabled ? ANSI_GREEN : ANSI_DARK_GRAY, false);
    display_draw_button(30, 11, 24, p2_str, ANSI_WHITE, s->pulse2.enabled ? ANSI_GREEN : ANSI_DARK_GRAY, false);
    display_draw_button(4, 13, 24, tri_str, ANSI_WHITE, s->triangle.enabled ? ANSI_CYAN : ANSI_DARK_GRAY, false);
    display_draw_button(30, 13, 24, noi_str, ANSI_WHITE, s->noise.enabled ? ANSI_YELLOW : ANSI_DARK_GRAY, false);

    /* Filter & Presets */
    display_draw_box(2, 16, 56, 7, "DSP FILTER & PRESETS", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_MAGENTA);
    char lpf_str[32];
    snprintf(lpf_str, sizeof(lpf_str), "Cutoff: %d%%", (int)(s->filter.cutoff * 100.0f));
    display_draw_string(4, 17, lpf_str, ANSI_WHITE, ANSI_BLACK);
    display_draw_button(20, 17, 8, "[ - ]", ANSI_WHITE, ANSI_BLUE, false);
    display_draw_button(30, 17, 8, "[ + ]", ANSI_WHITE, ANSI_BLUE, false);
    display_draw_button(40, 17, 16, s->filter.lpf_enabled ? "[LPF: PAA]" : "[LPF: AV ]", ANSI_WHITE, s->filter.lpf_enabled ? ANSI_MAGENTA : ANSI_DARK_GRAY, false);

    display_draw_button(4, 19, 11, "[LASER]", ANSI_WHITE, ANSI_RED, false);
    display_draw_button(17, 19, 11, "[COIN] ", ANSI_WHITE, ANSI_YELLOW, false);
    display_draw_button(30, 19, 11, "[BOOM] ", ANSI_WHITE, ANSI_RED, false);
    display_draw_button(43, 19, 13, "[RIFF]", ANSI_WHITE, ANSI_MAGENTA, false);

    /* Oktav & Volum */
    display_draw_string(2, 23, "OKT:", ANSI_LIGHT_CYAN, ANSI_BLACK);
    display_draw_button(7, 23, 7, "[O3]", ANSI_WHITE, (g_current_octave == 3) ? ANSI_CYAN : ANSI_DARK_GRAY, false);
    display_draw_button(15, 23, 7, "[O4]", ANSI_WHITE, (g_current_octave == 4) ? ANSI_CYAN : ANSI_DARK_GRAY, false);
    display_draw_button(23, 23, 7, "[O5]", ANSI_WHITE, (g_current_octave == 5) ? ANSI_CYAN : ANSI_DARK_GRAY, false);

    char vol_p_buf[16];
    snprintf(vol_p_buf, sizeof(vol_p_buf), "VOL:%2d%%", audio_get_volume_pct());
    display_draw_string(32, 23, vol_p_buf, ANSI_LIGHT_GREEN, ANSI_BLACK);
    display_draw_button(43, 23, 7, "[-]", ANSI_WHITE, ANSI_BLUE, false);
    display_draw_button(51, 23, 7, "[+]", ANSI_WHITE, ANSI_BLUE, false);

    display_draw_string(2, 25, g_status_msg, ANSI_YELLOW, ANSI_BLACK);

    /* Store Portrett-tangenter (Rad 27 til 48) */
    display_draw_box(1, 26, 58, 22, "PIANO TANGENTER", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_GREEN);

    for (int i = 0; i < 13; i++) {
        int ky = 27 + (i / 7) * 10;
        int kx = 3 + (i % 7) * 8;
        bool is_sharp = (i == 1 || i == 3 || i == 6 || i == 8 || i == 10);
        bool is_pressed = (g_active_note == i);

        uint8_t fg = is_pressed ? ANSI_BLACK : (is_sharp ? ANSI_WHITE : ANSI_BLACK);
        uint8_t bg = is_pressed ? ANSI_YELLOW : (is_sharp ? ANSI_DARK_GRAY : ANSI_WHITE);

        display_draw_box(kx, ky, 7, 9, "", fg, bg, ANSI_BLACK);
        display_draw_string(kx + 2, ky + 4, NOTE_NAMES[i], fg, bg);
        if (is_sharp) {
            display_draw_string(kx + 2, ky + 1, "#", ANSI_LIGHT_RED, bg);
        }
    }
}

static void synth_app_render(void) {
    if (orientation_is_portrait(display_get_orientation())) {
        synth_render_portrait();
    } else {
        synth_render_landscape();
    }
}

static void play_key(int note_idx) {
    if (note_idx < 0 || note_idx > 12) return;
    g_active_note = note_idx;

    float base = BASE_NOTE_FREQS[note_idx];
    float mult = 1.0f;
    if (g_current_octave == 3) mult = 0.5f;
    if (g_current_octave == 5) mult = 2.0f;

    float freq = base * mult;
    snprintf(g_status_msg, sizeof(g_status_msg), "Tone: %s%d (%.1f Hz)", NOTE_NAMES[note_idx], g_current_octave, freq);

    synth_play_note(freq, 0.25f);
    update_oscilloscope();
}

static void synth_app_touch(const TouchEvent *t) {
    ScreenOrientation orient = display_get_orientation();
    SynthState *s = synth_get_state();

    if (orientation_is_landscape(orient)) {
        if (t->is_down) {
            /* Tilbake til launcher */
            if (t->just_down && input_hit_box(t, 74, 1, 24, 1)) {
                os_switch_app(&app_launcher);
                return;
            }

            /* Kanaler - kun ved et nytt trykk */
            if (t->just_down && input_hit_box(t, 44, 4, 25, 2)) {
                s->pulse1.duty = (PulseDuty)((s->pulse1.duty + 1) % 4);
                update_oscilloscope();
                return;
            }
            if (t->just_down && input_hit_box(t, 71, 4, 25, 2)) {
                s->pulse2.enabled = !s->pulse2.enabled;
                update_oscilloscope();
                return;
            }
            if (t->just_down && input_hit_box(t, 44, 7, 25, 2)) {
                s->triangle.enabled = !s->triangle.enabled;
                update_oscilloscope();
                return;
            }
            if (t->just_down && input_hit_box(t, 71, 7, 25, 2)) {
                s->noise.enabled = !s->noise.enabled;
                update_oscilloscope();
                return;
            }

            /* Filter LPF */
            if (t->just_down && input_hit_box(t, 4, 14, 10, 1)) {
                s->filter.lpf_enabled = !s->filter.lpf_enabled;
                update_oscilloscope();
                return;
            }
            if (t->just_down && input_hit_box(t, 16, 14, 7, 1)) {
                s->filter.cutoff -= 0.10f;
                if (s->filter.cutoff < 0.10f) s->filter.cutoff = 0.10f;
                update_oscilloscope();
                return;
            }
            if (t->just_down && input_hit_box(t, 25, 14, 7, 1)) {
                s->filter.cutoff += 0.10f;
                if (s->filter.cutoff > 1.0f) s->filter.cutoff = 1.0f;
                update_oscilloscope();
                return;
            }
            if (t->just_down && input_hit_box(t, 34, 14, 14, 1)) {
                if (s->filter.bitcrush == 0) s->filter.bitcrush = 8;
                else if (s->filter.bitcrush == 8) s->filter.bitcrush = 4;
                else s->filter.bitcrush = 0;
                update_oscilloscope();
                return;
            }

            /* Presets - kun ved just_down */
            if (t->just_down && input_hit_box(t, 54, 12, 12, 1)) { synth_play_preset(1); snprintf(g_status_msg, sizeof(g_status_msg), "Lydeffekt: Laser Blaster"); update_oscilloscope(); return; }
            if (t->just_down && input_hit_box(t, 68, 12, 12, 1)) { synth_play_preset(2); snprintf(g_status_msg, sizeof(g_status_msg), "Lydeffekt: Retro Coin"); update_oscilloscope(); return; }
            if (t->just_down && input_hit_box(t, 82, 12, 12, 1)) { synth_play_preset(3); snprintf(g_status_msg, sizeof(g_status_msg), "Lydeffekt: 8-bit Eksplosjon"); update_oscilloscope(); return; }
            if (t->just_down && input_hit_box(t, 54, 14, 12, 1)) { synth_play_preset(4); snprintf(g_status_msg, sizeof(g_status_msg), "Lydeffekt: 1-UP Fanfare"); update_oscilloscope(); return; }
            if (t->just_down && input_hit_box(t, 68, 14, 12, 1)) { synth_play_preset(5); snprintf(g_status_msg, sizeof(g_status_msg), "Lydeffekt: Chiptune Bass"); update_oscilloscope(); return; }
            if (t->just_down && input_hit_box(t, 82, 14, 12, 1)) { synth_play_arpeggio(1); snprintf(g_status_msg, sizeof(g_status_msg), "Melodi: 8-bit Arpeggio Riff!"); update_oscilloscope(); return; }

            /* Oktav */
            if (t->just_down && input_hit_box(t, 12, 17, 8, 1)) { g_current_octave = 3; return; }
            if (t->just_down && input_hit_box(t, 21, 17, 8, 1)) { g_current_octave = 4; return; }
            if (t->just_down && input_hit_box(t, 30, 17, 8, 1)) { g_current_octave = 5; return; }

            /* Volum */
            if (t->just_down && input_hit_box(t, 51, 17, 5, 1)) { audio_volume_down(); return; }
            if (t->just_down && input_hit_box(t, 57, 17, 5, 1)) { audio_volume_up(); return; }

            /* Piano Tangenter: Spilles ved nytt trykk (just_down) ELLER nar fingeren glir til ny tangent */
            for (int i = 0; i < 13; i++) {
                int kx = 4 + (i * 7);
                if (input_hit_box(t, kx, 21, 6, 7)) {
                    if (t->just_down || g_active_note != i) {
                        play_key(i);
                    }
                    return;
                }
            }
        } else {
            g_active_note = -1;
        }
    } else {
        /* Portrett */
        if (t->is_down) {
            if (t->just_down && input_hit_box(t, 44, 1, 14, 1)) {
                os_switch_app(&app_launcher);
                return;
            }

            /* Kanaler */
            if (t->just_down && input_hit_box(t, 4, 11, 24, 2)) {
                s->pulse1.duty = (PulseDuty)((s->pulse1.duty + 1) % 4);
                update_oscilloscope();
                return;
            }
            if (t->just_down && input_hit_box(t, 30, 11, 24, 2)) {
                s->pulse2.enabled = !s->pulse2.enabled;
                update_oscilloscope();
                return;
            }
            if (t->just_down && input_hit_box(t, 4, 13, 24, 2)) {
                s->triangle.enabled = !s->triangle.enabled;
                update_oscilloscope();
                return;
            }
            if (t->just_down && input_hit_box(t, 30, 13, 24, 2)) {
                s->noise.enabled = !s->noise.enabled;
                update_oscilloscope();
                return;
            }

            /* Filter */
            if (t->just_down && input_hit_box(t, 20, 17, 8, 1)) {
                s->filter.cutoff -= 0.10f;
                if (s->filter.cutoff < 0.10f) s->filter.cutoff = 0.10f;
                update_oscilloscope();
                return;
            }
            if (t->just_down && input_hit_box(t, 30, 17, 8, 1)) {
                s->filter.cutoff += 0.10f;
                if (s->filter.cutoff > 1.0f) s->filter.cutoff = 1.0f;
                update_oscilloscope();
                return;
            }
            if (t->just_down && input_hit_box(t, 40, 17, 16, 1)) {
                s->filter.lpf_enabled = !s->filter.lpf_enabled;
                update_oscilloscope();
                return;
            }

            /* Presets */
            if (t->just_down && input_hit_box(t, 4, 19, 11, 1)) { synth_play_preset(1); update_oscilloscope(); return; }
            if (t->just_down && input_hit_box(t, 17, 19, 11, 1)) { synth_play_preset(2); update_oscilloscope(); return; }
            if (t->just_down && input_hit_box(t, 30, 19, 11, 1)) { synth_play_preset(3); update_oscilloscope(); return; }
            if (t->just_down && input_hit_box(t, 43, 19, 13, 1)) { synth_play_arpeggio(1); update_oscilloscope(); return; }

            /* Oktav & Volum */
            if (t->just_down && input_hit_box(t, 7, 23, 7, 1)) { g_current_octave = 3; return; }
            if (t->just_down && input_hit_box(t, 15, 23, 7, 1)) { g_current_octave = 4; return; }
            if (t->just_down && input_hit_box(t, 23, 23, 7, 1)) { g_current_octave = 5; return; }
            if (t->just_down && input_hit_box(t, 43, 23, 7, 1)) { audio_volume_down(); return; }
            if (t->just_down && input_hit_box(t, 51, 23, 7, 1)) { audio_volume_up(); return; }

            /* Tangenter portrett */
            for (int i = 0; i < 13; i++) {
                int ky = 27 + (i / 7) * 10;
                int kx = 3 + (i % 7) * 8;
                if (input_hit_box(t, kx, ky, 7, 9)) {
                    if (t->just_down || g_active_note != i) {
                        play_key(i);
                    }
                    return;
                }
            }
        } else {
            g_active_note = -1;
        }
    }
}

static void synth_app_resize(int cols, int rows) {
    (void)cols; (void)rows;
    update_oscilloscope();
}

static void synth_app_exit(void) {
    g_active_note = -1;
}

App app_synth = {
    .name = "synth",
    .title = "NES RETRO SYNTHESIZER",
    .on_start = synth_app_start,
    .on_update = synth_app_update,
    .on_render = synth_app_render,
    .on_touch = synth_app_touch,
    .on_resize = synth_app_resize,
    .on_exit = synth_app_exit
};
