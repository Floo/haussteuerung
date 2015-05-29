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

#include "pcf8574.h"

PCF8574::PCF8574(unsigned char address, E6TcpClient *e6)
{
    e6tcp = e6;
    adr = (address & 0x0E) >> 1;
    data = 255;
}

// einfach nur ein  Start und Stop senden, um zu testen ob Geraet vorhanden, aktuelen Schaltzustand der Ausg�ge abfragen
// Returns: 0 wenn Geraet antwortet,  <> 0 wenn Geraet nicht vorhanden
int PCF8574::init(void)
{
    char ecmd[32];
    sprintf(ecmd, "pcf8574x set %i 0 %x\n", adr, data);
    return(e6tcp->sendEcmd(ecmd));
}

//Port setzen/rcksetzen
//Input:	port	0...7
//		state	True/False
//Returns:	0 wenn Geraet antwortet,  <> 0 wenn Geraet nicht vorhanden
int PCF8574::set(unsigned char port, bool state)
{
    if(state)
    {
        return on(port);
    }
    else
    {
        return off(port);
    }
}

//Port umschalten
//Input:	port	0...7
//Returns:	0 wenn Geraet antwortet,  <> 0 wenn Geraet nicht vorhanden
int PCF8574::toggle(unsigned char port)
{
    SingleLock slock(this, true);
    //unsigned char data;
    data ^= (1 << port);
    char ecmd[32];
    sprintf(ecmd, "pcf8574x set %i 0 %x\n", adr, data);
    return(e6tcp->sendEcmd(ecmd));
}

//Port setzen
//Input:	port	0...7
//Returns:	0 wenn Geraet antwortet,  <> 0 wenn Geraet nicht vorhanden
int PCF8574::off(unsigned char port)
{
    SingleLock slock(this, true);
    //unsigned char data;
    data |= (1 << port);
    char ecmd[32];
    sprintf(ecmd, "pcf8574x set %i 0 %x\n", adr, data);
    return(e6tcp->sendEcmd(ecmd));
}

//Port rcksetzen
//Input:	port	0...7
//Returns:	0 wenn Geraet antwortet,  <> 0 wenn Geraet nicht vorhanden
int PCF8574::on(unsigned char port)
{
    SingleLock slock(this, true);
    //unsigned char data;
    data &= ~(1 << port);
    char ecmd[32];
    sprintf(ecmd, "pcf8574x set %i 0 %x\n", adr, data);
    return(e6tcp->sendEcmd(ecmd));
}

//Port einlesen
//Input:	port	0...7
//Returns:	True wenn Port HIGH,  False wenn Port LOW
int PCF8574::get(unsigned char port)
{
    unsigned char ret;
    char rBuf[TCP_BUFFSIZE];
    char ecmd[32];
    //Port auf HIGH setzen
    if(on(port))
    {
        return 1;
    }
    sprintf(ecmd, "pcf8574x get %i 0\n", adr);
    e6tcp->sendEcmd(ecmd, rBuf);
// TODO (florian#2#): Rueckgabe pruefen        ret = (unsigned char)atoi(rBuf);
    //alle Ausgaege auf urspruenglichen Wert zuruecksetzen
    if(out(data))
    {
        return 1;
    }
    return(ret & (1 << port));
}

//Byte einlesen
//Returns:	Datenbyte
unsigned char PCF8574::in(void)
{
    unsigned char ret;
    char rBuf[TCP_BUFFSIZE];
    char ecmd[32];
    //alle Ausgaege auf HIGH setzen, damit gelesen werden kann
    if(out(0xFF))
    {
        return 1;
    }
    sprintf(ecmd, "pcf8574x get %i 0\n", adr);
    e6tcp->sendEcmd(ecmd, rBuf);
// TODO (florian#2#): Rueckgabewert pruefen
    ret = (unsigned char)atoi(rBuf);
    //alle Ausgaege auf urspruenglichen Wert zuruecksetzen
    if(out(data))
    {
        return 1;
    }
    return(ret);
}

//Byte ausgeben
//Input:	data Datenbyte
//Returns:	0 wenn Geraet antwortet,  <> 0 wenn Geraet nicht vorhanden
int PCF8574::out(unsigned char databyte)
{
    SingleLock slock(this, true);
    char ecmd[32];
    sprintf(ecmd, "pcf8574x set %i 0 %x\n", adr, databyte);
    return(e6tcp->sendEcmd(ecmd));
}
