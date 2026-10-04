#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sound/asound.h>

#include "audio.h"
#include "sounds.h"
#include "display.h"

static const char *EAT_PATH   = "/data/local/tmp/tabos_eat.wav";
static const char *CRASH_PATH = "/data/local/tmp/tabos_crash.wav";
static const char *START_PATH = "/data/local/tmp/tabos_start.wav";

static int g_master_volume = 55; /* 0 .. 63 på sun5i ALSA-kontroller */
static int g_vol_hud_timer_ms = 0;

static void ensure_file(const char *path, const uint8_t *data, size_t size) {
    struct stat st;
    if (stat(path, &st) != 0 || st.st_size != (off_t)size) {
        FILE *f = fopen(path, "wb");
        if (f) {
            fwrite(data, 1, size, f);
            fclose(f);
        }
    }
}

void audio_init(void) {
    /* Automatisk opprydding av fullførte bakgrunnslyd-prosesser */
    signal(SIGCHLD, SIG_IGN);

    /* Sikre at 8-bit lydeffektfilene ligger klare i flash-minnet */
    ensure_file(EAT_PATH,   snd_eat_wav,   sizeof(snd_eat_wav));
    ensure_file(CRASH_PATH, snd_crash_wav, sizeof(snd_crash_wav));
    ensure_file(START_PATH, snd_start_wav, sizeof(snd_start_wav));

    /* Les gjeldende maskinvare-volum fra ALSA mikser */
    int fd = open("/dev/snd/controlC0", O_RDWR);
    if (fd >= 0) {
        /* Forsikre at PA (Power Amplifier) er aktivert */
        struct snd_ctl_elem_value pa_val;
        memset(&pa_val, 0, sizeof(pa_val));
        pa_val.id.numid = 19; /* 'PA Enable' */
        pa_val.value.integer.value[0] = 1;
        ioctl(fd, SNDRV_CTL_IOCTL_ELEM_WRITE, &pa_val);

        /* Les Master Playback Volume (numid 1) */
        struct snd_ctl_elem_value vol_val;
        memset(&vol_val, 0, sizeof(vol_val));
        vol_val.id.numid = 1;
        if (ioctl(fd, SNDRV_CTL_IOCTL_ELEM_READ, &vol_val) >= 0) {
            long cur = vol_val.value.integer.value[0];
            if (cur >= 0 && cur <= 63) {
                g_master_volume = (int)cur;
            }
        }
        close(fd);
    }
}

int audio_get_volume(void) {
    return g_master_volume;
}

int audio_get_volume_pct(void) {
    return (g_master_volume * 100 + 31) / 63;
}

void audio_set_volume(int vol) {
    if (vol < 0)  vol = 0;
    if (vol > 63) vol = 63;
    g_master_volume = vol;
    g_vol_hud_timer_ms = 1500; /* Vis HUD i 1.5 sekunder */

    int fd = open("/dev/snd/controlC0", O_RDWR);
    if (fd >= 0) {
        struct snd_ctl_elem_value val;
        memset(&val, 0, sizeof(val));
        val.id.numid = 1; /* Master Playback Volume */
        val.value.integer.value[0] = vol;
        ioctl(fd, SNDRV_CTL_IOCTL_ELEM_WRITE, &val);
        close(fd);
    }
}

void audio_set_volume_pct(int pct) {
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    int raw = (pct * 63) / 100;
    audio_set_volume(raw);
}

void audio_volume_up(void) {
    audio_set_volume(g_master_volume + 4);
}

void audio_volume_down(void) {
    audio_set_volume(g_master_volume - 4);
}

bool audio_vol_hud_active(void) {
    return g_vol_hud_timer_ms > 0;
}

bool audio_vol_hud_update(uint32_t elapsed_ms) {
    if (g_vol_hud_timer_ms > 0) {
        if ((int)elapsed_ms >= g_vol_hud_timer_ms) {
            g_vol_hud_timer_ms = 0;
            return true; /* Akkurat utløpt - krever redraw */
        } else {
            g_vol_hud_timer_ms -= elapsed_ms;
        }
    }
    return false;
}

void audio_render_vol_hud(void) {
    int cols = display_get_cols();
    int w = 26;
    int x = (cols - w) / 2;
    int y = 2;

    int pct = audio_get_volume_pct();
    int filled = (pct * 12) / 100;

    display_draw_box(x, y, w, 3, "VOLUM", ANSI_WHITE, ANSI_BLACK, ANSI_LIGHT_GREEN);

    char bar[32];
    int pos = 0;
    pos += snprintf(bar + pos, sizeof(bar) - pos, "[");
    for (int i = 0; i < 12; i++) {
        if (i < filled) pos += snprintf(bar + pos, sizeof(bar) - pos, "\xDB"); /* █ */
        else pos += snprintf(bar + pos, sizeof(bar) - pos, "\xB0");            /* ░ */
    }
    snprintf(bar + pos, sizeof(bar) - pos, "] %3d%%", pct);

    display_draw_string(x + 2, y + 1, bar, ANSI_LIGHT_GREEN, ANSI_BLACK);
}

void audio_play(SoundEffect sound) {
    const char *target = NULL;
    switch (sound) {
        case SOUND_EAT:   target = EAT_PATH; break;
        case SOUND_CRASH: target = CRASH_PATH; break;
        case SOUND_START: target = START_PATH; break;
    }

    if (!target) return;

    pid_t pid = fork();
    if (pid == 0) {
        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execl("/system/bin/stagefright", "stagefright", "-a", "-o", target, (char *)NULL);
        _exit(0);
    }
}
