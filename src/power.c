#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "power.h"

/* Allwinner display driver ioctls for sun5i */
#define DISP_CMD_LCD_ON 0x140
#define DISP_CMD_LCD_SET_BRIGHTNESS 0x142
#define DISP_CMD_LCD_GET_BRIGHTNESS 0x143

static int disp_fd = -1;
static int current_brightness = 190;  /* Standard ca 75% */
static int saved_brightness = 190;
static bool is_dimmed = false;

/* Dvale-konfigurasjon */
static uint32_t sleep_timeout_sec = 0; /* Standard AV: Skjermen forblir trygt paa til brukeren eventuelt aktiverer dvale */
static uint32_t inactivity_accum_ms = 0;

/* Batteri-konfigurasjon */
static BatteryMode battery_mode = BATTERY_MODE_SMART_VOLT;
static int filtered_voltage_mv = 0;

void power_init(void) {
    if (disp_fd < 0) {
        disp_fd = open("/dev/disp", O_RDWR);
    }

    if (disp_fd >= 0) {
        /* Forsikre at LCD-panelet er slatt paa */
        unsigned long on_args[4] = {0, 0, 0, 0};
        ioctl(disp_fd, DISP_CMD_LCD_ON, on_args);

        /* Sett en lys og klar standardstyrke */
        current_brightness = 190;
        saved_brightness = 190;
        power_set_brightness(190);
    }

    /* Optimaliser Allwinner A13 CPU: Bytt fra strømslukende 'performance' til 'ondemand' */
    power_set_cpu_governor("ondemand");
}

void power_close(void) {
    if (disp_fd >= 0) {
        close(disp_fd);
        disp_fd = -1;
    }
}

int power_get_brightness(void) {
    if (disp_fd >= 0) {
        unsigned long args[4] = {0, 0, 0, 0};
        int cur = ioctl(disp_fd, DISP_CMD_LCD_GET_BRIGHTNESS, args);
        if (cur >= 50 && cur <= 255) {
            current_brightness = cur;
        }
    }
    return current_brightness;
}

int power_get_brightness_pct(void) {
    int raw = power_get_brightness();
    int pct = (raw * 100 + 127) / 255;
    if (pct < 10) pct = 10;
    if (pct > 100) pct = 100;
    return pct;
}

void power_set_brightness(int raw_val) {
    /* Sikre mot at panelet blir beksvart: minimum 60 / 255 */
    if (raw_val < 60) raw_val = 60;
    if (raw_val > 255) raw_val = 255;
    current_brightness = raw_val;

    if (disp_fd >= 0) {
        unsigned long on_args[4] = {0, 0, 0, 0};
        ioctl(disp_fd, DISP_CMD_LCD_ON, on_args);

        unsigned long args[4] = {0, (unsigned long)raw_val, 0, 0};
        ioctl(disp_fd, DISP_CMD_LCD_SET_BRIGHTNESS, args);
    }
}

void power_set_brightness_pct(int pct) {
    if (pct < 15) pct = 15;
    if (pct > 100) pct = 100;
    int raw = (pct * 255) / 100;
    power_set_brightness(raw);
    saved_brightness = raw;
}

uint32_t power_get_sleep_timeout_sec(void) {
    return sleep_timeout_sec;
}

void power_set_sleep_timeout_sec(uint32_t sec) {
    sleep_timeout_sec = sec;
    inactivity_accum_ms = 0;
}

uint32_t power_get_seconds_until_sleep(void) {
    if (sleep_timeout_sec == 0) return 0;
    uint32_t elapsed_sec = inactivity_accum_ms / 1000;
    if (elapsed_sec >= sleep_timeout_sec) return 0;
    return sleep_timeout_sec - elapsed_sec;
}

bool power_is_dimmed(void) {
    return is_dimmed;
}

bool power_notify_activity(void) {
    inactivity_accum_ms = 0;
    if (is_dimmed) {
        /* Våkn opp fra dvale: Gjenopprett full lysstyrke */
        is_dimmed = false;
        if (saved_brightness < 120) saved_brightness = 190;
        power_set_brightness(saved_brightness);
        return true; /* Skjerm vekket */
    }
    return false;
}

bool power_update(uint32_t delta_ms) {
    if (sleep_timeout_sec == 0) {
        return false;
    }

    inactivity_accum_ms += delta_ms;

    if (!is_dimmed && (inactivity_accum_ms >= sleep_timeout_sec * 1000)) {
        /* Gå i dvale: Dimm baklyset ned til et synlig, men strømsparende nivå (70/255) */
        if (current_brightness > 100) {
            saved_brightness = current_brightness;
        } else {
            saved_brightness = 190;
        }
        is_dimmed = true;
        if (disp_fd >= 0) {
            unsigned long args[4] = {0, 70, 0, 0};
            ioctl(disp_fd, DISP_CMD_LCD_SET_BRIGHTNESS, args);
        }
        return true;
    }

    return false;
}

/* CPU DVFS funksjoner */
int power_get_cpu_freq_mhz(void) {
    FILE *f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
    if (!f) return 0;
    int khz = 0;
    if (fscanf(f, "%d", &khz) == 1) {
        fclose(f);
        return khz / 1000;
    }
    fclose(f);
    return 0;
}

void power_get_cpu_governor(char *buf, size_t len) {
    FILE *f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "r");
    if (!f) {
        snprintf(buf, len, "ukjent");
        return;
    }
    if (fgets(buf, len, f)) {
        size_t l = strlen(buf);
        if (l > 0 && buf[l - 1] == '\n') buf[l - 1] = '\0';
    } else {
        snprintf(buf, len, "ukjent");
    }
    fclose(f);
}

