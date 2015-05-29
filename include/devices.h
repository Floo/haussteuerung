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
#include <sys/errno.h>
#include <mqueue.h>


#ifndef DEVICES_H
#define DEVICES_H

#include "hs_config.h"
#include "threadsafe.h"
#include "pcf8574.h"
#include "max7311.h"
#include "log.h"
#include "temp.h"
#include "sharedMem.h"
#include "iniparser.h"
#include "jalousie.h"

#define R_1h 1
#define R_24h 2
#define R_7d 3
#define R_48h 4
#define R_72h 5

extern char *hslightpath;
extern char *confpath;
extern int conf_sem_ID;
//extern MsgQueue *msgQueue;
extern char *msg_name;

struct sdata_KS
{
    double temp;
    double feuchte;
    double wind;
    int cntRegen;
    int rec;
    time_t stamp;
};

struct sdata_AS
{
    double hell;
    int rec;
    time_t stamp;
};

class Wetter : public Threadsafe, protected sharedMem
{
private:
    struct sdata_KS last_KS;
    struct sdata_KS buf_KS[6];
    struct sdata_KS avg_KS;
    struct sdata_AS last_AS;
    struct sdata_AS buf_AS[6];
    struct sdata_AS avg_AS;
    int cnt_KSBuf, cnt_ASBuf, cnt_Regen_1h, cnt_Regen_1d;
    void init();
    bool regen;
	char *wetterpath;
	char *regenpath;
    int regenmenge_1h[24];
    time_t drvTimeStamp;
    bool drvTimer;
    bool closeOnWetter;
public:
    Wetter(char *chr_wetterpath, char *chr_regenpath):sharedMem()
    {
		wetterpath = chr_wetterpath;
		regenpath = chr_regenpath;
        init();
    };
    void updateKS(char *udpPaket);
    void updateAS(char *udpPaket);
    void average(void);
    void refreshRegenmenge(int hour);
    void save();
    void save_regen_1h();
    void save_regen_1d();
    double getTemp(void)
    {
        return(last_KS.temp);
    };
    double getFeuchte(void)
    {
        return(last_KS.feuchte);
    };
    double getWind(void)
    {
        return(last_KS.wind);
    };
    double getRegen(int intervall);
    double getHell(void)
    {
        return(last_AS.hell);
    };
    time_t getStampWetter(void)
    {
        return(last_KS.stamp);
    };
    time_t getStampHell(void)
    {
        return(last_AS.stamp);
    };
    void getStampWetter(char *str);
    void getStampHell(char *str);
    const char* getWetterSymbol();
    void WetterSteuerung(JALOUSIE *jal);
    void updateSHM();
};

class Hauscode
{
private:
    int tmphc1, tmphc2;
    void convertHauscode();
protected:
    int hc1;
    int hc2;
    int hc;
public:
    Hauscode();
    void setHauscode(int hauscode1, int hauscode2);
    int getHauscode();
    int getHauscode(int *hauscode1, int *hauscode2);
    int convertAddress(int Adresse);
    bool compareHauscode(int hauscode1, int hauscode2);
};

class RemoteFS20 : public Threadsafe, public Hauscode
{
private:
    pthread_t thr_id;
    E6TcpClient *e6tcp;
    MsgQueue *msgqueue;
public:
    RemoteFS20(E6TcpClient *ioE6);
    void setSzene(char *Szene);
    void setLampeStufe(char *Lampe, int HelligkeitsStufe); //Stufen von 0 (Aus) bis 16 (volle Helligkeit)
    void resetLampeTimer(char *Lampe); //Alle Timer fuer Lampe ruecksetzen
    void setLampeWert(char *Lampe, int HelligkeitsWert); // Helligkeit von 0...100%
    void setLampeBefehl(char *Lampe, int Befehl); //Befehle senden
    void toggleLampe(char *Lampe);
    void switchAlleLampen(bool value);
    void dimmLampeStart(char *Lampe, int DimmCmd); //dimmt Lampe entsprechend DimmCmd, Thread wird erzeugt, der alle 250 ms sendet
    void dimmLampeStop();//DimmThread wird beendet
};

void *dimmTimer(void *arg);
void cleanupDimmTimer(void *arg);

struct thr_dimmTimer{
    int hc;
    int addr;
    int cmd;
    E6TcpClient *e6tcp_p;
};



//#endif //USE_REC868
#endif //DEVICES_H



