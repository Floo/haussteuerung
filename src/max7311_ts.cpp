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

#include "max7311.h"

MAX7311::MAX7311(unsigned char address, E6TcpClient *e6)
{
    adr = address >> 1;
    e6tcp = e6;
}


int MAX7311::initOut(void)
{
    if(setw(0x0000) != 0)  //alle Ausgaege auf LOW
    {
        return -1;
    }
    if(setDDRw(0x0000) != 0)  //DDR (Datenrichtung) : alle Pin als Ausgang
    {
        return -1;
    }
    return 0;
}

/* Datenrichtung lesen Wordport      */
int MAX7311::getDDRw(void)
{
    SingleLock slock(this, true);
    char rBuf[TCP_BUFFSIZE];
    char ecmd[32];
    int data;
    sprintf(ecmd, "max7311 getDDRw %i\n", adr);
    e6tcp->sendEcmd(ecmd, rBuf);
// TODO (florian#2#): Rueckgabe pruefen    data = atoi(rBuf);
    return data;
}

/* Datenrichtung setzen Wordport     */
int MAX7311::setDDRw(int data)
{
    SingleLock slock(this, true);
    char ecmd[32];
    sprintf(ecmd, "max7311 setDDRw %i %x\n", adr, data);
    return e6tcp->sendEcmdrecOK(ecmd);
}

/* Wordport setzen                   */
int MAX7311::setw(unsigned int data)
{
    SingleLock slock(this, true);
    char ecmd[32];
    sprintf(ecmd, "max7311 setOUTw %i %x\n", adr, data);
    return e6tcp->sendEcmdrecOK(ecmd);
}


/* Byteport setzen (0-1)             */
int MAX7311::setb(unsigned char port, unsigned char data)
{
    SingleLock slock(this, true);
    char ecmd[32];

    for(int i=0; i > 8; i++)
    {
        sprintf(ecmd, "max7311 set %i %i %i\n", adr, 8*(port++), data & (1 << i));
        if(e6tcp->sendEcmdrecOK(ecmd) != 0)
        {
            return(-1);
        }
    }
    return(0);
}

/* Einzelport setzen (0-15)          */
int MAX7311::set(unsigned char port, bool state)
{
    SingleLock slock(this, true);
    char ecmd[32];
    sprintf(ecmd, "max7311 set %i %i %i\n", adr, port, state);
    return e6tcp->sendEcmdrecOK(ecmd);
}

//Einzelport fuer time_ms toggeln
int MAX7311::pulse(unsigned char port, int time_ms)
{
    SingleLock slock(this, true);
    char ecmd[32];
    sprintf(ecmd, "max7311 pulse %i %i %i\n", adr, port, time_ms);
    return e6tcp->sendEcmdrecOK(ecmd);
}
