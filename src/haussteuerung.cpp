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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "../include/haussteuerung.h"


extern int errno;

int mem_ID; //ID des sharedMemory
int mem_sem_ID; //ID der Semaphore fuer Zugriff auf shared Memory
int conf_sem_ID; ////ID der Semaphore fuer Zugriff auf .conf-File
int log_sem_ID; //ID der Semaphore fuer Zugriff auf .log-File
int wetter_sem_ID; //ID der Semaphore fuer Zugriff auf Wetter-Dateien

MsgQueue *msgQueue; //POSIX Message Queue

char *confpath;
char *wetterpath;
char *regenpath;
char *logpath;
char *hslightpath;
char *timestamppath;
bool logfile = false;
bool verbose = false;
int verbose_level = 0; //Verboselevel 0 ist nur die Standardausgabe,
//erweiterte Ausgaben bei Level 1...7

char *msg_name; //Name der Msg-Queue z.B. "/mymsg123"
int mem_key; //shared Memory Segment
int mem_sem; //die Semaphore fuer den schared Memory Zugriff
int conf_sem; //die semaphore fuer den Zugriff auf "haussteuerung.conf"
int log_sem; //die Semaphore fuer Zugriff auf "haussteuerung.log"
int wetter_sem; //die Semaphore fuer Zugriff auf "wetter.dbk" und "regen_1h.dbk

Temp *tempHWR;
Temp *tempWohnen;
Orient *orient;
HsGui *hsGui;
JALOUSIE *jal;
HWR *hwr;
Abluft *abluft;
RemoteFS20 *remoteFS20;
Wetter *wetter;
Terrasse *terrasse;

MAX7311 *max7311;
PCF8574 *pcf8574;
PCF8574 *pcf8574_terrasse;

char *ip;
int port;
E6TcpClient *e6TcpMain;
E6TcpClient *e6TcpHWR;
E6TcpClient *e6TcpTerrasse;
E6UdpServer *e6UdpServer;
GuiTcpServer *guiTcpServer;
fd_set fds;



