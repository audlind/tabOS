#ifndef AUDIO_H
#define AUDIO_H

typedef enum {
    SOUND_EAT,
    SOUND_CRASH,
    SOUND_START
} SoundEffect;

/* Initialiserer lydmotoren og sikrer lydeffektfiler */
void audio_init(void);

/* Spiller en lydeffekt asynkront i bakgrunnen uten a blokkere spill/UI */
void audio_play(SoundEffect sound);

#endif /* AUDIO_H */
