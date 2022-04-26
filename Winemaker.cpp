
#include "Winemaker.h"

void Winemaker::main()
{
    std::unique_lock<std::mutex> lck(this->canMakeNewBarrel);
    while (true)
    {
        this->randomSleep();
        debug("made barrel (safehouses left %d)", this->safehousesLeft);
        this->setGotWine(1);
        this->sendSafehouseRequests();
        if (WINEMAKERS_NUMBER <= 1)
        {
            if (this->canGrabSafehouse())
            {
                this->grabSafehouse();
            }
        }
        this->cond.wait(lck);
    }
}

void Winemaker::setGotWine(int x)
{
    pthread_mutex_lock(&this->gotWineMutex);
    this->gotWine = x;
    pthread_mutex_unlock(&this->gotWineMutex);
}

int Winemaker::getGotWine()
{
    int x;
    pthread_mutex_lock(&this->gotWineMutex);
    x = this->gotWine;
    pthread_mutex_unlock(&this->gotWineMutex);
    return x;
}

bool Winemaker::safehouseRequestCompare(const struct Entity::packet &a, const struct Entity::packet &b)
{
    if (a.clock == b.clock)
    {
        return a.rank < b.rank;
    }
    return a.clock < b.clock;
}

void Winemaker::sortSafehouseRequests(bool lock = false)
{
    if (lock)
    {
        pthread_mutex_lock(&this->safehouseRequestsMutex);
    }
    sort(this->safehouseRequests.begin(), this->safehouseRequests.end(), Winemaker::safehouseRequestCompare);
    if (lock)
    {
        pthread_mutex_unlock(&this->safehouseRequestsMutex);
    }
}

void Winemaker::communication()
{
    struct packet packet;
    MPI_Status status;
    while (true)
    {
        MPI_Recv(&packet, sizeof(packet), MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        switch ((messageTypes)status.MPI_TAG)
        {
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
            if (this->getGotWine() == 0)
            {
                debug("KURWA %d", status.MPI_SOURCE);
            }
            this->waitingForStudent = 0;
            this->releaseSafehouse();
            break;
        case SAFEHOUSE_RELEASE:
            this->safehousesLeft++;
            break;
        }
        if (this->getGotWine() == 1)
        {
            if (this->canGrabSafehouse())
            {
                this->grabSafehouse();
            }
        }
    }
}

void Winemaker::releaseSafehouse()
{
    this->setGotWine(0);
    struct packet shr = {
        this->incrementClock(),
        *this->rank};

    for (int destID = 0; destID < WINEMAKERS_NUMBER; destID++)
    {
        if (destID == *this->rank)
        {
            this->safehousesLeft++;
        }
        else
        {
            MPI_Send(&shr, sizeof(shr), MPI_BYTE, destID, SAFEHOUSE_RELEASE, MPI_COMM_WORLD);
        }
    }
    debug("wine grabbed, releasing safehouse (left: %d)", this->safehousesLeft);
    this->cond.notify_all();
}

bool Winemaker::canGrabSafehouse()
{
    return this->getTopSafehouseRequestRank() == *this->rank && this->safehouseRequestACKs == (WINEMAKERS_NUMBER - 1) && this->safehousesLeft > 0 && this->getGotWine();
}

void Winemaker::grabSafehouse()
{

    this->safehouseRequestACKs = 0;
    struct packet sht = {
        this->incrementClock(),
        *this->rank};

    this->safehouseRequests.erase(this->safehouseRequests.begin());
    for (int destID = 0; destID < WINEMAKERS_NUMBER; destID++)
    {
        if (destID == *this->rank)
        {
            this->safehousesLeft -= 1;
        }
        else
        {
            MPI_Send(&sht, sizeof(sht), MPI_BYTE, destID, SAFEHOUSE_TAKEOVER, MPI_COMM_WORLD);
        }
    }
    debug("safehouse grabbed (left: %d)", this->safehousesLeft);
    this->sendWineAnnouncement();
}

void Winemaker::sendWineAnnouncement()
{
    this->waitingForStudent = 1;
    struct packet wa = {
        this->getClock(),
        *this->rank};
    for (int studentID = WINEMAKERS_NUMBER; studentID < (WINEMAKERS_NUMBER + STUDENTS_NUMBER); studentID++)
    {
        MPI_Send(&wa, sizeof(wa), MPI_BYTE, studentID, WINE_ANNOUNCEMENT, MPI_COMM_WORLD);
    }
}

int Winemaker::getTopSafehouseRequestRank()
{
    pthread_mutex_lock(&this->safehouseRequestsMutex);
    int r = this->safehouseRequests[0].rank;
    pthread_mutex_unlock(&this->safehouseRequestsMutex);
    return r;
}

void Winemaker::sendSafehouseRequestACK(MPI_Status status)
{
    struct packet shr = {
        this->getClock(),
        *this->rank};
    MPI_Send(&shr, sizeof(shr), MPI_BYTE, status.MPI_SOURCE, SAFEHOUSE_REQUEST_ACK, MPI_COMM_WORLD);
}

void Winemaker::addSafehouseRequest(struct packet shr)
{
    pthread_mutex_lock(&this->safehouseRequestsMutex);
    this->safehouseRequests.push_back(shr);
    this->sortSafehouseRequests();
    pthread_mutex_unlock(&this->safehouseRequestsMutex);
}

void Winemaker::sendSafehouseRequests()
{
    struct packet shr = {
        this->incrementClock(),
        *this->rank};

    for (int destID = 0; destID < WINEMAKERS_NUMBER; destID++)
    {
        if (destID == *this->rank)
        {
            this->addSafehouseRequest(shr);
        }
        else
        {
            MPI_Send(&shr, sizeof(shr), MPI_BYTE, destID, SAFEHOUSE_REQUEST, MPI_COMM_WORLD);
        }
    }
}

Winemaker::Winemaker(int *c, pthread_mutex_t *m, int *r) : Entity(c, m, r)
{
    srand(time(nullptr) + *this->rank);
}
