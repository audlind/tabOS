#include <stdio.h>
#include <string.h>
#include "os.h"

#define MAX_TEXT_LEN 128
static char input_buffer[MAX_TEXT_LEN] = "tabOS 2026";
static const char *last_pressed_key = NULL;

static void keyboard_start(void) {
    last_pressed_key = NULL;
}

static void draw_big_key_c(int col, int row, int w, int h, const char *label, bool pressed, AnsiColor fg, AnsiColor bg) {
    AnsiColor final_fg = pressed ? ANSI_BLACK : fg;
    AnsiColor final_bg = pressed ? ANSI_LIGHT_CYAN : bg;
    AnsiColor border_col = pressed ? ANSI_YELLOW : ANSI_LIGHT_GRAY;

    int c2 = col + w - 1;
    int r2 = row + h - 1;

    display_put_cell(col, row, 0xDA, border_col, final_bg);
    display_put_cell(c2,  row, 0xBF, border_col, final_bg);
    display_put_cell(col, r2,  0xC0, border_col, final_bg);
    display_put_cell(c2,  r2,  0xD9, border_col, final_bg);

    for (int c = col + 1; c < c2; c++) {
        display_put_cell(c, row, 0xC4, border_col, final_bg);
        display_put_cell(c, r2,  0xC4, border_col, final_bg);
    }
    for (int r = row + 1; r < r2; r++) {
        display_put_cell(col, r, 0xB3, border_col, final_bg);
        display_put_cell(c2,  r, 0xB3, border_col, final_bg);
        for (int c = col + 1; c < c2; c++) {
            display_put_cell(c, r, ' ', final_fg, final_bg);
        }
    }

    int label_r = row + (h - 1) / 2;
    int label_c = col + (w - (int)strlen(label)) / 2;
    display_draw_string(label_c, label_r, label, final_fg, final_bg);
}

static void keyboard_render_portrait(void) {
    display_clear(ANSI_BLACK);

    /* Header */
    display_draw_box(0, 0, 60, 3, "", ANSI_WHITE, ANSI_BLUE, ANSI_YELLOW);
    display_draw_string(2, 1, "TOUCH-TASTATUR (STORE TASTER)", ANSI_WHITE, ANSI_BLUE);
    display_draw_button(42, 1, 16, " [< TILBAKE] ", ANSI_WHITE, ANSI_RED, false);

    /* Notat-tekstboks */
    display_draw_box(2, 3, 56, 5, "SKREVET TEKST", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_CYAN);
    char display_buf[64];
    snprintf(display_buf, sizeof(display_buf), "%s_", input_buffer);
    display_draw_string(4, 5, display_buf, ANSI_YELLOW, ANSI_BLACK);
    char count_buf[32];
    snprintf(count_buf, sizeof(count_buf), "Tegn: %d", (int)strlen(input_buffer));
    display_draw_string(4, 6, count_buf, ANSI_LIGHT_GREEN, ANSI_BLACK);

    /* Store taster (5 cols bred x 3 rader høy -> 40x48 piksler!) */
    int base_col = 5;
    int kw = 5;
    int kh = 3;

    /* Rad 1: 1..0 på rad 9 */
    const char *nums[10] = {"1","2","3","4","5","6","7","8","9","0"};
    for (int i = 0; i < 10; i++) {
        draw_big_key_c(base_col + i * kw, 9, kw, kh, nums[i], false, ANSI_WHITE, ANSI_DARK_GRAY);
    }

    /* Rad 2: QWERTY på rad 13 */
    const char *r2[10] = {"Q","W","E","R","T","Y","U","I","O","P"};
    for (int i = 0; i < 10; i++) {
        draw_big_key_c(base_col + i * kw, 13, kw, kh, r2[i], false, ANSI_LIGHT_CYAN, ANSI_DARK_GRAY);
    }

    /* Rad 3: ASDFGHJKL Æ Ø Å på rad 17 (12 taster x 5 cols = 60 kolonner!) */
    const char *r3[12] = {"A","S","D","F","G","H","J","K","L","\x92","\x9D","\x8F"};
    for (int i = 0; i < 12; i++) {
        draw_big_key_c(i * kw, 17, kw, kh, r3[i], false, ANSI_LIGHT_GREEN, ANSI_DARK_GRAY);
    }

    /* Rad 4: ZXCVBNM , . på rad 21 */
    const char *r4[9] = {"Z","X","C","V","B","N","M",",","."};
    for (int i = 0; i < 9; i++) {
        draw_big_key_c(7 + i * kw, 21, kw, kh, r4[i], false, ANSI_YELLOW, ANSI_DARK_GRAY);
    }

    /* Rad 5: SLETT, MELLOMROM, TOM på rad 25 */
    draw_big_key_c(5, 25, 14, kh, "SLETT", false, ANSI_WHITE, ANSI_RED);
    draw_big_key_c(21, 25, 22, kh, "MELLOMROM", false, ANSI_WHITE, ANSI_BLUE);
    draw_big_key_c(45, 25, 10, kh, "TOM", false, ANSI_WHITE, ANSI_MAGENTA);

    /* Kontrollknapper rad 30 */
    display_draw_button(5, 30, 24, " [< TILBAKE] ", ANSI_WHITE, ANSI_RED, false);
    display_draw_button(31, 30, 24, " [ ROTER SKJERM ] ", ANSI_WHITE, ANSI_BLUE, false);

    /* Statusinfo */
    display_draw_box(2, 34, 56, 14, "TASTE-INFO", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_GREEN);
    display_draw_string(4, 36, "> Tastestorrelse: 40 x 48 piksler", ANSI_LIGHT_GREEN, ANSI_BLACK);
    display_draw_string(4, 38, "> Stor flate sikrer at fingre treffer rent!", ANSI_WHITE, ANSI_BLACK);
}

