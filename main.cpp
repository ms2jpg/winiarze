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


// SETTINGS
#define WINEMAKERS_NUMBER 4
#define STUDENTS_NUMBER 4
#define SAFEHOUSE_NUMBER 3

#define MIN_SLEEP 1
#define MAX_SLEEP 10

// MESSAGE TYPES
#define SAFEHOUSE_REQUEST 0
#define SAFEHOUSE_REQUEST_ACK 1
#define SAFEHOUSE_TAKEOVER 2
#define SAFEHOUSE_RELEASE 3
#define WINE_ANNOUNCEMENT 4
#define WINE_REQUEST 5
#define WINE_REQUEST_ACK 6
#define GRAB_WINE 7



#define debug(FORMAT,...) printf("\033[%d;%dm[c:%d][r:%d]: " FORMAT "\033[0m\n",(*this->rank) % 2, 31 + ((*this->rank) / 2) % 7, *this->clock, *this->rank, ##__VA_ARGS__);


class Entity {
    public:
        int* clock;
        pthread_mutex_t* clock_mutex;
        int* rank;
    struct packet {
        int clock;
        int rank;
        int data;
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

        void randomSleep() {
            int r = Entity::randInt(MIN_SLEEP, MAX_SLEEP);
            debug("sleeping for %d seconds", r);
            Entity::threadSleep(r);
        }
};

class Winemaker : public Entity {
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
        void main() override {
            std::unique_lock<std::mutex> lck(this->canMakeNewBarrel);
            while (true) {
                this->randomSleep();
                debug("made barrel (safehouses left %d)", this->safehousesLeft);
                this->setGotWine(1);
                this->sendSafehouseRequests();
                this->cond.wait(lck);
            }
        }

        int setGotWine(int x) {
            pthread_mutex_lock(&this->gotWineMutex);
            this->gotWine = x;
            pthread_mutex_unlock(&this->gotWineMutex);
        }

