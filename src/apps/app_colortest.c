#include <stdio.h>
#include "os.h"

static void colortest_start(void) {
}

static void colortest_render_landscape(void) {
    display_clear(ANSI_BLACK);

    /* Header */
    display_draw_box(0, 0, 100, 3, "", ANSI_WHITE, ANSI_MAGENTA, ANSI_YELLOW);
    display_draw_string(2, 1, "ANSI 16-FARGE & CP437 GRAFIKKTEST", ANSI_WHITE, ANSI_MAGENTA);
    display_draw_button(82, 1, 16, " [< TILBAKE] ", ANSI_WHITE, ANSI_RED, false);

    /* 16 Farger Palett */
    display_draw_box(2, 4, 96, 6, "16-FARGERS PALETT (STANDARD + INTENSE)", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_CYAN);
    const char *names[16] = {
        "Svar", "Rod ", "Gron", "Brun", "Bla ", "Mage", "Cyan", "L.gr",
        "M.gr", "L.rd", "L.gr", "Gul ", "L.bl", "L.mg", "L.cy", "Hvit"
    };
    for (int i = 0; i < 8; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%2d %s", i, names[i]);
        display_draw_string(4 + i * 11, 6, buf, ANSI_WHITE, ANSI_BLACK);
        for (int b = 0; b < 4; b++) {
            display_put_cell(4 + i * 11 + 6 + b, 6, 0xDB, (AnsiColor)i, ANSI_BLACK);
        }
        int idx = i + 8;
        snprintf(buf, sizeof(buf), "%2d %s", idx, names[idx]);
        display_draw_string(4 + i * 11, 8, buf, ANSI_WHITE, ANSI_BLACK);
        for (int b = 0; b < 4; b++) {
            display_put_cell(4 + i * 11 + 6 + b, 8, 0xDB, (AnsiColor)idx, ANSI_BLACK);
        }
    }

    /* Gradienter og skygger */
    display_draw_box(2, 11, 46, 7, "SKYGGING & GRADIENTER (CP437)", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_GREEN);
    uint8_t shades[4] = {0xB0, 0xB1, 0xB2, 0xDB};
    AnsiColor dark_cols[5] = {ANSI_RED, ANSI_GREEN, ANSI_BLUE, ANSI_MAGENTA, ANSI_CYAN};
    AnsiColor light_cols[5] = {ANSI_LIGHT_RED, ANSI_LIGHT_GREEN, ANSI_LIGHT_BLUE, ANSI_LIGHT_MAGENTA, ANSI_LIGHT_CYAN};

    for (int r = 0; r < 5; r++) {
        char label[16];
        snprintf(label, sizeof(label), "FADE %d:", r + 1);
        display_draw_string(4, 13 + r, label, ANSI_WHITE, ANSI_BLACK);
        int pos = 13;
        for (int s = 0; s < 4; s++) {
            display_put_cell(pos, 13 + r, shades[s], dark_cols[r], ANSI_BLACK);
            display_put_cell(pos + 1, 13 + r, shades[s], dark_cols[r], ANSI_BLACK);
            pos += 2;
        }
        for (int s = 0; s < 4; s++) {
            display_put_cell(pos, 13 + r, shades[s], light_cols[r], ANSI_BLACK);
            display_put_cell(pos + 1, 13 + r, shades[s], light_cols[r], ANSI_BLACK);
            pos += 2;
        }
        display_draw_string(pos + 2, 13 + r, "100%", light_cols[r], ANSI_BLACK);
    }

    /* Bokser og symboler */
    display_draw_box(50, 11, 48, 7, "BOKSER, RAMMER & RETRO SYMBOLER", ANSI_WHITE, ANSI_BLACK, ANSI_YELLOW);
    display_draw_string(52, 13, "Enkel : \xDA\xC4\xC4\xC4\xC2\xC4\xC4\xC4\xBF \xC3\xC4\xC4\xC4\xC5\xC4\xC4\xC4\xB4 \xC0\xC4\xC4\xC4\xC1\xC4\xC4\xC4\xD9", ANSI_LIGHT_CYAN, ANSI_BLACK);
    display_draw_string(52, 14, "Dobbel: \xC9\xCD\xCD\xCD\xCB\xCD\xCD\xCD\xBB \xCC\xCD\xCD\xCD\xCE\xCD\xCD\xCD\xB9 \xC8\xCD\xCD\xCD\xCA\xCD\xCD\xCD\xBC", ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(52, 15, "Kort  : \x03 \x04 \x05 \x06  Piler: \x18 \x19 \x1A \x1B \x1D \x12", ANSI_LIGHT_BLUE, ANSI_BLACK);
    display_draw_string(52, 16, "Musikk: \x0D \x0E \x0F  Ikoner: \x01 \x02 \x0B \x0C \x7F \x1E", ANSI_LIGHT_GREEN, ANSI_BLACK);
    display_draw_string(52, 17, "Matte : \xF1 \xF2 \xF3 \xF6 \xF7 \xF8 \xFB \xFD", ANSI_LIGHT_MAGENTA, ANSI_BLACK);

    /* Kontrastmatrise */
    display_draw_box(2, 19, 96, 6, "TEKSTKONTRAST MATRIX (FG PA BAKGRUNN)", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_MAGENTA);
    AnsiColor bg_samples[7] = {ANSI_BLACK, ANSI_BLUE, ANSI_GREEN, ANSI_RED, ANSI_MAGENTA, ANSI_CYAN, ANSI_LIGHT_GRAY};
    for (int i = 0; i < 7; i++) {
        int c = 4 + i * 13;
        char buf[16];
        snprintf(buf, sizeof(buf), "BG %2d", (int)bg_samples[i]);
        display_draw_string(c, 21, buf, ANSI_WHITE, ANSI_BLACK);
        display_draw_string(c, 22, " HVIT ", ANSI_WHITE, bg_samples[i]);
        display_draw_string(c, 23, " GUL  ", ANSI_YELLOW, bg_samples[i]);
    }

    /* Bunnmeny */
    char bat_str[32];
    os_get_battery_str(bat_str, sizeof(bat_str));
    display_draw_box(0, 26, 100, 4, "", ANSI_WHITE, ANSI_DARK_GRAY, ANSI_LIGHT_GRAY);
    display_draw_button(4, 27, 20, " [< TILBAKE] ", ANSI_WHITE, ANSI_RED, false);
    display_draw_string(28, 27, bat_str, ANSI_YELLOW, ANSI_DARK_GRAY);
    display_draw_string(52, 27, "Allwinner A13 32-bit ARGB TrueColor Framebuffer OK", ANSI_LIGHT_CYAN, ANSI_DARK_GRAY);
    display_draw_string(52, 28, "16 ANSI-farger m/ IBM CP437 standard VGA font", ANSI_LIGHT_GREEN, ANSI_DARK_GRAY);
}

static void colortest_render_portrait(void) {
    display_clear(ANSI_BLACK);

    display_draw_box(0, 0, 60, 3, "", ANSI_WHITE, ANSI_MAGENTA, ANSI_YELLOW);
    display_draw_string(2, 1, "ANSI FARGETEST (PORTRETT)", ANSI_WHITE, ANSI_MAGENTA);
    display_draw_button(42, 1, 16, " [< TILBAKE] ", ANSI_WHITE, ANSI_RED, false);

    display_draw_box(2, 4, 56, 10, "16 ANSI FARGER", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_CYAN);
    for (int i = 0; i < 16; i++) {
        int col = 4 + (i % 4) * 13;
        int row = 6 + (i / 4) * 2;
        char buf[8];
        snprintf(buf, sizeof(buf), "%2d", i);
        display_draw_string(col, row, buf, ANSI_WHITE, ANSI_BLACK);
        for (int b = 0; b < 4; b++) {
            display_put_cell(col + 3 + b, row, 0xDB, (AnsiColor)i, ANSI_BLACK);
        }
    }

    display_draw_box(2, 15, 56, 8, "CP437 RAMMER & GRADIENTER", ANSI_WHITE, ANSI_BLACK, ANSI_YELLOW);
    display_draw_string(4, 17, "Enkel : \xDA\xC4\xC4\xC4\xC2\xC4\xC4\xC4\xBF \xC0\xC4\xC4\xC4\xC1\xC4\xC4\xC4\xD9", ANSI_LIGHT_CYAN, ANSI_BLACK);
    display_draw_string(4, 18, "Dobbel: \xC9\xCD\xCD\xCD\xCB\xCD\xCD\xCD\xBB \xC8\xCD\xCD\xCD\xCA\xCD\xCD\xCD\xBC", ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(4, 19, "Skygge: \xB0\xB0\xB1\xB1\xB2\xB2\xDB\xDB (0% -> 100%)", ANSI_LIGHT_GREEN, ANSI_BLACK);
    display_draw_string(4, 20, "Symbol: \x03 \x04 \x05 \x06 \x01 \x02 \x0D \x0E \x18 \x19 \x1A \x1B \xF1 \xF7", ANSI_LIGHT_MAGENTA, ANSI_BLACK);

    char bat_p[32];
    os_get_battery_str(bat_p, sizeof(bat_p));
    display_draw_box(0, 40, 60, 9, "KONTROLL", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_GRAY);
    display_draw_button(4, 43, 24, " [< TILBAKE] ", ANSI_WHITE, ANSI_RED, false);
    display_draw_string(32, 43, bat_p, ANSI_YELLOW, ANSI_BLACK);
}

static void colortest_render(void) {
    if (orientation_is_portrait(display_get_orientation())) {
        colortest_render_portrait();
    } else {
        colortest_render_landscape();
    }
}

static void colortest_touch(const TouchEvent *t) {
    if (t->just_up) {
        if (orientation_is_landscape(display_get_orientation())) {
            if (input_hit_box(t, 82, 1, 16, 1) || input_hit_box(t, 4, 27, 20, 1)) {
                os_switch_app(&app_launcher);
            }
        } else {
            if (input_hit_box(t, 42, 1, 16, 1) || input_hit_box(t, 4, 43, 24, 1)) {
                os_switch_app(&app_launcher);
            }
        }
    }
}

App app_colortest = {
    .name = "ColorTest",
    .title = "ANSI 16-Fargetest & CP437 Grafikk",
    .on_start = colortest_start,
    .on_update = NULL,
    .on_render = colortest_render,
    .on_touch = colortest_touch,
    .on_resize = NULL,
    .on_exit = NULL
};
