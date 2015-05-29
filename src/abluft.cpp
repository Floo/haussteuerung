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

#include "abluft.h"

Abluft::Abluft(PCF8574 *io, E6TcpClient *ioE6):sharedMem()
{
    pcf = io;
    e6tcp = ioE6;
    init();
}

void Abluft::init()
{
    dictionary *conf;
    semop(conf_sem_ID, &LOCK, 1);
    conf = iniparser_load(confpath);
    if(iniparser_getboolean(conf, "a/c:low", 0))
    {
        state = AUS;
        low();
    }
    else
    {
        state = LOW;
        aus();
    }
    iniparser_freedict(conf);
    semop(conf_sem_ID, &UNLOCK, 1);
}


void Abluft::toggleLow()
{
    bool lowstate;
    dictionary *conf;
    semop(conf_sem_ID, &LOCK, 1);
    conf = iniparser_load(confpath);
    lowstate = iniparser_getboolean(conf, "a/c:low", 0);
    iniparser_freedict(conf);
    semop(conf_sem_ID, &UNLOCK, 1);
    setLow(!lowstate);
}

void Abluft::setLow(bool lowState)
{
    FILE *confFile;
    dictionary *conf;
    semop(conf_sem_ID, &LOCK, 1);
    conf = iniparser_load(confpath);
    if(lowState)
    {
        if(state != HIGH) low();
        iniparser_setstr(conf, (char*)"a/c:low", (char*)"true");
    }
    else
    {
        if(state != HIGH) aus();
        iniparser_setstr(conf, (char*)"a/c:low", (char*)"false");
    }
    confFile=fopen(confpath, "w");
    iniparser_dump_ini(conf, confFile);
    fclose(confFile);
    iniparser_freedict(conf);
    semop(conf_sem_ID, &UNLOCK, 1);
}

void Abluft::updateLow()
{
    if(state != HIGH)
    {
        dictionary *conf;
        semop(conf_sem_ID, &LOCK, 1);
        conf = iniparser_load(confpath);
        if(iniparser_getboolean(conf, "a/c:low", 0))
        {
            low();
        }
        else
        {
            aus();
        }
        iniparser_freedict(conf);
        semop(conf_sem_ID, &UNLOCK, 1);
    }
}

void Abluft::setHigh()
{
    if(state != HIGH) high();
}

int Abluft::aus(void)
{
    SingleLock slock(this, true);
    if(state != AUS)
    {
        state = AUS;
        printLog("***Abluft: AUS");
        //hier muss der Schaltbefehl rein
        pcf->set(ABLUFT_0, true);
        pcf->set(ABLUFT_1, false);
        //LED einschalten
        sprintf(ecmd, "pin set LED_RED 0\n");
        e6tcp->sendEcmd(ecmd);
        sprintf(ecmd, "pin set LED_GREEN 0\n");
        e6tcp->sendEcmd(ecmd);
        updateSHM();
    }
    return 0;
}

int Abluft::low(void)
{
    SingleLock slock(this, true);
    if(state != LOW)
    {
        state = LOW;
        printLog("***Abluft: LOW");
        //hier muss der Schaltbefehl rein
        pcf->set(ABLUFT_0, false);
        pcf->set(ABLUFT_1, false);
        //LED einschalten
        sprintf(ecmd, "pin set LED_RED 0\n");
        e6tcp->sendEcmd(ecmd);
        sprintf(ecmd, "pin set LED_GREEN 1\n");
        e6tcp->sendEcmd(ecmd);
        updateSHM();
    }
    return 0;
}


//Nachtriggern sollte moeglich sein!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
int Abluft::high(void)
{
    SingleLock slock(this, true);
    struct thr_abluftData *timerData;
    timerData = (struct thr_abluftData *)malloc(sizeof(thr_abluftData));
    dictionary *conf;
    semop(conf_sem_ID, &LOCK, 1);
    conf = iniparser_load(confpath);
    timerData->zeit = iniparser_getint(conf, "a/c:high_time", 0);
    iniparser_freedict(conf);
    semop(conf_sem_ID, &UNLOCK, 1);

    timerData->abluft = this;
    if(pthread_create(&thr_id, NULL, abluftTimer, timerData) != 0)
    {
        printErr(errno, "AbluftTimer-Thread konnte nicht gestartet werden.");
        return -1;
    }
    pthread_detach(thr_id);
    state = HIGH;
    printLog("***Abluft: HIGH");
    //hier muss der Schaltbefehl rein
    pcf->set(ABLUFT_0, true);
    pcf->set(ABLUFT_1, true);
    //LED einschalten
    sprintf(ecmd, "pin set LED_RED 1\n");
    e6tcp->sendEcmd(ecmd);
    sprintf(ecmd, "pin set LED_GREEN 0\n");
    e6tcp->sendEcmd(ecmd);
    updateSHM();
    return 0;
}

void Abluft::getState(char* str)
{
    switch(state)
    {
    case AUS:
        sprintf(str, "Aus");
        break;
    case LOW:
        sprintf(str, "Low");
        break;
    case HIGH:
        sprintf(str, "High");
        break;
    }
}


