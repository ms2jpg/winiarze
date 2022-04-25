#ifndef DEFINES_H
#define DEFINES_H
#include <time.h>

// other defines go here
#define WINEMAKERS_NUMBER 3
#define STUDENTS_NUMBER 2
#define SAFEHOUSE_NUMBER 1

#define MIN_SLEEP 1
#define MAX_SLEEP 10
enum messageTypes
{
SAFEHOUSE_REQUEST,
SAFEHOUSE_REQUEST_ACK,
SAFEHOUSE_TAKEOVER,
SAFEHOUSE_RELEASE,
WINE_ANNOUNCEMENT,
WINE_REQUEST,
WINE_REQUEST_ACK,
GRAB_WINE,
};

#define debug(FORMAT, ...) printf("\033[%d;%dm[c:%d][r:%d][%c]: " FORMAT "\033[0m\n", (*this->rank) % 2, 31 + ((*this->rank) / 2) % 7, *this->clock, *this->rank, ((*this->rank) < WINEMAKERS_NUMBER ? 'W' : 'S'), ##__VA_ARGS__);


#endif // MY_DEFINES_H