static void keyboard_render_landscape(void) {
    display_clear(ANSI_BLACK);

    /* Header */
    display_draw_box(0, 0, 100, 3, "", ANSI_WHITE, ANSI_BLUE, ANSI_YELLOW);
    display_draw_string(2, 1, "TOUCH-TASTATUR (STORE TASTER - LANDSKAP)", ANSI_WHITE, ANSI_BLUE);
    display_draw_button(82, 1, 16, " [< TILBAKE] ", ANSI_WHITE, ANSI_RED, false);

    /* Tekstvisning */
    display_draw_box(2, 3, 96, 4, "SKREVET TEKST", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_CYAN);
    char display_buf[100];
    snprintf(display_buf, sizeof(display_buf), "%s_", input_buffer);
    display_draw_string(4, 4, display_buf, ANSI_YELLOW, ANSI_BLACK);
    char count_buf[32];
    snprintf(count_buf, sizeof(count_buf), "Tegn: %d", (int)strlen(input_buffer));
    display_draw_string(4, 5, count_buf, ANSI_LIGHT_GREEN, ANSI_BLACK);

    /* Store taster (7 kolonner bred x 3 rader høy -> 56x24 piksler per tast!) */
    int kw = 7;
    int kh = 3;

    /* Rad 1: Tall 1..0 på rad 7 (10 taster sentrert fra kolonne 15) */
    const char *nums[10] = {"1","2","3","4","5","6","7","8","9","0"};
    for (int i = 0; i < 10; i++) {
        draw_big_key_c(15 + i * kw, 7, kw, kh, nums[i], false, ANSI_WHITE, ANSI_DARK_GRAY);
    }

    /* Rad 2: QWERTY på rad 11 (10 taster sentrert fra kolonne 15) */
    const char *r2[10] = {"Q","W","E","R","T","Y","U","I","O","P"};
    for (int i = 0; i < 10; i++) {
        draw_big_key_c(15 + i * kw, 11, kw, kh, r2[i], false, ANSI_LIGHT_CYAN, ANSI_DARK_GRAY);
    }

    /* Rad 3: ASDFGHJKL + Æ Ø Å på rad 15 (12 taster fra kolonne 8, symmetrisk 84 bred) */
    const char *r3[12] = {"A","S","D","F","G","H","J","K","L","\x92","\x9D","\x8F"};
    for (int i = 0; i < 12; i++) {
        draw_big_key_c(8 + i * kw, 15, kw, kh, r3[i], false, ANSI_LIGHT_GREEN, ANSI_DARK_GRAY);
    }

    /* Rad 4: ZXCVBNM , . - på rad 19 (10 taster sentrert fra kolonne 15) */
    const char *r4[10] = {"Z","X","C","V","B","N","M",",",".","-"};
    for (int i = 0; i < 10; i++) {
        draw_big_key_c(15 + i * kw, 19, kw, kh, r4[i], false, ANSI_YELLOW, ANSI_DARK_GRAY);
    }

    /* Rad 5: Kontrolltaster på rad 23 */
    draw_big_key_c(8, 23, 14, kh, "SLETT", false, ANSI_WHITE, ANSI_RED);
    draw_big_key_c(24, 23, 30, kh, "MELLOMROM", false, ANSI_WHITE, ANSI_BLUE);
    draw_big_key_c(56, 23, 10, kh, "TOM", false, ANSI_WHITE, ANSI_MAGENTA);
    draw_big_key_c(68, 23, 11, kh, "ROTER", false, ANSI_WHITE, ANSI_CYAN);
    draw_big_key_c(81, 23, 11, kh, "TILBAKE", false, ANSI_WHITE, ANSI_RED);

    /* Hjelpelinje nederst */
    display_draw_string(8, 27, "> Trykk pa tastene for a skrive! [\x92] [\x9D] [\x8F] er inkludert.", ANSI_LIGHT_GREEN, ANSI_BLACK);
}

