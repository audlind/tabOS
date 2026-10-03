#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdbool.h>
#include "input.h"

/* Standardisert grensesnitt for alle apper i tabOS */
typedef struct App {
    const char *name;
    const char *title;
    
    /* Livssyklus */
    void (*on_start)(void);
    bool (*on_update)(uint32_t delta_ms);
    void (*on_render)(void);
    void (*on_touch)(const TouchEvent *touch);
    void (*on_resize)(int cols, int rows);
    void (*on_exit)(void);
} App;

#endif /* APP_H */
