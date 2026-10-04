#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SOUND_EAT,
    SOUND_CRASH,
    SOUND_START
} SoundEffect;

/* Initialiserer lydmotoren og sikrer lydeffektfiler */
void audio_init(void);

/* Spiller en lydeffekt asynkront i bakgrunnen uten a blokkere spill/UI */
void audio_play(SoundEffect sound);

/* Maskinvare-volumkontroll (ALSA Master Playback Volume på Allwinner sun5i-CODEC) */
int  audio_get_volume(void);         /* 0 .. 63 */
int  audio_get_volume_pct(void);     /* 0 .. 100% */
void audio_set_volume(int vol);      /* 0 .. 63 */
void audio_set_volume_pct(int pct);
void audio_volume_up(void);          /* +4 enheter (~6%) */
void audio_volume_down(void);        /* -4 enheter (~6%) */

/* HUD / OSD Volumvisning */
bool audio_vol_hud_active(void);
bool audio_vol_hud_update(uint32_t elapsed_ms);
void audio_render_vol_hud(void);

#endif /* AUDIO_H */
