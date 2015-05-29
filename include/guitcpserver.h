#ifndef GUITCPSERVER_H
#define GUITCPSERVER_H

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <mqueue.h>

#include "../include/log.h"
#include "../lib/iniparser-2.16/src/iniparser.h"
#include "../include/hs_config.h"
#include "../include/sharedMem.h"

//extern MsgQueue *msgQueue;
extern char *msg_name;

typedef struct list_entry_st
{
    int client;
    bool recStatus;
    struct list_entry_st *next;
} list_entry; //Listenelement, dass eine TCP-Verbindung zu GUI repraesentiert

typedef struct
{
    list_entry *last_list_entry; //Zeiger auf das letzte Element
    unsigned count; //
} list; //verkettete Liste aller TCP-Verbindungen zu Geraeten, die eine GUI anzeigen



class GuiTcpServer
{
public:
    GuiTcpServer(int port);
    virtual ~GuiTcpServer();
    int getLastError()
    {
        return lasterror;
    };
    int getSocketID()
    {
        return tcpsocket;
    };
    int sendHSInfo(char *buf, size_t buffsize);
    int recvIfFdIsset(fd_set *fd, mqu_entry *msg);
    void fillFdSet(fd_set *fd, int *max);//fuegt alle Clients zur Liste der Filedeskriptoren
    void addSdtinToList(); //fuegt STDIN zur Liste
    void addClientToList(int client); // neue Verbindung in die verkettete Liste aufnehmen
    int removeClientFromList(int client);
    void addSocketToFdSet(fd_set *fds, int *max);
    int acceptIfFdIsset(fd_set *fds);
    void setHSInfoFlag(int client);
    void sendConfig(int client, char *buf, size_t buffsize);
protected:
private:
    MsgQueue *msgqueue;
    int lasterror;
    int tcpsocket;
    struct sockaddr_in tcpaddr;
    list clientlist;
    char buffer[TCP_BUFFSIZE];
};

#endif // GUITCPSERVER_H
