#ifndef ANSI_H
#define ANSI_H

#include <stdint.h>
#include <stdbool.h>

/* Skjermdimensjoner for Allwinner A13 7" LCD */
#define SCREEN_PHYS_WIDTH   800
#define SCREEN_PHYS_HEIGHT  480

/* Tegnrutenett-dimensjoner */
#define FONT_W 8
#define FONT_H 16

/* Landskapsmodus: 800x480 -> 100 x 30 celler */
#define GRID_LANDSCAPE_COLS 100
#define GRID_LANDSCAPE_ROWS 30

/* Portrettmodus: 480x800 -> 60 x 50 celler */
#define GRID_PORTRAIT_COLS  60
#define GRID_PORTRAIT_ROWS  50

#define MAX_GRID_COLS 100
#define MAX_GRID_ROWS 50

typedef enum {
    ORIENTATION_LANDSCAPE = 0,
    ORIENTATION_PORTRAIT  = 1
} ScreenOrientation;

/* De 16 klassiske ANSI-fargene */
typedef enum {
    ANSI_BLACK        = 0,
    ANSI_RED          = 1,
    ANSI_GREEN        = 2,
    ANSI_BROWN        = 3,  /* Mørk gul / oransje */
    ANSI_BLUE         = 4,
    ANSI_MAGENTA      = 5,
    ANSI_CYAN         = 6,
    ANSI_LIGHT_GRAY   = 7,
    ANSI_DARK_GRAY    = 8,
    ANSI_LIGHT_RED    = 9,
    ANSI_LIGHT_GREEN  = 10,
    ANSI_YELLOW       = 11,
    ANSI_LIGHT_BLUE   = 12,
    ANSI_LIGHT_MAGENTA= 13,
    ANSI_LIGHT_CYAN   = 14,
    ANSI_WHITE        = 15
} AnsiColor;

/* Fargekonverteringstabell: 16 ANSI-indekser -> 32-bit ARGB8888 */
static const uint32_t ANSI_PALETTE_ARGB[16] = {
    0xFF000000, /* 0: Svart */
    0xFFAA0000, /* 1: Rød */
    0xFF00AA00, /* 2: Grønn */
    0xFFAA5500, /* 3: Brun / Mørk gul */
    0xFF0000AA, /* 4: Blå */
    0xFFAA00AA, /* 5: Magenta */
    0xFF00AAAA, /* 6: Cyan */
    0xFFAAAAAA, /* 7: Lys grå */
    0xFF555555, /* 8: Mørk grå */
    0xFFFF5555, /* 9: Lys rød */
    0xFF55FF55, /* 10: Lys grønn */
    0xFFFFFF55, /* 11: Gul */
    0xFF5555FF, /* 12: Lys blå */
    0xFFFF55FF, /* 13: Lys magenta */
    0xFF55FFFF, /* 14: Lys cyan */
    0xFFFFFFFF  /* 15: Hvit */
};

/* CP437 Boks- og grafikktegn (koder i fonten) */
#define CP437_HLINE         0xC4 /* ─ */
#define CP437_VLINE         0xB3 /* │ */
#define CP437_CORNER_TL     0xDA /* ┌ */
#define CP437_CORNER_TR     0xBF /* ┐ */
#define CP437_CORNER_BL     0xC0 /* └ */
#define CP437_CORNER_BR     0xD9 /* ┘ */
#define CP437_T_DOWN        0xC2 /* ┬ */
#define CP437_T_UP          0xC1 /* ┴ */
#define CP437_T_RIGHT       0xC3 /* ├ */
#define CP437_T_LEFT        0xB4 /* ┤ */
#define CP437_CROSS         0xC5 /* ┼ */

/* Doble linjer */
#define CP437_DHLINE        0xCD /* ═ */
#define CP437_DVLINE        0xBA /* ║ */
#define CP437_DCORNER_TL    0xC9 /* ╔ */
#define CP437_DCORNER_TR    0xBB /* ╗ */
#define CP437_DCORNER_BL    0xC8 /* ╚ */
#define CP437_DCORNER_BR    0xBC /* ╝ */

/* Blokk-grafikk */
#define CP437_BLOCK_FULL    0xDB /* █ */
#define CP437_BLOCK_LOWER   0xDC /* ▄ */
#define CP437_BLOCK_UPPER   0xDF /* ▀ */
#define CP437_BLOCK_LIGHT   0xB0 /* ░ */
#define CP437_BLOCK_MED     0xB1 /* ▒ */
#define CP437_BLOCK_DARK    0xB2 /* ▓ */

/* En tegn-celle i tabOS-rutenettet */
typedef struct {
    uint8_t glyph;    /* CP437 tegn (0-255) */
    uint8_t fg;       /* Forgrunnsfarge (0-15) */
    uint8_t bg;       /* Bakgrunnsfarge (0-15) */
    uint8_t attr;     /* Flagg: bit 0 = Invert, bit 1 = Underline, bit 2 = Bold */
} AnsiCell;

#endif /* ANSI_H */
