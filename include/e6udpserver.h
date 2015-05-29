#ifndef E6UDPSERVER_H
#define E6UDPSERVER_H

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <pthread.h>

#include "../include/log.h"
#include "../lib/iniparser-2.16/src/iniparser.h"
#include "../include/hs_config.h"
#include "../include/devices.h"
#include "../include/threadsafe.h"

#define UDP_BUFFSIZE 2048

extern char *confpath;
void *rebootTimer(void *arg);
struct client_t
{
    in_addr_t ip;
    int packetCountBuf[5];
    int pointer;
    bool rebootEnabled;
    pthread_t thrID;
};

class E6UdpServer : public Threadsafe
{
public:
    E6UdpServer(int port);
    virtual ~E6UdpServer();
    int getLastError()
    {
        return lasterror;
    };
    int getSocketID()
    {
        return serversocket;
    };
    void addSocketToFdSet(fd_set *fds, int *max);
    int recvIfFdIsset(fd_set *fds, mqu_entry *msg);
    void enableReboot(const char *ipAddress);
protected:
private:
    int lasterror;
    int serversocket;
    Hauscode *hauscode;
    struct sockaddr_in e6server;
    struct sockaddr_in e6client;
    char buffer[UDP_BUFFSIZE];
    unsigned int echolen, clientlen, serverlen;
    int received;
    client_t client[3];
    void convertUdpToMsg(mqu_entry *msg);
    void createRebootTimerThread(int i);
    void resetPacketCounter();
};

#endif // E6UDPSERVER_H
