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


#include "ds1631.h"

DS1631::DS1631(unsigned char address, E6TcpClient *e6){
	e6tcp = e6;
	adr = (address & 0x0E) >> 1;
}

// Initialisierung im 12bit-Mode
int DS1631::init(void){
    SingleLock slock(this, true);
    char ecmd[32];
    sprintf(ecmd, "ds1631 start %i\n", adr);
    return e6tcp->sendEcmdrecOK(ecmd);
}

double DS1631::read(void){
    SingleLock slock(this, true);
    char rBuf[TCP_BUFFSIZE];
    char ecmd[32];
    sprintf(ecmd, "ds1631 temp %i\n", adr);
// TODO (florian#1#): Fehlerbehandlung bei allen sendEcmd() Aufrufen einfügen; =0 ist fehlerfrei; <>0 Kommunikationsfehler
// hier ist Fehlerbehandlung schon eingefuegt: 19.09.2010
    int ret = e6tcp->sendEcmd(ecmd, rBuf);
    if((atof(rBuf) == 0 && rBuf[0] != '0') || ret != 3) //ret == 3: RET_BUFFER: im Buffer (rBuf) steht ein gültiger Wert
    {
        printErr(EBADMSG, "Fehler beim Lesen des Tempsensors. ECMD: ds1631 temp %i", adr);
        return(0);
    }
    return atof(rBuf);
}

int DS1631::stop(void){
  SingleLock slock(this, true);
  char ecmd[32];
  sprintf(ecmd, "ds1631 stop %i\n", adr);
  return e6tcp->sendEcmdrecOK(ecmd);
}


