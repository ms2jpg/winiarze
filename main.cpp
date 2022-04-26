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
#include "Winemaker.h"
#include "Student.h"

int main(int argc, char **argv)
{
    int clock = 0;
    pthread_mutex_t clockMutex = PTHREAD_MUTEX_INITIALIZER;
    int rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
    {
        // Print configuration
        int world_size;

        MPI_Comm_size(MPI_COMM_WORLD, &world_size);
        printf("Winemakers: %d\n", WINEMAKERS_NUMBER);
        printf("Students: %d\n", STUDENTS_NUMBER);
        printf("Safehouses: %d\n", SAFEHOUSE_NUMBER);
        printf("Number of processes running: %d\n\n", world_size);
        if (world_size != (WINEMAKERS_NUMBER + STUDENTS_NUMBER))
        {
            printf("Wrong process amount the simulation.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        if (WINEMAKERS_NUMBER <= 0 || STUDENTS_NUMBER <= 0 || SAFEHOUSE_NUMBER <= 0)
        {
            printf("There has to be at least one of every entity type and at least one safehouse.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    Entity *entity;
    if (rank < WINEMAKERS_NUMBER)
    {
        entity = new Winemaker(&clock, &clockMutex, &rank);
    }
    else if (rank < WINEMAKERS_NUMBER + STUDENTS_NUMBER)
    {
        entity = new Student(&clock, &clockMutex, &rank);
    }
    pthread_t thd;
    pthread_create(&thd, nullptr, &Entity::runComm, entity);
    entity->main();

    MPI_Finalize();
}