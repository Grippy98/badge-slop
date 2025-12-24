#include "chip_tunez.h"
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(chip_tunez);

// -- Note Definitions --
#define NOTE_B0 31
#define NOTE_C1 33
#define NOTE_CS1 35
#define NOTE_D1 37
#define NOTE_DS1 39
#define NOTE_E1 41
#define NOTE_F1 44
#define NOTE_FS1 46
#define NOTE_G1 49
#define NOTE_GS1 52
#define NOTE_A1 55
#define NOTE_AS1 58
#define NOTE_B1 62
#define NOTE_C2 65
#define NOTE_CS2 69
#define NOTE_D2 73
#define NOTE_DS2 78
#define NOTE_E2 82
#define NOTE_F2 87
#define NOTE_FS2 93
#define NOTE_G2 98
#define NOTE_GS2 104
#define NOTE_A2 110
#define NOTE_AS2 117
#define NOTE_B2 123
#define NOTE_C3 131
#define NOTE_CS3 139
#define NOTE_D3 147
#define NOTE_DS3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_FS3 185
#define NOTE_G3 196
#define NOTE_GS3 208
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_CS4 277
#define NOTE_D4 294
#define NOTE_DS4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_AS5 932
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_CS6 1109
#define NOTE_D6 1175
#define NOTE_DS6 1245
#define NOTE_E6 1319
#define NOTE_F6 1397
#define NOTE_FS6 1480
#define NOTE_G6 1568
#define NOTE_GS6 1661
#define NOTE_A6 1760
#define NOTE_AS6 1865
#define NOTE_B6 1976
#define NOTE_C7 2093
#define NOTE_CS7 2217
#define NOTE_D7 2349
#define NOTE_DS7 2489
#define NOTE_E7 2637
#define NOTE_F7 2794
#define NOTE_FS7 2960
#define NOTE_G7 3136
#define NOTE_GS7 3322
#define NOTE_A7 3520
#define NOTE_AS7 3729
#define NOTE_B7 3951
#define NOTE_C8 4186
#define NOTE_CS8 4435
#define NOTE_D8 4699
#define NOTE_DS8 4978
#define REST 0

typedef struct {
  uint16_t freq;
  uint16_t duration;
} Note;

typedef struct {
  const char *title;
  const Note *notes;
  int length;
} Song;

// -- Extended Songs --

// 1. Imperial March (Extended)
static const Note song_imperial[] = {
    {NOTE_A4, 500},  {NOTE_A4, 500},  {NOTE_A4, 500},  {NOTE_F4, 350},
    {NOTE_C5, 150},  {NOTE_A4, 500},  {NOTE_F4, 350},  {NOTE_C5, 150},
    {NOTE_A4, 1000}, {NOTE_E5, 500},  {NOTE_E5, 500},  {NOTE_E5, 500},
    {NOTE_F5, 350},  {NOTE_C5, 150},  {NOTE_GS4, 500}, {NOTE_F4, 350},
    {NOTE_C5, 150},  {NOTE_A4, 1000}, {NOTE_A5, 500},  {NOTE_A4, 350},
    {NOTE_A4, 150},  {NOTE_A5, 500},  {NOTE_GS5, 250}, {NOTE_G5, 250},
    {NOTE_FS5, 125}, {NOTE_F5, 125},  {NOTE_FS5, 250}, {REST, 250},
    {NOTE_AS4, 250}, {NOTE_DS5, 500}, {NOTE_D5, 250},  {NOTE_CS5, 250},
    {NOTE_C5, 125},  {NOTE_B4, 125},  {NOTE_C5, 250},  {REST, 250},
    {NOTE_F4, 250},  {NOTE_GS4, 500}, {NOTE_F4, 375},  {NOTE_A4, 125},
    {NOTE_C5, 500},  {NOTE_A4, 375},  {NOTE_C5, 125},  {NOTE_E5, 1000},
};

