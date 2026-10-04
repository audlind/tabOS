#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

#include "app.h"
#include "display.h"
#include "input.h"
#include "os.h"
#include "camera.h"

static CameraRenderMode g_mode = CAM_MODE_MATRIX;
static char g_status_msg[64] = "ASCII Cyber-Kamera aktivt!";
static uint32_t g_fps_frames = 0;
static uint32_t g_last_fps_time = 0;
static float g_current_fps = 0.0f;
static bool g_snapshot_flash = false;

/* Bayer dither matrise 2x2 for Game Boy modus */
static const int BAYER_2X2[2][2] = {
    { 0, 2 },
    { 3, 1 }
};

static void camera_app_start(void) {
    camera_init();
    snprintf(g_status_msg, sizeof(g_status_msg), "ASCII Cyber-Cam klar (%s)",
             camera_is_available() ? "GC0329 Maskinvare" : "Cybersensor");
}

static bool camera_app_update(uint32_t delta_ms) {
    g_last_fps_time += delta_ms;
    g_fps_frames++;
    if (g_last_fps_time >= 1000) {
        g_current_fps = (float)g_fps_frames * 1000.0f / (float)g_last_fps_time;
        g_fps_frames = 0;
        g_last_fps_time = 0;
    }
    return true; /* Krever kontinuerlig rendring for live video */
}

/*
 * Rendrer ASCII-kamerarammen inn i et gitt rektangel på tekstskjermen.
 */
static void render_camera_viewport(int vx, int vy, int vw, int vh) {
    const uint8_t *frame = camera_grab_y_frame();
    if (!frame) return;

    bool mirror = camera_get_mirror();
    int bri = camera_get_brightness();
    int con = camera_get_contrast();
    float con_mult = (con >= 0) ? (1.0f + con / 50.0f) : (1.0f + con / 100.0f);

    /* ASCII ramper */
    static const char *RAMP_MATRIX = " .:*+=#%@01";
    static const int   RAMP_MATRIX_LEN = 11;

    static const uint8_t RAMP_CP437[5] = { ' ', 0xB0, 0xB1, 0xB2, 0xDB }; /*   ░ ▒ ▓ █ */

    for (int y = 0; y < vh; y++) {
        int src_y = (y * 480) / vh;
        for (int x = 0; x < vw; x++) {
            int src_x = (x * 640) / vw;
            if (mirror) src_x = 640 - 1 - src_x;

            int raw = frame[src_y * 640 + src_x];
            /* Juster kontrast & lysstyrke */
            int lum = (int)(((float)(raw - 128) * con_mult) + 128) + bri;
            if (lum < 0)   lum = 0;
            if (lum > 255) lum = 255;

            uint8_t ch = ' ';
            uint8_t fg = ANSI_LIGHT_GREEN;
            uint8_t bg = ANSI_BLACK;

            switch (g_mode) {
                case CAM_MODE_MATRIX: {
                    int idx = (lum * RAMP_MATRIX_LEN) / 256;
                    ch = RAMP_MATRIX[idx];
                    fg = (idx > 7) ? ANSI_WHITE : ((idx > 3) ? ANSI_LIGHT_GREEN : ANSI_GREEN);
                    bg = ANSI_BLACK;
                    break;
                }
                case CAM_MODE_CP437: {
                    int idx = (lum * 5) / 256;
                    ch = RAMP_CP437[idx];
                    fg = (idx == 4) ? ANSI_WHITE : ((idx > 1) ? ANSI_LIGHT_GRAY : ANSI_DARK_GRAY);
                    bg = ANSI_BLACK;
                    break;
                }
                case CAM_MODE_GAMEBOY: {
                    /* Game Boy 4-nivås dither */
                    int dither = BAYER_2X2[y & 1][x & 1] * 32 - 48;
                    int d_lum = lum + dither;
                    if (d_lum < 0) d_lum = 0;
                    if (d_lum > 255) d_lum = 255;
                    int level = d_lum / 64; /* 0, 1, 2, 3 */
                    if (level == 0) {
                        ch = ' ';
                        bg = ANSI_BLACK;
                        fg = ANSI_BLACK;
                    } else if (level == 1) {
                        ch = 0xB0; /* ░ */
                        fg = ANSI_GREEN;
                        bg = ANSI_BLACK;
                    } else if (level == 2) {
                        ch = 0xB1; /* ▒ */
                        fg = ANSI_LIGHT_GREEN;
                        bg = ANSI_GREEN;
                    } else {
                        ch = 0xDB; /* █ */
                        fg = ANSI_WHITE;
                        bg = ANSI_LIGHT_GREEN;
                    }
                    break;
                }
                case CAM_MODE_AMBER: {
                    int idx = (lum * RAMP_MATRIX_LEN) / 256;
                    ch = RAMP_MATRIX[idx];
                    fg = (idx > 7) ? ANSI_YELLOW : ((idx > 3) ? ANSI_LIGHT_RED : ANSI_BROWN);
                    bg = ANSI_BLACK;
                    break;
                }
            }

            display_put_cell(vx + x, vy + y, ch, fg, bg);
        }
    }
}

