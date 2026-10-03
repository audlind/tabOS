#ifndef DISPLAY_H
#define DISPLAY_H

#include "ansi.h"

/* Initialiserer skjerm- og tekstmotoren med en framebuffer */
void display_init(uint32_t *framebuffer, ScreenOrientation orientation);

/* Endrer orientering mellom Landskap (100x30) og Portrett (60x50) */
void display_set_orientation(ScreenOrientation orientation);
ScreenOrientation display_get_orientation(void);

/* Henter nåværende antall kolonner og rader */
int display_get_cols(void);
int display_get_rows(void);

/* Tømmer hele skjermen til en bestemt bakgrunnsfarge */
void display_clear(uint8_t bg);

/* Plasserer en enkelt celle i rutenettet */
void display_put_cell(int col, int row, uint8_t glyph, uint8_t fg, uint8_t bg);

/* Tegner en streng i en gitt farge */
void display_draw_string(int col, int row, const char *str, uint8_t fg, uint8_t bg);

/* Tegner en boks med CP437-rammer og eventuell tittel */
void display_draw_box(int col, int row, int w, int h, const char *title, uint8_t fg, uint8_t bg, uint8_t border_fg);

/* Tegner en touch-knapp */
void display_draw_button(int col, int row, int w, const char *label, uint8_t fg, uint8_t bg, bool pressed);

/* Rendrer tekst-rutenettet over til den fysiske 32-bits framebufferen (inkl. 90-graders rotasjon for portrett) */
void display_render_frame(void);

#endif /* DISPLAY_H */