// 2. Super Mario (Extended Main Theme)
static const Note song_mario[] = {
    // Intro
    {NOTE_E5, 150},
    {NOTE_E5, 150},
    {REST, 150},
    {NOTE_E5, 150},
    {REST, 150},
    {NOTE_C5, 150},
    {NOTE_E5, 150},
    {REST, 150},
    {NOTE_G5, 150},
    {REST, 450},
    {NOTE_G4, 150},
    {REST, 450},
    // Theme A
    {NOTE_C5, 450},
    {NOTE_G4, 150},
    {REST, 300},
    {NOTE_E4, 450},
    {NOTE_A4, 150},
    {REST, 150},
    {NOTE_B4, 150},
    {REST, 150},
    {NOTE_AS4, 150},
    {NOTE_A4, 150},
    {REST, 150},
    {NOTE_G4, 200},
    {NOTE_E5, 200},
    {NOTE_G5, 200},
    {NOTE_A5, 200},
    {REST, 100},
    {NOTE_F5, 150},
    {NOTE_G5, 150},
    {REST, 150},
    {NOTE_E5, 150},
    {REST, 150},
    {NOTE_C5, 150},
    {NOTE_D5, 150},
    {NOTE_B4, 150},
    {REST, 300},
    // Repeat A (Simplified)
    {NOTE_C5, 450},
    {NOTE_G4, 150},
    {REST, 300},
    {NOTE_E4, 450},
    {NOTE_A4, 150},
    {REST, 150},
    {NOTE_B4, 150},
    {REST, 150},
    {NOTE_AS4, 150},
    {NOTE_A4, 150},
    {REST, 150},
    {NOTE_G4, 200},
    {NOTE_E5, 200},
    {NOTE_G5, 200},
    {NOTE_A5, 200},
    {REST, 100},
    {NOTE_F5, 150},
    {NOTE_G5, 150},
    {REST, 150},
    {NOTE_E5, 150},
    {REST, 150},
    {NOTE_C5, 150},
    {NOTE_D5, 150},
    {NOTE_B4, 150},
    {REST, 300},
};

// 3. Nokia Tune (Full)
static const Note song_nokia[] = {
    {NOTE_E5, 150},  {NOTE_D5, 150},  {NOTE_FS4, 300}, {NOTE_GS4, 300},
    {NOTE_CS5, 150}, {NOTE_B4, 150},  {NOTE_D4, 300},  {NOTE_E4, 300},
    {NOTE_B4, 150},  {NOTE_A4, 150},  {NOTE_CS4, 300}, {NOTE_E4, 300},
    {NOTE_A4, 600},  {REST, 600},     {NOTE_E5, 150},  {NOTE_D5, 150},
    {NOTE_FS4, 300}, {NOTE_GS4, 300}, {NOTE_CS5, 150}, {NOTE_B4, 150},
    {NOTE_D4, 300},  {NOTE_E4, 300},  {NOTE_B4, 150},  {NOTE_A4, 150},
    {NOTE_CS4, 300}, {NOTE_E4, 300},  {NOTE_A4, 600},
};

// 4. Doom E1M1 (Proper Extended Version)
// Structure: Riff A x4, Riff B x2, Riff A x2, Riff C (Bridge) x2, Riff A x2
static const Note song_doom[] = {
    // Riff A (Main Low) - Pass 1
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_B2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 300},

    // Riff A - Pass 2
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_B2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 300},

    // Riff A - Pass 3
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_B2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 300},

    // Riff A - Pass 4
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_B2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 300},

    // Riff B (High) - Pass 1
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_E4, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_D4, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_C4, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_AS3, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_B3, 100},
    {NOTE_C4, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_E4, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_D4, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_C4, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_AS3, 300},

    // Riff B (High) - Pass 2
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_E4, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_D4, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_C4, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_AS3, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_B3, 100},
    {NOTE_C4, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_E4, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_D4, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_C4, 100},
    {NOTE_E3, 100},
    {NOTE_E3, 100},
    {NOTE_AS3, 300},

    // Riff A - Pass 1 (Return from High)
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_B2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 300},

    // Riff A - Pass 2
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_B2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 300},

    // Riff C (Bridge) - Pass 1
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_E3, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_D3, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_C3, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_AS2, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_C3, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_E3, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_D3, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_C3, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_AS2, 600},

    // Riff C (Bridge) - Pass 2
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_E3, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_D3, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_C3, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_AS2, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_C3, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_E3, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_D3, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_C3, 150},
    {NOTE_B2, 150},
    {NOTE_B2, 150},
    {NOTE_AS2, 600},

    // Riff A (Finale) - Pass 1
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_B2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 300},

    // Riff A (Finale) - Pass 2
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_B2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_E3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_D3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_C3, 100},
    {NOTE_E2, 100},
    {NOTE_E2, 100},
    {NOTE_AS2, 300},
};

