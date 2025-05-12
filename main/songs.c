#include "songs.h"

song_t Songs[3]; 

void initialize_song_data(void){

    static uint16_t tequila_easy_beats[74];

    for(int i = 0; i < 74; i++){
        tequila_easy_beats[i] = i; 
    }
    //Position 1-4 is from pointer to pinky
    static const uint8_t tequila_beat_vals[74] = {
        15, 15, 15, 15, 15,
        15, 15, 15, 15,
        15, 15, 15, 15,
        15, 15, 15, 15,
        12, 3, 12, 3,
        12, 3, 12, 3,
        12, 3, 12, 3,
        12, 3, 12, 3,
        8, 4, 2, 14,
        8, 4, 2,
        8, 4, 2, 14,
        8, 4, 2,
        8, 4, 2, 14,
        8, 4, 2,
        8, 4, 2, 14,
        8, 4, 2,
        8, 8, 4, 4,
        8, 4, 2, 2,
        1, 1, 12, 3,
        15 };

    Songs[0].beat_numbers = tequila_easy_beats;
    Songs[0].beat_values = tequila_beat_vals;
    Songs[0].bpm = 91;
    Songs[0].beat_count = 74;

}