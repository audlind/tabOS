#ifndef SENSOR_H
#define SENSOR_H

#include <stdbool.h>
#include <stdint.h>
#include "ansi.h"

/* Initialiserer akselerometer-sensoren (MEMSIC MXC622x på /dev/input/event3 eller auto-detect).
   Returnerer filbeskrivelsen (fd) til event-noden, eller -1 hvis ingen sensor ble funnet. */
int sensor_init(void);

/* Leser og prosesserer innkommende sensor-hendelser fra non-blocking fd */
void sensor_process_events(int fd);

/* Henter nåværende glattede tyngdekraft-verdier (X, Y, Z) */
void sensor_get_values(int *x, int *y, int *z);

/* Sjekker om nettbrettet ligger flatt på et bord */
bool sensor_is_flat(void);

/* Bestemmer om skjermen bør endre orientering basert på sanntids tilting,
   med hysterese og tidsforsinkelse (debouncing) for å unngå blafring/utilsiktede bytter.
   Returnerer true hvis orienteringen skal endres, og setter *new_orient. */
bool sensor_check_tilt(ScreenOrientation current_orient, ScreenOrientation *new_orient, uint32_t delta_ms);

/* Slår auto-rotasjon av eller på */
void sensor_set_auto_rotate(bool enabled);
bool sensor_get_auto_rotate(void);

/* Veksler auto-rotasjon av/på */
void sensor_toggle_auto_rotate(void);

/* Bytter aksekartlegging (dersom landskap/portrett er motsatt) */
void sensor_set_axis_swap(bool swap);
bool sensor_get_axis_swap(void);

#endif /* SENSOR_H */
