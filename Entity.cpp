#include <mpi.h>
#include <pthread.h>
#include <cstdio>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <mutex>
#include <algorithm>
#include <condition_variable>
#include "defines.h"
#include "Entity.h"


    Entity::Entity(int *c, pthread_mutex_t *m, int *r)
    {
        this->clock = c;
        this->clock_mutex = m;
        this->rank = r;
    }

    int Entity::incrementClock()
    {
        pthread_mutex_lock(this->clock_mutex);
        int clock_value = ++*this->clock;
        pthread_mutex_unlock(this->clock_mutex);
        return clock_value;
    }

    int Entity::getClock()
    {
        pthread_mutex_lock(this->clock_mutex);
        int clock_value = *this->clock;
        pthread_mutex_unlock(this->clock_mutex);
        return clock_value;
    }

    int Entity::updateAndIncrementClock(int c)
    {
        pthread_mutex_lock(this->clock_mutex);
        if (c > *this->clock)
        {
            *this->clock = c + 1;
        }
        else
        {
            *this->clock += 1;
        }
        c = *this->clock;
        pthread_mutex_unlock(this->clock_mutex);
        return c;
    }

    void* Entity::runComm(void *e)
    {
        ((Entity *)e)->communication();
        return nullptr;
    }

    int Entity::randInt(int _min, int _max)
    {
        return _min + (rand() % (_max - _min));
    }

    void Entity::threadSleep(int s)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * s));
    }

    void Entity::randomSleep()
    {
        int r = Entity::randInt(MIN_SLEEP, MAX_SLEEP);
        debug("sleeping for %d seconds", r);
        Entity::threadSleep(r);
    }

