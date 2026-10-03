#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include <stdbool.h>
#include "ansi.h"

/* Representasjon av berøringsstatus på tabOS-skjermen */
typedef struct {
    int raw_x;           /* Rå pikselkoordinat fra digitizer (0..800) */
    int raw_y;           /* Rå pikselkoordinat fra digitizer (0..480) */
    int grid_col;        /* Beregnet kolonne i tegnrutenettet */
    int grid_row;        /* Beregnet rad i tegnrutenettet */
    bool is_down;        /* Er fingeren på skjermen nå? */
    bool just_down;      /* Ble fingeren akkurat trykket ned denne framen? */
    bool just_up;        /* Ble fingeren akkurat sluppet denne framen? */
} TouchEvent;

/* Initialiserer input-motoren */
void input_init(void);

#define TOUCH_HW_MAX_X 960
#define TOUCH_HW_MAX_Y 640

/* Skalerer rå hardware-koordinater fra Zet6221 (0..960, 0..640) til LCD-piksler (0..800, 0..480) */
void input_from_hw_digitizer(int hw_x, int hw_y, int *lcd_x, int *lcd_y);

/* Oppdaterer berøringsstatus og oversetter rå koordinater til rutenett basert på orientering */
void input_update_touch(TouchEvent *state, int raw_x, int raw_y, bool is_down, ScreenOrientation orient);

/* Hjelpefunksjon: Sjekker om et touch-treff er innenfor et definert boks-/knappeområde */
bool input_hit_box(const TouchEvent *touch, int col, int row, int width, int height);

#endif /* INPUT_H */
