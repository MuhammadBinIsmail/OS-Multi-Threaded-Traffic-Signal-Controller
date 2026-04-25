#ifndef SHARED_H
#define SHARED_H

#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define NUM_LANES           4
#define MAX_QUEUE           20
#define MAX_EMERGENCY_Q     10

#define BASE_GREEN_MS       2000
#define MAX_GREEN_MS        8000
#define YELLOW_MS           800
#define RED_SLEEP_MS        500
#define DENSITY_SCALE       120
#define GEN_INTERVAL_MS     400
#define EMERGENCY_WAIT_MS   2000

#define NORTH   0
#define SOUTH   1
#define EAST    2
#define WEST    3

typedef enum {
    RED    = 0,
    YELLOW = 1,
    GREEN  = 2
} SignalState;

typedef struct {
    int  vehicle_id;
    int  lane;
    struct timespec arrival_time;
} Vehicle;

typedef struct {
    int vehicle_id;
    int lane;
    int priority;
} EmergencyVehicle;

typedef struct {
    EmergencyVehicle heap[MAX_EMERGENCY_Q];
    int size;
} EmergencyPQ;

typedef struct {
    SignalState signal[NUM_LANES];
    Vehicle     lane_queue[NUM_LANES][MAX_QUEUE];
    int         queue_count[NUM_LANES];

    sem_t            intersection_sem;
    pthread_mutex_t  queue_mutex[NUM_LANES];
    pthread_mutex_t  stats_mutex;
    pthread_mutex_t  emergency_mutex;

    int    total_vehicles_passed;
    int    vehicles_per_lane[NUM_LANES];
    double total_wait_time_sec;
    double throughput_per_min;

    EmergencyPQ  emergency_pq;
    int          emergency_active;
    int          emergency_lane;
    int          running;
    int          next_vehicle_id;
    struct timespec sim_start_time;

} SharedState;

void shared_state_init(SharedState *state);
void shared_state_destroy(SharedState *state);
void  pq_push(EmergencyPQ *pq, EmergencyVehicle ev);
EmergencyVehicle pq_pop(EmergencyPQ *pq);
int   pq_is_empty(EmergencyPQ *pq);
double elapsed_seconds(SharedState *state);
const char *lane_name(int lane);

#endif