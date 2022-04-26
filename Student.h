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
#include "Entity.h"
#include "defines.h"

#ifndef STUDENT_H
#define STUDENT_H

class Student : public Entity
{
    std::mutex canRequestNewWine;
    pthread_mutex_t wineRequestMutex = PTHREAD_MUTEX_INITIALIZER;
    std::vector<struct packet> wineRequests;
    int wineRequestsACKs = 0;
    std::condition_variable cond;

public:
    std::vector<int> winemakerAnnouncements;
    void main() override;
    void communication() override;
    void sendWineRequestACK(MPI_Status status);
    static bool wineRequestCompare(const struct Entity::packet &a, const struct Entity::packet &b);
    void sortWineRequests(bool lock);
    void addWineRequest(struct Entity::packet wr);
    void sendWineRequest();
    int getTopWineRequestRank();
    bool isAnyWineAvailable();
    bool canGrabWine();
    void grabWine();
    Student(int *c, pthread_mutex_t *m, int *r);
};
#endif