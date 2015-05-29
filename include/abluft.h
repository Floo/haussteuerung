/***************************************************************************
 *   Copyright (C) 2007 by Jens Prager   *
 *   prag0338@mme144253   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <pthread.h>
#include <time.h>
#include <sys/errno.h>
#include <sys/sem.h>
#include <string.h>

#ifndef ABLUFT_H
#define ABLUFT_H

#include "hs_config.h"
#include "threadsafe.h"
#include "pcf8574.h"
#include "max7311.h"
#include "log.h"
#include "iniparser.h"
#include "sharedMem.h"
#include "../include/e6tcpclient.h"


extern char *confpath;
extern int conf_sem_ID;

extern struct sembuf LOCK;
extern struct sembuf UNLOCK;

extern bool verbose;
extern int verbose_level;

void *abluftTimer(void *arg);


class Abluft : public Threadsafe, protected sharedMem
{
private:
    PCF8574 *pcf;
    E6TcpClient *e6tcp;
    pthread_t thr_id;
    int state;
    int high(void);
    char ecmd[32];
public:
    Abluft(PCF8574 *io, E6TcpClient *ioE6);
    ~Abluft() {};
    void init();
    int aus(void);
    int low(void);
    void toggleLow();
    void setLow(bool lowState);
    void updateLow();
    void setHigh();
    int getState(void)
    {
        return state;
    };
    void getState(char* str);
    void updateSHM();
};

//Datenstruktur fuer Timer
struct thr_abluftData
{
    long zeit;
    Abluft *abluft;
};


class HWR : public Threadsafe, protected sharedMem
{
private:
    MAX7311 *max;
    int state;
public:
    void init();
    HWR(MAX7311 *io);
    int getState()
    {
        return(state);
    };
    const char* getState(const char* str)
    {
        if (state == AN) str = "An";
        else str = "Aus";
        return str;
    };
    void an();
    void aus();
    void updateSHM();
};

class Orient : public Threadsafe, protected sharedMem
{
private:
    PCF8574 *pcf;
    int state;
public:
    Orient(PCF8574 *io);
    void init();
    int getState()
    {
        return(state);
    };
    const char* getState(const char* str)
    {
        if (state == AN) str = "An";
        else str = "Aus";
        return str;
    };
    void an();
    void aus();
    void updateSHM();
};

class HsGui : public Threadsafe
{
private:
    PCF8574 *pcf;
public:
    void init();
    HsGui(PCF8574 *io)
    {
        pcf = io;
        init();
    };
    void resetDevice();
    void switchPower();
};

class Terrasse:public Threadsafe, protected sharedMem
{
private:
    PCF8574 *pcf;
    int stateVentil_1;
    int stateVentil_2;
    int stateSteckdose;
    pthread_t thr_id[2];
public:
    Terrasse(PCF8574 *io);
    void init();
    void setVentil(int index, int state);
    void setVentil_1(int state);
    void setVentil_2(int state);
    void setSteckdose(int state);
    int getStateVentil_1()
    {
        return(stateVentil_1);
    };
    void getStateVentil_1(char *str);
    int getStateVentil_2()
    {
        return(stateVentil_2);
    };
    void getStateVentil_2(char *str);
    int getStateSteckdose()
    {
        return(stateSteckdose);
    };
    void getStateSteckdose(char *str);
    void startVentilTimer(int index, int dauer);
    void startVentilTimer(int index);
    void updateSHM();
};

struct thr_ventilData
{
    int VentilNr;
    int zeit;
    Terrasse *ter;
};

void *ventilTimer(void *arg);
void cleanupVentilTimer(void *arg);

#endif

