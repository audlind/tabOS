#ifndef POWER_H
#define POWER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    BATTERY_MODE_SMART_VOLT, /* Beregnet fra analog Li-ion OCV kurve (jevn og stabil) */
    BATTERY_MODE_AXP_CHIP    /* Råverdi direkte fra AXP209 Coulomb counter */
} BatteryMode;

typedef struct {
    int voltage_mv;          /* Analog cellespenning i mV (f.eks. 4185) */
    int current_ma;          /* Strømtrekk/lading i mA (+175 lading, -280 forbruk) */
    int temp_c_tenth;        /* Temperatur i tiendedels grader (300 = 30.0 C) */
    int raw_capacity_pct;    /* Rå AXP209 fuel gauge (0..100) */
    int smart_capacity_pct;  /* Smart Li-ion OCV-beregnet (0..100) */
    int display_pct;         /* Aktiv prosent basert på valgt modus */
    bool is_charging;        /* Lader aktiv (USB/AC) */
    bool is_usb_online;      /* USB-kabel tilkoblet */
    char status[24];         /* "Charging", "Full", "Discharging" */
} BatteryTelemetry;

/* Initialiserer strømstyring (åpner /dev/disp, aktiverer ondemand DVFS) */
void power_init(void);
void power_close(void);

/* Baklysstyring (Allwinner A13 /dev/disp ioctl 0x142 / 0x143) */
int power_get_brightness(void);                 /* Råverdi 0..255 */
int power_get_brightness_pct(void);             /* Prosent 0..100 */
void power_set_brightness_pct(int pct);         /* Setter 0..100% */
void power_set_brightness(int raw_val);         /* Setter råverdi 0..255 */

/* Strømsparing & Auto-dvale */
uint32_t power_get_sleep_timeout_sec(void);     /* 0 (av), 15, 30, 60, 120 */
void power_set_sleep_timeout_sec(uint32_t sec);
uint32_t power_get_seconds_until_sleep(void);
bool power_notify_activity(void);               /* Kalles ved berøring; returnerer true hvis vekket fra dvale */
bool power_is_dimmed(void);                     /* Er skjermen i dvale/dimmet? */
bool power_update(uint32_t delta_ms);           /* Kalles fra main loop; returnerer true hvis tilstand endret */

/* CPU Governor & Frekvens */
int power_get_cpu_freq_mhz(void);
void power_get_cpu_governor(char *buf, size_t len);
void power_set_cpu_governor(const char *gov);

/* Batteri & PMIC Telemetri */
void power_get_telemetry(BatteryTelemetry *telem);
BatteryMode power_get_battery_mode(void);
void power_set_battery_mode(BatteryMode mode);
int power_get_display_pct(void);
bool power_is_charging(void);
void power_get_battery_str(char *buf, size_t len);

#endif /* POWER_H */
