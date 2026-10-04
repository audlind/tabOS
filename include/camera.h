#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CAM_MODE_MATRIX,     /* Grønn Matrix-stil med fallende kode-tegn */
    CAM_MODE_CP437,      /* Klassisk IBM CP437 gråtoner (░▒▓█) */
    CAM_MODE_GAMEBOY,    /* Game Boy Camera 4-toners dither */
    CAM_MODE_AMBER       /* Amber / oransje retro CRT-skjerm */
} CameraRenderMode;

/* Initialiserer kamera-driveren (/dev/video0) */
bool camera_init(void);

/* Stopper kamera og frigjør V4L2-buffere */
void camera_close(void);

/* Sjekker om kameraet er tilkoblet og klart */
bool camera_is_available(void);

/* Henter en ny videoramme (non-blocking eller kort timeout).
 * Returnerer peker til rå 8-bit Y-luminans (640x480) */
const uint8_t *camera_grab_y_frame(void);

/* Kameraspesifikke bildeinnstillinger */
void camera_set_mirror(bool mirror);
bool camera_get_mirror(void);
void camera_adjust_brightness(int delta);
int  camera_get_brightness(void);
void camera_adjust_contrast(int delta);
int  camera_get_contrast(void);

/* Genererer et stillbilde i ASCII og lagrer til fil */
bool camera_save_snapshot(const char *filepath, CameraRenderMode mode, int w, int h);

#endif /* CAMERA_H */