        int getGotWine() {
            int x;
            pthread_mutex_lock(&this->gotWineMutex);
            x = this->gotWine;
            pthread_mutex_unlock(&this->gotWineMutex);
            return x;
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
                        break;
                    case SAFEHOUSE_REQUEST_ACK:
                        this->safehouseRequestACKs += 1;
                        break;
                    case SAFEHOUSE_TAKEOVER:
                        this->safehousesLeft -= 1;
                        this->safehouseRequests.erase(this->safehouseRequests.begin());
                        break;
                    case GRAB_WINE:
                        if (this->getGotWine() == 0) {
                            debug("KURWA %d", status.MPI_SOURCE);
                        }
                        this->waitingForStudent = 0;
                        this->releaseSafehouse();
                        break;
                    case SAFEHOUSE_RELEASE:
                        this->safehousesLeft++;
                        break;
                }
                if (this->getGotWine() == 1) {
                    if (this->canGrabSafehouse()) {
                        this->grabSafehouse();
                    }
                }
            }
        }

        void releaseSafehouse() {
            this->setGotWine(0);
            struct packet shr = {
                    this->incrementClock(),
                    *this->rank
            };

            for (int destID = 0; destID < WINEMAKERS_NUMBER; destID++) {
                if (destID == *this->rank) {
                    this->safehousesLeft++;
                } else {
                    MPI_Send(&shr, sizeof(shr), MPI_BYTE, destID, SAFEHOUSE_RELEASE, MPI_COMM_WORLD);
                }
            }
            debug("wine grabbed, releasing safehouse (left: %d)", this->safehousesLeft);
            this->cond.notify_all();
        }

        bool canGrabSafehouse() {
            return this->getTopSafehouseRequestRank() == *this->rank
            && this->safehouseRequestACKs == (WINEMAKERS_NUMBER - 1)
            && this->safehousesLeft > 0
            && this->getGotWine();
        }

        void grabSafehouse() {

            this->safehouseRequestACKs = 0;
            struct packet sht = {
                    this->incrementClock(),
                    *this->rank
            };

            this->safehouseRequests.erase(this->safehouseRequests.begin());
            for (int destID = 0; destID < WINEMAKERS_NUMBER; destID++) {
                if (destID == *this->rank) {
                    this->safehousesLeft -= 1;
                } else {
                    MPI_Send(&sht, sizeof(sht), MPI_BYTE, destID, SAFEHOUSE_TAKEOVER, MPI_COMM_WORLD);
                }
            }
            debug("safehouse grabbed (left: %d)", this->safehousesLeft);
            this->sendWineAnnouncement();
        }

        void sendWineAnnouncement() {
            this->waitingForStudent = 1;
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
    std::mutex canRequestNewWine;
    pthread_mutex_t wineRequestMutex = PTHREAD_MUTEX_INITIALIZER;
    std::vector<struct packet> wineRequests;
    int wineRequestsACKs = 0;
    std::condition_variable cond;
    public:
        std::vector<int> winemakerAnnouncements;
        void main() override {
            std::unique_lock<std::mutex> lck(this->canRequestNewWine);
            while(true) {
                this->sendWineRequest();
                this->cond.wait(lck);
                this->randomSleep();
            }
        }

        void communication() override {
            struct packet packet;
            MPI_Status status;
            while(true) {
                MPI_Recv(&packet, sizeof(packet), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                switch (status.MPI_TAG) {
                    case WINE_ANNOUNCEMENT:
                        debug("got wine announcement from %d", status.MPI_SOURCE);
                        this->winemakerAnnouncements[status.MPI_SOURCE] = 1;
                        break;
                    case WINE_REQUEST:
                        this->addWineRequest(packet);
                        this->updateAndIncrementClock(packet.clock);
                        this->sendWineRequestACK(status);
                        break;
                    case WINE_REQUEST_ACK:
                        this->wineRequestsACKs += 1;
                        break;
                    case GRAB_WINE:
                        debug("grab wine from %d at pos%d, structure %d %d %d %d",
                              status.MPI_SOURCE,
                              packet.data,
                              this->winemakerAnnouncements[0],
                              this->winemakerAnnouncements[1],
                              this->winemakerAnnouncements[2],
                              this->winemakerAnnouncements[3]
                          );
                        this->winemakerAnnouncements[packet.data] = 0;
                        this->wineRequests.erase(this->wineRequests.begin());
                }
                debug("canGrabWine");
                if (this->canGrabWine()) {
                    this->grabWine();
                }
            }
        }

        void sendWineRequestACK(MPI_Status status) {
            struct packet shr = {
                    this->getClock(),
                    *this->rank
            };
            MPI_Send(&shr, sizeof(shr), MPI_BYTE, status.MPI_SOURCE, WINE_REQUEST_ACK, MPI_COMM_WORLD);
        }

        static bool wineRequestCompare(const struct packet& a, const struct packet& b) {
            if (a.clock == b.clock) {
                return a.rank < b.rank;
            }
            return a.clock < b.clock;
        }

        void sortWineRequests(bool lock = false) {
            if (lock) {
                pthread_mutex_lock(&this->wineRequestMutex);
            }
            sort(this->wineRequests.begin(), this->wineRequests.end(), Student::wineRequestCompare);
            if (lock) {
                pthread_mutex_unlock(&this->wineRequestMutex);
            }
        }

        void addWineRequest(struct packet wr) {
            pthread_mutex_lock(&this->wineRequestMutex);
            this->wineRequests.push_back(wr);
            this->sortWineRequests();
            pthread_mutex_unlock(&this->wineRequestMutex);
        }

        void sendWineRequest() {
            debug("give me wine")
            struct packet wr = {
                    this->incrementClock(),
                    *this->rank
            };

            for (int destID = WINEMAKERS_NUMBER; destID < (WINEMAKERS_NUMBER + STUDENTS_NUMBER); destID++) {
                if (destID == *this->rank) {
                    this->addWineRequest(wr);
                } else {
                    MPI_Send(&wr, sizeof(wr), MPI_BYTE, destID, WINE_REQUEST, MPI_COMM_WORLD);
                }
            }
        }

        int getTopWineRequestRank() {
            pthread_mutex_lock(&this->wineRequestMutex);
            int r = this->wineRequests.size() > 0 ? this->wineRequests[0].rank : -1;
            pthread_mutex_unlock(&this->wineRequestMutex);
            return r;
        }

        bool isAnyWineAvailable() {
            for (int i = 0; i < this->winemakerAnnouncements.size(); i++) {
                if (this->winemakerAnnouncements[i]) {
                    return true;
                }
            }
            return false;
        }

        bool canGrabWine() {
            return this->getTopWineRequestRank() == *this->rank
                   && this->wineRequestsACKs == (STUDENTS_NUMBER - 1)
                   && this->isAnyWineAvailable();
        }

        void grabWine() {
            debug("requests");
            for(int i = 0; i < this->wineRequests.size(); i++) {
                debug("#%d, clk: %d, rank: %d", i+1, this->wineRequests[i].clock, this->wineRequests[i].rank)
            }
            debug(" structure %d %d %d %d", this->winemakerAnnouncements[0], this->winemakerAnnouncements[1], this->winemakerAnnouncements[2], this->winemakerAnnouncements[3]);

            this->wineRequestsACKs = 0;
            int winemakerID;
            for (int id = 0; id < WINEMAKERS_NUMBER; id++) {
                if (this->winemakerAnnouncements[id] == 1) {
                    this->winemakerAnnouncements[id] = 0;
                    winemakerID = id;
                    break;
                }
            }
            debug("grab wine from winemaker %d", winemakerID);
            struct packet wg = {
                    this->incrementClock(),
                    *this->rank,
                    winemakerID
            };

            for (int destID = WINEMAKERS_NUMBER; destID < (WINEMAKERS_NUMBER + STUDENTS_NUMBER); destID++) {
                if (destID != *this->rank) {
                    MPI_Send(&wg, sizeof(wg), MPI_BYTE, destID, GRAB_WINE, MPI_COMM_WORLD);
                }
            }

            MPI_Send(&wg, sizeof(wg), MPI_BYTE, winemakerID, GRAB_WINE, MPI_COMM_WORLD);
            this->wineRequests.erase(this->wineRequests.begin());
            this->cond.notify_all();
        }

        Student(int* c, pthread_mutex_t* m, int* r) : Entity(c, m, r) {
            srand(time(nullptr) + *this->rank);
            for (int i = 0; i < WINEMAKERS_NUMBER; i++) {
                this->winemakerAnnouncements.push_back(0);
            }
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