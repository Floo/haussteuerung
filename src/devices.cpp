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


#include "devices.h"

extern Temp *tempWohnen;

void Wetter::init()
{
    int i, c;
	FILE *file_id;
	char strLine[64];

    cnt_ASBuf = 0;
    cnt_KSBuf = 0;
    cnt_Regen_1h = 0;
    cnt_Regen_1d = 0;
	//erstmal initialisieren
	last_KS.temp = 0.0;
    last_KS.wind = 0.0;
    last_KS.feuchte = 50;
    last_AS.hell = 0;
    last_KS.cntRegen = 0;
    //Vorgaben aus der wetter.dbk laden
    file_id = fopen(wetterpath, "r");
	if (file_id != NULL)
	{
		fseek(file_id, -50, SEEK_END);
		fgets(strLine, 64, file_id);
		c = fgetc(file_id);
		if(c != '#')
		{
			fscanf(file_id, "%*d %lf %lf %lf %d %*d %lf", &last_KS.temp, &last_KS.feuchte, &last_KS.wind, &last_KS.cntRegen, &last_AS.hell);
		}
		fclose(file_id);
	}

    last_KS.stamp = time(NULL);
    last_AS.stamp = time(NULL);
    for(i=0; i < 24; i++) regenmenge_1h[i] = last_KS.cntRegen;

    updateSHM();
    drvTimer = false;
    closeOnWetter = false;
    regen = false;
    drvTimeStamp = time(NULL);
}

void Wetter::getStampWetter(char *str)
{
    strcpy(str, ctime(&last_KS.stamp));
}

void Wetter::getStampHell(char *str)
{
    strcpy(str, ctime(&last_AS.stamp));
}

const char* Wetter::getWetterSymbol()
{
    if(regen)
        return "Regen.png";
    if(last_AS.hell < -200)
        return "Nacht.png";
    if(last_AS.hell > 680)
        return "Sonne.png";
    return "Wolke.png";
}

void Wetter::updateKS(char *udpPaket)
{
    char strValue[5];
    strncpy(strValue, strstr(udpPaket, "TEMP:")+5, 4);
    strValue[4] = '\0';
    last_KS.temp = (atof(strValue))/10;
    strncpy(strValue, strstr(udpPaket, "HUMID:")+6, 2);
    strValue[2] = '\0';
    last_KS.feuchte = atof(strValue);
    strncpy(strValue, strstr(udpPaket, "WIND:")+5, 3);
    strValue[3] = '\0';
    last_KS.wind = (atof(strValue))/10;
    strncpy(strValue, strstr(udpPaket, "RAINCOUNT:")+10, 4);
    strValue[4] = '\0';
    last_KS.cntRegen = atoi(strValue);
    strncpy(strValue, strstr(udpPaket, "RAIN:")+5, 1);
    strValue[1] = '\0';
    regen = (bool)atoi(strValue);
    last_KS.stamp = time(NULL);
    buf_KS[cnt_KSBuf].temp = last_KS.temp;
    buf_KS[cnt_KSBuf].feuchte = last_KS.feuchte;
    buf_KS[cnt_KSBuf].wind = last_KS.wind;
    cnt_KSBuf++;
    if(cnt_KSBuf > 5)cnt_KSBuf = 5; //Ueberlauf verhindern
    updateSHM();
}

void Wetter::updateAS(char *udpPaket)
{
    char strValue[5];
    strncpy(strValue, strstr(udpPaket, "TEMP:")+5, 4);
    strValue[4] = '\0';
    last_AS.hell = atoi(strValue);
    last_AS.stamp = time(NULL);
    buf_AS[cnt_ASBuf].hell = last_AS.hell;
    cnt_ASBuf++;
    if(cnt_ASBuf > 5)cnt_ASBuf = 5; //Ueberlauf verhindern
    updateSHM();
}

