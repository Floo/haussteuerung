#include "e6udpserver.h"

extern bool verbose;
extern int  verbose_level;

E6UdpServer::E6UdpServer(int port)
{
    int opt = 1;
    dictionary *conf;
    //ctor
    lasterror = 0;
    received = 0;
    /* Create the UDP socket */
    if ((serversocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        printErr(errno, "Failed to create UDP-socket (E6)");
        lasterror = 1;
        return;
    }
    if(setsockopt(serversocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == -1)
    {
        printErr(errno, "Failed to set Socketoption SO_REUSEADDR to UDP-Socket (E6)");
    }
    /* Construct the server sockaddr_in structure */
    memset(&e6server, 0, sizeof(e6server));       /* Clear struct */
    e6server.sin_family = AF_INET;                  /* Internet/IP */
    e6server.sin_addr.s_addr = htonl(INADDR_ANY);   /* Any IP address */
    e6server.sin_port = htons(port);       /* server port */
    /* Bind the socket */
    serverlen = sizeof(e6server);
    if (bind(serversocket, (struct sockaddr *) &e6server, serverlen) < 0)
    {
        printErr(errno, "Failed to bind UDP server socket (E6)");
        lasterror = 2;
        return;
    }
    semop(conf_sem_ID, &LOCK, 1);
    conf = iniparser_load(confpath);
    client[0].ip = inet_addr(const_cast<char*>(iniparser_getstring(conf, "network:ip_main", NULL)));
    client[1].ip = inet_addr(const_cast<char*>(iniparser_getstring(conf, "network:ip_hwr", NULL)));
    client[2].ip = inet_addr(const_cast<char*>(iniparser_getstring(conf, "network:ip_terrasse", NULL)));
    for(int i = 0; i < 3; i++)
    {
        client[i].rebootEnabled = true;
    }
    resetPacketCounter();
    iniparser_freedict(conf);
    semop(conf_sem_ID, &UNLOCK, 1);
    hauscode = new Hauscode();
}

void E6UdpServer::enableReboot(const char *ipAddress)
{
    int i;
    for(i = 0; i<3; i++)
    {
        if(client[i].ip == inet_addr(ipAddress))
        {
            client[i].rebootEnabled = true;
            return;
        }
    }
}

void E6UdpServer::resetPacketCounter()
{
    int i, j;
    for(i = 0; i < 3; i++)
    {
        client[i].pointer = 0;
        for(j = 0; j < 5; j++)
        {
            client[i].packetCountBuf[j] = -1;
        }
    }
}

E6UdpServer::~E6UdpServer()
{
    //dtor
    close(serversocket);
}


int E6UdpServer::recvIfFdIsset(fd_set *fds, mqu_entry *msg)
{
    int i, j;
    int packetCount;
    char *strEnd;
    SingleLock slock(this, true);
    if (FD_ISSET(serversocket, fds))
    {
        buffer[0] = '\0';
        /* Receive a message from the client */
        clientlen = sizeof(e6client);
        if((received = recvfrom(serversocket, buffer, UDP_BUFFSIZE, 0,
                                (struct sockaddr *) &e6client, &clientlen)) < 0)
        {
            printErr(errno, "Failed to receive message");
            return -1;
        }
        else
        {
            /* Send the message back to client */
            if (sendto(serversocket, "OK\n", 3, 0, (struct sockaddr *) &e6client,
                       sizeof(e6client)) != 3)
            {
                printErr(errno, "Mismatch in number of echo'd bytes");
            }
            buffer[received] = '\0';

            for(i = 0; i < 3; i++)
            {
                if(e6client.sin_addr.s_addr == client[i].ip)
                {
                    packetCount = (int)strtod(buffer, &strEnd);
                    if(client[i].rebootEnabled && strstr(buffer, "E6 UP"))
                    {
                        //das erste UDP-Paket nach dem reset des E6-Devices wurde empfangen ("E6 UP")
                        client[i].rebootEnabled = false;
                        createRebootTimerThread(i);
                        resetPacketCounter();
                        client[i].packetCountBuf[client[i].pointer] = packetCount;
                        client[i].pointer++;
                        convertUdpToMsg(msg);
                        return 0;
                    }
                    strEnd = buffer + 5;
                    for(j = 0; j < 5; j++)
                    {
                        if( packetCount == client[i].packetCountBuf[j])
                        {
                            //Paket ist schon einmal empfangen worden
                            msg->mtyp = NO_CMD;
                            break;
                        }
                    }
                    if(j == 5)
                    {
                        //Paket ist noch nicht empfangen worden
                        client[i].packetCountBuf[client[i].pointer] = packetCount;
                        client[i].pointer++;
                        if(client[i].pointer > 4) client[i].pointer = 0;
                        //Paket auswerten
                        convertUdpToMsg(msg);
                        return 0;
                    }
                    break;
                }
            }
        }
    }
    return -1;
}

void E6UdpServer::createRebootTimerThread(int i)
{
    if(pthread_create(&client[i].thrID, NULL, rebootTimer, (void *)&client[i]) != 0)
    {
        printErr(errno, "RebootTimer-Thread konnte nicht gestartet werden");
        return;
    }
    pthread_detach(client[i].thrID);
}

void *rebootTimer(void *arg)
{
    struct client_t *client = (struct client_t *)arg;
    if(verbose)
        printLog("RebootTimer-Thread gestartet");
    usleep(60000000); //1 min
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    client->rebootEnabled = true;
    //pthread_cleanup_pop(1);
    if(verbose)
        printLog("RebootTimer-Thread beendet");
    return NULL;
}

void E6UdpServer::addSocketToFdSet(fd_set *fds, int *max)
{
    FD_SET(serversocket, fds); //UDP server eintragen
    if (serversocket > *max)
        *max = serversocket;
}

void E6UdpServer::convertUdpToMsg(mqu_entry *msg)
{
    char *data;
    printLogVerbose(3, "UDP-Message empfangen: %s", buffer);
    if(strstr(buffer, "TAST"))
    {
        printLogVerbose(1, "***Taste empfangen.");
        msg->mtyp = CMD_ABLUFT;
        if(strstr(buffer, "LANG"))
        {
            strcpy(msg->mtext, "value=TOGGLELW");
        }
        if(strstr(buffer, "KURZ"))
        {
            strcpy(msg->mtext, "value=HIGH");
        }
    }
    if(strstr(buffer, "FS20"))
    {
        printLogVerbose(1, "FS20-Befehl empfangen.");
        unsigned char hc1, hc2, Adresse, Befehl_1, Befehl_2;
        char strValue[5];
        //Hauscode ueberpruefen
        strncpy(strValue, strstr(buffer, "HC1:") + 4, 3);
        strValue[3] = '\0';
        hc1 = atoi(strValue);
        strncpy(strValue, strstr(buffer, "HC2:") + 4, 3);
        strValue[3] = '\0';
        hc2 = atoi(strValue);
        if (hauscode->compareHauscode(hc1, hc2))
        {
            //Daten extrahieren
            strncpy(strValue, strstr(buffer, "ADDR:") + 5, 3);
            strValue[3] = '\0';
            Adresse = atoi(strValue);
            strncpy(strValue, strstr(buffer, "CMD:") + 4, 3);
            strValue[3] = '\0';
            Befehl_1 = atoi(strValue);
            strncpy(strValue, strstr(buffer, "EW:") + 3, 3);
            strValue[3] = '\0';
            Befehl_2 = atoi(strValue);
            printLogVerbose(1, "***FS20 empfangen. Adresse: %d, Befehl: %d, EW: %d", Adresse, Befehl_1, Befehl_2);
            if((Adresse >= ADR_JAL_ALL) && (Adresse <= ADR_JAL_3))
            {
                msg->mtyp = CMD_JAL;
                switch(Befehl_1)
                {
                case(FS20_LEFT):
                    strcpy(msg->mtext, "value=TAB_");
                    break;
                case(FS20_RIGHT):
                    strcpy(msg->mtext, "value=TAUF_");
                    break;
                case(FS20_LEFT_CONT):
                    strcpy(msg->mtext, "value=TIMPAB_");
                    break;
                case(FS20_RIGHT_CONT):
                    strcpy(msg->mtext, "value=TIMPAUF_");
                    break;
                }
                if(Adresse == ADR_JAL_ALL)
                {
                    sprintf(msg->mtext, "%s%s", msg->mtext, "ALL");
                }
                else
                {
                    sprintf(msg->mtext, "%s%d", msg->mtext, Adresse - ADR_JAL_0);
                }
            }
            if(Adresse == ADR_ORIENT)
            {
                msg->mtyp = CMD_FLOOR;
                switch(Befehl_1)
                {
                case(FS20_RIGHT):
                    strcpy(msg->mtext, "value=AN");
                    break;
                case(FS20_LEFT):
                    strcpy(msg->mtext, "value=AUS");
                    break;
                case(FS20_LEFT_CONT):
                    strcpy(msg->mtext, "value=DIMM_AB");
                    break;
                case(FS20_RIGHT_CONT):
                    strcpy(msg->mtext, "value=DIMM_AUF");
                }
            }
            if(Adresse == ADR_LICHT)
            {
                msg->mtyp = CMD_SZENE;
                switch(Befehl_1)
                {
                case(FS20_LEFT):
                    strcpy(msg->mtext, "value=ALLE_AUS");
                    break;
                case(FS20_RIGHT):
                    strcpy(msg->mtext, "value=ALLE_AN");
                }
            }
        }
    }
    if((data = strstr(buffer, "KS300")))
    {
        msg->mtyp = CMD_WETT;
        printLogVerbose(1, "Wetterdaten empfangen.");
        sprintf(msg->mtext, "value=%s", data + 6);
    }
    if((data = strstr(buffer, "AS2000")))
    {
        msg->mtyp = CMD_HELL;
        printLogVerbose(1, "Helligkeitsdaten empfangen.");
        sprintf(msg->mtext, "value=%s", data + 7);
    }
    if((data = strstr(buffer, "E6 UP")))
    {
        msg->mtyp = CMD_E6_INIT;
        printLogVerbose(1, "E6 initialisieren, IP: %s", inet_ntoa(e6client.sin_addr));
        sprintf(msg->mtext, "value=%s", inet_ntoa(e6client.sin_addr));
    }
}



