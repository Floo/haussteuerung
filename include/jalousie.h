
#include <pthread.h>
//#include <time.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <unistd.h>



#ifndef JALOUSIE_H
#define JALOUSIE_H

#include "hs_config.h"
#include "threadsafe.h"
#include "max7311.h"
#include "log.h"
#include "sharedMem.h"


void *jalTimer(void *arg);
void cleanupJalTimer(void *arg);
extern bool verbose;
extern int verbose_level;



class JALOUSIE : public Threadsafe, protected sharedMem
{
private:
    MAX7311 *max;
    int position[4];
    int drvstate[4];
    int offset_count;
    pthread_t thr_id[4];
    int drv(int JalNr, int direction, long fahrZeit, bool Sonne);
    int pdrvAufwaerts(int JalNr, int imptime);
    int pdrvAbwaerts(int JalNr, int imptime);
    int pstop(int JalNr);
    int offset(); //berechnet einen Offset von n*800ms (n = 0...3) um aufeinanderfolgende Steuerbefehle zeitlich zu verschieben
protected:

public:
    JALOUSIE(MAX7311 *maxdev);
    ~JALOUSIE() {};
    void init();
    int drvAufwaerts(int JalNr); // f�rt Auf, vorhergehender Fahrbefehl wird gestoppt
    int drvAufwaerts(int JalNr, int imptime); //f�rt Auf, vorhergehender Fahrbefehl wird gestoppt, Fahrdauer: imptime [msec]
    int toggleAufwaerts(int JalNr); // wenn Fahrt aktiv, dann Stoppen, sonst Auf fahren
    int drvAbwaerts(int JalNr);
    int drvAbwaerts(int JalNr, int imptime);
    int toggleAbwaerts(int JalNr);
    int stop(int JalNr);
    int getPosition(int JalNr)
    {
        return position[JalNr];
    };
    int getDrvState(int JalNr)
    {
        return drvstate[JalNr];
    };
    char *getPosition(int JalNr, char* str);
    char *getDrvState(int JalNr, char* str);
    void updateSHM(int JalNr);
    void setPosition(int JalNr, int pos)
    {
        position[JalNr] = pos;
    }; //{SingleLock slock(this, true);position[JalNr] = pos;};
    int drvSonne(int JalNr);
};

struct thr_jalData
{
    int JalNr;
    long zeit;
    JALOUSIE *jal;
    int direction;
    bool Sonne;
};

#endif