static void camera_render_landscape(void) {
    display_clear(ANSI_BLACK);

    /* Toppbanner (100 kolonner x 3 rader) */
    display_draw_box(0, 0, 100, 3, "", ANSI_WHITE, ANSI_BLUE, ANSI_YELLOW);
    display_draw_string(2, 1, "tabOS ASCII CYBER-CAM v1.0 (GalaxyCore GC0329 CSI)", ANSI_WHITE, ANSI_BLUE);

    char fps_buf[32];
    snprintf(fps_buf, sizeof(fps_buf), "FPS: %4.1f", g_current_fps);
    display_draw_string(54, 1, fps_buf, ANSI_LIGHT_GREEN, ANSI_BLUE);

    display_draw_button(74, 1, 24, " [X] TILBAKE TIL LAUNCHER ", ANSI_WHITE, ANSI_RED, false);

    /* Videoramme til venstre (X=1, Y=3, W=72, H=26) */
    display_draw_box(1, 3, 72, 27, "LIVE ASCII KAMERA-STRØM", ANSI_LIGHT_GREEN, ANSI_BLACK, ANSI_DARK_GRAY);
    render_camera_viewport(2, 4, 70, 25);

    /* Kontrollpanel til høyre (X=74, Y=3, W=25, H=27) */
    display_draw_box(74, 3, 25, 27, "KAMERA KONTROLL", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_CYAN);

    display_draw_string(76, 5, "VISUELL MODUS:", ANSI_LIGHT_CYAN, ANSI_BLACK);
    display_draw_button(76, 6, 21, "[1] MATRIX GRØNN", ANSI_WHITE, (g_mode == CAM_MODE_MATRIX) ? ANSI_GREEN : ANSI_DARK_GRAY, false);
    display_draw_button(76, 8, 21, "[2] CP437 SKYGGER", ANSI_WHITE, (g_mode == CAM_MODE_CP437) ? ANSI_LIGHT_GRAY : ANSI_DARK_GRAY, false);
    display_draw_button(76, 10, 21, "[3] GAME BOY DITHER", ANSI_WHITE, (g_mode == CAM_MODE_GAMEBOY) ? ANSI_YELLOW : ANSI_DARK_GRAY, false);
    display_draw_button(76, 12, 21, "[4] AMBER CRT", ANSI_WHITE, (g_mode == CAM_MODE_AMBER) ? ANSI_LIGHT_RED : ANSI_DARK_GRAY, false);

    /* Speilvend selfie */
    char mirror_str[24];
    snprintf(mirror_str, sizeof(mirror_str), "SPEIL: [%s]", camera_get_mirror() ? " PÅ " : " AV ");
    display_draw_button(76, 15, 21, mirror_str, ANSI_WHITE, ANSI_BLUE, false);

    /* Lysstyrke & Kontrast */
    char bri_str[24], con_str[24];
    snprintf(bri_str, sizeof(bri_str), "LYS: %+3d  [-][+]", camera_get_brightness());
    snprintf(con_str, sizeof(con_str), "KONT: %+2d  [-][+]", camera_get_contrast());
    display_draw_string(76, 18, bri_str, ANSI_WHITE, ANSI_BLACK);
    display_draw_button(86, 18, 5, "[-]", ANSI_WHITE, ANSI_BLUE, false);
    display_draw_button(92, 18, 5, "[+]", ANSI_WHITE, ANSI_BLUE, false);

    display_draw_string(76, 20, con_str, ANSI_WHITE, ANSI_BLACK);
    display_draw_button(86, 20, 5, "[-]", ANSI_WHITE, ANSI_BLUE, false);
    display_draw_button(92, 20, 5, "[+]", ANSI_WHITE, ANSI_BLUE, false);

    /* Snapshot knapp */
    display_draw_button(76, 23, 21, " [*] KNIPS BILDE ", ANSI_WHITE, g_snapshot_flash ? ANSI_LIGHT_GREEN : ANSI_RED, false);

    display_draw_string(76, 26, g_status_msg, ANSI_YELLOW, ANSI_BLACK);
}