void Wetter::average(void)
{
    avg_KS.feuchte = 0;
    avg_KS.temp = 0;
    avg_KS.wind = 0;
    for(int i = 0; i < cnt_KSBuf; i++)
    {
        avg_KS.feuchte += buf_KS[i].feuchte;
        avg_KS.temp += buf_KS[i].temp;
        if(avg_KS.wind < buf_KS[i].wind)
            avg_KS.wind = buf_KS[i].wind; // Maximum suchen
    }
    avg_KS.rec = cnt_KSBuf;
    avg_KS.feuchte = avg_KS.feuchte / cnt_KSBuf;
    avg_KS.temp = avg_KS.temp / cnt_KSBuf;
    avg_KS.cntRegen = last_KS.cntRegen;
    avg_AS.hell = 0;
    for(int i = 0; i < cnt_ASBuf; i++)
    {
        avg_AS.hell += buf_AS[i].hell;
    }
    avg_AS.hell = avg_AS.hell / cnt_ASBuf;
    avg_AS.rec = cnt_ASBuf;
    cnt_ASBuf = 0;
    cnt_KSBuf = 0;
}

double Wetter::getRegen(int intervall)
{
    int foo_1, foo_2;
	time_t time_1;
	int time_2, cntRegen, lineCount;
	char strLine[64];
	char *tempWetterpath;
	char *posExtension;
	time_1 = time(NULL);
	FILE *file_id;
    switch(intervall)
    {
    case R_1h:
        if (cnt_Regen_1h > 0)
        {
            foo_1 = cnt_Regen_1h;
            foo_2 = cnt_Regen_1h - 1;
        }
        else
        {
            foo_1 = 0;
            foo_2 = 23;
        }
        if (regenmenge_1h[foo_2] == 0)
            return 0;
        else
            return REGENFAKTOR * (regenmenge_1h[foo_1] - regenmenge_1h[foo_2]);
    case R_24h:
		time_1 -= 24 * 3600;
		break;
	case R_48h:
		time_1 -= 48 * 3600;
		break;
	case R_72h:
		time_1 -= 72 * 3600;
		break;
    case R_7d:
		time_1 -= 7 * 24 * 3600;
		break;
	}

	lineCount = 0;
	file_id = fopen(wetterpath, "r"); //wetter.dbk
	if(file_id == NULL) return 0;
	//Testen, ob wetter.dbk den gesuchten Zeitpunkt enthaelt oder in wetter._01 gesucht werden muss
	while(!feof(file_id) && lineCount < 2)
	{
		fgets(strLine, 64, file_id);
		lineCount++;
		if(strchr(strLine,'#') == NULL)
		{
			sscanf(strLine, "%d", &time_2);
			if(time_2 > (int)time_1)
			{
				fclose(file_id);
				tempWetterpath = (char*)malloc(strlen(wetterpath) * sizeof(char));
				strcpy(tempWetterpath, wetterpath);
				posExtension = strstr(tempWetterpath, ".dbk");
				strncpy (posExtension, "._01", 4);
				file_id = fopen(tempWetterpath, "r"); //wetter._01
				free(tempWetterpath);
				if(file_id == NULL) return 0;
			}
        break;
		}
	}
	while(!feof(file_id))
	{
		fgets(strLine, 64, file_id);
		if(strchr(strLine,'#') == NULL)
		{
			sscanf(strLine, "%d %*f %*f %*f %d", &time_2, &cntRegen);
			if(time_2 > (int)time_1)
			{
				fclose(file_id);
				if(last_KS.cntRegen < cntRegen){
                    return REGENFAKTOR * (last_KS.cntRegen + 4096 - cntRegen);
                }else{
                    return REGENFAKTOR * (last_KS.cntRegen - cntRegen);
                }
			}
		}
	}
	fclose(file_id);
    return 0;
}

void Wetter::refreshRegenmenge(int hour)
{
    cnt_Regen_1h++;
    if(cnt_Regen_1h > 23) cnt_Regen_1h = 0;
    regenmenge_1h[cnt_Regen_1h] = last_KS.cntRegen;
}

void Wetter::save()
{
    time_t tmp_time;
    FILE *file_id;
    file_id = fopen(wetterpath, "ab");
    tmp_time = time(NULL);
    fprintf(file_id, "#%s %ld %.1f %.0f %.1f %d %d %.0f %d %.1f\n",
            ctime(&tmp_time), tmp_time, avg_KS.temp, avg_KS.feuchte, avg_KS.wind, last_KS.cntRegen, avg_KS.rec, avg_AS.hell, avg_AS.rec, tempWohnen->getTempDbl());
    fclose(file_id);
}

