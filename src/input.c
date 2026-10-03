#include "input.h"

static bool last_down = false;

void input_init(void) {
    last_down = false;
}

void input_from_hw_digitizer(int hw_x, int hw_y, int *lcd_x, int *lcd_y) {
    *lcd_x = (hw_x * SCREEN_PHYS_WIDTH) / TOUCH_HW_MAX_X;
    *lcd_y = (hw_y * SCREEN_PHYS_HEIGHT) / TOUCH_HW_MAX_Y;
}

void input_update_touch(TouchEvent *state, int raw_x, int raw_y, bool is_down, ScreenOrientation orient) {
    state->raw_x = raw_x;
    state->raw_y = raw_y;
    state->is_down = is_down;
    state->just_down = (is_down && !last_down);
    state->just_up   = (!is_down && last_down);
    last_down = is_down;

    if (orient == ORIENTATION_LANDSCAPE) {
        /* Landskap (800x480) -> 100x30 tegn */
        state->grid_col = raw_x / FONT_W;
        state->grid_row = raw_y / FONT_H;
        if (state->grid_col < 0) state->grid_col = 0;
        if (state->grid_col >= GRID_LANDSCAPE_COLS) state->grid_col = GRID_LANDSCAPE_COLS - 1;
        if (state->grid_row < 0) state->grid_row = 0;
        if (state->grid_row >= GRID_LANDSCAPE_ROWS) state->grid_row = GRID_LANDSCAPE_ROWS - 1;
    } else {
        /* Portrett (480x800) -> 60x50 tegn */
        /* Invers rotasjonsmatematikk fra 90 grader klokken:
           x_phys = 799 - y_virt  ==> y_virt = 799 - x_phys
           y_phys = x_virt        ==> x_virt = y_phys
        */
        int virt_x = raw_y;
        int virt_y = (SCREEN_PHYS_WIDTH - 1) - raw_x;
        if (virt_y < 0) virt_y = 0;

        state->grid_col = virt_x / FONT_W;
        state->grid_row = virt_y / FONT_H;
        if (state->grid_col < 0) state->grid_col = 0;
        if (state->grid_col >= GRID_PORTRAIT_COLS) state->grid_col = GRID_PORTRAIT_COLS - 1;
        if (state->grid_row < 0) state->grid_row = 0;
        if (state->grid_row >= GRID_PORTRAIT_ROWS) state->grid_row = GRID_PORTRAIT_ROWS - 1;
    }
}

bool input_hit_box(const TouchEvent *touch, int col, int row, int width, int height) {
    if (!touch->is_down && !touch->just_up) return false;
    return (touch->grid_col >= col && touch->grid_col < col + width &&
            touch->grid_row >= row && touch->grid_row < row + height);
}
