#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "os.h"
#include "power.h"
#include "sensor.h"
#include "audio.h"

static int pressed_btn = -1;
static char status_feedback[64] = "Klar. Juster parametere nedenfor.";

static void settings_start(void) {
    pressed_btn = -1;
    snprintf(status_feedback, sizeof(status_feedback), "Innstillinger lastet fra maskinvare.");
}

static bool settings_update(uint32_t delta_ms) {
    (void)delta_ms;
    /* Kontinuerlig oppdatering for å vise live CPU-frekvens, mA, mV og nedtelling til dvale */
    return true;
}

static void settings_render_landscape(void) {
    display_clear(ANSI_BLACK);

    /* 1. Header */
    char bat_str[32];
    power_get_battery_str(bat_str, sizeof(bat_str));
    char top_buf[100];
    snprintf(top_buf, sizeof(top_buf), "tabOS HARDWARE KONTROLLPANEL & INNSTILLINGER       %s", bat_str);
    display_draw_box(0, 0, 100, 3, "", ANSI_WHITE, ANSI_MAGENTA, ANSI_YELLOW);
    display_draw_string(2, 1, top_buf, ANSI_WHITE, ANSI_MAGENTA);
    display_draw_button(82, 1, 16, " [< TILBAKE] ", ANSI_WHITE, ANSI_RED, (pressed_btn == 99));

    /* Hent live telemetri */
    BatteryTelemetry b;
    power_get_telemetry(&b);
    int cpu_mhz = power_get_cpu_freq_mhz();
    char gov[32];
    power_get_cpu_governor(gov, sizeof(gov));
    int b_pct = power_get_brightness_pct();
    int b_raw = power_get_brightness();
    uint32_t sleep_sec = power_get_sleep_timeout_sec();
    uint32_t rem_sec = power_get_seconds_until_sleep();

    /* BOKS 1: SKJERM & LYSSTYRKE (Kolonne 2..48, Rad 4..13) */
    display_draw_box(2, 3, 47, 10, "SKJERM & BAKLYS (PWM IOCTL 0x142)", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_CYAN);
    char l_buf[48];
    snprintf(l_buf, sizeof(l_buf), "Nivaa: %3d%% (PWM: %3d/255)", b_pct, b_raw);
    display_draw_string(4, 5, l_buf, ANSI_YELLOW, ANSI_BLACK);

    display_draw_button(4,  7, 7, " 20% ", ANSI_WHITE, (b_pct <= 25) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 11));
    display_draw_button(12, 7, 7, " 40% ", ANSI_WHITE, (b_pct > 25 && b_pct <= 45) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 12));
    display_draw_button(20, 7, 7, " 60% ", ANSI_WHITE, (b_pct > 45 && b_pct <= 65) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 13));
    display_draw_button(28, 7, 7, " 80% ", ANSI_WHITE, (b_pct > 65 && b_pct <= 85) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 14));
    display_draw_button(36, 7, 8, " 100% ", ANSI_WHITE, (b_pct > 85) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 15));

    display_draw_button(4,  10, 19, " [ -10% DIM ] ", ANSI_WHITE, ANSI_BLUE, (pressed_btn == 16));
    display_draw_button(25, 10, 19, " [ +10% LYS ] ", ANSI_WHITE, ANSI_BLUE, (pressed_btn == 17));

    /* BOKS 2: AUTO-DVALE & STRØMSPARING (Kolonne 51..97, Rad 4..13) */
    display_draw_box(51, 3, 47, 10, "AUTO-DVALE & STROMSPARING", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_GREEN);
    char d_buf[48];
    if (sleep_sec > 0) {
        snprintf(d_buf, sizeof(d_buf), "Tid: %us (Dimmer om %us)", sleep_sec, rem_sec);
    } else {
        snprintf(d_buf, sizeof(d_buf), "Tid: ALLTID PAA (Dvale deaktivert)");
    }
    display_draw_string(53, 5, d_buf, ANSI_LIGHT_GREEN, ANSI_BLACK);

    display_draw_button(53, 7, 8, " 15s ", ANSI_WHITE, (sleep_sec == 15) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 21));
    display_draw_button(62, 7, 8, " 30s ", ANSI_WHITE, (sleep_sec == 30) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 22));
    display_draw_button(71, 7, 8, " 60s ", ANSI_WHITE, (sleep_sec == 60) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 23));
    display_draw_button(80, 7, 8, " 2m  ", ANSI_WHITE, (sleep_sec == 120) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 24));
    display_draw_button(89, 7, 7, " AV ", ANSI_WHITE, (sleep_sec == 0) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 25));

    display_draw_string(53, 10, "> Skjerm dimmes til 4% ved inaktivitet", ANSI_LIGHT_GRAY, ANSI_BLACK);
    display_draw_string(53, 11, "> Beror beroringsskjermen for a vekke!", ANSI_YELLOW, ANSI_BLACK);

    /* BOKS 3: CPU DVFS & GOVERNOR (Kolonne 2..48, Rad 13..21) */
    display_draw_box(2, 13, 47, 8, "ALLWINNER A13 CPU DVFS", ANSI_WHITE, ANSI_BLACK, ANSI_YELLOW);
    char cpu_buf[48];
    snprintf(cpu_buf, sizeof(cpu_buf), "Klokke: %4d MHz  (Min: 60 / Maks: 1008)", cpu_mhz);
    display_draw_string(4, 15, cpu_buf, ANSI_YELLOW, ANSI_BLACK);

    display_draw_button(4,  17, 10, "[ONDEMAND]", ANSI_WHITE, (strcmp(gov, "ondemand") == 0) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 31));
    display_draw_button(15, 17, 9,  "[FANTASY]",  ANSI_WHITE, (strcmp(gov, "fantasy") == 0) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 32));
    display_draw_button(25, 17, 11, "[POWERSAVE]", ANSI_WHITE, (strcmp(gov, "powersave") == 0) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 33));
    display_draw_button(37, 17, 9,  "[PERF]",     ANSI_WHITE, (strcmp(gov, "performance") == 0) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 34));

    display_draw_string(4, 19, "> ONDEMAND senker CPU til 60 MHz ved hvile!", ANSI_LIGHT_GREEN, ANSI_BLACK);

    /* BOKS 4: AXP209 PMIC & BATTERI-TELEMETRI (Kolonne 51..97, Rad 13..21) */
    display_draw_box(51, 13, 47, 8, "AXP209 PMIC & BATTERI-TELEMETRI", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_RED);
    char v_buf[48], c_buf[48], t_buf[48];
    snprintf(v_buf, sizeof(v_buf), "Spenning: %4d mV  (Kjemi: Li-ion 3.7V)", b.voltage_mv);
    snprintf(c_buf, sizeof(c_buf), "Strom   : %+4d mA  (%s)", b.current_ma, b.is_charging ? "LADER" : "FORBRUK");
    snprintf(t_buf, sizeof(t_buf), "Temp    : %d.%d C   Kapasitet: %d%%", b.temp_c_tenth / 10, abs(b.temp_c_tenth % 10), b.display_pct);
    display_draw_string(53, 15, v_buf, ANSI_WHITE, ANSI_BLACK);
    display_draw_string(53, 16, c_buf, b.is_charging ? ANSI_LIGHT_GREEN : ANSI_YELLOW, ANSI_BLACK);
    display_draw_string(53, 17, t_buf, ANSI_LIGHT_CYAN, ANSI_BLACK);

    bool is_smart = (power_get_battery_mode() == BATTERY_MODE_SMART_VOLT);
    display_draw_button(53, 18, 22, is_smart ? "[*] SMART VOLT (OCV)" : "[ ] SMART VOLT (OCV)", ANSI_WHITE, is_smart ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 41));
    display_draw_button(76, 18, 20, !is_smart ? "[*] AXP209 RAW CHIP" : "[ ] AXP209 RAW CHIP", ANSI_WHITE, !is_smart ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 42));

    /* BOKS 5: SYSTEM, SENSORER & STATUS (Bunn rad 21..25) */
    display_draw_box(2, 21, 96, 5, "HARDWARE STATUS & NETTVERK", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_GRAY);
    char net_buf[90];
    snprintf(net_buf, sizeof(net_buf), "WiFi wlan0: 192.168.1.103 (Vestli)   Touch: Zet6221 I2C   Sensor: MEMSIC MXC622X [AUTO: %s]",
             sensor_get_auto_rotate() ? "PAA" : "AV");
    display_draw_string(4, 22, net_buf, ANSI_LIGHT_CYAN, ANSI_BLACK);

    display_draw_button(4,  23, 20, " [ TEST BEEP-LYD ] ", ANSI_WHITE, ANSI_BLUE, (pressed_btn == 51));
    display_draw_button(26, 23, 22, " [ AUTO-ROTASJON ] ", ANSI_WHITE, sensor_get_auto_rotate() ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 52));
    display_draw_string(50, 23, status_feedback, ANSI_YELLOW, ANSI_BLACK);

    /* Bunnlinje */
    display_draw_box(0, 26, 100, 4, "", ANSI_WHITE, ANSI_DARK_GRAY, ANSI_LIGHT_GRAY);
    display_draw_button(4, 27, 24, " [< TILBAKE TIL HOVEDMENY] ", ANSI_WHITE, ANSI_RED, (pressed_btn == 99));
    display_draw_string(32, 27, "tabOS Hardware Control Panel - Alle endringer skrives direkte til registere.", ANSI_LIGHT_GREEN, ANSI_DARK_GRAY);
    display_draw_string(32, 28, "Batterisparing: Reduser baklys og velg 30s auto-dvale for 3x driftstid!", ANSI_YELLOW, ANSI_DARK_GRAY);
}