void power_set_cpu_governor(const char *gov) {
    FILE *f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "w");
    if (f) {
        fprintf(f, "%s\n", gov);
        fclose(f);
    }
}

/* Smart Li-ion OCV kurve:
 * Standard 1S LiPo/Li-ion celle spenningsprofil:
 * 4.20V = 100%, 4.10V = 90%, 4.00V = 75%, 3.90V = 60%, 
 * 3.80V = 42%, 3.70V = 22%, 3.60V = 10%, 3.45V = 3%, <3.40V = 0%
 */
static int calc_smart_voltage_pct(int mv) {
    if (mv >= 4180) return 100;
    if (mv >= 4100) return 90 + (mv - 4100) * 10 / 80;
    if (mv >= 4000) return 75 + (mv - 4000) * 15 / 100;
    if (mv >= 3900) return 60 + (mv - 3900) * 15 / 100;
    if (mv >= 3800) return 42 + (mv - 3800) * 18 / 100;
    if (mv >= 3700) return 22 + (mv - 3700) * 20 / 100;
    if (mv >= 3600) return 10 + (mv - 3600) * 12 / 100;
    if (mv >= 3500) return 3  + (mv - 3500) * 7  / 100;
    if (mv >= 3400) return (mv - 3400) * 3 / 100;
    return 0;
}

void power_get_telemetry(BatteryTelemetry *telem) {
    memset(telem, 0, sizeof(*telem));
    telem->voltage_mv = 4000;
    telem->raw_capacity_pct = 50;
    strncpy(telem->status, "Discharging", sizeof(telem->status));

    /* 1. Spenning (uV -> mV) */
    FILE *f_v = fopen("/sys/class/power_supply/battery/voltage_now", "r");
    if (f_v) {
        int uv = 0;
        if (fscanf(f_v, "%d", &uv) == 1) {
            telem->voltage_mv = uv / 1000;
        }
        fclose(f_v);
    }

    /* Eksponensielt glidende gjennomsnitt for å hindre spenningsstøy ved CPU-spikes */
    if (filtered_voltage_mv == 0) {
        filtered_voltage_mv = telem->voltage_mv;
    } else {
        filtered_voltage_mv = (filtered_voltage_mv * 7 + telem->voltage_mv) / 8;
    }

    /* 2. Strømtrekk (uA -> mA) */
    FILE *f_c = fopen("/sys/class/power_supply/battery/current_now", "r");
    if (f_c) {
        int ua = 0;
        if (fscanf(f_c, "%d", &ua) == 1) {
            telem->current_ma = ua / 1000;
        }
        fclose(f_c);
    }

    /* 3. Temperatur (deci-Celsius -> tiendedeler) */
    FILE *f_t = fopen("/sys/class/power_supply/battery/temp", "r");
    if (f_t) {
        int t = 0;
        if (fscanf(f_t, "%d", &t) == 1) {
            telem->temp_c_tenth = t;
        }
        fclose(f_t);
    }

    /* 4. Rå AXP209 kapasitet */
    FILE *f_cap = fopen("/sys/class/power_supply/battery/capacity", "r");
    if (f_cap) {
        int cap = 0;
        if (fscanf(f_cap, "%d", &cap) == 1) {
            telem->raw_capacity_pct = cap;
        }
        fclose(f_cap);
    }

    /* 5. Ladestatus */
    FILE *f_st = fopen("/sys/class/power_supply/battery/status", "r");
    if (f_st) {
        if (fgets(telem->status, sizeof(telem->status), f_st)) {
            size_t l = strlen(telem->status);
            if (l > 0 && telem->status[l - 1] == '\n') telem->status[l - 1] = '\0';
        }
        fclose(f_st);
    }

    /* 6. USB online */
    FILE *f_usb = fopen("/sys/class/power_supply/usb/online", "r");
    if (f_usb) {
        int online = 0;
        if (fscanf(f_usb, "%d", &online) == 1) {
            telem->is_usb_online = (online == 1);
        }
        fclose(f_usb);
    }

    if (strcmp(telem->status, "Charging") == 0 || 
        strcmp(telem->status, "Full") == 0 || 
        telem->is_usb_online) {
        telem->is_charging = true;
    }

    /* 7. Beregn Smart OCV kapasitet */
    telem->smart_capacity_pct = calc_smart_voltage_pct(filtered_voltage_mv);

    /* Velg aktiv visningsprosent */
    if (battery_mode == BATTERY_MODE_SMART_VOLT) {
        telem->display_pct = telem->smart_capacity_pct;
    } else {
        telem->display_pct = telem->raw_capacity_pct;
    }
}

BatteryMode power_get_battery_mode(void) {
    return battery_mode;
}

void power_set_battery_mode(BatteryMode mode) {
    battery_mode = mode;
}

int power_get_display_pct(void) {
    BatteryTelemetry t;
    power_get_telemetry(&t);
    return t.display_pct;
}

bool power_is_charging(void) {
    BatteryTelemetry t;
    power_get_telemetry(&t);
    return t.is_charging;
}

void power_get_battery_str(char *buf, size_t len) {
    BatteryTelemetry t;
    power_get_telemetry(&t);
    snprintf(buf, len, "BAT: %d%% [%s]", t.display_pct, t.is_charging ? "LADER" : "BAT");
}
