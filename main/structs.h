#pragma once
#include "stdint.h"
#include <stdlib.h>

struct beat
{
    uint8_t counts;
    uint16_t bpm; 
};

typedef struct song
{
    uint16_t bpm; 
    uint8_t beat_count;         // Number of beats stored
    uint16_t *beat_numbers;  // Keys: up to beat numbers
    uint8_t *beat_values;   //expected  input for each input
}song_t;

extern song_t Songs[3];