static void keyboard_render(void) {
    if (orientation_is_portrait(display_get_orientation())) {
        keyboard_render_portrait();
    } else {
        keyboard_render_landscape();
    }
}

static void keyboard_touch(const TouchEvent *t) {
    if (!t->just_up) return;

    ScreenOrientation orient = display_get_orientation();
    int w = orientation_is_landscape(orient) ? 100 : 60;
    if (input_hit_box(t, w - 18, 1, 16, 1)) {
        os_switch_app(&app_launcher);
        return;
    }

    if (orientation_is_portrait(orient)) {
        int base_col = 5;
        int kw = 5;
        int kh = 3;

        /* Tall */
        const char *nums[10] = {"1","2","3","4","5","6","7","8","9","0"};
        for (int i = 0; i < 10; i++) {
            if (input_hit_box(t, base_col + i * kw, 9, kw, kh)) {
                int len = (int)strlen(input_buffer);
                if (len < MAX_TEXT_LEN - 1) {
                    input_buffer[len] = nums[i][0];
                    input_buffer[len + 1] = '\0';
                }
                return;
            }
        }
        /* QWERTY */
        const char *r2[10] = {"Q","W","E","R","T","Y","U","I","O","P"};
        for (int i = 0; i < 10; i++) {
            if (input_hit_box(t, base_col + i * kw, 13, kw, kh)) {
                int len = (int)strlen(input_buffer);
                if (len < MAX_TEXT_LEN - 1) {
                    input_buffer[len] = r2[i][0];
                    input_buffer[len + 1] = '\0';
                }
                return;
            }
        }
        /* ASDF + Æ Ø Å */
        const char *r3[12] = {"A","S","D","F","G","H","J","K","L","\x92","\x9D","\x8F"};
        for (int i = 0; i < 12; i++) {
            if (input_hit_box(t, i * kw, 17, kw, kh)) {
                int len = (int)strlen(input_buffer);
                if (len < MAX_TEXT_LEN - 1) {
                    input_buffer[len] = r3[i][0];
                    input_buffer[len + 1] = '\0';
                }
                return;
            }
        }
        /* ZXCV */
        const char *r4[9] = {"Z","X","C","V","B","N","M",",","."};
        for (int i = 0; i < 9; i++) {
            if (input_hit_box(t, 7 + i * kw, 21, kw, kh)) {
                int len = (int)strlen(input_buffer);
                if (len < MAX_TEXT_LEN - 1) {
                    input_buffer[len] = r4[i][0];
                    input_buffer[len + 1] = '\0';
                }
                return;
            }
        }
        /* Kontroller */
        if (input_hit_box(t, 5, 25, 14, kh)) {
            int len = (int)strlen(input_buffer);
            if (len > 0) input_buffer[len - 1] = '\0';
            return;
        }
        if (input_hit_box(t, 21, 25, 22, kh)) {
            int len = (int)strlen(input_buffer);
            if (len < MAX_TEXT_LEN - 1) {
                input_buffer[len] = ' ';
                input_buffer[len + 1] = '\0';
            }
            return;
        }
        if (input_hit_box(t, 45, 25, 10, kh)) {
            input_buffer[0] = '\0';
            return;
        }
        if (input_hit_box(t, 5, 30, 24, 1)) {
            os_switch_app(&app_launcher);
            return;
        }
        if (input_hit_box(t, 31, 30, 24, 1)) {
            os_toggle_orientation();
            return;
        }
    } else {
        /* LANDSKAP TOUCH-HÅNDTERING */
        int kw = 7;
        int kh = 3;

        /* Tall på rad 7 */
        const char *nums[10] = {"1","2","3","4","5","6","7","8","9","0"};
        for (int i = 0; i < 10; i++) {
            if (input_hit_box(t, 15 + i * kw, 7, kw, kh)) {
                int len = (int)strlen(input_buffer);
                if (len < MAX_TEXT_LEN - 1) {
                    input_buffer[len] = nums[i][0];
                    input_buffer[len + 1] = '\0';
                }
                return;
            }
        }

        /* QWERTY på rad 11 */
        const char *r2[10] = {"Q","W","E","R","T","Y","U","I","O","P"};
        for (int i = 0; i < 10; i++) {
            if (input_hit_box(t, 15 + i * kw, 11, kw, kh)) {
                int len = (int)strlen(input_buffer);
                if (len < MAX_TEXT_LEN - 1) {
                    input_buffer[len] = r2[i][0];
                    input_buffer[len + 1] = '\0';
                }
                return;
            }
        }

        /* ASDF + Æ Ø Å på rad 15 */
        const char *r3[12] = {"A","S","D","F","G","H","J","K","L","\x92","\x9D","\x8F"};
        for (int i = 0; i < 12; i++) {
            if (input_hit_box(t, 8 + i * kw, 15, kw, kh)) {
                int len = (int)strlen(input_buffer);
                if (len < MAX_TEXT_LEN - 1) {
                    input_buffer[len] = r3[i][0];
                    input_buffer[len + 1] = '\0';
                }
                return;
            }
        }

        /* ZXCV på rad 19 */
        const char *r4[10] = {"Z","X","C","V","B","N","M",",",".","-"};
        for (int i = 0; i < 10; i++) {
            if (input_hit_box(t, 15 + i * kw, 19, kw, kh)) {
                int len = (int)strlen(input_buffer);
                if (len < MAX_TEXT_LEN - 1) {
                    input_buffer[len] = r4[i][0];
                    input_buffer[len + 1] = '\0';
                }
                return;
            }
        }

        /* Kontrolltaster på rad 23 */
        if (input_hit_box(t, 8, 23, 14, kh)) { /* SLETT */
            int len = (int)strlen(input_buffer);
            if (len > 0) input_buffer[len - 1] = '\0';
            return;
        }
        if (input_hit_box(t, 24, 23, 30, kh)) { /* MELLOMROM */
            int len = (int)strlen(input_buffer);
            if (len < MAX_TEXT_LEN - 1) {
                input_buffer[len] = ' ';
                input_buffer[len + 1] = '\0';
            }
            return;
        }
        if (input_hit_box(t, 56, 23, 10, kh)) { /* TOM */
            input_buffer[0] = '\0';
            return;
        }
        if (input_hit_box(t, 68, 23, 11, kh)) { /* ROTER */
            os_toggle_orientation();
            return;
        }
        if (input_hit_box(t, 81, 23, 11, kh)) { /* TILBAKE */
            os_switch_app(&app_launcher);
            return;
        }
    }
}

App app_keyboard = {
    .name = "Keyboard",
    .title = "Touch-Tastatur & Notatblokk",
    .on_start = keyboard_start,
    .on_update = NULL,
    .on_render = keyboard_render,
    .on_touch = keyboard_touch,
    .on_resize = NULL,
    .on_exit = NULL
};
