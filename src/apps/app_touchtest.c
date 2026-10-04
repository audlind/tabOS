#include <stdio.h>
#include <string.h>
#include "os.h"
#include "sensor.h"

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

static bool touchtest_update(uint32_t delta_ms) {
    (void)delta_ms;
    /* Kontinuerlig oppdatering for å vise live tyngdekraftsdata og touch-posisjon */
    return true;
}

static void touchtest_render(void) {
    display_clear(ANSI_BLACK);
    ScreenOrientation orient = display_get_orientation();
    int w = orientation_is_landscape(orient) ? 100 : 60;

    /* Header */
    display_draw_box(0, 0, w, 3, "", ANSI_WHITE, ANSI_GREEN, ANSI_YELLOW);
    display_draw_string(2, 1, "TOUCH & TILT KALIBRERING", ANSI_WHITE, ANSI_GREEN);
    char bat_t[32];
    os_get_battery_str(bat_t, sizeof(bat_t));
    display_draw_string(orientation_is_landscape(orient) ? 42 : 27, 1, bat_t, ANSI_LIGHT_CYAN, ANSI_GREEN);
    display_draw_button(w - 18, 1, 16, " [< TILBAKE] ", ANSI_WHITE, ANSI_RED, false);

    /* HUD */
    display_draw_box(2, 3, w - 4, 5, "SANNTIDS HARDWARE-DATA", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_CYAN);
    char buf1[64], buf2[64], buf3[64];
    snprintf(buf1, sizeof(buf1), "TOUCH: %s", last_is_down ? "NEDE (AKTIV)" : "OPPE (FRI)");
    snprintf(buf2, sizeof(buf2), "RAW HW: X=%3d Y=%3d", last_raw_x, last_raw_y);
    snprintf(buf3, sizeof(buf3), "LCD: X=%3d Y=%3d", last_lcd_x, last_lcd_y);

    display_draw_string(4, 4, buf1, last_is_down ? ANSI_LIGHT_GREEN : ANSI_LIGHT_GRAY, ANSI_BLACK);
    display_draw_string(28, 4, buf2, ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(4, 5, buf3, ANSI_LIGHT_CYAN, ANSI_BLACK);

    char buf4[32];
    snprintf(buf4, sizeof(buf4), "TRYKK: %d", total_touches);
    display_draw_string(w - 20, 4, buf4, ANSI_WHITE, ANSI_BLACK);

    /* Live akselerometer */
    int sx, sy, sz;
    sensor_get_values(&sx, &sy, &sz);
    const char *orient_name = "LANDSKAP (0)";
    switch (orient) {
        case ORIENTATION_LANDSCAPE:          orient_name = "0' LND"; break;
        case ORIENTATION_PORTRAIT:           orient_name = "90' PRT"; break;
        case ORIENTATION_LANDSCAPE_INVERTED: orient_name = "180' LND"; break;
        case ORIENTATION_PORTRAIT_INVERTED:  orient_name = "270' PRT"; break;
    }

    char buf_gsensor[90];
    snprintf(buf_gsensor, sizeof(buf_gsensor), "G: X=%+6d Y=%+6d Z=%+6d [%s] %s AUTO:%s",
             sx, sy, sz, sensor_is_flat() ? "FLAT" : "TILT", orient_name,
             sensor_get_auto_rotate() ? "ON" : "OFF");
    display_draw_string(4, 6, buf_gsensor, ANSI_LIGHT_MAGENTA, ANSI_BLACK);

    /* Tegneområde */
    int canvas_top = 9;
    int canvas_bottom = orientation_is_landscape(orient) ? 25 : 40;
    display_draw_box(2, canvas_top, w - 4, canvas_bottom - canvas_top, "TEGNEFELT - DRA FINGEREN HER", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_GREEN);

    for (int i = 0; i < point_count; i++) {
        if (points[i].col >= 3 && points[i].col < w - 3 &&
            points[i].row > canvas_top && points[i].row < canvas_bottom - 1) {
            display_put_cell(points[i].col, points[i].row, 0xDB, points[i].color, ANSI_BLACK);
        }
    }

    /* Knapper nederst */
    int btn_y = orientation_is_landscape(orient) ? 26 : 41;
    display_draw_box(0, btn_y, w, orientation_is_landscape(orient) ? 4 : 8, "", ANSI_WHITE, ANSI_DARK_GRAY, ANSI_LIGHT_GRAY);
    
    char auto_btn[24];
    snprintf(auto_btn, sizeof(auto_btn), " [ AUTO-ROT: %s ] ", sensor_get_auto_rotate() ? "PAA" : "AV ");
    char swap_btn[24];
    snprintf(swap_btn, sizeof(swap_btn), " [ AKSE: %s ] ", sensor_get_axis_swap() ? "SWAP" : "NORM");

    if (orientation_is_landscape(orient)) {
        display_draw_button(4,  btn_y + 1, 20, " [< HOVEDMENY] ", ANSI_WHITE, ANSI_RED, false);
        display_draw_button(27, btn_y + 1, 20, " [ TOM SKJERM ] ", ANSI_WHITE, ANSI_BROWN, false);
        display_draw_button(50, btn_y + 1, 24, auto_btn, ANSI_WHITE, sensor_get_auto_rotate() ? ANSI_GREEN : ANSI_DARK_GRAY, false);
        display_draw_button(77, btn_y + 1, 20, swap_btn, ANSI_WHITE, ANSI_MAGENTA, false);
    } else {
        display_draw_button(4,  btn_y + 1, 25, " [< HOVEDMENY] ", ANSI_WHITE, ANSI_RED, false);
        display_draw_button(31, btn_y + 1, 25, " [ TOM SKJERM ] ", ANSI_WHITE, ANSI_BROWN, false);
        display_draw_button(4,  btn_y + 4, 25, auto_btn, ANSI_WHITE, sensor_get_auto_rotate() ? ANSI_GREEN : ANSI_DARK_GRAY, false);
        display_draw_button(31, btn_y + 4, 25, swap_btn, ANSI_WHITE, ANSI_MAGENTA, false);
    }
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
    int w = orientation_is_landscape(orient) ? 100 : 60;
    int btn_y = orientation_is_landscape(orient) ? 26 : 41;

    if (t->is_down) {
        /* Tegn på lerret */
        int canvas_top = 9;
        int canvas_bottom = orientation_is_landscape(orient) ? 25 : 40;
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
        if (input_hit_box(t, w - 18, 1, 16, 1)) {
            os_switch_app(&app_launcher);
        } else if (orientation_is_landscape(orient)) {
            if (input_hit_box(t, 4, btn_y + 1, 20, 1)) {
                os_switch_app(&app_launcher);
            } else if (input_hit_box(t, 27, btn_y + 1, 20, 1)) {
                point_count = 0;
                total_touches = 0;
            } else if (input_hit_box(t, 50, btn_y + 1, 24, 1)) {
                sensor_toggle_auto_rotate();
            } else if (input_hit_box(t, 77, btn_y + 1, 20, 1)) {
                sensor_set_axis_swap(!sensor_get_axis_swap());
            }
        } else {
            if (input_hit_box(t, 4, btn_y + 1, 25, 1)) {
                os_switch_app(&app_launcher);
            } else if (input_hit_box(t, 31, btn_y + 1, 25, 1)) {
                point_count = 0;
                total_touches = 0;
            } else if (input_hit_box(t, 4, btn_y + 4, 25, 1)) {
                sensor_toggle_auto_rotate();
            } else if (input_hit_box(t, 31, btn_y + 4, 25, 1)) {
                sensor_set_axis_swap(!sensor_get_axis_swap());
            }
        }
    }
}

App app_touchtest = {
    .name = "TouchTest",
    .title = "Touch- og Tilt-Sensor Kalibrering",
    .on_start = touchtest_start,
    .on_update = touchtest_update,
    .on_render = touchtest_render,
    .on_touch = touchtest_touch,
    .on_resize = NULL,
    .on_exit = NULL
};