// 5. Fur Elise (Extended)
static const Note song_furelise[] = {
    {NOTE_E5, 150},  {NOTE_DS5, 150}, {NOTE_E5, 150},  {NOTE_DS5, 150},
    {NOTE_E5, 150},  {NOTE_B4, 150},  {NOTE_D5, 150},  {NOTE_C5, 150},
    {NOTE_A4, 400},  {REST, 150},     {NOTE_C4, 150},  {NOTE_E4, 150},
    {NOTE_A4, 150},  {NOTE_B4, 400},  {REST, 150},     {NOTE_E4, 150},
    {NOTE_GS4, 150}, {NOTE_B4, 150},  {NOTE_C5, 400},  {REST, 150},
    {NOTE_E4, 150},  {NOTE_E5, 150},  {NOTE_DS5, 150}, {NOTE_E5, 150},
    {NOTE_DS5, 150}, {NOTE_E5, 150},  {NOTE_B4, 150},  {NOTE_D5, 150},
    {NOTE_C5, 150},  {NOTE_A4, 400},  {REST, 150},     {NOTE_C4, 150},
    {NOTE_E4, 150},  {NOTE_A4, 150},  {NOTE_B4, 400},  {REST, 150},
    {NOTE_E4, 150},  {NOTE_C5, 150},  {NOTE_B4, 150},  {NOTE_A4, 800},
};

// 6. Happy Birthday (Extended)
static const Note song_birthday[] = {
    {NOTE_C4, 250},  {NOTE_C4, 250},  {NOTE_D4, 500},  {NOTE_C4, 500},
    {NOTE_F4, 500},  {NOTE_E4, 1000}, {NOTE_C4, 250},  {NOTE_C4, 250},
    {NOTE_D4, 500},  {NOTE_C4, 500},  {NOTE_G4, 500},  {NOTE_F4, 1000},
    {NOTE_C4, 250},  {NOTE_C4, 250},  {NOTE_C5, 500},  {NOTE_A4, 500},
    {NOTE_F4, 500},  {NOTE_E4, 500},  {NOTE_D4, 1000}, {NOTE_AS4, 250},
    {NOTE_AS4, 250}, {NOTE_A4, 500},  {NOTE_F4, 500},  {NOTE_G4, 500},
    {NOTE_F4, 1000},
};

// 7. Game of Thrones (Extended)
static const Note song_got[] = {
    {NOTE_G4, 500},  {NOTE_C4, 500},  {NOTE_DS4, 250}, {NOTE_F4, 250},
    {NOTE_G4, 500},  {NOTE_C4, 500},  {NOTE_DS4, 250}, {NOTE_F4, 250},
    {NOTE_D4, 1000}, {NOTE_F4, 500},  {NOTE_AS3, 500}, {NOTE_DS4, 250},
    {NOTE_D4, 250},  {NOTE_F4, 500},  {NOTE_AS3, 500}, {NOTE_DS4, 250},
    {NOTE_D4, 250},  {NOTE_C4, 1000}, {NOTE_G4, 500},  {NOTE_C4, 500},
    {NOTE_DS4, 250}, {NOTE_F4, 250},  {NOTE_G4, 500},  {NOTE_C4, 500},
    {NOTE_DS4, 250}, {NOTE_F4, 250},  {NOTE_D4, 1000}, {NOTE_F4, 500},
    {NOTE_AS3, 500}, {NOTE_DS4, 250}, {NOTE_D4, 250},  {NOTE_F4, 500},
    {NOTE_AS3, 500}, {NOTE_DS4, 250}, {NOTE_D4, 250},  {NOTE_C4, 1000},
};