void Wetter::save_regen_1d()
{
    time_t tmp_time;
    FILE *file_id;
    regenpath[strlen(regenpath) - 5] = 'd';
    file_id = fopen(regenpath, "ab");
    tmp_time = time(NULL);
    fprintf(file_id, "#%s %ld %1.1f\n", ctime(&tmp_time), tmp_time, getRegen(R_24h));
    fclose(file_id);
}

void Wetter::save_regen_1h()
{
    time_t tmp_time;
    FILE *file_id;
    regenpath[strlen(regenpath) - 5] = 'h';
    file_id = fopen(regenpath, "ab");
    tmp_time = time(NULL);
    fprintf(file_id, "#%s %ld %1.1f\n", ctime(&tmp_time), tmp_time, getRegen(R_1h));
    fclose(file_id);
}

void Wetter::updateSHM()
{
    char newValue[16];
    char startTag[20];
    char stamp[30];

    sprintf(startTag, "<EmpfangWetter>");
    getStampWetter(stamp);
    updateHelper(startTag, stamp);
    sprintf(startTag, "<EmpfangHell>");
    getStampHell(stamp);
    updateHelper(startTag, stamp);
    sprintf(startTag, "<TempAussen>");
    sprintf(newValue, "%1.1f", getTemp());
    updateHelper(startTag, newValue);
    sprintf(startTag, "<Feuchte>");
    sprintf(newValue, "%1.0f", getFeuchte());
    updateHelper(startTag, newValue);
    sprintf(startTag, "<Wind>");
    sprintf(newValue, "%1.1f", getWind());
    updateHelper(startTag, newValue);
    sprintf(startTag, "<Regen_1h>");
    sprintf(newValue, "%1.1f", getRegen(R_1h));
    updateHelper(startTag, newValue);
    sprintf(startTag, "<Regen_24h>");
    sprintf(newValue, "%1.1f", getRegen(R_24h));
    updateHelper(startTag, newValue);
    sprintf(startTag, "<Regen_7d>");
    sprintf(newValue, "%1.1f", getRegen(R_7d));
    updateHelper(startTag, newValue);
    sprintf(startTag, "<Symbol>");
    updateHelper(startTag, getWetterSymbol());
    sendSharedMem();
}


