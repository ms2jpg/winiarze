#include <mpi.h>
#include <pthread.h>
#include <cstdio>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <thread>
#include <algorithm>

// SETTINGS
#define WINEMAKERS_NUMBER 4
#define STUDENTS_NUMBER 4
#define SAFEHOUSE_NUMBER 3

#define MIN_SLEEP 5
#define MAX_SLEEP 30

// MESSAGE TYPES
#define SAFEHOUSE_REQUEST 0
#define SAFEHOUSE_REQUEST_ACK 1
#define SAFEHOUSE_TAKEOVER 2
#define SAFEHOUSE_RELEASE 3
#define WINE_ANNOUNCEMENT 4
#define GRAB_WINE 5



#define debug(FORMAT,...) printf("\033[%d;%dm[c:%d][r:%d]: " FORMAT "\033[0m\n",(*this->rank) % 2, 31 + ((*this->rank) / 2) % 7, *this->clock, *this->rank, ##__VA_ARGS__);


class Entity {
    public:
        int* clock;
        pthread_mutex_t* clock_mutex;
        int* rank;
    struct packet {
        int clock;
        int rank;
    };
        virtual void main() = 0;
        virtual void communication() = 0;
        Entity(int* c, pthread_mutex_t* m, int* r) {
            this->clock = c;
            this->clock_mutex = m;
            this->rank = r;
        }

        int incrementClock() {
            pthread_mutex_lock(this->clock_mutex);
            int clock_value = ++*this->clock;
            pthread_mutex_unlock(this->clock_mutex);
            return clock_value;
        }

        int getClock() {
            pthread_mutex_lock(this->clock_mutex);
            int clock_value = *this->clock;
            pthread_mutex_unlock(this->clock_mutex);
            return clock_value;
        }

        int updateAndIncrementClock(int c) {
            pthread_mutex_lock(this->clock_mutex);
            if (c > *this->clock) {
                *this->clock = c + 1;
            } else {
                *this->clock += 1;
            }
            c = *this->clock;
            pthread_mutex_unlock(this->clock_mutex);
            return c;
        }

        static void* runComm(void* e) {
            ((Entity*)e)->communication();
            return nullptr;
        }

        static int randInt(int _min, int _max) {
            return _min + (rand() % (_max - _min));
        }

        static void threadSleep(int s) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 * s));
        }

        static void randomSleep() {
            Entity::threadSleep(Entity::randInt(MIN_SLEEP, MAX_SLEEP));
        }
};

class Winemaker : public Entity {
    public:
        pthread_mutex_t safehouseRequestsMutex = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_t canMakeNewBarrel = PTHREAD_MUTEX_INITIALIZER;
        std::vector<struct packet> safehouseRequests;
        int safehousesLeft = SAFEHOUSE_NUMBER;
        int safehouse_request_acks = 0;

        void main() override {
            while (true) {
                this->randomSleep();
                debug("make barrel");
                this->sendSafehouseRequests();
                pthread_mutex_lock(&this->canMakeNewBarrel);
                pthread_mutex_lock(&this->canMakeNewBarrel);
            }
        }

        static bool safehouseRequestCompare(const struct packet& a, const struct packet& b) {
            if (a.clock == b.clock) {
                return a.rank < b.rank;
            }
            return a.clock < b.clock;
        }

        void sortSafehouseRequests(bool lock = false) {
            if (lock) {
                pthread_mutex_lock(&this->safehouseRequestsMutex);
            }
            sort(this->safehouseRequests.begin(), this->safehouseRequests.end(), Winemaker::safehouseRequestCompare);
            if (lock) {
                pthread_mutex_unlock(&this->safehouseRequestsMutex);
            }
        }