// 8. Mii Channel (Extended)
static const Note song_mii[] = {
    {NOTE_FS4, 200},
    {NOTE_A4, 200},
    {NOTE_CS5, 200},
    {NOTE_A4, 200},
    {NOTE_FS4, 200},
    {NOTE_D4, 150},
    {NOTE_D4, 150},
    {NOTE_D4, 150},
    {NOTE_CS4, 200},
    {NOTE_D4, 200},
    {NOTE_FS4, 200},
    {NOTE_A4, 200},
    {NOTE_CS5, 200},
    {NOTE_A4, 200},
    {NOTE_FS4, 200},
    {NOTE_E5, 250},
    {NOTE_DS5, 250},
    {NOTE_D5, 250},
    {NOTE_GS4, 200},
    {NOTE_CS5, 200},
    {NOTE_FS4, 200},
    {NOTE_CS5, 200},
    {NOTE_GS4, 200},
    {NOTE_CS5, 200},
    {NOTE_G4, 200},
    {NOTE_FS4, 200},
    {NOTE_E4, 200},
    // Loop
    {NOTE_FS4, 200},
    {NOTE_A4, 200},
    {NOTE_CS5, 200},
    {NOTE_A4, 200},
    {NOTE_FS4, 200},
    {NOTE_D4, 150},
    {NOTE_D4, 150},
    {NOTE_D4, 150},
    {NOTE_CS4, 200},
    {NOTE_D4, 200},
    {NOTE_FS4, 200},
    {NOTE_A4, 200},
    {NOTE_CS5, 200},
    {NOTE_A4, 200},
    {NOTE_FS4, 200},
    {NOTE_E5, 250},
    {NOTE_DS5, 250},
    {NOTE_D5, 250},
    {NOTE_GS4, 200},
    {NOTE_CS5, 200},
    {NOTE_FS4, 200},
    {NOTE_CS5, 200},
    {NOTE_GS4, 200},
    {NOTE_CS5, 200},
    {NOTE_G4, 200},
    {NOTE_FS4, 200},
    {NOTE_E4, 200},
};

// 9. Never Gonna Give You Up (Extended)
static const Note song_rick[] = {
    {NOTE_G4, 200},
    {NOTE_A4, 200},
    {NOTE_C5, 200},
    {NOTE_A4, 200},
    {NOTE_E5, 400},
    {NOTE_E5, 400},
    {NOTE_D5, 600},
    {REST, 200},
    {NOTE_G4, 200},
    {NOTE_A4, 200},
    {NOTE_C5, 200},
    {NOTE_A4, 200},
    {NOTE_D5, 400},
    {NOTE_D5, 400},
    {NOTE_C5, 200},
    {NOTE_B4, 200},
    {NOTE_A4, 400},
    {REST, 200},
    {NOTE_G4, 200},
    {NOTE_A4, 200},
    {NOTE_C5, 200},
    {NOTE_A4, 200},
    {NOTE_C5, 400},
    {NOTE_D5, 200},
    {NOTE_B4, 200},
    {NOTE_A4, 200},
    {NOTE_G4, 200},
    {NOTE_E5, 400},
    // Verse 2
    {NOTE_G4, 200},
    {NOTE_A4, 200},
    {NOTE_C5, 200},
    {NOTE_A4, 200},
    {NOTE_E5, 400},
    {NOTE_E5, 400},
    {NOTE_D5, 600},
    {REST, 200},
    {NOTE_G4, 200},
    {NOTE_A4, 200},
    {NOTE_C5, 200},
    {NOTE_A4, 200},
    {NOTE_D5, 600},
    {NOTE_C5, 200},
    {NOTE_B4, 200},
    {NOTE_A4, 400},
};

#define SONG_COUNT 9
static const Song songs[SONG_COUNT] = {
    {"Imperial March", song_imperial, sizeof(song_imperial) / sizeof(Note)},
    {"Super Mario", song_mario, sizeof(song_mario) / sizeof(Note)},
    {"Nokia Tune", song_nokia, sizeof(song_nokia) / sizeof(Note)},
    {"Doom E1M1", song_doom, sizeof(song_doom) / sizeof(Note)},
    {"Fur Elise", song_furelise, sizeof(song_furelise) / sizeof(Note)},
    {"Happy Birthday", song_birthday, sizeof(song_birthday) / sizeof(Note)},
    {"Game of Thrones", song_got, sizeof(song_got) / sizeof(Note)},
    {"Mii Channel", song_mii, sizeof(song_mii) / sizeof(Note)},
    {"Rick Roll", song_rick, sizeof(song_rick) / sizeof(Note)},
};

// -- UI & Player --

static bool is_playing = false;
static int selected_index = 0;
// STATE MIGRATION: Viewport Start Index needed for stable scrolling
static int view_start_idx = 0;

static lv_obj_t *tune_list_cont;
static lv_obj_t *status_label;
static lv_obj_t *count_label; // NEW: Track Counter
static lv_obj_t *arrow_up;
static lv_obj_t *arrow_down;

// Player
static void refresh_list(void);