static void settings_render_portrait(void) {
    display_clear(ANSI_BLACK);

    /* 1. Header (60 kolonner) */
    char bat_str[32];
    power_get_battery_str(bat_str, sizeof(bat_str));
    display_draw_box(0, 0, 60, 3, "", ANSI_WHITE, ANSI_MAGENTA, ANSI_YELLOW);
    display_draw_string(2, 1, "INNSTILLINGER", ANSI_WHITE, ANSI_MAGENTA);
    display_draw_string(18, 1, bat_str, ANSI_LIGHT_CYAN, ANSI_MAGENTA);
    display_draw_button(44, 1, 14, "[< TILBAKE]", ANSI_WHITE, ANSI_RED, (pressed_btn == 99));

    BatteryTelemetry b;
    power_get_telemetry(&b);
    int cpu_mhz = power_get_cpu_freq_mhz();
    char gov[32];
    power_get_cpu_governor(gov, sizeof(gov));
    int b_pct = power_get_brightness_pct();
    int b_raw = power_get_brightness();
    uint32_t sleep_sec = power_get_sleep_timeout_sec();
    uint32_t rem_sec = power_get_seconds_until_sleep();

    /* BOKS 1: SKJERM & BAKLYS */
    display_draw_box(2, 3, 56, 8, "SKJERM & BAKLYS (0x142)", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_CYAN);
    char l_buf[48];
    snprintf(l_buf, sizeof(l_buf), "Lysstyrke: %d%% (PWM: %d/255)", b_pct, b_raw);
    display_draw_string(4, 5, l_buf, ANSI_YELLOW, ANSI_BLACK);
    display_draw_button(4,  7, 9, " 20% ", ANSI_WHITE, (b_pct <= 25) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 11));
    display_draw_button(14, 7, 9, " 40% ", ANSI_WHITE, (b_pct > 25 && b_pct <= 45) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 12));
    display_draw_button(24, 7, 9, " 60% ", ANSI_WHITE, (b_pct > 45 && b_pct <= 65) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 13));
    display_draw_button(34, 7, 9, " 80% ", ANSI_WHITE, (b_pct > 65 && b_pct <= 85) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 14));
    display_draw_button(44, 7, 10, " 100% ", ANSI_WHITE, (b_pct > 85) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 15));

    /* BOKS 2: AUTO-DVALE */
    display_draw_box(2, 11, 56, 8, "STR\x8FMSPARING & DVALE", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_GREEN);
    char d_buf[48];
    if (sleep_sec > 0) snprintf(d_buf, sizeof(d_buf), "Dvale: %us (%us til dimming)", sleep_sec, rem_sec);
    else snprintf(d_buf, sizeof(d_buf), "Dvale: AV (Alltid aktiv)");
    display_draw_string(4, 13, d_buf, ANSI_LIGHT_GREEN, ANSI_BLACK);
    display_draw_button(4,  15, 9, " 15s ", ANSI_WHITE, (sleep_sec == 15) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 21));
    display_draw_button(14, 15, 9, " 30s ", ANSI_WHITE, (sleep_sec == 30) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 22));
    display_draw_button(24, 15, 9, " 60s ", ANSI_WHITE, (sleep_sec == 60) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 23));
    display_draw_button(34, 15, 9, " 2m  ", ANSI_WHITE, (sleep_sec == 120) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 24));
    display_draw_button(44, 15, 10, " AV  ", ANSI_WHITE, (sleep_sec == 0) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 25));

    /* BOKS 3: CPU DVFS */
    display_draw_box(2, 19, 56, 8, "CPU FREKVENS (DVFS)", ANSI_WHITE, ANSI_BLACK, ANSI_YELLOW);
    char cpu_buf[48];
    snprintf(cpu_buf, sizeof(cpu_buf), "Frekvens: %4d MHz [%s]", cpu_mhz, gov);
    display_draw_string(4, 21, cpu_buf, ANSI_YELLOW, ANSI_BLACK);
    display_draw_button(4,  23, 12, "[ONDEMAND]", ANSI_WHITE, (strcmp(gov, "ondemand") == 0) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 31));
    display_draw_button(17, 23, 11, "[FANTASY]",  ANSI_WHITE, (strcmp(gov, "fantasy") == 0) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 32));
    display_draw_button(29, 23, 13, "[POWERSAVE]", ANSI_WHITE, (strcmp(gov, "powersave") == 0) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 33));
    display_draw_button(43, 23, 11, "[PERF]",     ANSI_WHITE, (strcmp(gov, "performance") == 0) ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 34));

    /* BOKS 4: AXP209 PMIC */
    display_draw_box(2, 27, 56, 10, "AXP209 PMIC & BATTERI", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_RED);
    char v_buf[48], c_buf[48];
    snprintf(v_buf, sizeof(v_buf), "Spenning: %4d mV  (Temp: %d.%d C)", b.voltage_mv, b.temp_c_tenth / 10, abs(b.temp_c_tenth % 10));
    snprintf(c_buf, sizeof(c_buf), "Strom   : %+4d mA  [%s]", b.current_ma, b.is_charging ? "LADER" : "BAT");
    display_draw_string(4, 29, v_buf, ANSI_WHITE, ANSI_BLACK);
    display_draw_string(4, 30, c_buf, b.is_charging ? ANSI_LIGHT_GREEN : ANSI_YELLOW, ANSI_BLACK);

    bool is_smart = (power_get_battery_mode() == BATTERY_MODE_SMART_VOLT);
    display_draw_button(4, 32, 24, is_smart ? "[*] SMART VOLT (OCV)" : "[ ] SMART VOLT (OCV)", ANSI_WHITE, is_smart ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 41));
    display_draw_button(30, 32, 24, !is_smart ? "[*] AXP209 RAW CHIP" : "[ ] AXP209 RAW CHIP", ANSI_WHITE, !is_smart ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 42));
    display_draw_string(4, 35, "> Smart Volt hindrer 0->88% hopp!", ANSI_LIGHT_GREEN, ANSI_BLACK);

    /* BOKS 5: KONTROLLER & STATUS */
    display_draw_box(2, 38, 56, 6, "STATUS & FEEDBACK", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_GRAY);
    display_draw_string(4, 40, status_feedback, ANSI_YELLOW, ANSI_BLACK);
    display_draw_button(4,  41, 24, " [ TEST BEEP-LYD ] ", ANSI_WHITE, ANSI_BLUE, (pressed_btn == 51));
    display_draw_button(30, 41, 24, " [ AUTO-ROTASJON ] ", ANSI_WHITE, sensor_get_auto_rotate() ? ANSI_GREEN : ANSI_DARK_GRAY, (pressed_btn == 52));

    /* Hovedmeny knapp */
    display_draw_button(4, 45, 52, " [< TILBAKE TIL HOVEDMENY] ", ANSI_WHITE, ANSI_RED, (pressed_btn == 99));
}

