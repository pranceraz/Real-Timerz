// global.h
#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdbool.h>

// Declare variables
extern bool start;
extern bool running;
extern bool should_terminate;

extern int current_input;
extern int correct_input;

typedef struct {
    int hit;
    int miss;
} Scoreboard;

extern Scoreboard scoreboard;

#endif // GLOBAL_H