        void communication() override {
            struct packet packet;
            MPI_Status status;
            while(true) {
                MPI_Recv(&packet, sizeof(packet), MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                switch (status.MPI_TAG) {
                    case SAFEHOUSE_REQUEST:
                        this->addSafehouseRequest(packet);
                        this->updateAndIncrementClock(packet.clock);
                        this->sendSafehouseRequestACK(status);
                        if (this->canGrabSafehouse()) {
                            this->grabSafehouse();
                        }
                        break;
                    case SAFEHOUSE_REQUEST_ACK:
                        safehouse_request_acks += 1;
                        if (this->canGrabSafehouse()) {
                            this->grabSafehouse();
                        }
                        break;
                    case SAFEHOUSE_TAKEOVER:
                        this->safehousesLeft -= 1;
                        this->safehouseRequests.erase(this->safehouseRequests.begin());
                        if (this->canGrabSafehouse()) {
                            this->grabSafehouse();
                        }
                        break;
                    case GRAB_WINE:
                        this->releaseSafehouse();
                        pthread_mutex_unlock(&this->canMakeNewBarrel);
                        break;
                    case SAFEHOUSE_RELEASE:
                        this->safehousesLeft++;
                        if (this->canGrabSafehouse()) {
                            this->grabSafehouse();
                        }
                }
            }
        }

        void releaseSafehouse() {
            struct packet shr = {
                    this->incrementClock(),
                    *this->rank
            };

            this->safehousesLeft++;
            for (int destID = 0; destID < WINEMAKERS_NUMBER; destID++) {
                if (destID != *this->rank) {
                    MPI_Send(&shr, sizeof(shr), MPI_BYTE, destID, SAFEHOUSE_RELEASE, MPI_COMM_WORLD);
                }
            }
        }

        bool canGrabSafehouse() {
            return this->getTopSafehouseRequestRank() == *this->rank
            && safehouse_request_acks == (WINEMAKERS_NUMBER - 1)
            && this->safehousesLeft > 0;
        }

        void grabSafehouse() {
            debug("grab safehouse");
            struct packet sht = {
                    this->incrementClock(),
                    *this->rank
            };

            this->safehousesLeft -= 1;
            this->safehouseRequests.erase(this->safehouseRequests.begin());

            for (int destID = 0; destID < WINEMAKERS_NUMBER; destID++) {
                if (destID != *this->rank) {
                    MPI_Send(&sht, sizeof(sht), MPI_BYTE, destID, SAFEHOUSE_TAKEOVER, MPI_COMM_WORLD);
                }
            }
            this->sendWineAnnouncement();
        }

        void sendWineAnnouncement() {
            struct packet wa = {
                    this->getClock(),
                    *this->rank
            };
            for(int studentID = WINEMAKERS_NUMBER; studentID < (WINEMAKERS_NUMBER + STUDENTS_NUMBER); studentID++) {
                MPI_Send(&wa, sizeof(wa), MPI_BYTE, studentID, WINE_ANNOUNCEMENT, MPI_COMM_WORLD);
            }
        }

        int getTopSafehouseRequestRank() {
            pthread_mutex_lock(&this->safehouseRequestsMutex);
            int r = this->safehouseRequests[0].rank;
            pthread_mutex_unlock(&this->safehouseRequestsMutex);
            return r;
        }

        void sendSafehouseRequestACK(MPI_Status status) {
            struct packet shr = {
                    this->getClock(),
                    *this->rank
            };
            MPI_Send(&shr, sizeof(shr), MPI_BYTE, status.MPI_SOURCE, SAFEHOUSE_REQUEST_ACK, MPI_COMM_WORLD);
        }

        void addSafehouseRequest(struct packet shr) {
            pthread_mutex_lock(&this->safehouseRequestsMutex);
            this->safehouseRequests.push_back(shr);
            this->sortSafehouseRequests();
            pthread_mutex_unlock(&this->safehouseRequestsMutex);
        }

        void sendSafehouseRequests() {
            struct packet shr = {
                    this->incrementClock(),
                    *this->rank
            };

            for (int destID = 0; destID < WINEMAKERS_NUMBER; destID++) {
                if (destID == *this->rank) {
                    this->addSafehouseRequest(shr);
                } else {
                    MPI_Send(&shr, sizeof(shr), MPI_BYTE, destID, SAFEHOUSE_REQUEST, MPI_COMM_WORLD);
                }
            }
        }

        Winemaker(int* c, pthread_mutex_t* m, int* r) : Entity(c, m, r) {
            srand(time(nullptr) + *this->rank);
        }
};

class Student : public Entity {
    public:
        void main() override {
//            printf("%d main student\n", *this->rank);
        }

        void communication() override {
//            printf("%d communication student\n", *this->rank);
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
    pthread_create(&thd, nullptr, &Entity::runComm, entity);
    entity->main();

    MPI_Finalize();
}