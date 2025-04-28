#include "global.h"
int work_ready = 0;
int workers_done = 0;

int current_input = 0;
int correct_input = 0;

bool start = 0;
bool should_terminate = 0;
bool running = 0;

Scoreboard scoreboard = {0, 0};  