int main(int argc, char *argv[])
{
    mqu_entry msg;
    int fds_max, client;
    int index;
    int JalNr, Helligkeit, DimmCmd, Dauer;
    char *tmp, *tmp1, *tmp2;
    char *strLampe = NULL;
    char *strSzene = NULL;
    char *strSHM;

    FILE *f;

    string strconfpath;
    dictionary *conf, *confHs;
    pthread_t cron_thread;

    DS1631 *ds1631[2];

    //Kommandozeilenargumente auswerten
    opterr = 0;
    while ((index = getopt (argc, argv, "hlv:")) != -1)
        switch (index)
        {
        case 'h':
            //Hilfe ausgeben
            show_help();
            exit(0);
        case 'v':
            verbose = true;
            sscanf(optarg, "%d", &verbose_level);
            if(verbose_level < 1 && verbose_level > 7)
            {
                show_help();
                exit(0);
            }
            break;
        case 'l':
            logfile = true;
            break;
        case '?':
            if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,
                         "Unknown option character `\\x%x'.\n",
                         optopt);
            show_help();
            exit(1);
        }
    if(optind < argc)
    {
        strconfpath.assign(argv[optind]);
        if(strconfpath.compare(strconfpath.length() - 1, 1, "/") != 0)
            strconfpath += '/';
    }
    string strtemp = strconfpath + "haussteuerung.conf";
    confpath = (char*)malloc(strtemp.length() + 1);
    strtemp.copy(confpath, strtemp.length());
    confpath[strtemp.length()] = '\0';
    strtemp = strconfpath + "wetter.dbk";
    wetterpath = (char*)malloc(strtemp.length() + 1);
    strtemp.copy(wetterpath, strtemp.length());
    wetterpath[strtemp.length()] = '\0';
    strtemp = strconfpath + "regen_1h.dbk";
    regenpath = (char*)malloc(strtemp.length() + 1);
    strtemp.copy(regenpath, strtemp.length());
    regenpath[strtemp.length()] = '\0';
    strtemp = strconfpath + "haussteuerung.log";
    logpath = (char*)malloc(strtemp.length() + 1);
    strtemp.copy(logpath, strtemp.length());
    logpath[strtemp.length()] = '\0';
    strtemp = strconfpath + "hslight.conf";
    hslightpath = (char*)malloc(strtemp.length() + 1);
    strtemp.copy(hslightpath, strtemp.length());
    hslightpath[strtemp.length()] = '\0';
    strtemp = strconfpath + "timestamp";
    timestamppath = (char*)malloc(strtemp.length() + 1);
    strtemp.copy(timestamppath, strtemp.length());
    timestamppath[strtemp.length()] = '\0';
    //Ende der Auswertung

    //Ausgabe des STDERR in LogFile umlenken
    if( logfile )
        freopen( logpath, "a+", stderr );

    printLog("Haussteuerung wird initialisiert...");
    //Initialisierungen
    conf = iniparser_load(confpath); //Konfiguration laden
    if (conf == NULL)
    {
        printErr(errno, "cannot parse haussteuerung.conf" );
        exit(1);
    }
    mem_key = iniparser_getint(conf, "system:mem_key", 10000);
    mem_sem = iniparser_getint(conf, "system:mem_sem", 10000);
    char *msg_name_temp = iniparser_getstring(conf, "system:msgqueue", NULL);
    msg_name = (char*)malloc(strlen(msg_name_temp));
    strcpy(msg_name, msg_name_temp);

    //zu Testen, ob Datei vorhanden, mal die Licht-Konfiguration laden
    confHs = iniparser_load(hslightpath);
    if (confHs == NULL)
    {
        printErr( errno, "cannot parse hslight.conf" );
        exit(1);
    }
    iniparser_freedict(confHs);

    //festlegen, was zuletzt noch getan werden muss
    atexit(cleanup);
    //..und das SIGINT (Strg-C) abfangen
    signal(SIGINT, progBeenden);


    //Semaphoren initialisieren
    struct stat confstat;
    stat(confpath, &confstat); //die ID wird aus dem Inode der Datei erzeugt
    conf_sem = confstat.st_ino;
    stat(logpath, &confstat); //die ID wird aus dem Inode der Datei erzeugt
    log_sem = confstat.st_ino;
    stat(wetterpath, &confstat); //die ID wird aus dem Inode der Datei erzeugt
    wetter_sem = confstat.st_ino;

    mem_sem_ID = initSemaphore(mem_sem); //fuer Zugriff auf SharedMemory
    log_sem = initSemaphore(log_sem); //fuer LogDatei
    conf_sem_ID = initSemaphore(conf_sem); //...und nochmal fuer den Zugriff auf die .conf-Datei
    wetter_sem_ID = initSemaphore(wetter_sem); //...und nochmal fuer den Zugriff auf die .dbk-Dateien

    //  ...und so wird zugegriffen:
    //  semop(conf_sem_ID, &LOCK, 1);
    //  semop(conf_sem_ID, &UNLOCK, 1);

    //  ret = semctl(mem_sem_ID, 0, GETVAL, 0);


    //Einrichten des SharedMemory
    mem_ID = shmget(mem_key, MEM_MAX_SIZE, MEM_PERM|IPC_CREAT|IPC_EXCL);
    if(mem_ID < 0)
    {
        if(errno == EEXIST)
        {
            //Memorysegment schon vorhanden, also erstmal uebernehmen, dann loeschen und ein neues anlegen
            mem_ID = shmget(mem_key, 1, MEM_PERM);
            if(mem_ID < 0)
            {
                printErr(errno, "Uebernehmen des vorhandenen Shared-Memory nicht erfolgreich!");
                exit(1);
            }
            else
            {
                if(shmctl(mem_ID, IPC_RMID, NULL) == 0)  //loeschen
                {
                    mem_ID = shmget(mem_key, MEM_MAX_SIZE, MEM_PERM|IPC_CREAT|IPC_EXCL); //2.Versuch
                }
                else
                {
                    mem_ID = -1;
                }
                if(mem_ID < 0)
                {
                    printErr(errno, "shmget failed, mem_id = %d\n Moeglicherweise wird der MEM_KEY schon verwendet.\n(.conf-File editiern!)", mem_ID);
                    exit(1);
                }
            }
        }
    }
    if((strSHM = (char *)shmat(mem_ID, NULL, 0)) == (void *)-1)
    {
        printErr(errno, "shmat failed, Fehler beim Zugriff auf Shared-Memory.");
        exit(1);
    }
    //Platzhalter fuer Shared-Memory
    sprintf(strSHM, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><hsinfo> <TempInnen></TempInnen> <posJal_0></posJal_0> <TempHWR></TempHWR> <TempAussen></TempAussen> "\
            "<Feuchte></Feuchte> <Wind></Wind> <Regen_1h></Regen_1h> <Regen_24h></Regen_24h> <Regen_7d></Regen_7d> <Symbol></Symbol> "\
            "<posJal_1></posJal_1> <posJal_2></posJal_2> <posJal_3></posJal_3> <drvJal_0></drvJal_0> <drvJal_1></drvJal_1> "\
            "<drvJal_2></drvJal_2> <drvJal_3></drvJal_3> <Orient></Orient> <HWR></HWR> <Abluft></Abluft> <EmpfangWetter></EmpfangWetter> "\
            "<EmpfangHell></EmpfangHell> <SA></SA> <SU></SU> <TCPConnected></TCPConnected> <Ventil_1></Ventil_1> <Ventil_2></Ventil_2> "\
            " <Steckdose></Steckdose></hsinfo>");


    //Einrichten der POSIX-Message-Queue
    msgQueue = new MsgQueue(msg_name);

    //Init Network
    ip = iniparser_getstring(conf, "network:ip_main", NULL);
    port = iniparser_getint(conf, "network:port_main", 2701);
    e6TcpMain = new E6TcpClient(ip, port);
    if(e6TcpMain->getLastError() != 0)
    {
        exit(1);
    }
    if(!e6TcpMain->isDummy())
    {
        char ecmd[32];
        e6TcpMain->i2cDetect();
        sprintf(ecmd, "rec868 start\n");
        e6TcpMain->sendEcmdrecOK(ecmd);
    }

    ip = iniparser_getstring(conf, "network:ip_hwr", NULL);
    port = iniparser_getint(conf, "network:port_hwr", 2701);
    e6TcpHWR = new E6TcpClient(ip, port);
    if(e6TcpHWR->getLastError() != 0)
    {
        exit(1);
    }
    if(!e6TcpHWR->isDummy())
    {
        e6TcpHWR->i2cDetect();
    }

    ip = iniparser_getstring(conf, "network:ip_terrasse", NULL);
    port = iniparser_getint(conf, "network:port_terrasse", 2701);
    e6TcpTerrasse = new E6TcpClient(ip, port);
    if(e6TcpTerrasse->getLastError() != 0)
    {
        exit(1);
    }
    if(!e6TcpTerrasse->isDummy())
    {
        e6TcpTerrasse->i2cDetect();
    }

    port = iniparser_getint(conf, "network:port_e6_udp", 2701);
    e6UdpServer = new E6UdpServer(port);
    if(e6UdpServer->getLastError() != 0)
    {
        exit(1);
    }
    port = iniparser_getint(conf, "hsgui:port_gui_tcp", 7000);
    guiTcpServer= new GuiTcpServer(port);
    if(guiTcpServer->getLastError() != 0)
    {
        exit(1);
    }

    iniparser_freedict(conf);

    //angeschlossene Geraete initialisieren
    ds1631[0] = new DS1631(I2C_DS_HWR, e6TcpHWR);
    ds1631[1] = new DS1631(I2C_DS_INNEN, e6TcpMain);
    max7311 = new MAX7311(I2C_MAX, e6TcpHWR);
    pcf8574 = new PCF8574(I2C_PCF, e6TcpMain);
    pcf8574_terrasse = new PCF8574(I2C_PCF_T, e6TcpTerrasse);

    //I2C-Geraete initialisieren
    max7311->initOut();
    pcf8574->init();
    pcf8574_terrasse->init();
    remoteFS20 = new RemoteFS20(e6TcpMain);
    wetter = new Wetter(wetterpath, regenpath);
    abluft = new Abluft(pcf8574, e6TcpMain);
    tempHWR = new Temp(ds1631[0], "TempHWR");
    tempWohnen = new Temp(ds1631[1], "TempInnen");
    jal = new JALOUSIE(max7311);
    hwr = new HWR(max7311);
    orient = new Orient(pcf8574);
    hsGui = new HsGui(pcf8574);
    terrasse = new Terrasse(pcf8574_terrasse);

    //Cron-Timer starten
    pthread_create(&cron_thread, NULL, cronTimer, NULL);

    printLog("Haussteuerung gestartet...");
    //alle Initialisierungen durchgefuehrt

    //Haupt-Message-Pipe: auf Message, UDP-Paket, oder GUI-TCP-Kommando warten...
    guiTcpServer->addSdtinToList(); //falls Eingabe von Kommandos ueber STDIN gebraucht wird
    while(1)
    {
        FD_ZERO(&fds); //Liste der Filedeskriptoren leeren
        fds_max = 0;
        client = 0;
        msg.mtyp = NO_CMD;
        guiTcpServer->fillFdSet(&fds, &fds_max); //alle Clients in fds-Liste eintragen
        guiTcpServer->addSocketToFdSet(&fds, &fds_max);
        e6UdpServer->addSocketToFdSet(&fds, &fds_max);
        msgQueue->addQueueToFdSet(&fds, &fds_max);

        select(fds_max + 1, &fds, NULL, NULL, NULL); //blockiert, bis irgendwo Daten eingetroffen sind
        if(verbose)
        {
            for (int sender = 0; sender <= fds_max; sender++)
            {
                if (FD_ISSET(sender, &fds))
                    printLogVerbose(3, "***SELECT: Daten von %d zur Auswertung eingegangen.***", sender);
            }
        }
        if((client = guiTcpServer->acceptIfFdIsset(&fds)) != -1)
        {
            guiTcpServer->addClientToList(client);
            printLogVerbose(3, "GUI-Client zur Liste hinzugefuegt.");
        }
        else if (e6UdpServer->recvIfFdIsset(&fds, &msg) != -1)
        {
            ;
        }
        else if(FD_ISSET(msgQueue->getMsgID(), &fds)) //in der POSIX-Message-Queue stehen Daten bereit
        {
            /* getting a message */
            msgQueue->readFromQueue(&msg);
        }
        else if ((client = guiTcpServer->recvIfFdIsset(&fds, &msg)) != -1)
        {
            ;
        }
        printLogVerbose(3, "Daten aus der Messagepipe zur Auswertung: cmd=%d %s", msg.mtyp, msg.mtext);
        if(msg.mtyp > NO_CMD)
        {
            switch(msg.mtyp)
            {
            case(CMD_CMD):
                //allg. Steuerbefehle
                //Groeße der Logfiles und Wetterfiles begrenzen
                limitFileSize(logpath, log_sem_ID);
                limitFileSize(wetterpath, wetter_sem_ID);
                limitFileSize(regenpath, wetter_sem_ID);
                break;
            case(CMD_JAL):
                //alle JalousieSteuerBefehle vom Webinterface
                tmp1 = (strstr(msg.mtext, "value=") + 6);
                tmp = strstr(tmp1, "_");

                if(strstr(tmp, "ALL") != NULL)
                {
                    msgsndAllJal(&msg, false);
                    break;
                }
                JalNr = ((int)*(tmp + 1)) - 48;
                if(strstr(tmp1, "STP") != NULL)
                {
                    jal->stop(JalNr);
                    break;
                }
                if(strstr(tmp1, "IMPAB") != NULL)
                {
                    jal->drvAbwaerts(JalNr, IMPULSZEIT);
                    break;
                }
                if(strstr(tmp1, "IMPAUF") != NULL)
                {
                    jal->drvAufwaerts(JalNr, IMPULSZEIT);
                    break;
                }
                if(strstr(tmp1, "TAUF") != NULL)
                {
                    jal->toggleAufwaerts(JalNr);
                    break;
                }
                if(strstr(tmp1, "TAB") != NULL)
                {
                    jal->toggleAbwaerts(JalNr);
                    break;
                }
                if(strstr(tmp1, "TIMPAB") != NULL)
                {
                    if(jal->getDrvState(JalNr) == STOP)
                    {
                        jal->drvAbwaerts(JalNr, IMPULSZEIT);
                    }
                    break;
                }
                if(strstr(tmp1, "TIMPAUF") != NULL)
                {
                    if(jal->getDrvState(JalNr) == STOP)
                    {
                        jal->drvAufwaerts(JalNr, IMPULSZEIT);
                    }
                    break;
                }
                if(strstr(tmp1, "AUF") != NULL)
                {
                    jal->drvAufwaerts(JalNr);
                    break;
                }
                if(strstr(tmp1, "AB") != NULL)
                {
                    jal->drvAbwaerts(JalNr);
                    break;
                }
                if(strstr(tmp1, "SUN") != NULL)
                {
                    jal->drvSonne(JalNr);
                }
                break;
            case(CMD_FLOOR):
                //Flurbeleuchtung
                tmp = (strstr(msg.mtext, "value=") + 6);
                if(strstr(tmp, "AN") != NULL)
                {
                    //Beleuchtung einschalten
                    orient->an();
                    break;
                }
                if(strstr(tmp, "AUS") != NULL)
                {
                    //Beleuchtung ausschalten
                    orient->aus();
                    break;
                }
                if(strstr(tmp, "DIMM_AUF") != NULL)
                {
                    //remoteFS20->dimmOrient(AUF);
                    break;
                }
                if(strstr(tmp, "DIMM_AB") != NULL)
                {
                    //remoteFS20->dimmOrient(AB);
                    break;
                }
            case(CMD_ABLUFT):
                //zentr. Abluft
                tmp = (strstr(msg.mtext, "value=") + 6);
                if(strstr(tmp, "TOGGLELW") != NULL)
                {
                    abluft->toggleLow();
                    break;
                }
                if(strstr(tmp, "LOW") != NULL)
                {
                    abluft->setLow(true);
                    break;
                }
                if(strstr(tmp, "HIGH") != NULL)
                {
                    abluft->setHigh();
                    break;
                }
                if(strstr(tmp, "AUS") != NULL)
                {
                    abluft->setLow(false);
                    break;
                }
                if(strstr(tmp, "UPDATE") != NULL)
                {
                    abluft->updateLow();
                    break;
                }
            case(CMD_HWR):
                //Abluft HWR
                tmp = (strstr(msg.mtext, "value=") + 6);
                if(strstr(tmp, "AN") != NULL)
                {
                    //AbluftHWR einschalten
                    hwr->an();
                }
                if(strstr(tmp, "AUS") != NULL)
                {
                    //AbluftHWR ausschalten
                    hwr->aus();
                }
                if(strstr(tmp, "UPDATE") != NULL)
                {
                    tempControlHWR();
                }
                break;
            case(CMD_WATER):
                //Bewaesserung
                //value=VENTIL_1_START; value=VENTIL_2_STOP; value=VENTIL_1_TIMER; value=VENTIL_2_TIMER:10 (Dauer in min)
                tmp = (strstr(msg.mtext, "value=") + 6);
                if(strstr(tmp, "VENTIL_1") != NULL)
                {
                    index = VENTIL_1;
                }
                else if(strstr(tmp, "VENTIL_2") != NULL)
                {
                    index = VENTIL_2;
                }
                else break;
                if(strstr(tmp, "START") != NULL)
                {
                    terrasse->setVentil(index, true);
                }
                else if(strstr(tmp, "STOP") != NULL)
                {
                    terrasse->setVentil(index, false);
                }
                else if(strstr(tmp, "TIMER:") != NULL)
                {
                    tmp1 = strstr(tmp, "TIMER:") + 6;
                    sscanf(tmp1, "%d", &Dauer);
                    terrasse->startVentilTimer(index, Dauer);
                }
                else if(strstr(tmp, "TIMER") != NULL)
                {
                    terrasse->startVentilTimer(index);
                }
                break;
            case(CMD_STECKDOSE_TERRASSE):
                tmp = (strstr(msg.mtext, "value=") + 6);
                if(strstr(tmp, "AN"))
                {
                    terrasse->setSteckdose(AN);
                }
                else if(strstr(tmp, "AUS"))
                {
                    terrasse->setSteckdose(AUS);
                }
                break;
            case(CMD_LICHT):
                tmp1 = (strstr(msg.mtext, "value=") + 6);
                tmp = strsep(&tmp1, ":");
                sscanf(tmp1, "%d", &Helligkeit);
                //Helligkeit = ((int)*(tmp1)) - 48;
                strLampe = (char*)realloc(strLampe, strlen(tmp) + 1);
                strcpy(strLampe, tmp);
                printLog("***Lampe \"%s\" auf %d setzen", strLampe, Helligkeit);
                //Lampe schalten
                remoteFS20->setLampeStufe(strLampe, Helligkeit);
                break;
            case(CMD_LICHT_RESET_TIMER):
                tmp1 = (strstr(msg.mtext, "value=") + 6);
                tmp = strsep(&tmp1, " ");
                strLampe = (char*)realloc(strLampe, strlen(tmp) + 1);
                strcpy(strLampe, tmp);
                printLog("***Alle Timer fuer Lampe \"%s\" ruecksetzen", strLampe);
                //Lampe schalten
                remoteFS20->resetLampeTimer(strLampe);
                break;
            case(CMD_SZENE):
                tmp1 = (strstr(msg.mtext, "value=") + 6);
                tmp = strsep(&tmp1, " ");
                if(strstr(tmp, "ALLE_AUS") != NULL)
                {
                    remoteFS20->switchAlleLampen(false);
                    break;
                }
                else if(strstr(tmp, "ALLE_AN") != NULL)
                {
                    remoteFS20->switchAlleLampen(true);
                    break;
                }
                else
                {
                    strSzene = (char*)realloc(strSzene, strlen(tmp) + 1);
                    strcpy(strSzene, tmp);
                    printLog("***Szene \"%s\" setzen", strSzene);
                    //Szene setzen
                    remoteFS20->setSzene(strSzene);
                    break;
                }
            case(CMD_DIMM):
                tmp1 = (strstr(msg.mtext, "value=") + 6);
                tmp = strsep(&tmp1, ":");
                sscanf(tmp1, "%d", &DimmCmd);
                strLampe = (char*)realloc(strLampe, strlen(tmp) + 1);
                strcpy(strLampe, tmp);
                if(verbose)
                {
                    switch(DimmCmd)
                    {
                    case(DIMM_DOWN_START):
                        printLog("***Lampe \"%s\" DIMM_DOWN_START", strLampe);
                        break;
                    case(DIMM_DOWN_STOP):
                        printLog("***Lampe \"%s\" DIMM_DOWN_STOP", strLampe);
                        break;
                    case(DIMM_UP_START):
                        printLog("***Lampe \"%s\" DIMM_UP_START", strLampe);
                        break;
                    case(DIMM_UP_STOP):
                        printLog("***Lampe \"%s\" DIMM_UP_STOP", strLampe);
                        break;
                    }
                }
                if(DimmCmd == DIMM_DOWN_START || DimmCmd == DIMM_UP_START)
                    remoteFS20->dimmLampeStart(strLampe, DimmCmd);
                if(DimmCmd == DIMM_DOWN_STOP || DimmCmd == DIMM_UP_STOP)
                    remoteFS20->dimmLampeStop();
                break;
            case(CMD_WETT):
                tmp = (strstr(msg.mtext, "value=") + 6);
                wetter->updateKS(tmp);
                break;
            case(CMD_HELL):
                tmp = (strstr(msg.mtext, "value=") + 6);
                wetter->updateAS(tmp);
                break;
            case(CMD_GET_HSINFO):
                if((tmp = strstr(msg.mtext, "client=")))
                {
                    guiTcpServer->setHSInfoFlag(atof(tmp + 7));
                }
                else
                {
                    guiTcpServer->setHSInfoFlag(client);
                }
                break;
            case(CMD_GET_CONFIG):
                FILE *configfile;
                long filesize, filesizeChk;
                int offset;
                long *zeilen;
                int zeilenAnzahl;
                int zeilenAnzahlAuszugeben;
                char *filebuffer;
                char startTag[8];
                bool semLogLocked;
                bool semConfLocked;
                configfile = NULL;
                semLogLocked = false;
                semConfLocked = false;
                tmp = (strstr(msg.mtext, "value=") + 6);
                if(atoi(tmp) == CONFIG_HS)
                {
                    semConfLocked = true;
                    semop(conf_sem_ID, &LOCK, 1);
                    configfile = fopen(confpath, "r");
                    strcpy(startTag, "hs_conf");
                }
                else if(atoi(tmp) == CONFIG_LIGHT)
                {
                    configfile = fopen(hslightpath, "r");
                    strcpy(startTag, "hslight");
                }
                else if(atoi(tmp) == LOGFILE || atoi(tmp) == LOGFILE_INVERS)
                {
                    semLogLocked = true;
                    semop(log_sem_ID, &LOCK, 1);
                    configfile = fopen(logpath, "r");
                    if(atoi(tmp) == LOGFILE)
                    {
                        strcpy(startTag, "logfile");
                    }else{
                        strcpy(startTag, "log_inv");
                    }
                }
                if(configfile == NULL)
                {
                    printErr(0, "Fehler beim Öffnen der Configdatei.");
                }
                else
                {
                    // obtain file size:
                    fseek(configfile, 0, SEEK_END);
                    filesize = ftell(configfile);
                    if((strcmp(startTag, "logfile") == 0) && (filesize > 10000))
                    {
                        filesize = 10000;
                        fseek(configfile, -10000, SEEK_END);
                    }
                    else if(strcmp(startTag, "log_inv") == 0)
                    {
                        zeilenAnzahlAuszugeben = 50;
                        zeilenAnzahl = countLines(logpath, &zeilen);
                        if(zeilenAnzahlAuszugeben > zeilenAnzahl)
                        {
                            zeilenAnzahlAuszugeben = zeilenAnzahl; //begrenzen
                        }
                        filesize = filesize - zeilen[zeilenAnzahl - zeilenAnzahlAuszugeben];
                    }
                    else
                    {
                        rewind (configfile);
                    }
                    // allocate memory to contain the whole file:
                    filebuffer = (char*) malloc (sizeof(char)*(filesize + 59));
                    if(filebuffer == NULL)
                    {
                        printErr(0, "Fehler bei Speicherallokation.");
                    }
                    sprintf(filebuffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><%s>", startTag);
                    if (strcmp(startTag, "log_inv") == 0)
                    {
                        offset = 0;
                        for(int i = 0; i < zeilenAnzahlAuszugeben; i++)
                        {
                            fseek(configfile, zeilen[zeilenAnzahl - i - 1], SEEK_SET);
                            offset = offset + strlen(fgets(filebuffer + 47 + offset, filesize - offset + 1, configfile));
                        }
                        free(zeilen);
                        filesizeChk = offset;
                    }
                    else
                    {
                        filesizeChk = (long)fread(filebuffer + 47, 1, filesize, configfile);
                    }
                    // copy the file into the buffer:
                    if(filesizeChk != filesize)
                    {
                        printErr(ferror(configfile), "Fehler beim Einlesen der Configdatei.");
                    }
                    else
                    {
                        sprintf(filebuffer + 47 + filesize, "</%s>\n", startTag);
                        filebuffer = stringReplace("&", "&amp;", filebuffer);  //Funktion noch nicht getestet
                        if((tmp = strstr(msg.mtext, "client=")))
                        {
                            guiTcpServer->sendConfig(atof(tmp + 7), filebuffer, strlen(filebuffer));
                        }
                        else
                        {
                            guiTcpServer->sendConfig(client, filebuffer, strlen(filebuffer));
                        }
                    }
                    fclose (configfile);
                    free (filebuffer);
                }
                if(semLogLocked)
                {
                    semop(log_sem_ID, &UNLOCK, 1);
                }
                if(semConfLocked)
                {
                    semop(conf_sem_ID, &UNLOCK, 1);
                }
                break;
            case(CMD_SEND_HSINFO):
                //char *sharedMemPointer;
                //sharedMemPointer = (char *)shmat(mem_ID, NULL, 0);
                //guiTcpServer->sendHSInfo(sharedMemPointer, strlen(sharedMemPointer));
                guiTcpServer->sendHSInfo(strSHM, strlen(strSHM));
                break;
            case(CMD_E6_INIT):
                tmp = (strstr(msg.mtext, "value=") + 6);
                init_E6TCPClient(inet_addr(tmp));
                break;
            case(CMD_SET_CONFIG):
                tmp = strstr(msg.mtext, "key=");
                tmp1 = strstr(msg.mtext, "value=") + 6;
                *tmp = '\0';
                tmp = tmp + 4;
                if((tmp2 = strstr(tmp, "client=")))
                {
                    tmp2 = tmp2 - 1;
                    *tmp2 ='\0';
                }
                dictionary *conf;
                semop(conf_sem_ID, &LOCK, 1);
                conf = iniparser_load(confpath);
                iniparser_setstr(conf, tmp, tmp1);
                f = fopen(confpath, "w");
                iniparser_dump_ini(conf, f);
                fclose(f);
                iniparser_freedict(conf);
                semop(conf_sem_ID, &UNLOCK, 1);
            }
        }
    }
    return EXIT_SUCCESS;
}

void init_E6TCPClient(in_addr_t ip)
{
    char ecmd[32];
    if(ip == inet_addr(e6TcpMain->getIPAddress()))
    {
        e6TcpMain->connectToServer();
        e6TcpMain->i2cDetect();
        sprintf(ecmd, "rec868 start\n");
        e6TcpMain->sendEcmdrecOK(ecmd);
        pcf8574->init();
        abluft->init();
        orient->init();
        hsGui->init();
        tempWohnen->init();
    }
    else if(ip == inet_addr(e6TcpHWR->getIPAddress()))
    {
        e6TcpHWR->connectToServer();
        e6TcpHWR->i2cDetect();
        max7311->initOut();
        jal->init();
        hwr->init();
        tempHWR->init();
    }
    else if(ip == inet_addr(e6TcpTerrasse->getIPAddress()))
    {
        e6TcpTerrasse->connectToServer();
        e6TcpTerrasse->i2cDetect();
        pcf8574_terrasse->init();
        terrasse->init();
    }
}


//Timer fuer Zeitablaufsteuerung
void *cronTimer(void *arg)
{
    struct tm  daytime;
    int min_alt;
    int day_alt;
    time_t time_tmp;

    printLog("Crontimer gestartet...");

    time_tmp = time(NULL);
    daytime = *localtime(&time_tmp);
    min_alt = 60; //damit beim ersten Mal alles ausgefuehrt wird
    day_alt = daytime.tm_mday;
    while(1)
    {
        while(min_alt == daytime.tm_min)
        {
            sleep(10);
            time_tmp = time(NULL);
            daytime = *localtime(&time_tmp);
        }
        min_alt = daytime.tm_min;
        // eine neue Minute ist da
//    printLog("Neue Minute wird ausgewertet.");
        timestampAktualisieren();
        timeControl(&daytime);
        timeControlLicht(&daytime);
        tempControlHWR();
        writeUpdatesToSharedMem();
        wetter->WetterSteuerung(jal);
        if(daytime.tm_min % 10 == 0)  //nur alle 10 Minuten
        {
            wetter->average();
            wetter->save();
        }
        if(daytime.tm_min == 0)
        {
            //1x pro Stunde
            wetter->refreshRegenmenge(daytime.tm_hour);
            wetter->save_regen_1h();
        }
        if(day_alt != daytime.tm_mday)
        {
            //neuer Tag ist angebrochen
            wetter->save_regen_1d();
            day_alt = daytime.tm_mday;
        }
    }
    return(NULL);
}


//Temperatursteuerung fuer AbluftHWR
void tempControlHWR(void)
{
    dictionary *conf;
    double temp_schwelle, ret = 0;
    char entry[32];
    mqu_entry msg;

    semop(conf_sem_ID, &LOCK, 1);
    conf = iniparser_load(confpath);
    if(conf == NULL)
    {
        printErr(errno, "cannot parse conf-File");
    }
    msg.mtyp = CMD_HWR;
    sprintf(entry, "hwr:threshold");
    temp_schwelle = iniparser_getdouble(conf, entry, ret);

    if((hwr->getState() == AN) && iniparser_getboolean(conf, "hwr:auto", 0) && ((tempHWR->getTempDbl() < (temp_schwelle - 0.5)) || (tempHWR->getTempDbl() < wetter->getTemp())))
    {
        strcpy(msg.mtext, "value=AUS");
        msgQueue->writeToQueue(&msg);
    }
    if((hwr->getState() == AUS) && (tempHWR->getTempDbl() > temp_schwelle) && (tempHWR->getTempDbl() > wetter->getTemp() + 2) && iniparser_getboolean(conf, "hwr:auto", 0))
    {
        strcpy(msg.mtext, "value=AN");
        msgQueue->writeToQueue(&msg);
    }
    if((hwr->getState() == AUS) && iniparser_getboolean(conf, "hwr:permanent", 0))
    {
        strcpy(msg.mtext, "value=AN");
        msgQueue->writeToQueue(&msg);
    }
    if((hwr->getState() == AN) && !(iniparser_getboolean(conf, "hwr:permanent", 0)) && !(iniparser_getboolean(conf, "hwr:auto", 0)))
    {
        strcpy(msg.mtext, "value=AUS");
        msgQueue->writeToQueue(&msg);
    }
    iniparser_freedict(conf);
    semop(conf_sem_ID, &UNLOCK, 1);
}

//Zeitueberprüfungen im Minutentakt für
//Licht
void timeControlLicht(struct tm *daytime)
{
    dictionary *hslight;
    char *strValue, *strSubValue, *strTime;
    char strKey[64];
    char strSection[16];
    int helligkeit;
    struct sun SASU;

    hslight = iniparser_load(hslightpath);
    if (hslight == NULL)
    {
        printErr(errno, "cannot parse file");
        exit(1);
    }
    //Sonnenauf- und -Untergang berechnen
    sunTime(&SASU);
    //
    strValue = iniparser_getstring(hslight, "system:devices", NULL);
    strSubValue = strtok(strValue, " ,");
    while(strSubValue != NULL)
    {
        sprintf(strSection, "timerOn");
        while(strSection[0] != '\0')
        {
            sprintf(strKey, "%s:%s", strSection, strSubValue);
            if((strTime = iniparser_getstring(hslight, strKey, NULL)) != NULL)
            {
                helligkeit = 0;
                if(strcmp(strSection, "timerOn") == 0)
                {
                    sprintf(strKey, "timerOnValue:%s", strSubValue);
                    if((helligkeit = iniparser_getint(hslight, strKey, -1)) == -1)
                    {
                        sprintf(strSection, "timerOff");
                        continue;
                    }
                }
                if(compareTimeString(strTime, &SASU, daytime))
                {
                    //Lampe schalten
                    remoteFS20->setLampeWert(strSubValue, helligkeit);
                }
            }
            if(strcmp(strSection, "timerOn") == 0)
            {
                sprintf(strSection, "timerOff");
            }
            else if(strcmp(strSection, "timerOff") == 0)
            {
                strSection[0] = '\0';
            }
        }
        strSubValue = strtok(NULL, " ,");
    }
    iniparser_freedict(hslight);
}

//Zeitueberprüfungen im Minutentakt für
//Jalousie, Orientierungslicht, Reset, Ventile...
void timeControl(struct tm *daytime)
{
    dictionary *conf;
    char strTime[6];
    struct sun SASU;
    mqu_entry message;

    semop(conf_sem_ID, &LOCK, 1);
    conf = iniparser_load(confpath);
    if (conf == NULL)
    {
        printErr(errno, "cannot parse file");
        exit(1);
    }
    //Sonnenauf- und -Untergang berechnen
    sunTime(&SASU);
    //Jalousien
    if(iniparser_getboolean(conf, "blinds:auto_time", 0))
    {
        message.mtyp = CMD_JAL;
        //Uhrzeiten verwenden
        if(compareTimeString(iniparser_getstr(conf, "blinds:down_time"), &SASU, daytime))
        {
            if(iniparser_getboolean(conf, "blinds:close_to_sun", false))
            {
                strcpy(message.mtext, "value=SUN_");
            }else{
                strcpy(message.mtext, "value=ABW_");
            }
            msgsndAllJal(&message, iniparser_getboolean(conf, "blinds:jal_2_open", false));
        }
        if(compareTimeString(iniparser_getstr(conf, "blinds:up_time"), &SASU, daytime))
        {
            strcpy(message.mtext, "value=AUF_");
            msgsndAllJal(&message, false);
        }
    }
    //Orientierungslicht Flur
    if(iniparser_getboolean(conf, "floor_lightning:auto_time", 0))
    {
        message.mtyp = CMD_FLOOR;
        //Uhrzeiten verwenden
        if(compareTimeString(iniparser_getstr(conf, "floor_lightning:on_time"), &SASU, daytime))
        {
            strcpy(message.mtext, "value=AN");
            msgQueue->writeToQueue(&message);
        }
        if(compareTimeString(iniparser_getstr(conf, "floor_lightning:off_time"), &SASU, daytime))
        {
            strcpy(message.mtext, "value=AUS");
            msgQueue->writeToQueue(&message);
        }
    }
    if(compareTimeString(iniparser_getstring(conf, "hsgui:display_wakeup", NULL), &SASU, daytime))
    {
        hsGui->resetDevice();
    }
    if(compareTimeString(iniparser_getstring(conf, "network:reboot_time_main", NULL), &SASU, daytime) && !e6TcpMain->isDummy())
    {
        e6UdpServer->enableReboot(e6TcpMain->getIPAddress());
        e6TcpMain->rebootE6();
    }
    if(compareTimeString(iniparser_getstring(conf, "network:reboot_time_hwr", NULL), &SASU, daytime) && !e6TcpHWR->isDummy())
    {
        e6UdpServer->enableReboot(e6TcpHWR->getIPAddress());
        e6TcpHWR->rebootE6();
    }
    if(compareTimeString(iniparser_getstring(conf, "network:reboot_time_terrasse", NULL), &SASU, daytime) && !e6TcpTerrasse->isDummy())
    {
        e6UdpServer->enableReboot(e6TcpTerrasse->getIPAddress());
        e6TcpTerrasse->rebootE6();
    }
    if(iniparser_getboolean(conf, "irrigation:ventil_1_auto", false))
    {
        if(compareTimeString(iniparser_getstring(conf, "irrigation:ventil_1_start", NULL), &SASU, daytime))
        {
            int dauer = 0;
            dauer = iniparser_getint(conf, "irrigation:ventil_1_duration", 0);
            if (dauer > 0)
            {
                message.mtyp = CMD_WATER;
                sprintf(message.mtext, "value=VENTIL_1_TIMER:%u", dauer);
                //strcpy(message.mtext, "value=VENTIL_1_TIMER");
                msgQueue->writeToQueue(&message);
            }
        }
    }
    if(iniparser_getboolean(conf, "irrigation:ventil_2_auto", false))
    {
        if(compareTimeString(iniparser_getstring(conf, "irrigation:ventil_2_start", NULL), &SASU, daytime))
        {
            int dauer = 0;
            dauer = iniparser_getint(conf, "irrigation:ventil_2_duration", 0);
            if (dauer > 0)
            {
                message.mtyp = CMD_WATER;
                sprintf(message.mtext, "value=VENTIL_2_TIMER:%u", dauer);
                //strcpy(message.mtext, "value=VENTIL_2_TIMER");
                msgQueue->writeToQueue(&message);
            }
        }
    }
    iniparser_freedict(conf);
    semop(conf_sem_ID, &UNLOCK, 1);
    //Um Mitternacht Systemaufgaben ausfueheren
    sprintf(strTime, "00:01");
    if(compareTimeString(strTime, &SASU, daytime))
    {
        message.mtyp = CMD_CMD;
        strcpy(message.mtext, "value=");
        msgQueue->writeToQueue(&message);
    }
}

//alle 4 Jalusien gleichzeitig fahren
void msgsndAllJal(mqu_entry *message, bool jal_2_open = false)
{
    int i, length;
    char strtmp[MSG_MAX_LENGTH];
    char *punderline;
    punderline = strstr(message->mtext, "_");
    length = punderline - message->mtext + 1;
    strncpy(strtmp, message->mtext, length);
    strtmp[length] = '\0';
    for(i = 0; i <= 3; i++)
    {
        if(!(i == 1 && jal_2_open))
        {
            sprintf(message->mtext, "%s%d", strtmp, i);
            msgQueue->writeToQueue(message);
        }
    }
}

//Charakter im String durch neuen String ersetzen
// TODO (florian#1#): Funktion testen und anwenden um Zeichen in XML-Dateien zu ersetzen (z.B. & durch &amp;)
//Funktion noch nicht getestet!!!!!!!!!!!
char* stringReplace(const char *charakter, const char *strReplace, char *strString) {
    char *tempString, *newString;
    char *searchStart;
    int anzahl = 0;
    int stringLen;

    //Anzahl der Vorkommen ermitteln
    searchStart = strstr(strString, charakter);
    while(searchStart != NULL) {
        anzahl++;
        searchStart = strstr(searchStart + 1, charakter);
    }
    if(anzahl == 0) //nichts zum Ersetzen gefunden
    {
        return strString;
    }

    stringLen = strlen(strString) + anzahl * (strlen(strReplace) - 1) + 1;
    newString = (char*) malloc(sizeof(char)*stringLen);
    if(newString == NULL)
    {
        printErr(0, "Fehler bei der Speicherallokation in stringReplace.");
    }
    newString[0] = '\0';

    tempString = strtok (strString, charakter);
    while (tempString != NULL) {
        strcat(newString, tempString);
        tempString = strtok (NULL, charakter);
        if(tempString != NULL) {
            strcat(newString, strReplace);
        }
    }
    free(strString);
    return newString;
}

//Vergleich der aktuellen Zeit mit Steuerzeiten in conf-Datei
//Zeitformat: SA+120; SU-10; 22:30; SA
bool compareTimeString(char *strTime, struct sun *SASU, struct tm *daytime)
{
    char *strTimeSun, *str_end;
    int min = 0;
    int hour;
    int min_offset;
    if(strTime != NULL)
    {
        if(strstr(strTime, ":"))
        {
            hour = strtol(strTime, &str_end, 10);
            min = strtol(str_end + 1, NULL, 10);
            min = min + hour * 60;
        }
        else
        {
            if((strTimeSun = strstr(strTime, "SA")) != NULL)
            {
                min = SASU->SA_hour * 60 + SASU->SA_min;
            }
            else if((strTimeSun = strstr(strTime, "SU")) != NULL)
            {
                min = SASU->SU_hour * 60 + SASU->SU_min;
            }
            min_offset = strtol(strTimeSun + 2, NULL, 10);
            min += min_offset;
        }
        return(daytime->tm_hour * 60 + daytime->tm_min == min);
    }
    return(false);
}

// Berechnung von Sonnenauf- und untergangszeiten
void sunTime(struct sun *SASU)
{
    /*    Formenln aus http://lexikon.astronomie.info/zeitgleichung/ */
    double Zeitgleiche, B, Deklination, Zeitdifferenz, AufgangOZ, UntergangOZ, Aufgang, Untergang;
    time_t time_tmp;
    struct tm daytime;

    time_tmp = time(NULL);
    daytime = *localtime(&time_tmp);
    Zeitgleiche = -0.1752 * sin(0.033430 * daytime.tm_yday + 0.5474) - 0.1340 * sin(0.018234 * daytime.tm_yday - 0.1939);
    B = M_PI * GEO_BREITE_BERLIN / 180;
    Deklination = 0.40954 * sin(0.0172 * (daytime.tm_yday - 79.35));
    Zeitdifferenz = 12 * acos((sin(-0.0145) - sin(B) * sin(Deklination)) / (cos(B) * cos(Deklination))) /  M_PI;
    AufgangOZ = 12 - Zeitdifferenz - Zeitgleiche;
    UntergangOZ = 12 + Zeitdifferenz - Zeitgleiche;
    Aufgang = AufgangOZ - GEO_LAENGE_BERLIN /15 + 1;
    Untergang = UntergangOZ - GEO_LAENGE_BERLIN /15 + 1;
    if(daytime.tm_isdst)  //Sommenzeit
    {
        Aufgang = Aufgang + 1;
        Untergang = Untergang + 1;
    }
    SASU->SA_hour = (int)floor(Aufgang);
    SASU->SU_hour = (int)floor(Untergang);
    SASU->SA_min = (int)((Aufgang - floor(Aufgang)) * 60);
    SASU->SU_min = (int)((Untergang - floor(Untergang)) * 60);
}

//SA und SU und TCP-Connection-Status der E6-Devices in SharedMem schreiben
void writeUpdatesToSharedMem()
{
    static sharedMem mem;
    char strSA[6];
    char strSU[6];
    char strFlag[4];
    struct sun SASU;
    int flag = 0;
    sunTime(&SASU);
    sprintf(strSA, "%02d:%02d", SASU.SA_hour, SASU.SA_min);
    sprintf(strSU, "%02d:%02d", SASU.SU_hour, SASU.SU_min);
    mem.updateHelper("<SA>", strSA);
    mem.updateHelper("<SU>", strSU);

    if(e6TcpMain->isConnected() && !e6TcpMain->isDummy()) flag += MAIN_IS_CONNECTED;
    if(e6TcpHWR->isConnected() && !e6TcpHWR->isDummy()) flag += HWR_IS_CONNECTED;
    if(e6TcpTerrasse->isConnected() && !e6TcpTerrasse->isDummy()) flag += TERRASSE_IS_CONNECTED;
    sprintf(strFlag, "%03d", flag);
    mem.updateHelper("<TCPConnected>", strFlag);

    mem.sendSharedMem();
}

void timestampAktualisieren()
{
    FILE *f;
    f=fopen(timestamppath, "w");
    fclose(f);
}

int initSemaphore(int sem)
{
    int sem_ID;
    // Einrichten der Semaphoren fuer Nutzung in PHP
    // PHP benoetigt einen Set aus 3 Semaphoren bei dem die 2. immer > 0 sein muss (gibt die Anzahl der
    // zugreifenden Prozesse an)
    sem_ID = semget(sem, 3, IPC_CREAT | 0666);
    if(sem_ID < 0)
    {
        //einen Versuch haben wir noch
        if(errno == 22)
        {
            sem_ID = semget(sem, 1, IPC_CREAT | 0666); // Uebernehmen
            if(sem_ID < 0)
            {
                printErr(errno, "Uebernehmen der vorhandenen Semophore %i nicht erfolgreich.", sem);
                exit(1);
            }
            else
            {
                if(semctl(sem_ID, 1, IPC_RMID, 0) == 0)  //und Loeschen
                {
                    sem_ID = semget(sem, 3, IPC_CREAT | 0666); // und nochmal probieren
                }
                else
                {
                    sem_ID = -1;
                }
                if(sem_ID < 0)
                {
                    printErr(errno, "Semaphore %i konnte nicht initialisiert werden.", sem);
                    exit(1);
                }
            }
        }
    }
    //Semaphoren-Set initialisieren
    union semun semarg;
    semarg.val = 1;
    semctl(sem_ID, 0, SETVAL, semarg);
    semctl(sem_ID, 1, SETVAL, semarg);
    return sem_ID;
}


void progBeenden(int sig)
{
    signal(SIGINT, SIG_IGN);
    cleanup();
    _exit(0);
}

//Exithandler fuer main-Thread
static void cleanup(void)
{
    //alle Netzwerkverbindungen schliessen
    delete e6TcpMain;
    delete e6TcpHWR;
    delete e6TcpTerrasse;
    delete e6UdpServer;
    delete guiTcpServer;

    if(shmctl(mem_ID, IPC_RMID, NULL) == -1)
    {
        printErr(errno, "Loeschen des Shared Memory gescheitert. Fehlende Benutzerrechte.");
    }
    if(semctl(mem_sem_ID, 1, IPC_RMID, 0) == -1)
    {
        printErr(errno, "Loeschen der Memory-Semaphore gescheitert. Fehlende Benutzerrechte.");
    }
    if(semctl(conf_sem_ID, 1, IPC_RMID, 0) == -1)
    {
        printErr(errno, "Loeschen der Conf-File-Semaphore gescheitert. Fehlende Benutzerrechte.");
    }
    printLog("Haussteuerung wird beendet...");
}

void show_help(void)
{
    printf("\nProgrammaufruf: haussteuerung [OPTION] [PFAD]\n"\
           "Folgende Optionen stehen"\
           "Ihnen zur Verfuegung:\n\n"\
           "\t-v [VB]  Verbose ein\n"\
           "\t-l  alle Meldungen werden ins Logfile ausgegeben\n"\
           "\t-h  Dieser Text\n"\
           "VB Verboselevel 1...7\n"\
           "PFAD des Arbeitsverzeichnisses\n\n");
}