static void camera_render_portrait(void) {
    display_clear(ANSI_BLACK);

    /* Toppbanner (60 kolonner x 3 rader) */
    display_draw_box(0, 0, 60, 3, "", ANSI_WHITE, ANSI_BLUE, ANSI_YELLOW);
    display_draw_string(2, 1, "ASCII CYBER-CAM", ANSI_WHITE, ANSI_BLUE);

    char fps_buf[16];
    snprintf(fps_buf, sizeof(fps_buf), "%4.1f FPS", g_current_fps);
    display_draw_string(30, 1, fps_buf, ANSI_LIGHT_GREEN, ANSI_BLUE);
    display_draw_button(44, 1, 14, " [X] LUKK ", ANSI_WHITE, ANSI_RED, false);

    /* Hovedvisning for kamera (X=1, Y=3, W=58, H=34) */
    display_draw_box(1, 3, 58, 34, "LIVE ASCII KAMERA", ANSI_LIGHT_GREEN, ANSI_BLACK, ANSI_DARK_GRAY);
    render_camera_viewport(2, 4, 56, 32);

    /* Kontrollpanel under kameraet (Y=38 til 49) */
    display_draw_box(1, 38, 58, 11, "KONTROLLPANEL", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_CYAN);

    display_draw_button(3, 40, 13, "[1: MATRIX]", ANSI_WHITE, (g_mode == CAM_MODE_MATRIX) ? ANSI_GREEN : ANSI_DARK_GRAY, false);
    display_draw_button(17, 40, 13, "[2: CP437] ", ANSI_WHITE, (g_mode == CAM_MODE_CP437) ? ANSI_LIGHT_GRAY : ANSI_DARK_GRAY, false);
    display_draw_button(31, 40, 13, "[3: GAMEBOY]", ANSI_WHITE, (g_mode == CAM_MODE_GAMEBOY) ? ANSI_YELLOW : ANSI_DARK_GRAY, false);
    display_draw_button(45, 40, 13, "[4: AMBER] ", ANSI_WHITE, (g_mode == CAM_MODE_AMBER) ? ANSI_LIGHT_RED : ANSI_DARK_GRAY, false);

    char mirror_str[16];
    snprintf(mirror_str, sizeof(mirror_str), "[SPEIL: %s]", camera_get_mirror() ? "PÅ" : "AV");
    display_draw_button(3, 43, 13, mirror_str, ANSI_WHITE, ANSI_BLUE, false);

    display_draw_button(17, 43, 7, "[LYS-]", ANSI_WHITE, ANSI_BLUE, false);
    display_draw_button(25, 43, 7, "[LYS+]", ANSI_WHITE, ANSI_BLUE, false);
    display_draw_button(33, 43, 7, "[KNT-]", ANSI_WHITE, ANSI_BLUE, false);
    display_draw_button(41, 43, 7, "[KNT+]", ANSI_WHITE, ANSI_BLUE, false);

    display_draw_button(3, 46, 25, " [*] KNIPS STILLBILDE ", ANSI_WHITE, g_snapshot_flash ? ANSI_LIGHT_GREEN : ANSI_RED, false);
    display_draw_string(30, 46, g_status_msg, ANSI_YELLOW, ANSI_BLACK);
}