static void settings_render(void) {
    if (orientation_is_portrait(display_get_orientation())) {
        settings_render_portrait();
    } else {
        settings_render_landscape();
    }
}

static void settings_touch(const TouchEvent *t) {
    ScreenOrientation orient = display_get_orientation();
    bool port = orientation_is_portrait(orient);

    if (t->is_down) {
        if (port) {
            /* Portrett touch hitboxes */
            if (input_hit_box(t, 44, 1, 14, 1)) pressed_btn = 99;
            else if (input_hit_box(t, 4, 7, 9, 1))  pressed_btn = 11;
            else if (input_hit_box(t, 14, 7, 9, 1)) pressed_btn = 12;
            else if (input_hit_box(t, 24, 7, 9, 1)) pressed_btn = 13;
            else if (input_hit_box(t, 34, 7, 9, 1)) pressed_btn = 14;
            else if (input_hit_box(t, 44, 7, 10, 1)) pressed_btn = 15;
            else if (input_hit_box(t, 4, 15, 9, 1))  pressed_btn = 21;
            else if (input_hit_box(t, 14, 15, 9, 1)) pressed_btn = 22;
            else if (input_hit_box(t, 24, 15, 9, 1)) pressed_btn = 23;
            else if (input_hit_box(t, 34, 15, 9, 1)) pressed_btn = 24;
            else if (input_hit_box(t, 44, 15, 10, 1)) pressed_btn = 25;
            else if (input_hit_box(t, 4, 23, 12, 1)) pressed_btn = 31;
            else if (input_hit_box(t, 17, 23, 11, 1)) pressed_btn = 32;
            else if (input_hit_box(t, 29, 23, 13, 1)) pressed_btn = 33;
            else if (input_hit_box(t, 43, 23, 11, 1)) pressed_btn = 34;
            else if (input_hit_box(t, 4, 32, 24, 1)) pressed_btn = 41;
            else if (input_hit_box(t, 30, 32, 24, 1)) pressed_btn = 42;
            else if (input_hit_box(t, 4, 41, 24, 1)) pressed_btn = 51;
            else if (input_hit_box(t, 30, 41, 24, 1)) pressed_btn = 52;
            else if (input_hit_box(t, 4, 45, 52, 1)) pressed_btn = 99;
        } else {
            /* Landskap touch hitboxes */
            if (input_hit_box(t, 82, 1, 16, 1)) pressed_btn = 99;
            else if (input_hit_box(t, 4, 7, 7, 1))  pressed_btn = 11;
            else if (input_hit_box(t, 12, 7, 7, 1)) pressed_btn = 12;
            else if (input_hit_box(t, 20, 7, 7, 1)) pressed_btn = 13;
            else if (input_hit_box(t, 28, 7, 7, 1)) pressed_btn = 14;
            else if (input_hit_box(t, 36, 7, 8, 1)) pressed_btn = 15;
            else if (input_hit_box(t, 4, 10, 19, 1)) pressed_btn = 16;
            else if (input_hit_box(t, 25, 10, 19, 1)) pressed_btn = 17;
            else if (input_hit_box(t, 53, 7, 8, 1)) pressed_btn = 21;
            else if (input_hit_box(t, 62, 7, 8, 1)) pressed_btn = 22;
            else if (input_hit_box(t, 71, 7, 8, 1)) pressed_btn = 23;
            else if (input_hit_box(t, 80, 7, 8, 1)) pressed_btn = 24;
            else if (input_hit_box(t, 89, 7, 7, 1)) pressed_btn = 25;
            else if (input_hit_box(t, 4, 17, 10, 1)) pressed_btn = 31;
            else if (input_hit_box(t, 15, 17, 9, 1)) pressed_btn = 32;
            else if (input_hit_box(t, 25, 17, 11, 1)) pressed_btn = 33;
            else if (input_hit_box(t, 37, 17, 9, 1)) pressed_btn = 34;
            else if (input_hit_box(t, 53, 18, 22, 1)) pressed_btn = 41;
            else if (input_hit_box(t, 76, 18, 20, 1)) pressed_btn = 42;
            else if (input_hit_box(t, 4, 23, 20, 1)) pressed_btn = 51;
            else if (input_hit_box(t, 26, 23, 22, 1)) pressed_btn = 52;
            else if (input_hit_box(t, 4, 27, 24, 1)) pressed_btn = 99;
        }
    } else if (t->just_up) {
        int act = pressed_btn;
        pressed_btn = -1;

        switch (act) {
            case 99:
                os_switch_app(&app_launcher);
                break;
            case 11:
                power_set_brightness_pct(20);
                snprintf(status_feedback, sizeof(status_feedback), "Lysstyrke satt til 20%% (Lavt stromforbruk).");
                break;
            case 12:
                power_set_brightness_pct(40);
                snprintf(status_feedback, sizeof(status_feedback), "Lysstyrke satt til 40%% (Innendors).");
                break;
            case 13:
                power_set_brightness_pct(60);
                snprintf(status_feedback, sizeof(status_feedback), "Lysstyrke satt til 60%% (Balansert).");
                break;
            case 14:
                power_set_brightness_pct(80);
                snprintf(status_feedback, sizeof(status_feedback), "Lysstyrke satt til 80%% (Lyssterk).");
                break;
            case 15:
                power_set_brightness_pct(100);
                snprintf(status_feedback, sizeof(status_feedback), "Lysstyrke satt til 100%% (Maksimal).");
                break;
            case 16: {
                int cur = power_get_brightness_pct();
                power_set_brightness_pct(cur - 10);
                snprintf(status_feedback, sizeof(status_feedback), "Lysstyrke redusert til %d%%.", power_get_brightness_pct());
                break;
            }
            case 17: {
                int cur = power_get_brightness_pct();
                power_set_brightness_pct(cur + 10);
                snprintf(status_feedback, sizeof(status_feedback), "Lysstyrke okt til %d%%.", power_get_brightness_pct());
                break;
            }
            case 21:
                power_set_sleep_timeout_sec(15);
                snprintf(status_feedback, sizeof(status_feedback), "Dvale satt til 15 sekunder.");
                break;
            case 22:
                power_set_sleep_timeout_sec(30);
                snprintf(status_feedback, sizeof(status_feedback), "Dvale satt til 30 sekunder (Anbefalt).");
                break;
            case 23:
                power_set_sleep_timeout_sec(60);
                snprintf(status_feedback, sizeof(status_feedback), "Dvale satt til 60 sekunder.");
                break;
            case 24:
                power_set_sleep_timeout_sec(120);
                snprintf(status_feedback, sizeof(status_feedback), "Dvale satt til 2 minutter.");
                break;
            case 25:
                power_set_sleep_timeout_sec(0);
                snprintf(status_feedback, sizeof(status_feedback), "Dvale deaktivert (Alltid pa).");
                break;
            case 31:
                power_set_cpu_governor("ondemand");
                snprintf(status_feedback, sizeof(status_feedback), "CPU Governor: ONDEMAND aktivert.");
                break;
            case 32:
                power_set_cpu_governor("fantasy");
                snprintf(status_feedback, sizeof(status_feedback), "CPU Governor: FANTASY DVFS aktivert.");
                break;
            case 33:
                power_set_cpu_governor("powersave");
                snprintf(status_feedback, sizeof(status_feedback), "CPU Governor: POWERSAVE (60 MHz) aktivert.");
                break;
            case 34:
                power_set_cpu_governor("performance");
                snprintf(status_feedback, sizeof(status_feedback), "CPU Governor: PERFORMANCE (1008 MHz).");
                break;
            case 41:
                power_set_battery_mode(BATTERY_MODE_SMART_VOLT);
                snprintf(status_feedback, sizeof(status_feedback), "Batteri-modus: SMART VOLT (Jevn OCV kurve).");
                break;
            case 42:
                power_set_battery_mode(BATTERY_MODE_AXP_CHIP);
                snprintf(status_feedback, sizeof(status_feedback), "Batteri-modus: AXP209 RAW CHIP registrert.");
                break;
            case 51:
                audio_play(SOUND_START);
                snprintf(status_feedback, sizeof(status_feedback), "Beeper lydtest avspilt.");
                break;
            case 52:
                sensor_toggle_auto_rotate();
                snprintf(status_feedback, sizeof(status_feedback), "Auto-rotasjon: %s.", sensor_get_auto_rotate() ? "AKTIVERT" : "DEAKTIVERT");
                break;
        }
    }
}

App app_settings = {
    .name = "Settings",
    .title = "Hardware Kontrollpanel & Stromsparing",
    .on_start = settings_start,
    .on_update = settings_update,
    .on_render = settings_render,
    .on_touch = settings_touch,
    .on_resize = NULL,
    .on_exit = NULL
};
