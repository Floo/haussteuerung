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


#include "jalousie.h"

JALOUSIE::JALOUSIE(MAX7311 *maxdev):sharedMem()
{
    //sharedMem();
    max = maxdev;

    offset_count = 0;

    int i;
    //Variablen initialisieren
    for(i = 0; i <= 3; i++)
        position[i] = UNDEF;
    for(i = 0; i <= 3; i++)
        drvstate[i] = STOP;
    //alle Relais schalten
    init();
    //for(i = 0; i <= 7; i++)
    //    max->set((unsigned char)i, false);
    for(i = 0; i <= 3; i++)
        updateSHM(i);
}

void JALOUSIE::init()
{
    int JalNr;
    for(JalNr = 0; JalNr <= 3; JalNr++)
    {
        switch(drvstate[JalNr])
        {
            case(AUF):
                max->set((unsigned char) 2 * JalNr, false);
                max->set((unsigned char) 2 * JalNr + 1, true);
                break;
            case(AB):
                max->set((unsigned char) 2 * JalNr, true);
                max->set((unsigned char) 2 * JalNr + 1, false);
                break;
            case(STOP):
                max->set((unsigned char) 2 * JalNr, false);
                max->set((unsigned char) 2 * JalNr + 1, false);
                break;
        }
    }
}

char* JALOUSIE::getPosition(int JalNr, char* str)
{
    switch(position[JalNr])
    {
    case AUF:
        sprintf(str, "Offen");
        break;
    case UNDEF:
        sprintf(str, "Undefiniert");
        break;
    case AB:
        sprintf(str, "Geschlossen");
        break;
    }
    return str;
}

char* JALOUSIE::getDrvState(int JalNr, char* str)
{
    switch(drvstate[JalNr])
    {
    case AUF:
        sprintf(str, "Aufwaerts");
        break;
    case STOP:
        sprintf(str, "Stop");
        break;
    case AB:
        sprintf(str, "Abwaerts");
        break;
    }
    return str;
}


void JALOUSIE::updateSHM(int JalNr)
{
    char newValue[16];
    char startTag[16];
    getPosition(JalNr, newValue);
    sprintf(startTag, "<posJal_%d>", JalNr);
    updateHelper(startTag, newValue);
    getDrvState(JalNr, newValue);
    sprintf(startTag, "<drvJal_%d>", JalNr);
    updateHelper(startTag, newValue);
    sendSharedMem();
}


int JALOUSIE::drvAufwaerts(int JalNr, int imptime)
{
    SingleLock slock(this, true);
    return pdrvAufwaerts(JalNr, imptime);
}

int JALOUSIE::drvAufwaerts(int JalNr)
{
    SingleLock slock(this, true);
    return pdrvAufwaerts(JalNr, FAHRZEIT + offset());
}

int JALOUSIE::toggleAufwaerts(int JalNr)
{
    SingleLock slock(this, true);
    if(drvstate[JalNr] == STOP)
    {
        return pdrvAufwaerts(JalNr, FAHRZEIT + offset());
    }
    else
    {
        return pstop(JalNr);
    }
}

int JALOUSIE::drvAbwaerts(int JalNr, int imptime)
{
    SingleLock slock(this, true);
    return pdrvAbwaerts(JalNr, imptime);
}

int JALOUSIE::drvAbwaerts(int JalNr)
{
    SingleLock slock(this, true);
    return pdrvAbwaerts(JalNr, FAHRZEIT + offset());
}

int JALOUSIE::toggleAbwaerts(int JalNr)
{
    SingleLock slock(this, true);
    if(drvstate[JalNr] == STOP)
    {
        return pdrvAbwaerts(JalNr, FAHRZEIT + offset());
    }
    else
    {
        return pstop(JalNr);
    }
}

int JALOUSIE::drvSonne(int JalNr)
{
    SingleLock slock(this, true);
    if(drvstate[JalNr] != STOP)
    {
        pstop(JalNr);
    }
    if(position[JalNr] == AB)
    {
        return drv(JalNr, AUF, SONNENZEIT, false); // schon geschlossen, also nur noch auf Luecke stellen
    }
    else
    {
        return drv(JalNr, AB, FAHRZEIT + offset(), true); // erstmal Schliessen
    }
}

int JALOUSIE::stop(int JalNr)
{
    SingleLock slock(this, true);
    return pstop(JalNr);
}

