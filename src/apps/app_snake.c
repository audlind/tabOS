#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "os.h"
#include "audio.h"

#define BOARD_W 22
#define BOARD_H 22
#define MAX_SNAKE (BOARD_W * BOARD_H)

/* Plassering av spillvinduet i 60x50 portrett-rutenett */
#define BOARD_BOX_COL 7
#define BOARD_BOX_ROW 4
#define BOARD_BOX_W   46  /* 44 kolonner innvendig = 352 piksler */
#define BOARD_BOX_H   24  /* 22 rader innvendig    = 352 piksler */

typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

typedef struct {
    int x;
    int y;
} Point;

static Point snake[MAX_SNAKE];
static int snake_len = 3;
static Direction current_dir = DIR_RIGHT;
static Direction next_dir = DIR_RIGHT;
static Point food = {15, 11};
static bool game_over = false;
static bool paused = false;
static uint32_t score = 0;
static uint32_t high_score = 0;
static uint32_t move_interval_ms = 140;
static uint32_t move_timer_ms = 0;
static int active_pressed_btn = -1;

/* Egendefinert stor touch-knapp for D-Pad */
static void draw_dpad_key(int col, int row, int w, int h, const char *label, bool pressed, AnsiColor fg, AnsiColor bg) {
    AnsiColor final_fg = pressed ? ANSI_BLACK : fg;
    AnsiColor final_bg = pressed ? ANSI_LIGHT_GREEN : bg;
    AnsiColor border_col = pressed ? ANSI_YELLOW : ANSI_LIGHT_CYAN;

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

static void spawn_food(void) {
    bool valid = false;
    int attempts = 0;
    while (!valid && attempts < 200) {
        food.x = rand() % BOARD_W;
        food.y = rand() % BOARD_H;
        valid = true;
        for (int i = 0; i < snake_len; i++) {
            if (snake[i].x == food.x && snake[i].y == food.y) {
                valid = false;
                break;
            }
        }
        attempts++;
    }
}

static void reset_game(void) {
    snake_len = 4;
    snake[0] = (Point){8, 11};
    snake[1] = (Point){7, 11};
    snake[2] = (Point){6, 11};
    snake[3] = (Point){5, 11};

    current_dir = DIR_RIGHT;
    next_dir = DIR_RIGHT;
    game_over = false;
    paused = false;
    score = 0;
    move_interval_ms = 140;
    move_timer_ms = 0;
    active_pressed_btn = -1;
    spawn_food();
    audio_play(SOUND_START);
}

static void snake_start(void) {
    /* Sikre at spillet kjører i portrettmodus */
    if (!orientation_is_portrait(display_get_orientation())) {
        display_set_orientation(ORIENTATION_PORTRAIT);
    }
    srand(time(NULL));
    reset_game();
}

static bool snake_update(uint32_t delta_ms) {
    if (game_over || paused) return false;

    move_timer_ms += delta_ms;
    if (move_timer_ms < move_interval_ms) {
        return false;
    }
    move_timer_ms = 0;

    /* Bekreft retning (hindre 180-graders snuing rett inn i kroppen) */
    if ((next_dir == DIR_UP && current_dir != DIR_DOWN) ||
        (next_dir == DIR_DOWN && current_dir != DIR_UP) ||
        (next_dir == DIR_LEFT && current_dir != DIR_RIGHT) ||
        (next_dir == DIR_RIGHT && current_dir != DIR_LEFT)) {
        current_dir = next_dir;
    }

    Point new_head = snake[0];
    switch (current_dir) {
        case DIR_UP:    new_head.y--; break;
        case DIR_DOWN:  new_head.y++; break;
        case DIR_LEFT:  new_head.x--; break;
        case DIR_RIGHT: new_head.x++; break;
    }

    /* Kollisjon mot vegger */
    if (new_head.x < 0 || new_head.x >= BOARD_W ||
        new_head.y < 0 || new_head.y >= BOARD_H) {
        game_over = true;
        if (score > high_score) high_score = score;
        audio_play(SOUND_CRASH);
        return true;
    }

    /* Kollisjon mot egen kropp */
    for (int i = 0; i < snake_len - 1; i++) {
        if (snake[i].x == new_head.x && snake[i].y == new_head.y) {
            game_over = true;
            if (score > high_score) high_score = score;
            audio_play(SOUND_CRASH);
            return true;
        }
    }

    /* Sjekk om mat spises */
    if (new_head.x == food.x && new_head.y == food.y) {
        score += 10;
        if (score > high_score) high_score = score;
        if (snake_len < MAX_SNAKE - 1) {
            snake_len++;
        }
        /* Øk hastighet litt etter hvert */
        if (move_interval_ms > 60) {
            move_interval_ms -= 3;
        }
        audio_play(SOUND_EAT);
        spawn_food();
    }

    /* Flytt slangen */
    for (int i = snake_len - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
    }
    snake[0] = new_head;

    return true; /* Be om skjermoppdatering */
}

static void snake_render(void) {
    display_clear(ANSI_BLACK);

    /* 1. Topplinje med Score & Highscore */
    display_draw_box(0, 0, 60, 3, "", ANSI_WHITE, ANSI_BLUE, ANSI_YELLOW);
    display_draw_string(2, 1, "SNAKE", ANSI_YELLOW, ANSI_BLUE);

    char score_str[32];
    snprintf(score_str, sizeof(score_str), "POENG: %3u", score);
    display_draw_string(14, 1, score_str, ANSI_WHITE, ANSI_BLUE);

    char high_str[32];
    snprintf(high_str, sizeof(high_str), "REKORD: %3u", high_score);
    display_draw_string(29, 1, high_str, ANSI_LIGHT_GREEN, ANSI_BLUE);

    display_draw_button(46, 1, 12, "[< MENY]", ANSI_WHITE, ANSI_RED, (active_pressed_btn == 99));

    /* 2. Kvadratisk spillvindu (352x352 piksler nøyaktig!) */
    display_draw_box(BOARD_BOX_COL, BOARD_BOX_ROW, BOARD_BOX_W, BOARD_BOX_H,
                     "KVADRATISK SPILLFELT (352x352)", ANSI_LIGHT_CYAN, ANSI_BLACK, ANSI_CYAN);

    /* Tegn mat (Hjerte 0x03 i blinkende rød) */
    int food_c = BOARD_BOX_COL + 1 + food.x * 2;
    int food_r = BOARD_BOX_ROW + 1 + food.y;
    display_put_cell(food_c,     food_r, 0x03, ANSI_LIGHT_RED, ANSI_BLACK);
    display_put_cell(food_c + 1, food_r, 0x03, ANSI_LIGHT_RED, ANSI_BLACK);

    /* Tegn slangen */
    for (int i = snake_len - 1; i >= 0; i--) {
        int sc = BOARD_BOX_COL + 1 + snake[i].x * 2;
        int sr = BOARD_BOX_ROW + 1 + snake[i].y;
        if (i == 0) {
            /* Hode: Smiley 0x02 */
            display_put_cell(sc,     sr, 0x02, ANSI_YELLOW, ANSI_BLACK);
            display_put_cell(sc + 1, sr, 0x02, ANSI_YELLOW, ANSI_BLACK);
        } else {
            /* Kropp: Solide blokker med sjattering */
            AnsiColor col = (i % 2 == 0) ? ANSI_LIGHT_GREEN : ANSI_GREEN;
            display_put_cell(sc,     sr, 0xDB, col, ANSI_BLACK);
            display_put_cell(sc + 1, sr, 0xDB, col, ANSI_BLACK);
        }
    }

    /* Spillstatus / Game Over banner */
    if (game_over) {
        display_draw_box(14, 12, 32, 7, "GAME OVER", ANSI_WHITE, ANSI_RED, ANSI_YELLOW);
        display_draw_string(18, 14, "DU KROSTET SLANGEN!", ANSI_YELLOW, ANSI_RED);
        char fin_score[32];
        snprintf(fin_score, sizeof(fin_score), "Poeng: %u  Rekord: %u", score, high_score);
        display_draw_string(18, 15, fin_score, ANSI_WHITE, ANSI_RED);
        display_draw_string(16, 17, "Trykk [START] for nytt spill", ANSI_LIGHT_CYAN, ANSI_RED);
    } else if (paused) {
        display_draw_box(17, 13, 26, 5, "PAUSE", ANSI_WHITE, ANSI_DARK_GRAY, ANSI_YELLOW);
        display_draw_string(19, 15, "Trykk START for a spille", ANSI_YELLOW, ANSI_DARK_GRAY);
    }

    /* 3. D-Pad og berøringskontrollere på nedre halvdel (Rader 29..49) */
    char status_txt[48];
    if (game_over) {
        snprintf(status_txt, sizeof(status_txt), "> SPILLET ER SLUTT! Trykk [START]");
    } else if (paused) {
        snprintf(status_txt, sizeof(status_txt), "> SPILLET ER PAUSET");
    } else {
        snprintf(status_txt, sizeof(status_txt), "> Slangelengde: %d   Fart: %d ms", snake_len, move_interval_ms);
    }
    display_draw_string(6, 29, status_txt, ANSI_LIGHT_GREEN, ANSI_BLACK);

    /* D-Pad knapper: 128x64 piksler per retningsknapp! */
    /* OPP (rad 31..34) */
    draw_dpad_key(22, 31, 16, 4, "\x1E OPP", (active_pressed_btn == 1), ANSI_WHITE, ANSI_DARK_GRAY);

    /* VENSTRE, START/PAUSE, HOYRE (rad 36..39) */
    draw_dpad_key(5,  36, 16, 4, "\x11 VENSTRE", (active_pressed_btn == 3), ANSI_WHITE, ANSI_DARK_GRAY);
    if (game_over) {
        draw_dpad_key(22, 36, 16, 4, "NYTT!", (active_pressed_btn == 5), ANSI_WHITE, ANSI_RED);
    } else if (paused) {
        draw_dpad_key(22, 36, 16, 4, "START", (active_pressed_btn == 5), ANSI_WHITE, ANSI_GREEN);
    } else {
        draw_dpad_key(22, 36, 16, 4, "PAUSE", (active_pressed_btn == 5), ANSI_WHITE, ANSI_BLUE);
    }
    draw_dpad_key(39, 36, 16, 4, "HOYRE \x10", (active_pressed_btn == 4), ANSI_WHITE, ANSI_DARK_GRAY);

    /* NED (rad 41..44) */
    draw_dpad_key(22, 41, 16, 4, "\x1F NED", (active_pressed_btn == 2), ANSI_WHITE, ANSI_DARK_GRAY);

    /* Bunnmeny (rad 46..48) */
    draw_dpad_key(5,  46, 24, 3, "[< HOVEDMENY]", (active_pressed_btn == 99), ANSI_WHITE, ANSI_RED);
    draw_dpad_key(31, 46, 24, 3, "[ NYTT SPILL ]", (active_pressed_btn == 88), ANSI_WHITE, ANSI_CYAN);
}

static void snake_touch(const TouchEvent *t) {
    if (t->is_down) {
        /* Sjekk hvilken knapp som holdes */
        if (input_hit_box(t, 22, 31, 16, 4)) active_pressed_btn = 1;      /* OPP */
        else if (input_hit_box(t, 22, 41, 16, 4)) active_pressed_btn = 2; /* NED */
        else if (input_hit_box(t, 5, 36, 16, 4))  active_pressed_btn = 3; /* VENSTRE */
        else if (input_hit_box(t, 39, 36, 16, 4)) active_pressed_btn = 4; /* HOYRE */
        else if (input_hit_box(t, 22, 36, 16, 4)) active_pressed_btn = 5; /* START/PAUSE */
        else if (input_hit_box(t, 5, 46, 24, 3))  active_pressed_btn = 99;/* MENY */
        else if (input_hit_box(t, 31, 46, 24, 3)) active_pressed_btn = 88;/* NYTT */
        else if (input_hit_box(t, 46, 1, 12, 1))  active_pressed_btn = 99;/* MENY TOPP */

        /* Direkte berøring i spillvinduet (rask retningsendring) */
        else if (input_hit_box(t, BOARD_BOX_COL + 1, BOARD_BOX_ROW + 1, BOARD_BOX_W - 2, BOARD_BOX_H - 2)) {
            int cx = BOARD_BOX_COL + BOARD_BOX_W / 2;
            int cy = BOARD_BOX_ROW + BOARD_BOX_H / 2;
            int cell_x = t->grid_col;
            int cell_y = t->grid_row;
            int dx = cell_x - cx;
            int dy = cell_y - cy;
            if (abs(dx) > abs(dy)) {
                if (dx > 0 && current_dir != DIR_LEFT) next_dir = DIR_RIGHT;
                else if (dx < 0 && current_dir != DIR_RIGHT) next_dir = DIR_LEFT;
            } else {
                if (dy > 0 && current_dir != DIR_UP) next_dir = DIR_DOWN;
                else if (dy < 0 && current_dir != DIR_DOWN) next_dir = DIR_UP;
            }
        }
    } else if (t->just_up) {
        int clicked = active_pressed_btn;
        active_pressed_btn = -1;

        switch (clicked) {
            case 1: /* OPP */
                if (current_dir != DIR_DOWN) next_dir = DIR_UP;
                if (paused) paused = false;
                break;
            case 2: /* NED */
                if (current_dir != DIR_UP) next_dir = DIR_DOWN;
                if (paused) paused = false;
                break;
            case 3: /* VENSTRE */
                if (current_dir != DIR_RIGHT) next_dir = DIR_LEFT;
                if (paused) paused = false;
                break;
            case 4: /* HOYRE */
                if (current_dir != DIR_LEFT) next_dir = DIR_RIGHT;
                if (paused) paused = false;
                break;
            case 5: /* START / PAUSE */
                if (game_over) reset_game();
                else paused = !paused;
                break;
            case 88: /* NYTT SPILL */
                reset_game();
                break;
            case 99: /* HOVEDMENY */
                os_switch_app(&app_launcher);
                break;
        }
    }
}

App app_snake = {
    .name = "Snake",
    .title = "Retro Cyberdeck Snake",
    .on_start = snake_start,
    .on_update = snake_update,
    .on_render = snake_render,
    .on_touch = snake_touch,
    .on_resize = NULL,
    .on_exit = NULL
};
