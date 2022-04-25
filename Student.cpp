#include "Student.h"

    void Student::main()
    {
        std::unique_lock<std::mutex> lck(this->canRequestNewWine);
        while (true)
        {
            this->sendWineRequest();
            this->cond.wait(lck);
            this->randomSleep();
        }
    }

    void Student::communication() 
    {
        struct Entity::packet packet;
        MPI_Status status;
        while (true)
        {
            MPI_Recv(&packet, sizeof(packet), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            switch ((messageTypes)status.MPI_TAG)
            {
            case WINE_ANNOUNCEMENT:
                this->winemakerAnnouncements[status.MPI_SOURCE] += 1;
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
                this->winemakerAnnouncements[packet.data] -= 1;
                this->wineRequests.erase(this->wineRequests.begin());
            }
            if (this->canGrabWine())
            {
                this->grabWine();
            }
        }
    }

    void Student::sendWineRequestACK(MPI_Status status)
    {
        struct packet shr = {
            this->getClock(),
            *this->rank};
        MPI_Send(&shr, sizeof(shr), MPI_BYTE, status.MPI_SOURCE, WINE_REQUEST_ACK, MPI_COMM_WORLD);
    }

    bool Student::wineRequestCompare(const struct Entity::packet &a, const struct Entity::packet &b)
    {
        if (a.clock == b.clock)
        {
            return a.rank < b.rank;
        }
        return a.clock < b.clock;
    }

    void Student::sortWineRequests(bool lock = false)
    {
        if (lock)
        {
            pthread_mutex_lock(&this->wineRequestMutex);
        }
        sort(this->wineRequests.begin(), this->wineRequests.end(), Student::wineRequestCompare);
        if (lock)
        {
            pthread_mutex_unlock(&this->wineRequestMutex);
        }
    }

    void Student::addWineRequest(struct packet wr)
    {
        pthread_mutex_lock(&this->wineRequestMutex);
        this->wineRequests.push_back(wr);
        this->sortWineRequests();
        pthread_mutex_unlock(&this->wineRequestMutex);
    }

    void Student::sendWineRequest()
    {
        debug("give me wine") struct packet wr = {
            this->incrementClock(),
            *this->rank};

        for (int destID = WINEMAKERS_NUMBER; destID < (WINEMAKERS_NUMBER + STUDENTS_NUMBER); destID++)
        {
            if (destID == *this->rank)
            {
                this->addWineRequest(wr);
            }
            else
            {
                MPI_Send(&wr, sizeof(wr), MPI_BYTE, destID, WINE_REQUEST, MPI_COMM_WORLD);
            }
        }
    }

    int Student::getTopWineRequestRank()
    {
        pthread_mutex_lock(&this->wineRequestMutex);
        int r = this->wineRequests.size() > 0 ? this->wineRequests[0].rank : -1;
        pthread_mutex_unlock(&this->wineRequestMutex);
        return r;
    }

    bool Student::isAnyWineAvailable()
    {
        for (int i = 0; i < this->winemakerAnnouncements.size(); i++)
        {
            if (this->winemakerAnnouncements[i] > 0)
            {
                return true;
            }
        }
        return false;
    }

    bool Student::canGrabWine()
    {
        return this->getTopWineRequestRank() == *this->rank && this->wineRequestsACKs == (STUDENTS_NUMBER - 1) && this->isAnyWineAvailable();
    }

    void Student::grabWine()
    {
        this->wineRequestsACKs = 0;
        int winemakerID;
        for (int id = 0; id < WINEMAKERS_NUMBER; id++)
        {
            if (this->winemakerAnnouncements[id] == 1)
            {
                this->winemakerAnnouncements[id] -= 1;
                winemakerID = id;
                break;
            }
        }
        debug("grab wine from winemaker %d", winemakerID);
        struct packet wg = {
            this->incrementClock(),
            *this->rank,
            winemakerID};

        for (int destID = WINEMAKERS_NUMBER; destID < (WINEMAKERS_NUMBER + STUDENTS_NUMBER); destID++)
        {
            if (destID != *this->rank)
            {
                MPI_Send(&wg, sizeof(wg), MPI_BYTE, destID, GRAB_WINE, MPI_COMM_WORLD);
            }
        }

        MPI_Send(&wg, sizeof(wg), MPI_BYTE, winemakerID, GRAB_WINE, MPI_COMM_WORLD);
        this->wineRequests.erase(this->wineRequests.begin());
        this->cond.notify_all();
    }

    Student::Student(int *c, pthread_mutex_t *m, int *r) : Entity(c, m, r)
    {
        srand(time(nullptr) + *this->rank);
        for (int i = 0; i < WINEMAKERS_NUMBER; i++)
        {
            this->winemakerAnnouncements.push_back(0);
        }
    }

