#pragma once
#include <pthread.h>

class Entity
{
public:
    int *clock;
    pthread_mutex_t *clock_mutex;
    int *rank;
    struct packet
    {
        int clock;
        int rank;
        int data;
    };
    virtual void main() = 0;
    virtual void communication() = 0;
    Entity(int *c, pthread_mutex_t *m, int *r);

    int incrementClock();

    int getClock();

    int updateAndIncrementClock(int c);
    static void *runComm(void *e);

    static int randInt(int _min, int _max);

    static void threadSleep(int s);

    void randomSleep();
};