static void camera_app_render(void) {
    if (orientation_is_portrait(display_get_orientation())) {
        camera_render_portrait();
    } else {
        camera_render_landscape();
    }
    if (g_snapshot_flash) g_snapshot_flash = false;
}

static void do_snapshot(void) {
    g_snapshot_flash = true;
    const char *path = "/data/local/tmp/tabos_snapshot.txt";
    if (camera_save_snapshot(path, g_mode, 100, 40)) {
        snprintf(g_status_msg, sizeof(g_status_msg), "Lagret: snapshot.txt!");
    } else {
        snprintf(g_status_msg, sizeof(g_status_msg), "Snapshot feilet!");
    }
}

static void camera_app_touch(const TouchEvent *t) {
    if (!t->is_down || !t->just_down) return;
    ScreenOrientation orient = display_get_orientation();

    if (orientation_is_landscape(orient)) {
        /* Tilbake til launcher */
        if (input_hit_box(t, 74, 1, 24, 1)) {
            os_switch_app(&app_launcher);
            return;
        }

        /* Moduser */
        if (input_hit_box(t, 76, 6, 21, 1))  { g_mode = CAM_MODE_MATRIX; return; }
        if (input_hit_box(t, 76, 8, 21, 1))  { g_mode = CAM_MODE_CP437; return; }
        if (input_hit_box(t, 76, 10, 21, 1)) { g_mode = CAM_MODE_GAMEBOY; return; }
        if (input_hit_box(t, 76, 12, 21, 1)) { g_mode = CAM_MODE_AMBER; return; }

        /* Speil */
        if (input_hit_box(t, 76, 15, 21, 1)) {
            camera_set_mirror(!camera_get_mirror());
            return;
        }

        /* Lysstyrke & Kontrast */
        if (input_hit_box(t, 86, 18, 5, 1)) { camera_adjust_brightness(-15); return; }
        if (input_hit_box(t, 92, 18, 5, 1)) { camera_adjust_brightness(+15); return; }
        if (input_hit_box(t, 86, 20, 5, 1)) { camera_adjust_contrast(-10); return; }
        if (input_hit_box(t, 92, 20, 5, 1)) { camera_adjust_contrast(+10); return; }

        /* Snapshot */
        if (input_hit_box(t, 76, 23, 21, 1)) {
            do_snapshot();
            return;
        }
    } else {
        /* Portrett */
        if (input_hit_box(t, 44, 1, 14, 1)) {
            os_switch_app(&app_launcher);
            return;
        }

        /* Moduser */
        if (input_hit_box(t, 3, 40, 13, 1))  { g_mode = CAM_MODE_MATRIX; return; }
        if (input_hit_box(t, 17, 40, 13, 1)) { g_mode = CAM_MODE_CP437; return; }
        if (input_hit_box(t, 31, 40, 13, 1)) { g_mode = CAM_MODE_GAMEBOY; return; }
        if (input_hit_box(t, 45, 40, 13, 1)) { g_mode = CAM_MODE_AMBER; return; }

        /* Speil */
        if (input_hit_box(t, 3, 43, 13, 1)) {
            camera_set_mirror(!camera_get_mirror());
            return;
        }

        /* Lys & Kontrast */
        if (input_hit_box(t, 17, 43, 7, 1)) { camera_adjust_brightness(-15); return; }
        if (input_hit_box(t, 25, 43, 7, 1)) { camera_adjust_brightness(+15); return; }
        if (input_hit_box(t, 33, 43, 7, 1)) { camera_adjust_contrast(-10); return; }
        if (input_hit_box(t, 41, 43, 7, 1)) { camera_adjust_contrast(+10); return; }

        /* Snapshot */
        if (input_hit_box(t, 3, 46, 25, 1)) {
            do_snapshot();
            return;
        }
    }
}

static void camera_app_resize(int cols, int rows) {
    (void)cols; (void)rows;
}

static void camera_app_exit(void) {
    camera_close();
}

App app_camera = {
    .name = "camera",
    .title = "ASCII CYBER-CAM",
    .on_start = camera_app_start,
    .on_update = camera_app_update,
    .on_render = camera_app_render,
    .on_touch = camera_app_touch,
    .on_resize = camera_app_resize,
    .on_exit = camera_app_exit
};
