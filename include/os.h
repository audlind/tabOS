#ifndef OS_H
#define OS_H

#include "display.h"
#include "input.h"
#include "app.h"

#define MAX_APPS 16

/* Initialiserer operativsystemet */
void os_init(uint32_t *framebuffer, ScreenOrientation orientation);

/* Registrerer en applikasjon i systemet */
void os_register_app(App *app);

/* Bytter til en gitt applikasjon */
void os_switch_app(App *app);

/* Henter nåværende aktive app */
App* os_get_active_app(void);

/* Veksler mellom Landskap (100x30) og Portrett (60x50) */
void os_toggle_orientation(void);

/* Kjører én komplett system-syklus (input, update, render, flush) */
void os_step(int touch_raw_x, int touch_raw_y, bool is_touch_down, uint32_t delta_ms);

/* Innebygde kjerneapper */
extern App app_launcher;
extern App app_colortest;
extern App app_touchtest;
extern App app_keyboard;
extern App app_snake;
extern App app_bbs;
void bbs_set_node(int node_idx);

#endif /* OS_H */
