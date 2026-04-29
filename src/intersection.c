#include "intersection.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define msleep(ms) usleep((ms) * 1000)

int calculate_green_duration(int queue_count) {

    int duration = BASE_GREEN_MS + (queue_count * DENSITY_SCALE);
    
    if (duration > MAX_GREEN_MS) {
        duration = MAX_GREEN_MS;
    }
    
    return duration;
}