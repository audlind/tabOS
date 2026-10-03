#include <stdio.h>
#include <string.h>
#include "os.h"

#define MAX_POINTS 128

typedef struct {
    int col;
    int row;
    AnsiColor color;
} DrawPoint;

static DrawPoint points[MAX_POINTS];
static int point_count = 0;
static int total_touches = 0;
static int last_raw_x = 0;
static int last_raw_y = 0;
static int last_lcd_x = 0;
static int last_lcd_y = 0;
static bool last_is_down = false;

static void touchtest_start(void) {
    point_count = 0;
    total_touches = 0;
}

static void touchtest_render(void) {
    display_clear(ANSI_BLACK);
    ScreenOrientation orient = display_get_orientation();
    int w = (orient == ORIENTATION_LANDSCAPE) ? 100 : 60;

    /* Header */
    display_draw_box(0, 0, w, 3, "", ANSI_WHITE, ANSI_GREEN, ANSI_YELLOW);
    display_draw_string(2, 1, "TOUCH-SENSOR KALIBRERING & TEGNEFLATE", ANSI_WHITE, ANSI_GREEN);
    display_draw_button(w - 18, 1, 16, " [< TILBAKE] ", ANSI_WHITE, ANSI_RED, false);

    /* HUD */
    display_draw_box(2, 3, w - 4, 4, "SANNTIDS DATA", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_CYAN);
    char buf1[64], buf2[64], buf3[64];
    snprintf(buf1, sizeof(buf1), "STATUS: %s", last_is_down ? "NEDE (TOUCH)" : "OPPE (FRI)");
    snprintf(buf2, sizeof(buf2), "RAW HW: X=%3d Y=%3d", last_raw_x, last_raw_y);
    snprintf(buf3, sizeof(buf3), "LCD: X=%3d Y=%3d", last_lcd_x, last_lcd_y);

    display_draw_string(4, 4, buf1, last_is_down ? ANSI_LIGHT_GREEN : ANSI_LIGHT_GRAY, ANSI_BLACK);
    display_draw_string(28, 4, buf2, ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(4, 5, buf3, ANSI_LIGHT_CYAN, ANSI_BLACK);

    char buf4[32];
    snprintf(buf4, sizeof(buf4), "TRYKK: %d", total_touches);
    display_draw_string(w - 20, 4, buf4, ANSI_WHITE, ANSI_BLACK);

    /* Tegneområde */
    int canvas_top = 8;
    int canvas_bottom = (orient == ORIENTATION_LANDSCAPE) ? 25 : 41;
    display_draw_box(2, canvas_top, w - 4, canvas_bottom - canvas_top, "TEGNEFELT - DRA FINGEREN HER", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_GREEN);

    for (int i = 0; i < point_count; i++) {
        if (points[i].col >= 3 && points[i].col < w - 3 &&
            points[i].row > canvas_top && points[i].row < canvas_bottom - 1) {
            display_put_cell(points[i].col, points[i].row, 0xDB, points[i].color, ANSI_BLACK);
        }
    }

    /* Knapper nederst */
    int btn_y = (orient == ORIENTATION_LANDSCAPE) ? 26 : 43;
    display_draw_box(0, btn_y, w, (orient == ORIENTATION_LANDSCAPE) ? 4 : 7, "", ANSI_WHITE, ANSI_DARK_GRAY, ANSI_LIGHT_GRAY);
    display_draw_button(4, btn_y + 1, 20, " [< HOVEDMENY] ", ANSI_WHITE, ANSI_RED, false);
    display_draw_button(28, btn_y + 1, 20, " [ TOM SKJERM ] ", ANSI_WHITE, ANSI_BROWN, false);
    display_draw_button(52, btn_y + 1, 20, " [ ROTER SKJERM ] ", ANSI_WHITE, ANSI_BLUE, false);
}

static void touchtest_touch(const TouchEvent *t) {
    last_is_down = t->is_down;
    last_raw_x = t->raw_x;
    last_raw_y = t->raw_y;
    input_from_hw_digitizer(t->raw_x, t->raw_y, &last_lcd_x, &last_lcd_y);

    if (t->just_down) {
        total_touches++;
    }

    ScreenOrientation orient = display_get_orientation();
    int w = (orient == ORIENTATION_LANDSCAPE) ? 100 : 60;
    int btn_y = (orient == ORIENTATION_LANDSCAPE) ? 26 : 43;

    if (t->is_down) {
        /* Tegn på lerret */
        int canvas_top = 8;
        int canvas_bottom = (orient == ORIENTATION_LANDSCAPE) ? 25 : 41;
        if (t->grid_col >= 3 && t->grid_col < w - 3 &&
            t->grid_row > canvas_top && t->grid_row < canvas_bottom - 1) {
            if (point_count < MAX_POINTS - 1) {
                points[point_count].col = t->grid_col;
                points[point_count].row = t->grid_row;
                points[point_count].color = (AnsiColor)((total_touches % 14) + 1);
                point_count++;
            }
        }
    } else if (t->just_up) {
        if (input_hit_box(t, w - 18, 1, 16, 1) || input_hit_box(t, 4, btn_y + 1, 20, 1)) {
            os_switch_app(&app_launcher);
        } else if (input_hit_box(t, 28, btn_y + 1, 20, 1)) {
            point_count = 0;
            total_touches = 0;
        } else if (input_hit_box(t, 52, btn_y + 1, 20, 1)) {
            os_toggle_orientation();
        }
    }
}

App app_touchtest = {
    .name = "TouchTest",
    .title = "Touch-Sensor Kalibrering & Tegning",
    .on_start = touchtest_start,
    .on_update = NULL,
    .on_render = touchtest_render,
    .on_touch = touchtest_touch,
    .on_resize = NULL,
    .on_exit = NULL
};
