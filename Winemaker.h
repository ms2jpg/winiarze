#pragma once
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

#ifndef WINEMAKER_H
#define WINEMAKER_H

class Winemaker : public Entity
{
public:
    pthread_mutex_t safehouseRequestsMutex = PTHREAD_MUTEX_INITIALIZER;
    std::mutex canMakeNewBarrel;
    pthread_mutex_t gotWineMutex = PTHREAD_MUTEX_INITIALIZER;
    std::condition_variable cond;
    std::vector<struct packet> safehouseRequests;
    int safehousesLeft = SAFEHOUSE_NUMBER;
    int safehouseRequestACKs = 0;
    int gotWine = 0;
    int waitingForStudent = 0;
    void main() override;
    void setGotWine(int x);
    int getGotWine();
    static bool safehouseRequestCompare(const struct packet &a, const struct packet &b);
    void sortSafehouseRequests(bool lock);
    void communication();
    void releaseSafehouse();
    bool canGrabSafehouse();
    void grabSafehouse();
    void sendWineAnnouncement();
    int getTopSafehouseRequestRank();
    void sendSafehouseRequestACK(MPI_Status status);
    void addSafehouseRequest(struct packet shr);
    void sendSafehouseRequests();
    Winemaker(int *c, pthread_mutex_t *m, int *r);
};
#endif