void Wetter::WetterSteuerung(JALOUSIE *jal)
{
    bool wetter;
    bool wind;
    bool rain;
    bool jal2;
    bool closeToSun;
    int hystTime;
    int thrsHellOpen;
    int thrsHellClose;
    int thrsWind;
    int thrsTemp;
    bool jalauf;
    bool jalzu;
    bool fahren = false;
    int i;
    dictionary *conf;
    //wenn es grad faehrt mache nix
    if((jal->getDrvState(0) != STOP) || (jal->getDrvState(1) != STOP) || (jal->getDrvState(2) != STOP) || (jal->getDrvState(3) != STOP)) return;
    //Konfigs laden
    semop(conf_sem_ID, &LOCK, 1);
    conf = iniparser_load(confpath);
    wetter = iniparser_getboolean(conf, "blinds:weather", 0);
    wind = iniparser_getboolean(conf, "blinds:wind_protection", 0);
    rain = iniparser_getboolean(conf, "blinds:open_on_rain", 0);
    jal2 = !iniparser_getboolean(conf, "blinds:jal_2_open", 0);
    closeToSun = iniparser_getboolean(conf, "blinds:close_to_sun", 0);
    hystTime = iniparser_getint(conf, "blinds:hytersis_time", 0);
    thrsHellOpen = iniparser_getint(conf, "blinds:brightness_thrs_open", 0);
    thrsHellClose = iniparser_getint(conf, "blinds:brightness_thrs_close", 0);
    thrsWind= iniparser_getint(conf, "blinds:wind_speed_thrs", 0);
    thrsTemp = iniparser_getint(conf, "blinds:outside_temp_thrs", 0);
    iniparser_freedict(conf);
    semop(conf_sem_ID, &UNLOCK, 1);
    //feststellen, wo die Jals stehen
    jalauf = ((jal->getPosition(0) == AUF) && (jal->getPosition(1) == AUF) && (jal->getPosition(2) == AUF) && (jal->getPosition(3) == AUF));
    jalzu = ((jal->getPosition(0) == AB) && (jal->getPosition(2) == AB) && (jal->getPosition(3) == AB));
    if(jal2) jalzu = jalzu && (jal->getPosition(1) == AB);
    if(!jalzu) closeOnWetter = false; //
    //wenn nicht alle ganz auf sind, Sicherheitsmechanismen und Helligkeit auswerten
    //besser gemittelte Werte fuer Helligkeit und Temp nehmen)
    if(!jalauf)
    {
        if(wind && (last_KS.wind > thrsWind))
        {
            printLog("Wettersteuerung: Jalousien oeffnen (starker Wind)");
            fahren = true;
        }
        if(rain && regen)
        {
            printLog("Wettersteuerung: Jalousien oeffen (Regen)");
            fahren = true;
        }
        if(wetter && ((last_AS.hell < thrsHellOpen) || regen))
        {
            if(!drvTimer || ((time(NULL) - drvTimeStamp) >= (hystTime * 60)))
            {
                if(closeOnWetter)
                {
                    printLog("Wettersteuerung: Jalousien oeffen (keine Sonne mehr, oder Regen)");
                    fahren = true;
                }
            }
        }
        if(fahren)
        {
            for(i = 0; i < 4; i++)
            {
                jal->drvAufwaerts(i);
            }
            drvTimeStamp = time(NULL);
            drvTimer = true;
            fahren = false;
            closeOnWetter = false;
        }
    }
    //wenn alle ganz offen sind, wetterabhaengig schliessen
    if(jalauf)
    {
        if((wetter && !drvTimer) || (wetter && ((time(NULL) - drvTimeStamp) >= (hystTime * 60))))
        {
            if((last_AS.hell > thrsHellClose) && (last_KS.temp > thrsTemp) && (last_KS.wind < thrsWind) && !regen)
            {
                printLog("Wettersteuerung: Jalousien schliessen");
                fahren = true;
            }
        }
        if(fahren)
        {
            if(closeToSun)
            {
                jal->drvSonne(0);
                if(jal2) jal->drvSonne(1);
                jal->drvSonne(2);
                jal->drvSonne(3);
            }
            else
            {
                jal->drvAbwaerts(0);
                if(jal2) jal->drvAbwaerts(1);
                jal->drvAbwaerts(2);
                jal->drvAbwaerts(3);
            }
            drvTimeStamp = time(NULL);
            drvTimer = true;
            fahren = false;
            closeOnWetter = true;
        }
    }
}
//#endif //USE_REC868

RemoteFS20::RemoteFS20(E6TcpClient *ioE6)
{
    e6tcp = ioE6;
    msgqueue = new MsgQueue(msg_name);
}

void RemoteFS20::setSzene(char *Szene)
{
    char *devices;
    char *device;
    char iniKey[256];
    int Helligkeit;
    dictionary *hslight;
    hslight = iniparser_load(hslightpath);
    devices = iniparser_getstring(hslight, "system:devices", 0);
    device = strtok(devices, " ,");
    while(device != NULL)
    {
        strcpy(iniKey, Szene);
        strcat(iniKey, ":");
        strcat(iniKey, device);
        Helligkeit = iniparser_getint(hslight, iniKey, -1);
        if(Helligkeit > -1)
        {
            setLampeWert(device, Helligkeit);
        }
        device = strtok(NULL, " ,");
    }
    iniparser_freedict(hslight);
}

void RemoteFS20::setLampeStufe(char *Lampe, int HelligkeitsStufe)  //Stufen von 0 (Aus) bis 16 (volle Helligkeit)
{
    setLampeBefehl(Lampe, HelligkeitsStufe);
}

void RemoteFS20::setLampeWert(char *Lampe, int HelligkeitsWert)  //zwischen 0 und 100%
{
    int i;
    int Stufe = 0;
    if(HelligkeitsWert == 0)
    {
        setLampeStufe(Lampe, 0);
        return;
    }
    for(i = 625; i <= 10000; i += 625)
    {
        Stufe++;
        if(HelligkeitsWert * 100 <= i)
        {
            setLampeBefehl(Lampe, Stufe);
            break;
        }
    }
}