void Abluft::updateSHM()
{
    char newValue[8];
    char startTag[] = "<Abluft>";
    getState(newValue);
    updateHelper(startTag, newValue);
    sendSharedMem();
}


HWR::HWR(MAX7311 *io):sharedMem()
{
    max = io;
    state = AUS;
    init();
}

void HWR::init()
{
    if(state == AUS)
    {
        aus();
    }else{
        an();
    }
    updateSHM();
}

void HWR::an()
{
    SingleLock slock(this, true);
    printLog("***Abluft-HWR einschalten");
    state = AN;
    max->set(8, true);
    updateSHM();
}

void HWR::aus()
{
    SingleLock slock(this, true);
    printLog("***Abluft-HWR ausschalten");
    state = AUS;
    max->set(8, false);
    updateSHM();
}

void HWR::updateSHM()
{
    const char *newValue = NULL;
    const char startTag[] = "<HWR>";
    newValue = getState(newValue);
    updateHelper(startTag, newValue);
    sendSharedMem();
}

Orient::Orient(PCF8574 *io):sharedMem()
{
    pcf = io;
    state = AUS;
    init();
}

void Orient::init()
{
    if(state == AUS)
    {
        aus();
    }else{
        an();
    }
    updateSHM();
}

void Orient::an()
{
    SingleLock slock(this, true);
    printLog("***Orientierungslicht einschalten");
    state = AN;
    pcf->set(ORIENT, true);
    updateSHM();
}

void Orient::aus()
{
    SingleLock slock(this, true);
    printLog("***Orientierungslicht ausschalten");
    state = AUS;
    pcf->set(ORIENT, false);
    updateSHM();
}

void Orient::updateSHM()
{
    const char *newValue = NULL;
    char startTag[] = "<Orient>";
    newValue = getState(newValue);
    updateHelper(startTag, newValue);
    sendSharedMem();
}


void *abluftTimer(void *arg)
{
    dictionary *conf;
    struct thr_abluftData *timerData = (struct thr_abluftData *)arg;
    if(verbose)
        printLog("Abluft-Thread gestartet");
    sleep(timerData->zeit);
    semop(conf_sem_ID, &LOCK, 1);
    conf = iniparser_load(confpath);
    if(iniparser_getboolean(conf, "a/c:low", 0))
    {
        timerData->abluft->low();
    }
    else
    {
        timerData->abluft->aus();
    }
    iniparser_freedict(conf);
    semop(conf_sem_ID, &UNLOCK, 1);
    free(timerData);
    if(verbose)
        printLog("Timer-Thread fuer Abluft ordnungsgemaess beendet");
    return NULL;
}

void HsGui::resetDevice()
{
    printLog("Resetting GUI");
    pcf->set(HSGUI_RESET, true);
    usleep(200000);
    pcf->set(HSGUI_RESET, false);
}

void HsGui::switchPower()
{
    pcf->set(HSGUI_POWER, true);
    usleep(200000);
    pcf->set(HSGUI_POWER, false);
}

void HsGui::init()
{
    pcf->set(HSGUI_POWER, false);
    pcf->set(HSGUI_RESET, false);
}

Terrasse::Terrasse(PCF8574 *io):sharedMem()
{
    pcf = io;
    stateVentil_1 = AUS;
    stateVentil_2 = AUS;
    stateSteckdose = AUS;
    init();
    updateSHM();
}

void Terrasse::init()
{
    SingleLock slock(this, true);
    if(stateVentil_1 == AUS)
        pcf->set(VENTIL_1, AUS);
    else
        pcf->set(VENTIL_1, AN);
    if(stateVentil_2 == AUS)
        pcf->set(VENTIL_2, AUS);
    else
        pcf->set(VENTIL_2, AN);
    if(stateSteckdose == AUS)
        pcf->set(STECKDOSE, AUS);
    else
        pcf->set(STECKDOSE, AN);
}

void Terrasse::setVentil(int number, int state)
{
    if(number == VENTIL_1)
    {
        setVentil_1(state);
    }
    if(number == VENTIL_2)
    {
        setVentil_2(state);
    }
}

void Terrasse::setVentil_1(int state)
{
    if(state != TIMER_AN)
        SingleLock slock(this, true);
    if(state == AN && stateVentil_1 == TIMER_AN)
        return;
    if(stateVentil_1 != state)
    {
        if(state == AUS)
        {
            printLog("***Ventil Nr. 1: SCHLIESSEN");
            if(stateVentil_1 == TIMER_AN)
            {
                pthread_cancel(thr_id[VENTIL_1]);
                pthread_join(thr_id[VENTIL_1], NULL);
            }
            pcf->set(VENTIL_1, AUS);
        }
        else
        {
            printLog("***Ventil Nr. 1: OEFFNEN");
            pcf->set(VENTIL_1, AN);
        }
        stateVentil_1 = state;
        updateSHM();
    }
}

