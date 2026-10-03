#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

#include "audio.h"
#include "sounds.h"

static const char *EAT_PATH   = "/data/local/tmp/tabos_eat.wav";
static const char *CRASH_PATH = "/data/local/tmp/tabos_crash.wav";
static const char *START_PATH = "/data/local/tmp/tabos_start.wav";

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
        /* Lukk standard output/error for å unngå forstyrrelser i konsollen */
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
