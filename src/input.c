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

    int virt_x = 0;
    int virt_y = 0;
    int max_cols = GRID_LANDSCAPE_COLS;
    int max_rows = GRID_LANDSCAPE_ROWS;

    switch (orient) {
        case ORIENTATION_LANDSCAPE:
            /* 0 grader */
            virt_x = raw_x;
            virt_y = raw_y;
            max_cols = GRID_LANDSCAPE_COLS;
            max_rows = GRID_LANDSCAPE_ROWS;
            break;

        case ORIENTATION_LANDSCAPE_INVERTED:
            /* 180 grader opp-ned: x_virt = 799 - x_phys, y_virt = 479 - y_phys */
            virt_x = (SCREEN_PHYS_WIDTH - 1) - raw_x;
            virt_y = (SCREEN_PHYS_HEIGHT - 1) - raw_y;
            max_cols = GRID_LANDSCAPE_COLS;
            max_rows = GRID_LANDSCAPE_ROWS;
            break;

        case ORIENTATION_PORTRAIT:
            /* 90 grader med klokken:
               x_phys = 799 - y_virt ==> y_virt = 799 - x_phys
               y_phys = x_virt       ==> x_virt = y_phys */
            virt_x = raw_y;
            virt_y = (SCREEN_PHYS_WIDTH - 1) - raw_x;
            max_cols = GRID_PORTRAIT_COLS;
            max_rows = GRID_PORTRAIT_ROWS;
            break;

        case ORIENTATION_PORTRAIT_INVERTED:
            /* 270 grader med klokken / 90 grader mot klokken:
               x_phys = y_virt       ==> y_virt = x_phys
               y_phys = 479 - x_virt ==> x_virt = 479 - y_phys */
            virt_x = (SCREEN_PHYS_HEIGHT - 1) - raw_y;
            virt_y = raw_x;
            max_cols = GRID_PORTRAIT_COLS;
            max_rows = GRID_PORTRAIT_ROWS;
            break;
    }

    if (virt_x < 0) virt_x = 0;
    if (virt_y < 0) virt_y = 0;

    state->grid_col = virt_x / FONT_W;
    state->grid_row = virt_y / FONT_H;
    if (state->grid_col < 0) state->grid_col = 0;
    if (state->grid_col >= max_cols) state->grid_col = max_cols - 1;
    if (state->grid_row < 0) state->grid_row = 0;
    if (state->grid_row >= max_rows) state->grid_row = max_rows - 1;
}

bool input_hit_box(const TouchEvent *touch, int col, int row, int width, int height) {
    if (!touch->is_down && !touch->just_up) return false;
    return (touch->grid_col >= col && touch->grid_col < col + width &&
            touch->grid_row >= row && touch->grid_row < row + height);
}