static void tone_play(uint32_t freq, uint32_t duration_ms) {
  if (freq == 0 || freq > 20000) {
    k_sleep(K_MSEC(duration_ms));
    return;
  }

  uint32_t period_us = 1000000 / freq;
  uint32_t half_period = period_us / 2;
  if (half_period < 10)
    half_period = 10;

  int64_t end_time = k_uptime_get() + duration_ms;

  while (k_uptime_get() < end_time) {
    gpio_pin_set_dt(&buzzer, 1);
    k_usleep(half_period);
    gpio_pin_set_dt(&buzzer, 0);
    k_usleep(half_period);
  }
}

static void play_song_blocking(int idx) {
  is_playing = true;
  lv_label_set_text_fmt(status_label, "> PLAYING: %s <", songs[idx].title);

  // FORCE UI SYNC: Ensure list selection is visually correct before blocking
  refresh_list();
  lv_obj_update_layout(tune_list_cont);
  lv_refr_now(NULL);
  k_sleep(K_MSEC(10));

  const Song *s = &songs[idx];

  for (int i = 0; i < s->length; i++) {
    // Poll Exit/Stop
    if (gpio_pin_get_dt(&btn_left) || gpio_pin_get_dt(&btn_select)) {
      if (gpio_pin_get_dt(&btn_left)) {
        break;
      }
      if (gpio_pin_get_dt(&btn_select) && i > 5) {
        break;
      }
    }

    tone_play(s->notes[i].freq, s->notes[i].duration);
    k_usleep(20);
  }

  is_playing = false;
  lv_label_set_text(status_label, "Select to Play");
  lv_task_handler();
}

// -- New UI Logic (E-Ink Friendly List) --
#define VISIBLE_ITEMS 5

static void refresh_list(void) {
  lv_obj_clean(tune_list_cont);

  // Viewport Logic (Now driven by view_start_idx)
  // view_start_idx is updated in update() function

  int end_idx = view_start_idx + VISIBLE_ITEMS;
  if (end_idx > SONG_COUNT)
    end_idx = SONG_COUNT;

  for (int i = view_start_idx; i < end_idx; i++) {
    lv_obj_t *cont = lv_obj_create(tune_list_cont);
    // REDUCED HEIGHT: 38px (was 40px) to prevent bottom clipping
    lv_obj_set_size(cont, LV_PCT(100), 38);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_radius(cont, 0, 0);

    lv_obj_t *lbl = lv_label_create(cont);
    lv_label_set_text(lbl, songs[i].title);
    lv_obj_center(lbl);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);

    if (i == selected_index) {
      lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
      lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    } else {
      lv_obj_set_style_bg_color(cont, lv_color_white(), 0);
      lv_obj_set_style_text_color(lbl, lv_color_black(), 0);
    }
  }

  // Update Arrows
  if (arrow_up) {
    if (view_start_idx > 0)
      lv_obj_set_style_text_opa(arrow_up, LV_OPA_COVER, 0);
    else
      lv_obj_set_style_text_opa(arrow_up, LV_OPA_TRANSP, 0);
  }
  if (arrow_down) {
    if (view_start_idx < SONG_COUNT - VISIBLE_ITEMS)
      lv_obj_set_style_text_opa(arrow_down, LV_OPA_COVER, 0);
    else
      lv_obj_set_style_text_opa(arrow_down, LV_OPA_TRANSP, 0);
  }

  // Update Counter
  if (count_label) {
    // Format: "01/09"
    lv_label_set_text_fmt(count_label, "%02d/%02d", selected_index + 1,
                          SONG_COUNT);
  }
}

static void tunez_update(void) {
  static int prev_up = 0;
  static int prev_down = 0;
  static int prev_select = 0;
  static int prev_left = 0;

  int up = gpio_pin_get_dt(&btn_up);
  int down = gpio_pin_get_dt(&btn_down);
  int select = gpio_pin_get_dt(&btn_select);
  int left = gpio_pin_get_dt(&btn_left);

  bool needs_redraw = false;

  if (left && !prev_left) {
    return_to_menu();
    return;
  } else if (select && !prev_select) {
    play_song_blocking(selected_index);
  } else if (up && !prev_up) {
    selected_index--;
    // Wrap Around
    if (selected_index < 0) {
      selected_index = SONG_COUNT - 1;
      // Jump view to bottom
      view_start_idx = SONG_COUNT - VISIBLE_ITEMS;
      if (view_start_idx < 0)
        view_start_idx = 0; // Safety
    } else {
      // Scroll Up Logic
      if (selected_index < view_start_idx) {
        view_start_idx = selected_index;
      }
    }
    play_beep_move();
    needs_redraw = true;
  } else if (down && !prev_down) {
    selected_index++;
    // Wrap Around
    if (selected_index >= SONG_COUNT) {
      selected_index = 0;
      // Jump view to top
      view_start_idx = 0;
    } else {
      // Scroll Down Logic
      if (selected_index >= view_start_idx + VISIBLE_ITEMS) {
        view_start_idx = selected_index - VISIBLE_ITEMS + 1;
      }
    }
    play_beep_move();
    needs_redraw = true;
  }

  if (needs_redraw) {
    refresh_list();
  }

  prev_up = up;
  prev_down = down;
  prev_select = select;
  prev_left = left;
}

