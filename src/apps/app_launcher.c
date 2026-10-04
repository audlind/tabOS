#include <stdio.h>
#include <string.h>
#include "os.h"

static char status_msg[64] = "Trykk pa en knapp pa skjermen!";
static int pressed_button_id = -1;
static uint32_t uptime_sec = 0;
static uint32_t timer_accum = 0;

static void launcher_start(void) {
    strcpy(status_msg, "Velkommen til tabOS! Trykk pa skjermen.");
    pressed_button_id = -1;
}

static bool launcher_update(uint32_t delta_ms) {
    timer_accum += delta_ms;
    if (timer_accum >= 1000) {
        uptime_sec += timer_accum / 1000;
        timer_accum %= 1000;
        return true;
    }
    return false;
}

static void launcher_render_landscape(void) {
    display_clear(ANSI_BLACK);

    /* Topplinje / Status header */
    char bat_str[32];
    os_get_battery_str(bat_str, sizeof(bat_str));
    char top_buf[100];
    snprintf(top_buf, sizeof(top_buf), "tabOS BBS TERMINAL v0.1     [ NODE 1 ONLINE ]     UPTIME: %us   %s", uptime_sec, bat_str);
    display_draw_box(0, 0, 100, 3, "", ANSI_WHITE, ANSI_BLUE, ANSI_YELLOW);
    display_draw_string(2, 1, top_buf, ANSI_WHITE, ANSI_BLUE);

    /* ASCII Banner (tabOS med horisontale streker intakt) */
    display_draw_string(6, 3,  "_______________________________/\\\\\\______________/\\\\\\\\\\__________/\\\\\\\\\\\\\\\\\\\\\\___", ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(6, 4,  " ______________________________\\/\\\\\\____________/\\\\\\///\\\\\\______/\\\\\\/////////\\\\\\_", ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(6, 5,  "  _____/\\\\\\_____________________\\/\\\\\\__________/\\\\\\/__\\///\\\\\\___\\//\\\\\\______\\///__", ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(6, 6,  "   __/\\\\\\\\\\\\\\\\\\\\\\__/\\\\\\\\\\\\\\\\\\____\\/\\\\\\_________/\\\\\\______\\//\\\\\\___\\////\\\\\\_________", ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(6, 7,  "    _\\////\\\\\\////__\\////////\\\\\\___\\/\\\\\\\\\\\\\\\\\\__\\/\\\\\\_______\\/\\\\\\______\\////\\\\\\______", ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(6, 8,  "     ____\\/\\\\\\________/\\\\\\\\\\\\\\\\\\\\__\\/\\\\\\////\\\\\\_\\//\\\\\\______/\\\\\\__________\\////\\\\\\___", ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(6, 9,  "      ____\\/\\\\\\_/\\\\___/\\\\\\/////\\\\\\__\\/\\\\\\__\\/\\\\\\__\\///\\\\\\__/\\\\\\_____/\\\\\\______\\//\\\\\\__", ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(6, 10, "       ____\\//\\\\\\\\\\___\\//\\\\\\\\\\\\\\\\/\\\\_\\/\\\\\\\\\\\\\\\\\\_____\\///\\\\\\\\\\/_____\\///\\\\\\\\\\\\\\\\\\\\\\/___", ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(6, 11, "        _____\\/////_____\\////////\\//__\\/////////________\\/////_________\\\\///////////_____", ANSI_YELLOW, ANSI_BLACK);

    /* Kolonne 1: Online BBS */
    display_draw_box(4, 12, 28, 12, "BBS ONLINE PORTER", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_CYAN);
    display_draw_button(6, 14, 24, " [1] TELEHACK BBS ", ANSI_WHITE, ANSI_RED, (pressed_button_id == 1));
    display_draw_button(6, 17, 24, " [2] VERTRAUEN BBS", ANSI_WHITE, ANSI_RED, (pressed_button_id == 2));
    display_draw_button(6, 20, 24, " [3] TITANTIC BBS ", ANSI_WHITE, ANSI_RED, (pressed_button_id == 3));

    /* Kolonne 2: Tester & Verktøy */
    display_draw_box(36, 12, 28, 12, "TEST & APPER", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_GREEN);
    display_draw_button(38, 14, 24, " [4] RETRO SNAKE  ", ANSI_WHITE, ANSI_GREEN, (pressed_button_id == 4));
    display_draw_button(38, 17, 24, " [5] TASTATUR-TEST", ANSI_WHITE, ANSI_GREEN, (pressed_button_id == 5));
    display_draw_button(38, 20, 24, " [6] TOUCH-TEST   ", ANSI_WHITE, ANSI_GREEN, (pressed_button_id == 6));

    /* Kolonne 3: Kontrollpanel & Grafikk */
    display_draw_box(68, 12, 28, 12, "KONTROLLPANEL", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_MAGENTA);
    display_draw_button(70, 14, 24, " [7] INNSTILLINGER  ", ANSI_WHITE, ANSI_MAGENTA, (pressed_button_id == 7));
    display_draw_button(70, 17, 24, " [8] ANSI FARGETEST", ANSI_WHITE, ANSI_MAGENTA, (pressed_button_id == 8));
    display_draw_button(70, 20, 24, " [X] NULLSTILL    ", ANSI_WHITE, ANSI_MAGENTA, (pressed_button_id == 9));

    /* Bunnlinje / Touch-meny og Live status */
    display_draw_box(0, 26, 100, 4, "", ANSI_WHITE, ANSI_DARK_GRAY, ANSI_LIGHT_GRAY);
    char status_line[100];
    snprintf(status_line, sizeof(status_line), "STATUS: %s", status_msg);
    display_draw_string(2, 27, status_line, ANSI_YELLOW, ANSI_DARK_GRAY);
    display_draw_string(2, 28, "TOUCH : [5] TASTATUR, [6] TOUCH, [7] INNSTILLINGER, [8] FARGER", ANSI_LIGHT_CYAN, ANSI_DARK_GRAY);
}

static void launcher_render_portrait(void) {
    display_clear(ANSI_BLACK);

    /* Topplinje (60 kolonner x 50 rader) */
    display_draw_box(0, 0, 60, 3, "", ANSI_WHITE, ANSI_BLUE, ANSI_YELLOW);
    display_draw_string(2, 1, "tabOS PORTRETT TERMINAL", ANSI_WHITE, ANSI_BLUE);
    char upt_buf[32];
    char bat_p_str[32];
    os_get_battery_str(bat_p_str, sizeof(bat_p_str));
    snprintf(upt_buf, sizeof(upt_buf), "%s", bat_p_str);
    display_draw_string(36, 1, upt_buf, ANSI_LIGHT_CYAN, ANSI_BLUE);

    /* ASCII Banner */
    display_draw_string(15, 4, "  _        _      ___  ____  ", ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(15, 5, " | |_ __ _| |__  / _ \\/ ___| ", ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(15, 6, " | __/ _` | '_ \\| | | \\___ \\ ", ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(15, 7, " | || (_| | |_) | |_| |___) |", ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(15, 8, "  \\__\\__,_|_.__/ \\___/|____/ ", ANSI_YELLOW, ANSI_BLACK);

    display_draw_string(15, 10, "=== CYBERDECK PORTABLE BBS ===", ANSI_LIGHT_MAGENTA, ANSI_BLACK);

    /* Vertikal Hovedmeny */
    display_draw_box(2, 12, 56, 18, "HOVEDMENY", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_CYAN);
    display_draw_button(5, 13, 50, "  [1] TELEHACK ARPANET BBS ", ANSI_WHITE, ANSI_RED, (pressed_button_id == 1));
    display_draw_button(5, 15, 50, "  [2] VERTRAUEN SYNCHRONET ", ANSI_WHITE, ANSI_RED, (pressed_button_id == 2));
    display_draw_button(5, 17, 50, "  [3] TITANTIC RETRO BBS   ", ANSI_WHITE, ANSI_RED, (pressed_button_id == 3));
    display_draw_button(5, 19, 50, "  [4] RETRO SNAKE ARKADE   ", ANSI_WHITE, ANSI_GREEN, (pressed_button_id == 4));
    display_draw_button(5, 21, 50, "  [5] TASTATUR-TEST (STORE)", ANSI_WHITE, ANSI_GREEN, (pressed_button_id == 5));
    display_draw_button(5, 23, 50, "  [6] TOUCH-TEST & TEGNING ", ANSI_WHITE, ANSI_GREEN, (pressed_button_id == 6));
    display_draw_button(5, 25, 50, "  [7] INNSTILLINGER & STROM", ANSI_WHITE, ANSI_MAGENTA, (pressed_button_id == 7));
    display_draw_button(5, 27, 50, "  [8] ANSI FARGE- & GRAFIKK", ANSI_WHITE, ANSI_MAGENTA, (pressed_button_id == 8));

    /* Statusboks */
    display_draw_box(2, 31, 56, 6, "TERMINAL STATUS", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_GREEN);
    char status_line[60];
    snprintf(status_line, sizeof(status_line), "> %s", status_msg);
    display_draw_string(4, 33, status_line, ANSI_LIGHT_GREEN, ANSI_BLACK);
    display_draw_string(4, 34, "> Trykk pa en av knappene for a starte!", ANSI_YELLOW, ANSI_BLACK);

    /* Hurtignavigasjon nederst */
    display_draw_box(0, 38, 60, 11, "HURTIGNAVIGASJON", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_GRAY);
    display_draw_button(4, 41, 24, " [ RETRO SNAKE ] ", ANSI_WHITE, ANSI_GREEN, (pressed_button_id == 4));
    display_draw_button(32, 41, 24, " [ TASTATUR ] ", ANSI_WHITE, ANSI_GREEN, (pressed_button_id == 5));
    display_draw_button(4, 45, 24, " [ TOUCH-TEST ] ", ANSI_WHITE, ANSI_CYAN, (pressed_button_id == 6));
    display_draw_button(32, 45, 24, " [ INNSTILLING ] ", ANSI_WHITE, ANSI_MAGENTA, (pressed_button_id == 7));
}

static void launcher_render(void) {
    if (orientation_is_portrait(display_get_orientation())) {
        launcher_render_portrait();
    } else {
        launcher_render_landscape();
    }
}

static void launcher_touch(const TouchEvent *t) {
    ScreenOrientation orient = display_get_orientation();

    if (orientation_is_landscape(orient)) {
        /* Landskap sjekk */
        if (t->is_down) {
            if (input_hit_box(t, 6, 14, 24, 1))  pressed_button_id = 1;
            else if (input_hit_box(t, 6, 17, 24, 1)) pressed_button_id = 2;
            else if (input_hit_box(t, 6, 20, 24, 1)) pressed_button_id = 3;
            else if (input_hit_box(t, 38, 14, 24, 1)) pressed_button_id = 4;
            else if (input_hit_box(t, 38, 17, 24, 1)) pressed_button_id = 5;
            else if (input_hit_box(t, 38, 20, 24, 1)) pressed_button_id = 6;
            else if (input_hit_box(t, 70, 14, 24, 1)) pressed_button_id = 7;
            else if (input_hit_box(t, 70, 17, 24, 1)) pressed_button_id = 8;
            else if (input_hit_box(t, 70, 20, 24, 1)) pressed_button_id = 9;
        } else if (t->just_up) {
            int clicked = pressed_button_id;
            pressed_button_id = -1;

            switch (clicked) {
                case 1:
                    bbs_set_node(0);
                    os_switch_app(&app_bbs);
                    break;
                case 2:
                    bbs_set_node(1);
                    os_switch_app(&app_bbs);
                    break;
                case 3:
                    bbs_set_node(2);
                    os_switch_app(&app_bbs);
                    break;
                case 4:
                    os_switch_app(&app_snake);
                    break;
                case 5:
                    os_switch_app(&app_keyboard);
                    break;
                case 6:
                    os_switch_app(&app_touchtest);
                    break;
                case 7:
                    os_switch_app(&app_settings);
                    break;
                case 8:
                    os_switch_app(&app_colortest);
                    break;
                case 9:
                    snprintf(status_msg, sizeof(status_msg), "Systemstatus nullstilt.");
                    break;
            }
        }
    } else {
        /* Portrett sjekk */
        if (t->is_down) {
            if (input_hit_box(t, 5, 13, 50, 1))       pressed_button_id = 1;
            else if (input_hit_box(t, 5, 15, 50, 1))  pressed_button_id = 2;
            else if (input_hit_box(t, 5, 17, 50, 1))  pressed_button_id = 3;
            else if (input_hit_box(t, 5, 19, 50, 1))  pressed_button_id = 4;
            else if (input_hit_box(t, 5, 21, 50, 1))  pressed_button_id = 5;
            else if (input_hit_box(t, 5, 23, 50, 1))  pressed_button_id = 6;
            else if (input_hit_box(t, 5, 25, 50, 1))  pressed_button_id = 7;
            else if (input_hit_box(t, 5, 27, 50, 1))  pressed_button_id = 8;
            else if (input_hit_box(t, 4, 41, 24, 1))  pressed_button_id = 4;
            else if (input_hit_box(t, 32, 41, 24, 1)) pressed_button_id = 5;
            else if (input_hit_box(t, 4, 45, 24, 1))  pressed_button_id = 6;
            else if (input_hit_box(t, 32, 45, 24, 1)) pressed_button_id = 7;
        } else if (t->just_up) {
            int clicked = pressed_button_id;
            pressed_button_id = -1;

            if (clicked == 1) {
                bbs_set_node(0);
                os_switch_app(&app_bbs);
            } else if (clicked == 2) {
                bbs_set_node(1);
                os_switch_app(&app_bbs);
            } else if (clicked == 3) {
                bbs_set_node(2);
                os_switch_app(&app_bbs);
            } else if (clicked == 4) {
                os_switch_app(&app_snake);
            } else if (clicked == 5) {
                os_switch_app(&app_keyboard);
            } else if (clicked == 6) {
                os_switch_app(&app_touchtest);
            } else if (clicked == 7) {
                os_switch_app(&app_settings);
            } else if (clicked == 8) {
                os_switch_app(&app_colortest);
            }
        }
    }
}

App app_launcher = {
    .name = "Launcher",
    .title = "tabOS BBS Cyberdeck Hovedmeny",
    .on_start = launcher_start,
    .on_update = launcher_update,
    .on_render = launcher_render,
    .on_touch = launcher_touch,
    .on_resize = NULL,
    .on_exit = NULL
};
