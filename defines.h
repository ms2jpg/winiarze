#ifndef DEFINES_H
#define DEFINES_H


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

inline char* printTime()
{
    time_t rawtime;
  struct tm * timeinfo;
  char *buffer = new char[10];

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer,10,"%H:%M:%S",timeinfo);
     return buffer;
};
#define debug(FORMAT, ...) printf("\033[%d;%dm[c:%d][r:%d][%c][%s]: " FORMAT "\033[0m\n", (*this->rank) % 2, 31 + ((*this->rank) / 2) % 7, *this->clock, *this->rank, ((*this->rank) < WINEMAKERS_NUMBER ? 'W' : 'S'),printTime(), ##__VA_ARGS__);


#endif 