void RemoteFS20::setLampeBefehl(char *Lampe, int Befehl)
{
    char iniKey[256];
    char *Adresse;
    int intAdresse;
    char ecmd[32];
    mqu_entry msg;
    dictionary *hslight;
    hslight = iniparser_load(hslightpath);
    strcpy(iniKey, "system:");
    strcat(iniKey, Lampe);
    Adresse = iniparser_getstring(hslight, iniKey, 0);
    sscanf(Adresse, "%x", &intAdresse);

    if(intAdresse == 0)
    {
        //Steckdose Terrasse schalten
        msg.mtyp = CMD_STECKDOSE_TERRASSE;
        if(Befehl==FS20_AUS)
        {
            sprintf(msg.mtext, "value=AUS\n");
            msgqueue->writeToQueue(&msg); //Nachricht in die Queue stellen
        }else if(Befehl > FS20_AUS && Befehl <= FS20_AN){
            sprintf(msg.mtext, "value=AN\n");
            msgqueue->writeToQueue(&msg); //Nachricht in die Queue stellen
        }
    }else{
        //  fs20 send HOUSECODE ADDR CMD
        sprintf(ecmd, "fs20 send %X %X %X\n", hc, convertAddress(intAdresse), Befehl);
        e6tcp->sendEcmdrecOK(ecmd);
    }
    iniparser_freedict(hslight);
}

void RemoteFS20::resetLampeTimer(char *Lampe)
{
    char iniKey[256];
    char *Adresse;
    int intAdresse;
    char ecmd[32];
    dictionary *hslight;
    hslight = iniparser_load(hslightpath);
    strcpy(iniKey, "system:");
    strcat(iniKey, Lampe);
    Adresse = iniparser_getstring(hslight, iniKey, 0);
    sscanf(Adresse, "%x", &intAdresse);
//  fs20 send HOUSECODE ADDR CMD
    sprintf(ecmd, "fs20 send %X %X 3C 0\n", hc, convertAddress(intAdresse));
    e6tcp->sendEcmdrecOK(ecmd);
    usleep(400000);
    sprintf(ecmd, "fs20 send %X %X 3D 0\n", hc, convertAddress(intAdresse));
    e6tcp->sendEcmdrecOK(ecmd);
    usleep(400000);
    sprintf(ecmd, "fs20 send %X %X 36 0\n", hc, convertAddress(intAdresse));
    e6tcp->sendEcmdrecOK(ecmd);
    iniparser_freedict(hslight);
}

void RemoteFS20::toggleLampe(char *Lampe)
{
    setLampeBefehl(Lampe, FS20_TOGGLE);
}

void RemoteFS20::switchAlleLampen(bool value)
{
    char *devices;
    char *device;
    dictionary *hslight;
    hslight = iniparser_load(hslightpath);
    devices = iniparser_getstring(hslight, "system:devices", 0);
    device = strtok(devices, " ,");
    while(device != NULL)
    {
        setLampeStufe(device, (value?16:0));
        device = strtok(NULL, " ,");
    }
    iniparser_freedict(hslight);
}


void RemoteFS20::dimmLampeStart(char *Lampe, int DimmCmd)
{
    char iniKey[256];
    char *Adresse;
    int intAdresse;
    dictionary *hslight;
    thr_dimmTimer *timerData;
    timerData = (struct thr_dimmTimer *)malloc(sizeof(struct thr_dimmTimer));
    hslight = iniparser_load(hslightpath);
    strcpy(iniKey, "system:");
    strcat(iniKey, Lampe);
    Adresse = iniparser_getstring(hslight, iniKey, 0);
    sscanf(Adresse, "%x", &intAdresse);
    timerData->e6tcp_p = e6tcp;
    timerData->addr = convertAddress(intAdresse);
    timerData->hc = hc;
    if(DimmCmd == DIMM_DOWN_START)
        timerData->cmd = FS20_DIMM_DOWN;
    if(DimmCmd == DIMM_UP_START)
        timerData->cmd = FS20_DIMM_UP;
    iniparser_freedict(hslight);
    if(pthread_create(&thr_id, NULL, dimmTimer, timerData) != 0)
    {
        printErr(errno, "Timer-Thread fuer Dimmer konnte nicht gestartet werden.");
        return;
    }
    pthread_detach(thr_id);
}

