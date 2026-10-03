#include <string.h>
#include "display.h"
#include "font_cp437_8x16.h"

/* Statisk buffer for tekst-rutenettet */
static AnsiCell grid[MAX_GRID_ROWS][MAX_GRID_COLS];
static uint32_t *fb_target = NULL;
static ScreenOrientation cur_orientation = ORIENTATION_LANDSCAPE;
static int cur_cols = GRID_LANDSCAPE_COLS;
static int cur_rows = GRID_LANDSCAPE_ROWS;

void display_init(uint32_t *framebuffer, ScreenOrientation orientation) {
    fb_target = framebuffer;
    display_set_orientation(orientation);
    display_clear(ANSI_BLACK);
}

void display_set_orientation(ScreenOrientation orientation) {
    cur_orientation = orientation;
    if (orientation == ORIENTATION_PORTRAIT) {
        cur_cols = GRID_PORTRAIT_COLS;
        cur_rows = GRID_PORTRAIT_ROWS;
    } else {
        cur_cols = GRID_LANDSCAPE_COLS;
        cur_rows = GRID_LANDSCAPE_ROWS;
    }
}

ScreenOrientation display_get_orientation(void) {
    return cur_orientation;
}

int display_get_cols(void) {
    return cur_cols;
}

int display_get_rows(void) {
    return cur_rows;
}

void display_clear(uint8_t bg) {
    for (int r = 0; r < MAX_GRID_ROWS; r++) {
        for (int c = 0; c < MAX_GRID_COLS; c++) {
            grid[r][c].glyph = ' ';
            grid[r][c].fg = ANSI_LIGHT_GRAY;
            grid[r][c].bg = bg;
            grid[r][c].attr = 0;
        }
    }
}

void display_put_cell(int col, int row, uint8_t glyph, uint8_t fg, uint8_t bg) {
    if (col < 0 || col >= cur_cols || row < 0 || row >= cur_rows) {
        return;
    }
    grid[row][col].glyph = glyph;
    grid[row][col].fg = fg & 0x0F;
    grid[row][col].bg = bg & 0x0F;
}

void display_draw_string(int col, int row, const char *str, uint8_t fg, uint8_t bg) {
    if (!str || row < 0 || row >= cur_rows) return;
    int c = col;
    while (*str && c < cur_cols) {
        if (*str == '\n') {
            row++;
            c = col;
            if (row >= cur_rows) break;
            str++;
            continue;
        }
        display_put_cell(c, row, (uint8_t)*str, fg, bg);
        c++;
        str++;
    }
}

void display_draw_box(int col, int row, int w, int h, const char *title, uint8_t fg, uint8_t bg, uint8_t border_fg) {
    if (w < 2 || h < 2) return;
    int r2 = row + h - 1;
    int c2 = col + w - 1;

    /* Hjørner */
    display_put_cell(col, row, CP437_CORNER_TL, border_fg, bg);
    display_put_cell(c2,  row, CP437_CORNER_TR, border_fg, bg);
    display_put_cell(col, r2,  CP437_CORNER_BL, border_fg, bg);
    display_put_cell(c2,  r2,  CP437_CORNER_BR, border_fg, bg);

    /* Horisontale linjer */
    for (int c = col + 1; c < c2; c++) {
        display_put_cell(c, row, CP437_HLINE, border_fg, bg);
        display_put_cell(c, r2,  CP437_HLINE, border_fg, bg);
    }

    /* Vertikale linjer */
    for (int r = row + 1; r < r2; r++) {
        display_put_cell(col, r, CP437_VLINE, border_fg, bg);
        display_put_cell(c2,  r, CP437_VLINE, border_fg, bg);
    }

    /* Fyll innvendig */
    for (int r = row + 1; r < r2; r++) {
        for (int c = col + 1; c < c2; c++) {
            display_put_cell(c, r, ' ', fg, bg);
        }
    }

    /* Tittel */
    if (title && *title) {
        int len = (int)strlen(title);
        if (len + 4 <= w) {
            int tcol = col + (w - len - 2) / 2;
            display_put_cell(tcol - 1, row, ' ', border_fg, bg);
            display_draw_string(tcol, row, title, fg, bg);
            display_put_cell(tcol + len, row, ' ', border_fg, bg);
        }
    }
}

