#include "../include/guitcpserver.h"

GuiTcpServer::GuiTcpServer(int port)
{
    //Verbindung zur MessageQueue herstellen
    msgqueue = new MsgQueue(msg_name);
    // Create the TCP socket, can serve multiple clients: GUI, Webinterface...
    lasterror = 0;
    int opt = 1;
    memset(&tcpaddr, 0, sizeof(tcpaddr));       /* Clear struct */

    tcpaddr.sin_addr.s_addr = INADDR_ANY;
    tcpaddr.sin_port = htons(port);
    tcpaddr.sin_family = AF_INET;

    tcpsocket = socket(PF_INET, SOCK_STREAM, 0);
    if (tcpsocket < 0)
    {
        printErr(errno, "Failed to create TCP-socket (GUI)");
        lasterror = 1;
        return;
    }
    if(setsockopt(tcpsocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == -1)
    {
        printErr(errno, "Failed to set Socketoption SO_REUSEADDR to TCP-Socket (GUI)");
    }
    if (bind(tcpsocket, (struct sockaddr*)&tcpaddr, sizeof(tcpaddr)) < 0)
    {
        printErr(errno, "Failed to bind TCP-Server-Socket (GUI)");
        lasterror = 2;
        return;
    }

    if (listen(tcpsocket, 3) < 0)
    {
        printErr(errno, "Failed to listen with TCP-Server-Socket (GUI)");
        lasterror = 3;
        return;
    }
    //init client list
    clientlist.count = 0;
    clientlist.last_list_entry = NULL;
}

GuiTcpServer::~GuiTcpServer()
{
    //dtor
    close(tcpsocket);
}

//Inhalt des SharedMem an Client senden
int GuiTcpServer::sendHSInfo(char *buf, size_t buffsize)
{
    list_entry *lz;
    int nbRecClients = 0;

    for (lz = clientlist.last_list_entry; lz; lz = lz->next)
    {
        if(lz->recStatus)
        {
            write(lz->client, buf, buffsize);
            nbRecClients++;
        }
    }
    return nbRecClients;
}

// Nachricht vom Client lesen, und Message erzeugen
int GuiTcpServer::recvIfFdIsset(fd_set *fds, mqu_entry *msg)
{
    list_entry *lz;
    int bytes;
    char *pvalue, *nextCmd;
    msg->mtyp = NO_CMD;
    msg->mtext[0] = '\0';

    if (clientlist.count == 0)
    {
        return -1; //keinGUI-Client in der Liste
    }
    for (lz = clientlist.last_list_entry; lz; lz = lz->next)
    {
        //alle bekannten GUI-Clients ueberpruefen
        if(FD_ISSET(lz->client, fds))
        {
            bytes = read(lz->client, buffer, TCP_BUFFSIZE);
            if (bytes == 0) //Verbindung wurde geschlossen
            {
                //TCP-Client aus der verketteten Liste entfernen
                removeClientFromList(lz->client);
                printLogVerbose(3, "GUI-Client aus der Liste entfernt.");
            }
            else
            {
                //Message erzeugen
                buffer[bytes] = '\0'; //Stringende ergaenzen
                printLogVerbose(3, "TCP-Message empfangen: %s", buffer);
                nextCmd = strtok(buffer, "\n");
                while(nextCmd != NULL)
                {
                    if(strstr(nextCmd, "cmd=") && strstr(nextCmd, "value=")) //ist die Nachricht vollständig
                    {
                        pvalue = strstr(nextCmd, "value=");
                        msg->mtyp = strtol(strstr(nextCmd, "cmd=") + 4, &pvalue, 0);
                        //strcpy(msg->mtext, strstr(nextCmd, "value="));
                        sprintf(msg->mtext, "%s client=%d\n", strstr(nextCmd, "value="), lz->client);
                        nextCmd = strtok(NULL, "\n");
                        msgqueue->writeToQueue(msg); //Nachricht in die Queue stellen
                    }
                }
                msg->mtyp = NO_CMD; //Als Ergebnis wird keine Message zurückgegeben, da alle in der Queue stehen
                msg->mtext[0] = '\0';
            }
            return lz->client;
        }
    }
    return -1;
}


// alle Clientverbindungen in die Liste der Filedeskriptoren einfuegen
void GuiTcpServer::fillFdSet(fd_set *fds, int *max)
{
    list_entry *lz;

    for (lz = clientlist.last_list_entry; lz; lz = lz->next)
    {
        if (lz->client > *max)
            *max = lz->client;
        FD_SET(lz->client, fds);
    }
}

//fuegt STDIN zur Liste
void GuiTcpServer::addSdtinToList()
{
    addClientToList(STDIN_FILENO);
    printLogVerbose(3, "STDIN zur Liste hinzugefuegt.");
}

//Client aus der verketten Liste entfernen, wenn Verbindung geschlossen wurde
int GuiTcpServer::removeClientFromList(int client)
{
    list_entry *lz, *lst = NULL;

    if (!clientlist.count)
        return 1;

    for (lz = clientlist.last_list_entry; lz; lz = lz->next)
    {
        if (lz->client == client)
            break;
        lst = lz;
    }

    if (!lz)
        return 1;

    if (lst)
        lst->next = lz->next;
    else
        clientlist.last_list_entry = lz->next;

    free(lz);
    clientlist.count--;
    return 0;
}

// Socket zur Liste der Filedeskriptoren hinzufuegen
// damit select() auch ausgeloest wird, wenn neue Verbindung aufgebaut werden soll
void GuiTcpServer::addSocketToFdSet(fd_set *fds, int *max)
{
    FD_SET(tcpsocket, fds); //TCP Socket eintragen
    if (tcpsocket > *max)
        *max = tcpsocket;
}

//neue Client-Verbindung akzeptieren
int GuiTcpServer::acceptIfFdIsset(fd_set *fds)
{
    int client;
    if (FD_ISSET(tcpsocket, fds))
    {
        //TCP-Verbindungsanfrage beantworten
        client = accept(tcpsocket, NULL, 0);
        addClientToList(client); // neue Verbindung in die verkettete Liste aufnehmen
        return client;
    }
    else
    {
        return -1;
    }
}

// neue Verbindung in die verkettete Liste aufnehmen
void GuiTcpServer::addClientToList(int client)
{
    list_entry *entry;
    entry = (list_entry*)malloc(sizeof(*entry));
    if (entry == NULL)
    {
        printErr(errno, "malloc() failed in Funktion GuiTcpServer::addClient()");
        exit(1);
    }

    entry->client = client;
    entry->recStatus = false;
    entry->next = clientlist.last_list_entry;

    clientlist.last_list_entry = entry;
    clientlist.count++;
}

//markiert Clientverbindung, damit sie Inhalt der SharedMem bekommt, wenn er geaendert wird
void GuiTcpServer::setHSInfoFlag(int client)
{
    list_entry *lz;
    mqu_entry msg;

    for (lz = clientlist.last_list_entry; lz; lz = lz->next)
    {
        if(lz->client == client)
        {
            lz->recStatus = true;
            break;
        }
    }
    //...und das erste Mal Senden ausloesen
    msg.mtyp = CMD_SEND_HSINFO;
    msg.mtext[0] = '\0';
    msgqueue->writeToQueue(&msg); //Nachricht in die Queue stellen
}


//Inhalt des ConfigFiles an Client senden
void GuiTcpServer::sendConfig(int client, char *buf, size_t buffsize)
{
    write(client, buf, buffsize);

}