void RemoteFS20::dimmLampeStop()
{
    pthread_cancel(thr_id);
    pthread_join(thr_id, NULL);
}

//Dimm-Timer-Thread fuer wiederholtes Senden des Dimm-Command
void *dimmTimer(void *arg)
{
    char ecmd[32];
    struct thr_dimmTimer *timerData = (struct thr_dimmTimer *)arg;

    pthread_cleanup_push(cleanupDimmTimer, (void *)timerData);
    if(verbose)
        printLog("Timer-Thread fuer Dimmer gestartet");
    int count = 0;
    while(count < 20)
    {
        //  fs20 send HOUSECODE ADDR CMD
        sprintf(ecmd, "fs20 send %X %X %X\n", timerData->hc, timerData->addr, timerData->cmd);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        timerData->e6tcp_p->sendEcmdrecOK(ecmd); //diesen Befehl zu Ende ausfuehren, bevor Thread beendet werden kann
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        count++;
        usleep(200000);
    }
    pthread_cleanup_pop(1);
    return NULL;
}

//Exithandler fuer Dimm-Timer-Thread
void cleanupDimmTimer(void *arg)
{
    struct thr_dimmTimer *timerData = (struct thr_dimmTimer *)arg;
    if(verbose)
        printLog("Timer-Thread fuer Dimmer ordnungsgemaess beendet.");
    free(timerData);
}



Hauscode::Hauscode()
{
    char *strHauscode;
    dictionary *conf;
    semop(conf_sem_ID, &LOCK, 1);
    conf = iniparser_load(confpath);
    strHauscode = iniparser_getstring(conf, "system:hauscode", 0);
    sscanf(strHauscode, "%4x%4x", &tmphc1, &tmphc2);
    iniparser_freedict(conf);
    semop(conf_sem_ID, &UNLOCK, 1);
    convertHauscode();
}

void Hauscode::convertHauscode()
{
    hc1 = 0;
    hc1=(hc1 + (((tmphc1 >> 12) & 0x000F)-1)) << 2;
    hc1=(hc1 + (((tmphc1 >> 8) & 0x000F)-1)) << 2;
    hc1=(hc1 + (((tmphc1 >> 4) & 0x000F)-1)) << 2;
    hc1=(hc1 + ((tmphc1 & 0x000F)-1));
    hc2 = 0;
    hc2=(hc2 + (((tmphc2 >> 12) & 0x000F)-1)) << 2;
    hc2=(hc2 + (((tmphc2 >> 8) & 0x000F)-1)) << 2;
    hc2=(hc2 + (((tmphc2 >> 4) & 0x000F)-1)) << 2;
    hc2=(hc2 + ((tmphc2 & 0x000F)-1));
    hc = (hc1 << 8) + hc2;
}

void Hauscode::setHauscode(int hauscode1, int hauscode2)
{
    tmphc1 = hauscode1;
    tmphc2 = hauscode2;
    convertHauscode();
}

int Hauscode::getHauscode()
{
    return hc;
}

int Hauscode::getHauscode(int *hauscode1, int *hauscode2)
{
    *hauscode1 = hc1;
    *hauscode2 = hc2;
    return hc;
}

int Hauscode::convertAddress(int Adresse)
{
    int fs20adr;
    fs20adr = 0;
    fs20adr=(fs20adr + (((Adresse >> 12) & 0x000F)-1)) << 2;
    fs20adr=(fs20adr + (((Adresse >> 8) & 0x000F)-1)) << 2;
    fs20adr=(fs20adr + (((Adresse >> 4) & 0x000F)-1)) << 2;
    fs20adr=(fs20adr + ((Adresse & 0x000F)-1));
    return fs20adr;
}

bool Hauscode::compareHauscode(int hauscode1, int hauscode2)
{
    return((hauscode1 == hc1) && (hauscode2 == hc2));
}