void display_draw_button(int col, int row, int w, const char *label, uint8_t fg, uint8_t bg, bool pressed) {
    uint8_t final_fg = pressed ? bg : fg;
    uint8_t final_bg = pressed ? fg : bg;

    display_put_cell(col, row, '[', final_fg, final_bg);
    int c = col + 1;
    int len = (int)strlen(label);
    
    /* Sentrert tekst */
    int pad = (w - 2 - len) / 2;
    for (int i = 0; i < pad && c < col + w - 1; i++) {
        display_put_cell(c++, row, ' ', final_fg, final_bg);
    }
    while (*label && c < col + w - 1) {
        display_put_cell(c++, row, (uint8_t)*label++, final_fg, final_bg);
    }
    while (c < col + w - 1) {
        display_put_cell(c++, row, ' ', final_fg, final_bg);
    }
    display_put_cell(col + w - 1, row, ']', final_fg, final_bg);
}

void display_render_frame(void) {
    if (!fb_target) return;

    if (cur_orientation == ORIENTATION_LANDSCAPE) {
        /* Landskap: 100 kolonner x 30 rader -> 800 x 480 piksler */
        for (int r = 0; r < cur_rows; r++) {
            for (int c = 0; c < cur_cols; c++) {
                AnsiCell cell = grid[r][c];
                uint32_t fg_color = ANSI_PALETTE_ARGB[cell.fg];
                uint32_t bg_color = ANSI_PALETTE_ARGB[cell.bg];
                const uint8_t *glyph_bitmap = font_cp437_8x16[cell.glyph];

                int base_x = c * FONT_W;
                int base_y = r * FONT_H;

                for (int py = 0; py < FONT_H; py++) {
                    uint8_t row_bits = glyph_bitmap[py];
                    int y = base_y + py;
                    uint32_t *fb_row = &fb_target[y * SCREEN_PHYS_WIDTH + base_x];

                    for (int px = 0; px < FONT_W; px++) {
                        fb_row[px] = (row_bits & (0x80 >> px)) ? fg_color : bg_color;
                    }
                }
            }
        }
    } else {
        /* Portrett: 60 kolonner x 50 rader -> Virtuelt 480 x 800 piksler */
        /* Roter 90 grader med klokken til fysisk 800 x 480 */
        for (int r = 0; r < cur_rows; r++) {
            for (int c = 0; c < cur_cols; c++) {
                AnsiCell cell = grid[r][c];
                uint32_t fg_color = ANSI_PALETTE_ARGB[cell.fg];
                uint32_t bg_color = ANSI_PALETTE_ARGB[cell.bg];
                const uint8_t *glyph_bitmap = font_cp437_8x16[cell.glyph];

                int base_xv = c * FONT_W;
                int base_yv = r * FONT_H;

                for (int py = 0; py < FONT_H; py++) {
                    uint8_t row_bits = glyph_bitmap[py];
                    int yv = base_yv + py; /* 0..799 */

                    /* Rotasjonsformel: x_phys = 799 - yv, y_phys = xv */
                    int x_phys = (SCREEN_PHYS_WIDTH - 1) - yv;

                    for (int px = 0; px < FONT_W; px++) {
                        int xv = base_xv + px; /* 0..479 */
                        int y_phys = xv;

                        uint32_t color = (row_bits & (0x80 >> px)) ? fg_color : bg_color;
                        fb_target[y_phys * SCREEN_PHYS_WIDTH + x_phys] = color;
                    }
                }
            }
        }
    }
}