static void tunez_exit(void) {}

static void tunez_enter(void) {
  // Reset Viewport
  view_start_idx = 0;
  if (selected_index >= VISIBLE_ITEMS) {
    // Ensure valid view state if re-entering with saved selection
    if (selected_index >= SONG_COUNT)
      selected_index = 0;
    view_start_idx = selected_index - VISIBLE_ITEMS + 1;
    if (view_start_idx < 0)
      view_start_idx = 0;
  }

  lv_obj_clean(lv_scr_act());
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);

  // Header (Title)
  lv_obj_t *header = lv_label_create(lv_scr_act());
  lv_label_set_text(header, "CHIP TUNEZ");
  lv_obj_set_style_text_font(header, &lv_font_montserrat_48, 0); // Large Font
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);

  // Top Divider (Reverted to standard position)
  lv_obj_t *line_top = lv_obj_create(lv_scr_act());
  lv_obj_set_size(line_top, 300, 3);
  lv_obj_set_style_bg_color(line_top, lv_color_black(), 0);
  lv_obj_set_style_border_width(line_top, 0, 0);
  lv_obj_align_to(line_top, header, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

  // Status (Bottom)
  status_label = lv_label_create(lv_scr_act());
  lv_label_set_text(status_label, "Select to Play");
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_18, 0);
  lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -5);

  // Track Counter (Bottom Left)
  count_label = lv_label_create(lv_scr_act());
  lv_label_set_text(count_label, "01/09"); // Init
  lv_obj_set_style_text_font(count_label, &lv_font_montserrat_18, 0);
  // Align relative to screen bottom left, with some padding
  lv_obj_align(count_label, LV_ALIGN_BOTTOM_LEFT, 5, -5);

  // Bottom Divider
  lv_obj_t *line_bot = lv_obj_create(lv_scr_act());
  lv_obj_set_size(line_bot, 300, 3);
  lv_obj_set_style_bg_color(line_bot, lv_color_black(), 0);
  lv_obj_set_style_border_width(line_bot, 0, 0);
  lv_obj_align_to(line_bot, status_label, LV_ALIGN_OUT_TOP_MID, 0, -5);

  // List Container
  tune_list_cont = lv_obj_create(lv_scr_act());
  // REDUCED height: 190px (5 * 38px)
  lv_obj_set_size(tune_list_cont, 260, 190);
  // Shift left (-10) and DOWN (10)
  lv_obj_align_to(tune_list_cont, line_top, LV_ALIGN_OUT_BOTTOM_MID, -10, 10);

  lv_obj_set_style_pad_all(tune_list_cont, 0, 0);
  lv_obj_set_flex_flow(tune_list_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_gap(tune_list_cont, 0, 0);
  lv_obj_set_style_border_width(tune_list_cont, 0, 0);

  // Up Arrow
  arrow_up = lv_label_create(lv_scr_act());
  lv_label_set_text(arrow_up, LV_SYMBOL_UP);
  lv_obj_set_style_text_font(arrow_up, &lv_font_montserrat_24, 0);
  lv_obj_align_to(arrow_up, tune_list_cont, LV_ALIGN_OUT_RIGHT_TOP, 5, 10);

  // Down Arrow
  arrow_down = lv_label_create(lv_scr_act());
  lv_label_set_text(arrow_down, LV_SYMBOL_DOWN);
  lv_obj_set_style_text_font(arrow_down, &lv_font_montserrat_24, 0);
  lv_obj_align_to(arrow_down, tune_list_cont, LV_ALIGN_OUT_RIGHT_BOTTOM, 5,
                  -10);

  refresh_list();
}

App chip_tunez_app = {.name = "Chip Tunez",
                      .enter = tunez_enter,
                      .update = tunez_update,
                      .exit = tunez_exit};
