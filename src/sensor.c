#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#include "sensor.h"

/* Rå og filtrerte akselerasjonsverdier (1G er ca. 32768) */
static int raw_x = 0;
static int raw_y = 0;
static int raw_z = 0;

static int filt_x = 0;
static int filt_y = 0;
static int filt_z = -32768; /* Standard: Ligger flatt mot bordet */

static bool auto_rotate_enabled = true;
static bool axis_swap = false;

/* Tidsstyring for debouncing */
static ScreenOrientation candidate_orient = ORIENTATION_LANDSCAPE;
static uint32_t stable_time_ms = 0;

#define HYSTERESIS_THRESHOLD 7000   /* Ca. 12 graders sikkerhetsmargin */
#define FLAT_Z_THRESHOLD     22000  /* Grense for når nettbrettet regnes som liggende flatt */
#define DEBOUNCE_TIME_MS     350    /* Må holdes stabilt i 350 ms før orientering skifter */

int sensor_init(void) {
    int fd = -1;

    /* Søk etter sensor-eventnoden dynamisk */
    for (int i = 0; i < 8; i++) {
        char devpath[32];
        snprintf(devpath, sizeof(devpath), "/dev/input/event%d", i);
        int test_fd = open(devpath, O_RDONLY | O_NONBLOCK);
        if (test_fd >= 0) {
            char name[128] = {0};
            if (ioctl(test_fd, EVIOCGNAME(sizeof(name) - 1), name) >= 0) {
                if (strstr(name, "ecompass") || strstr(name, "mxc") || strstr(name, "gsensor")) {
                    fd = test_fd;
                    printf("[*] tabOS Sensor: Funnet akselerometer pa %s (\"%s\")\n", devpath, name);
                    break;
                }
            }
            close(test_fd);
        }
    }

    /* Fallback til standard /dev/input/event3 dersom navnesøk ikke ga treff */
    if (fd < 0) {
        fd = open("/dev/input/event3", O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            printf("[*] tabOS Sensor: Apnet standard /dev/input/event3\n");
        } else {
            perror("[!] tabOS Sensor: Kunne ikke apne akselerometer");
            return -1;
        }
    }

    /* Hent umiddelbare initiell-verdier via EVIOCGABS så vi slipper å vente på bevegelse */
    struct input_absinfo abs_info;
    if (ioctl(fd, EVIOCGABS(ABS_X), &abs_info) >= 0) {
        raw_x = abs_info.value;
        filt_x = raw_x;
    }
    if (ioctl(fd, EVIOCGABS(ABS_Y), &abs_info) >= 0) {
        raw_y = abs_info.value;
        filt_y = raw_y;
    }
    if (ioctl(fd, EVIOCGABS(ABS_Z), &abs_info) >= 0) {
        raw_z = abs_info.value;
        filt_z = raw_z;
    }

    printf("[*] tabOS Sensor kalibrert: X=%d, Y=%d, Z=%d (Flat: %s)\n", 
           filt_x, filt_y, filt_z, sensor_is_flat() ? "JA" : "NEI");

    return fd;
}

void sensor_process_events(int fd) {
    if (fd < 0) return;

    struct input_event ev[32];
    ssize_t bytes = read(fd, ev, sizeof(ev));
    if (bytes <= 0) return;

    int count = (int)(bytes / sizeof(struct input_event));
    for (int i = 0; i < count; i++) {
        if (ev[i].type == EV_ABS) {
            if (ev[i].code == ABS_X) {
                raw_x = ev[i].value;
            } else if (ev[i].code == ABS_Y) {
                raw_y = ev[i].value;
            } else if (ev[i].code == ABS_Z) {
                raw_z = ev[i].value;
            }
        } else if (ev[i].type == EV_SYN && ev[i].code == SYN_REPORT) {
            /* Lavpassfilter (IIR) for å filtrere bort vibrasjoner og støy */
            filt_x = (filt_x * 3 + raw_x) / 4;
            filt_y = (filt_y * 3 + raw_y) / 4;
            filt_z = (filt_z * 3 + raw_z) / 4;
        }
    }
}

void sensor_get_values(int *x, int *y, int *z) {
    if (x) *x = filt_x;
    if (y) *y = filt_y;
    if (z) *z = filt_z;
}