void Terrasse::setVentil_2(int state)
{
    if(state != TIMER_AN)
        SingleLock slock(this, true);
    if(state == AN && stateVentil_2 == TIMER_AN)
        return;
    if(stateVentil_2 != state)
    {
        if(state == AUS)
        {
            printLog("***Ventil Nr. 2: SCHLIESSEN");
            if(stateVentil_2 == TIMER_AN)
            {
                pthread_cancel(thr_id[VENTIL_2]);
                pthread_join(thr_id[VENTIL_2], NULL);
            }
            pcf->set(VENTIL_2, AUS);
        }
        else
        {
            printLog("***Ventil Nr. 2: OEFFNEN");
            pcf->set(VENTIL_2, AN);
        }
        stateVentil_2 = state;
        updateSHM();
    }
}

void Terrasse::setSteckdose(int state)
{
    SingleLock slock(this, true);
    if(stateSteckdose != state)
    {
        stateSteckdose = state;
        if(state == AUS)
        {
            pcf->set(STECKDOSE, AUS);
            printLog("***Steckdose Terrasse AUS");
        }else{
            pcf->set(STECKDOSE, AN);
            printLog("***Steckdose Terrasse AN");
        }
        updateSHM();
    }
}

void Terrasse::getStateVentil_1(char* str)
{
    switch(stateVentil_1)
    {
        case(AN):
            sprintf(str, "An");
            break;
        case(AUS):
            sprintf(str, "Aus");
            break;
        case(TIMER_AN):
            sprintf(str, "Timer");
            break;
    }
}

void Terrasse::getStateVentil_2(char* str)
{
    switch(stateVentil_2)
    {
        case(AN):
            sprintf(str, "An");
            break;
        case(AUS):
            sprintf(str, "Aus");
            break;
        case(TIMER_AN):
            sprintf(str, "Timer");
            break;
    }
}

void Terrasse::getStateSteckdose(char* str)
{
    switch(stateSteckdose)
    {
        case(AN):
            sprintf(str, "An");
            break;
        case(AUS):
            sprintf(str, "Aus");
            break;
    }
}

void Terrasse::startVentilTimer(int index)
{
    dictionary *conf;
    int dauer = 0;
    semop(conf_sem_ID, &LOCK, 1);
    conf = iniparser_load(confpath);
    if(index == VENTIL_1)
        dauer = iniparser_getint(conf, "irrigation:ventil_1_duration", 0);
    if(index == VENTIL_2)
        dauer = iniparser_getint(conf, "irrigation:ventil_2_duration", 0);
    iniparser_freedict(conf);
    semop(conf_sem_ID, &UNLOCK, 1);
    if(dauer > 0)
        startVentilTimer(index, dauer);
}

void Terrasse::startVentilTimer(int index, int dauer)
{
    SingleLock slock(this, true);
    if(dauer < 1)
        return;
    if((index == VENTIL_1) && (stateVentil_1 != AUS))
        return;
    if((index == VENTIL_2) && (stateVentil_2 != AUS))
        return;
    struct thr_ventilData *timerData;
    timerData = (struct thr_ventilData *)malloc(sizeof(struct thr_ventilData));
    timerData->zeit = dauer * 60;
    timerData->VentilNr = index;
    timerData->ter = this;
    if(pthread_create(&thr_id[index], NULL, ventilTimer, timerData) != 0)
    {
        printErr(errno, "VentilTimer-Thread Nr. %d konnte nicht gestartet werden.", index + 1);
        return;
    }
    pthread_detach(thr_id[index]);
    setVentil(index, TIMER_AN);
}

void Terrasse::updateSHM()
{
    char newValue[8];
    char startTag[12];
    sprintf(startTag, "<Ventil_1>");
    getStateVentil_1(newValue);
    updateHelper(startTag, newValue);
    sprintf(startTag, "<Ventil_2>");
    getStateVentil_2(newValue);
    updateHelper(startTag, newValue);
    sprintf(startTag, "<Steckdose>");
    getStateSteckdose(newValue);
    updateHelper(startTag, newValue);
    sendSharedMem();
}

//Timer-Thread fuer Ventile
void *ventilTimer(void *arg)
{
    struct thr_ventilData *timerData = (struct thr_ventilData *)arg;

    pthread_cleanup_push(cleanupVentilTimer, (void *)timerData);
    if(verbose)
        printLog("Timer-Thread fuer Ventil Nr. %d gestartet", timerData->VentilNr + 1);
    sleep(timerData->zeit);
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    timerData->ter->setVentil(timerData->VentilNr, AUS);
    pthread_cleanup_pop(1);
    return NULL;
}

//Exithandler fuer Ventil-Timer-Thread
void cleanupVentilTimer(void *arg)
{
    struct thr_ventilData *timerData = (struct thr_ventilData *)arg;
    if(verbose)
        printLog("Timer-Thread fuer Ventil Nr. %d ordnungsgemaess beendet", timerData->VentilNr + 1);
    free(timerData);
}

