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


#include "temp.h"

Temp::Temp(DS1631 *io, const char *tag):sharedMem()
{
    ds = io;
    strcpy(strTag, tag);
    init();
    if(pthread_create(&thr_id, NULL, tempTimer, this) != 0)
    {
        printErr(errno, "Temperatur-Timer-Thread konnte nicht gestartet werden.");
    }
    pthread_detach(thr_id);
    updateSHM();
}

int Temp::init()
{
    temperature = 0;
    if(ds->init() == 0)
    {
        printLog("Temperatursensor \"%s\" aktivert.", strTag);
    }
    else
    {
        printLog("Temperatursensor \"%s\" konnte nicht aktivert wertden.", strTag);
        return(1);
    }
    update();
    return(0);
}

double Temp::getTempDbl(void)
{
    SingleLock slock(this, true);
    return(temperature);
}

int Temp::getTemp(void)
{
    SingleLock slock(this, true);
    return((int)floor(temperature * 10 + 0.5));
}

void Temp::update(void)
{
    SingleLock slock(this, true);
    temperature = ds->read();
    printLogVerbose(1, "Temperatursensor ausgelesen: %1.1f", temperature);
    updateSHM();
}

void *tempTimer(void *arg)
{
    Temp *temp = (Temp *)arg;
    while(1)
    {
        usleep(TEMP_INTERVAL);
        temp->update();
        if(verbose && verbose_level > 0) printLog("Temperatursensor ausgelesen.");
    }
    return NULL;
}

void Temp::updateSHM()
{
    char newValue[20];
    char startTag[20];
    sprintf(startTag, "<%s>", strTag);
    sprintf(newValue, "%1.1f", temperature);
    updateHelper(startTag, newValue);
    sendSharedMem();
}