bool sensor_is_flat(void) {
    int az = abs(filt_z);
    int ax = abs(filt_x);
    int ay = abs(filt_y);

    /* Hvis Z-aksen dominerer (tyngdekraften peker rett inn i eller ut av skjermen) */
    return (az > FLAT_Z_THRESHOLD && az > (ax * 3 / 2) && az > (ay * 3 / 2));
}

bool sensor_check_tilt(ScreenOrientation current_orient, ScreenOrientation *new_orient, uint32_t delta_ms) {
    if (!auto_rotate_enabled) {
        stable_time_ms = 0;
        return false;
    }

    /* Hvis brettet ligger på et bord, beholder vi alltid eksisterende orientering */
    if (sensor_is_flat()) {
        stable_time_ms = 0;
        return false;
    }

    /* Aksevurdering med støtte for alle 4 retninger:
       eff_x: Landskapsakse (0 grader ved +, 180 grader ved -)
       eff_y: Portrettakse  (90 grader ved +, 270 grader ved -)
    */
    int eff_x = axis_swap ? filt_y : filt_x;
    int eff_y = axis_swap ? filt_x : filt_y;

    int mag_x = abs(eff_x);
    int mag_y = abs(eff_y);

    ScreenOrientation detected = current_orient;

    /* Sjekk om portrett- eller landskapsaksen dominerer */
    if (mag_y > mag_x + HYSTERESIS_THRESHOLD) {
        /* Portrett dominerer: Skille mellom 90 grader og 270 grader */
        detected = (eff_y > 0) ? ORIENTATION_PORTRAIT : ORIENTATION_PORTRAIT_INVERTED;
    } else if (mag_x > mag_y + HYSTERESIS_THRESHOLD) {
        /* Landskap dominerer: Skille mellom 0 grader (normal) og 180 grader (opp-ned) */
        detected = (eff_x > 0) ? ORIENTATION_LANDSCAPE : ORIENTATION_LANDSCAPE_INVERTED;
    } else if (orientation_is_portrait(current_orient)) {
        /* Allerede i portrett: bytt kun til motsatt portrett hvis motsatt side er markant */
        if (current_orient == ORIENTATION_PORTRAIT && eff_y < -HYSTERESIS_THRESHOLD) {
            detected = ORIENTATION_PORTRAIT_INVERTED;
        } else if (current_orient == ORIENTATION_PORTRAIT_INVERTED && eff_y > HYSTERESIS_THRESHOLD) {
            detected = ORIENTATION_PORTRAIT;
        }
    } else if (orientation_is_landscape(current_orient)) {
        /* Allerede i landskap: bytt kun til motsatt landskap hvis motsatt side er markant */
        if (current_orient == ORIENTATION_LANDSCAPE && eff_x < -HYSTERESIS_THRESHOLD) {
            detected = ORIENTATION_LANDSCAPE_INVERTED;
        } else if (current_orient == ORIENTATION_LANDSCAPE_INVERTED && eff_x > HYSTERESIS_THRESHOLD) {
            detected = ORIENTATION_LANDSCAPE;
        }
    }

    /* Debouncing: Må holdes stabilt i DEBOUNCE_TIME_MS før orientering byttes */
    if (detected != current_orient) {
        if (detected == candidate_orient) {
            stable_time_ms += delta_ms;
            if (stable_time_ms >= DEBOUNCE_TIME_MS) {
                stable_time_ms = 0;
                if (new_orient) *new_orient = detected;
                return true;
            }
        } else {
            candidate_orient = detected;
            stable_time_ms = 0;
        }
    } else {
        stable_time_ms = 0;
        candidate_orient = current_orient;
    }

    return false;
}

void sensor_set_auto_rotate(bool enabled) {
    auto_rotate_enabled = enabled;
    stable_time_ms = 0;
}

bool sensor_get_auto_rotate(void) {
    return auto_rotate_enabled;
}

void sensor_toggle_auto_rotate(void) {
    auto_rotate_enabled = !auto_rotate_enabled;
    stable_time_ms = 0;
}

void sensor_set_axis_swap(bool swap) {
    axis_swap = swap;
    stable_time_ms = 0;
}

bool sensor_get_axis_swap(void) {
    return axis_swap;
}