//da alle public-Funktionen threadsicher sein muessen, kommen hier die Privaten
int JALOUSIE::pdrvAufwaerts(int JalNr, int imptime)
{
    if(drvstate[JalNr] == AUF)
    {
        return 0;
    }
    if(drvstate[JalNr] == AB)
    {
        pstop(JalNr);
    }
    if(position[JalNr] == AUF)
    {
        return 0;
    }
    return drv(JalNr, AUF, imptime, false);
}

int JALOUSIE::pdrvAbwaerts(int JalNr, int imptime)
{
    if(drvstate[JalNr] == AB)
    {
        return 0;
    }
    if(drvstate[JalNr] == AUF)
    {
        pstop(JalNr);
    }
    if(position[JalNr] == AB)
    {
        return 0;
    }
    return drv(JalNr, AB, imptime, false);
}

int JALOUSIE::pstop(int JalNr)
{
    if(drvstate[JalNr] != STOP)
    {
        drvstate[JalNr] = STOP;
        printLog("***Jalousie Nr. %d: STOP", JalNr);
        //hier muss der Fahrbefehl rein
        max->set((unsigned char)(2*JalNr), false);
        max->set((unsigned char)(2*JalNr + 1), false);
        updateSHM(JalNr);
        //
        pthread_cancel(thr_id[JalNr]);
        pthread_join(thr_id[JalNr], NULL);
    }
    return 0;
}

int JALOUSIE::drv(int JalNr, int direction, long fahrZeit, bool Sonne)
{
    struct thr_jalData *timerData;
    timerData = (struct thr_jalData *)malloc(sizeof(struct thr_jalData));
    timerData->zeit = fahrZeit;
    timerData->JalNr = JalNr;
    timerData->direction = direction;
    timerData->Sonne = Sonne;
    timerData->jal = this;
    position[JalNr] = UNDEF; //da ein Fahrbefehl kommt, ist die Position undefiniert, bis ein Endzustand sicher erreicht wurde (Timer ist abgelaufen)
    if(pthread_create(&thr_id[JalNr], NULL, jalTimer, timerData) != 0)
    {
        printErr(errno, "JalTimer-Thread Nr. %d konnte nicht gestartet werden.", JalNr);
        return -1;
    }
    pthread_detach(thr_id[JalNr]);
    drvstate[JalNr] = direction;
    if(direction == AUF)
    {
        printLog("***Jalousie Nr. %d: AUFWAERTS", JalNr);
        //hier muss der Fahrbefehl rein
        max->set((unsigned char)(2*JalNr + 1), true);
        //
    }
    else
    {
        printLog("***Jalousie Nr. %d: ABWAERTS", JalNr);
        //hier muss der Fahrbefehl rein
        max->set((unsigned char)(2*JalNr), true);
        //
    }
    updateSHM(JalNr);
    return 0;
}

int JALOUSIE::offset()
{
    offset_count++;
    if (offset_count == 4)
        offset_count = 0;
    return offset_count * 800000;
}

//Timer-Thread fuer Jalousie
void *jalTimer(void *arg)
{
    struct thr_jalData *timerData = (struct thr_jalData *)arg;

    pthread_cleanup_push(cleanupJalTimer, (void *)timerData);
    if(verbose)
        printLog("Timer-Thread fuer Jalousie Nr. %d gestartet", timerData->JalNr);
    usleep(timerData->zeit);
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    if(timerData->zeit >= FAHRZEIT)
    {
        timerData->jal->setPosition(timerData->JalNr, timerData->direction); //Timer fuer gesamte FAHRZEIT ist abgelaufen, also wird angenommem, dass auch die Endposition erreicht wurde
    }
    if(timerData->Sonne)
    {
        timerData->jal->drvSonne(timerData->JalNr);
    }
    else
    {
        timerData->jal->stop(timerData->JalNr);
    }
    pthread_cleanup_pop(1);
    return NULL;
}

//Exithandler fuer Jalousie-Timer-Thread
void cleanupJalTimer(void *arg)
{
    struct thr_jalData *timerData = (struct thr_jalData *)arg;
    if(verbose)
        printLog("Timer-Thread fuer Jalousie Nr. %d ordnungsgemaess beendet", timerData->JalNr);
    free(timerData);
}
