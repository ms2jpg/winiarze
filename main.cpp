#include <mpi.h>
#include <pthread.h>
#include <cstdio>
#include <vector>
#include <ctime>
#include <cstdlib>
// SETTINGS
#define WINEMAKERS_NUMBER 4
#define STUDENTS_NUMBER 4
#define SAFEHOUSE_NUMBER 3

// MESSAGE TYPES

// TYPEDEFS
class Entity {
    public:
        int* clock;
        pthread_mutex_t* clock_mutex;
        int* rank;
        virtual void main() = 0;
        virtual void communication() = 0;
        Entity(int* c, pthread_mutex_t* m, int* r) {
            this->clock = c;
            this->clock_mutex = m;
            this->rank = r;
        }

        int getClock() {
            pthread_mutex_lock(this->clock_mutex);
            int clock_value = *this->clock;
            pthread_mutex_unlock(this->clock_mutex);
            return clock_value;
        }

        int updateClock(int c) {
            pthread_mutex_lock(this->clock_mutex);
            if (c > *this->clock) {
                *this->clock = c;
            }
            c = *this->clock;
            pthread_mutex_unlock(this->clock_mutex);
            return c;
        }

        static void* runComm(void* e) {
            ((Entity*)e)->communication();
        }
};

class Winemaker : public Entity {
    public:
        struct safehouseRequest {
            int clock;
            int rank;
        };
        std::vector<int> winemakersAnnouncements;
        int safehousesLeft = SAFEHOUSE_NUMBER;
        std::vector<struct safehouseRequest> safehouseRequests;

        void main() override {
            printf("%d main winemaker\n", *this->rank);
        }

        void communication() override {
            printf("%d communication winemaker", *this->rank);
        }

        Winemaker(int* c, pthread_mutex_t* m, int* r) : Entity(c, m, r) {
            srand(time(nullptr) + *this->rank);
        }
};

class Student : public Entity {
    public:
        void main() override {
            printf("%d main student\n", *this->rank);
        }

        void communication() override {
            printf("%d communication student\n", *this->rank);
        }

        Student(int* c, pthread_mutex_t* m, int* r) : Entity(c, m, r) {

        }
};


int main(int argc, char **argv)
{
    int clock = 0;
    pthread_mutex_t clockMutex = PTHREAD_MUTEX_INITIALIZER;
    int rank;
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    Entity* entity;
    if (rank < WINEMAKERS_NUMBER) {
        entity = new Winemaker(&clock, &clockMutex, &rank);
    } else if (rank < WINEMAKERS_NUMBER + STUDENTS_NUMBER) {
        entity = new Student(&clock, &clockMutex, &rank);
    }
    pthread_t thd;
    entity->communication();
    pthread_create(&thd, nullptr, &Entity::runComm, entity);
    entity->main();

    MPI_Finalize();
}