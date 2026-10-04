#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <linux/input.h>

#include "os.h"
#include "audio.h"
#include "sensor.h"
#include "power.h"

static volatile bool keep_running = true;

static void sig_handler(int sig) {
    (void)sig;
    keep_running = false;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("============================================================\n");
    printf("   tabOS Native Bare-Metal / Linux Engine (ARMv7 sun5i)     \n");
    printf("============================================================\n");

    /* 1. Åpne og mmap Allwinner A13 framebuffer (/dev/graphics/fb0) */
    int fb_fd = open("/dev/graphics/fb0", O_RDWR);
    if (fb_fd < 0) {
        perror("Feil ved åpning av /dev/graphics/fb0");
        return 1;
    }

    size_t fb_size = SCREEN_PHYS_WIDTH * SCREEN_PHYS_HEIGHT * 4; /* 1 536 000 bytes */
    uint32_t *fb = (uint32_t *)mmap(NULL, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb == MAP_FAILED) {
        perror("Feil ved mmap av fb0");
        close(fb_fd);
        return 1;
    }
    printf("[*] LCD Framebuffer mmap OK (%dx%d 32-bit ARGB)\n", SCREEN_PHYS_WIDTH, SCREEN_PHYS_HEIGHT);

    /* 2. Åpne Zet6221 touch-kontroller, Power-tast og fysiske knapper (/dev/input/event0) */
    int touch_fd = open("/dev/input/event2", O_RDONLY | O_NONBLOCK);
    if (touch_fd < 0) {
        perror("Feil ved åpning av /dev/input/event2");
        munmap(fb, fb_size);
        close(fb_fd);
        return 1;
    }
    printf("[*] Zet6221 I2C Touch-driver koblet til (/dev/input/event2)\n");
    int pwr_fd = open("/dev/input/event1", O_RDONLY | O_NONBLOCK);
    int kbd_fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
    if (kbd_fd >= 0) {
        printf("[*] Fysiske taster koblet til (/dev/input/event0 - Volum+/Volum-)\n");
    }
    int sensor_fd = sensor_init();

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    /* Hold Allwinner A13 CPU våken ved frakobling av USB */
    int wl = open("/sys/power/wake_lock", O_WRONLY);
    if (wl >= 0) {
        write(wl, "tabos", 5);
        close(wl);
    }

    /* 3. Initialiser tabOS kjernen, strømstyring og registrer alle apper */
    os_init(fb, ORIENTATION_LANDSCAPE);
    audio_init();
    power_init();
    os_register_app(&app_launcher);
    os_register_app(&app_keyboard);
    os_register_app(&app_touchtest);
    os_register_app(&app_colortest);
    os_register_app(&app_snake);
    os_register_app(&app_bbs);
    os_register_app(&app_settings);
    os_register_app(&app_synth);
    os_switch_app(&app_launcher);

    /* Første skjermoppdatering */
    App *active = os_get_active_app();
    if (active && active->on_render) {
        active->on_render();
    }
    display_render_frame();
    printf("[*] tabOS er na aktivt og tegnet direkte til LCD-maskinvaren!\n");
    printf("[*] 0.1ms sanntids respons - trykk med fingeren pa nettbrettet!\n");

    /* 4. Touch tilstand */
    int raw_x = 480;
    int raw_y = 320;
    bool is_down = false;
    TouchEvent touch_state;
    memset(&touch_state, 0, sizeof(touch_state));

    struct input_event ev[32];
    bool dirty = false;
    struct timeval last_time;
    gettimeofday(&last_time, NULL);

    /* 5. Sanntids hendelsesløkke (0 ms forsinkelse, direkte interrupt-drevet) */
    while (keep_running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(touch_fd, &read_fds);
        int max_fd = touch_fd;
        if (pwr_fd >= 0) {
            FD_SET(pwr_fd, &read_fds);
            if (pwr_fd > max_fd) max_fd = pwr_fd;
        }
        if (kbd_fd >= 0) {
            FD_SET(kbd_fd, &read_fds);
            if (kbd_fd > max_fd) max_fd = kbd_fd;
        }
        if (sensor_fd >= 0) {
            FD_SET(sensor_fd, &read_fds);
            if (sensor_fd > max_fd) max_fd = sensor_fd;
        }

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = power_is_dimmed() ? 80000 : 15000; /* 80ms ved dvale (12.5Hz), 15ms ved aktiv bruk (66Hz) */

        int ret = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        if (ret > 0) {
            /* Fysisk Power-knapp på brettet */
            if (pwr_fd >= 0 && FD_ISSET(pwr_fd, &read_fds)) {
                struct input_event pev[8];
                ssize_t pb = read(pwr_fd, pev, sizeof(pev));
                if (pb > 0) {
                    power_notify_activity();
                    power_set_brightness(190);
                    dirty = true;
                }
            }
            /* Fysiske knapper på siden (Volum opp / Volum ned) */
            if (kbd_fd >= 0 && FD_ISSET(kbd_fd, &read_fds)) {
                struct input_event kev[8];
                ssize_t kb = read(kbd_fd, kev, sizeof(kev));
                if (kb > 0) {
                    int kcount = (int)(kb / sizeof(struct input_event));
                    for (int i = 0; i < kcount; i++) {
                        if (kev[i].type == EV_KEY && (kev[i].value == 1 || kev[i].value == 2)) {
                            power_notify_activity();
                            if (kev[i].code == 115) { /* KEY_VOLUMEUP */
                                audio_volume_up();
                                dirty = true;
                            } else if (kev[i].code == 114) { /* KEY_VOLUMEDOWN */
                                audio_volume_down();
                                dirty = true;
                            }
                        }
                    }
                }
            }
            /* Prosesser akselerometer-hendelser */
            if (sensor_fd >= 0 && FD_ISSET(sensor_fd, &read_fds)) {
                sensor_process_events(sensor_fd);
            }

            /* Prosesser touch-hendelser */
            if (FD_ISSET(touch_fd, &read_fds)) {
                ssize_t bytes = read(touch_fd, ev, sizeof(ev));
                if (bytes > 0) {
                    /* Berøring registrert: Nullstill dvaletimer og vekk skjerm hvis dimmet */
                    if (power_notify_activity()) {
                        dirty = true;
                    }

                    int count = (int)(bytes / sizeof(struct input_event));
                    for (int i = 0; i < count; i++) {
                        if (ev[i].type == EV_ABS) {
                            if (ev[i].code == 0x30) {        /* ABS_MT_TOUCH_MAJOR (finger down/up) */
                                is_down = (ev[i].value > 0);
                            } else if (ev[i].code == 0x35) { /* ABS_MT_POSITION_X (0..960) */
                                raw_x = ev[i].value;
                            } else if (ev[i].code == 0x36) { /* ABS_MT_POSITION_Y (0..640) */
                                raw_y = ev[i].value;
                            }
                        } else if (ev[i].type == EV_KEY && ev[i].code == 0x14a) {
                            is_down = (ev[i].value == 1);
                        } else if (ev[i].type == EV_SYN && ev[i].code == SYN_REPORT) {
                            /* Oversett Zet6221 råkoordinater til LCD piksler og oppdater aktiv app */
                            int lcd_x, lcd_y;
                            input_from_hw_digitizer(raw_x, raw_y, &lcd_x, &lcd_y);
                            input_update_touch(&touch_state, lcd_x, lcd_y, is_down, display_get_orientation());

                            active = os_get_active_app();
                            if (active && active->on_touch) {
                                active->on_touch(&touch_state);
                            }
                            dirty = true;
                        }
                    }
                }
            }
        }

        /* Sanntids tikk for aktive apper (spillfart, animasjoner, oppetid) */
        struct timeval now_time;
        gettimeofday(&now_time, NULL);
        long elapsed_ms = (now_time.tv_sec - last_time.tv_sec) * 1000 + 
                          (now_time.tv_usec - last_time.tv_usec) / 1000;
        if (elapsed_ms >= 10) {
            last_time = now_time;

            /* Oppdater strømstyring & auto-dvaletimer */
            if (power_update((uint32_t)elapsed_ms)) {
                dirty = true;
            }

            /* Oppdater volum-HUD timer */
            if (audio_vol_hud_update((uint32_t)elapsed_ms)) {
                dirty = true;
            }

            /* Sjekk akselerometer for automatisk tilting / rotasjon */
            ScreenOrientation cur_orient = display_get_orientation();
            ScreenOrientation new_orient = cur_orient;
            if (sensor_check_tilt(cur_orient, &new_orient, (uint32_t)elapsed_ms)) {
                const char *or_name = "LANDSKAP (0 deg)";
                switch (new_orient) {
                    case ORIENTATION_LANDSCAPE:          or_name = "LANDSKAP (0 deg)"; break;
                    case ORIENTATION_PORTRAIT:           or_name = "PORTRETT HOYRE (90 deg)"; break;
                    case ORIENTATION_LANDSCAPE_INVERTED: or_name = "LANDSKAP OPP-NED (180 deg)"; break;
                    case ORIENTATION_PORTRAIT_INVERTED:  or_name = "PORTRETT VENSTRE (270 deg)"; break;
                }
                printf("[*] tabOS Sensor: Tilt oppdaget! Bytter orientering til %s\n", or_name);
                os_set_orientation(new_orient);
                dirty = true;
            }

            active = os_get_active_app();
            if (active && active->on_update) {
                if (active->on_update((uint32_t)elapsed_ms)) {
                    dirty = true;
                }
            }
        }

        /* Umiddelbar rendring direkte til LCD ved minste endring */
        if (dirty) {
            active = os_get_active_app();
            if (active && active->on_render) {
                active->on_render();
            }
            if (audio_vol_hud_active()) {
                audio_render_vol_hud();
            }
            display_render_frame();
            dirty = false;
        }
    }

    printf("\n[*] Avslutter tabOS Native...\n");
    power_close();
    if (sensor_fd >= 0) close(sensor_fd);
    if (kbd_fd >= 0) close(kbd_fd);
    if (pwr_fd >= 0) close(pwr_fd);
    munmap(fb, fb_size);
    close(fb_fd);
    close(touch_fd);
    return 0;
}
