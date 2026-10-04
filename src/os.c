#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "os.h"

static App *registered_apps[MAX_APPS];
static int app_count = 0;
static App *active_app = NULL;
static TouchEvent current_touch;

void os_init(uint32_t *framebuffer, ScreenOrientation orientation) {
    display_init(framebuffer, orientation);
    input_init();
    app_count = 0;
    active_app = NULL;

    /* Registrer standard hovedmeny */
    os_register_app(&app_launcher);
    os_switch_app(&app_launcher);
}

void os_register_app(App *app) {
    if (app && app_count < MAX_APPS) {
        registered_apps[app_count++] = app;
    }
}

void os_switch_app(App *app) {
    if (!app) return;
    if (active_app && active_app->on_exit) {
        active_app->on_exit();
    }
    active_app = app;
    if (active_app->on_start) {
        active_app->on_start();
    }
    if (active_app->on_resize) {
        active_app->on_resize(display_get_cols(), display_get_rows());
    }
}

App* os_get_active_app(void) {
    return active_app;
}

void os_set_orientation(ScreenOrientation new_orient) {
    if (display_get_orientation() == new_orient) return;
    display_set_orientation(new_orient);
    if (active_app && active_app->on_resize) {
        active_app->on_resize(display_get_cols(), display_get_rows());
    }
}

void os_toggle_orientation(void) {
    ScreenOrientation cur = display_get_orientation();
    ScreenOrientation new_orient = orientation_is_portrait(cur) 
                                   ? ORIENTATION_LANDSCAPE 
                                   : ORIENTATION_PORTRAIT;
    os_set_orientation(new_orient);
}

void os_step(int touch_raw_x, int touch_raw_y, bool is_touch_down, uint32_t delta_ms) {
    ScreenOrientation orient = display_get_orientation();
    input_update_touch(&current_touch, touch_raw_x, touch_raw_y, is_touch_down, orient);

    if (active_app) {
        if (active_app->on_touch) {
            active_app->on_touch(&current_touch);
        }
        if (active_app->on_update) {
            active_app->on_update(delta_ms);
        }
        if (active_app->on_render) {
            active_app->on_render();
        }
    }

    display_render_frame();
}

int os_get_battery_level(void) {
    int cap = 100;
    FILE *f = fopen("/sys/class/power_supply/battery/capacity", "r");
    if (f) {
        if (fscanf(f, "%d", &cap) != 1) {
            cap = 100;
        }
        fclose(f);
    }
    return cap;
}

bool os_is_charging(void) {
    /* 1. Sjekk USB online */
    FILE *f_usb = fopen("/sys/class/power_supply/usb/online", "r");
    if (f_usb) {
        int val = 0;
        if (fscanf(f_usb, "%d", &val) == 1 && val > 0) {
            fclose(f_usb);
            return true;
        }
        fclose(f_usb);
    }

    /* 2. Sjekk AC adapter online */
    FILE *f_ac = fopen("/sys/class/power_supply/ac/online", "r");
    if (f_ac) {
        int val = 0;
        if (fscanf(f_ac, "%d", &val) == 1 && val > 0) {
            fclose(f_ac);
            return true;
        }
        fclose(f_ac);
    }

    /* 3. Sjekk batteristatus (Charging / Full) */
    FILE *f_stat = fopen("/sys/class/power_supply/battery/status", "r");
    if (f_stat) {
        char status[32] = {0};
        if (fscanf(f_stat, "%31s", status) == 1) {
            if (strstr(status, "Charg") || strstr(status, "Full")) {
                fclose(f_stat);
                return true;
            }
        }
        fclose(f_stat);
    }

    return false;
}

void os_get_battery_str(char *buf, size_t buf_size) {
    if (!buf || buf_size == 0) return;
    int lvl = os_get_battery_level();
    bool chg = os_is_charging();
    snprintf(buf, buf_size, "BAT: %d%% [%s]", lvl, chg ? "LADER" : "BAT");
}
