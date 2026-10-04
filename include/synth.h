#ifndef TABOS_SYNTH_H
#define TABOS_SYNTH_H

#include <stdint.h>
#include <stddef.h>

#define SYNTH_SAMPLE_RATE 44100

/* Pulse duty cycles (NES APU standard) */
typedef enum {
    DUTY_12_5 = 0, /* 12.5% (1/8 high, 7/8 low) - tynt, nasalt */
    DUTY_25_0 = 1, /* 25.0% (2/8 high, 6/8 low) - klassisk NES blyt */
    DUTY_50_0 = 2, /* 50.0% (4/8 high, 4/8 low) - ren firkantbølge */
    DUTY_75_0 = 3  /* 75.0% (invertert 25%)                      */
} PulseDuty;

/* Kanal 1 & 2: Pulse / Firkantbølge */
typedef struct {
    uint8_t enabled;
    float freq;         /* Hz */
    uint8_t volume;     /* 0 .. 15 */
    PulseDuty duty;
    float phase;        /* 0.0 .. 1.0 */
} SynthPulseChannel;

/* Kanal 3: Trekantbølge (Triangle) */
typedef struct {
    uint8_t enabled;
    float freq;         /* Hz */
    float phase;        /* 0.0 .. 1.0 */
} SynthTriangleChannel;

/* Kanal 4: Støy (15-bit LFSR Pseudo-Random Noise) */
typedef struct {
    uint8_t enabled;
    uint8_t volume;     /* 0 .. 15 */
    uint16_t lfsr;      /* 15-bit shift register */
    float shift_rate;   /* Frekvens for hvor ofte LFSR skiftes */
    float phase;        /* Teller for skift-timing */
    int16_t last_sample;
} SynthNoiseChannel;

/* Enkle filtere og lydeffekter */
typedef struct {
    uint8_t lpf_enabled;
    float cutoff;       /* 0.01 .. 1.0 (1.0 = helt åpent filter) */
    float lpf_state;    /* Akkumulator for 1-pols IIR lavpassfilter */
    
    uint8_t bitcrush;   /* 0 = av, 8 = 8-bit, 4 = 4-bit */
    
    /* ADSR Enveloppe */
    float attack_time;  /* sekunder */
    float decay_time;   /* sekunder */
    float sustain_lvl;  /* 0.0 .. 1.0 */
    float release_time; /* sekunder */
} SynthFilterConfig;

/* Samlet synthesizer-tilstand */
typedef struct {
    SynthPulseChannel pulse1;
    SynthPulseChannel pulse2;
    SynthTriangleChannel triangle;
    SynthNoiseChannel noise;
    SynthFilterConfig filter;
} SynthState;

/* Initialiserer synth til standard NES-verdier */
void synth_init(void);

/* Hent tilgang til aktiv synth-tilstand */
SynthState *synth_get_state(void);

/* Genererer rå PCM-lyd (signed 16-bit, 44100 Hz) i buffer */
void mix_nes_buffer(int16_t *buffer, size_t length);

/* Syntetiserer en tone eller lydeffekt og lagrer som WAV-fil i flash */
void synth_render_wav(const char *path, float duration_sec, float pitch_start, float pitch_end);

/* Hurtigfunksjoner for å spille av lyder i sanntid via høyttaleren */
void synth_play_note(float freq, float duration_sec);
void synth_play_preset(int preset_id);
void synth_play_arpeggio(int pattern_id);

#endif /* TABOS_SYNTH_H */
