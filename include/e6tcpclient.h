#ifndef E6TCPCLIENT_H
#define E6TCPCLIENT_H

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/time.h>

#include "../include/log.h"
#include "../include/threadsafe.h"
#include "../include/hs_config.h"

#define RET_DUMMY 1
#define RET_PARSER_ERROR 2
#define RET_BUFFER 3
#define RET_OK 0
#define RET_ERROR -1

class E6TcpClient:public Threadsafe
{
    public:
        E6TcpClient(char *ipAddr, int port);
        virtual ~E6TcpClient();
        //ecmd senden, echo zurückgeben
        int sendEcmd(char *ecmd, char *retString);
        //ecmd senden, OK erwartet, echo zurückgeben
        int sendEcmdrecOK(char *ecmd, char *retString);
        //ecmd senden, echo wird erwartet
        int sendEcmd(char *ecmd);
        //ecmd senden, OK wird erwartet
        int sendEcmdrecOK(char *ecmd);
        int i2cDetect();
        int getLastError(){return lasterror;};
        int getSocketID(){return clientsocket;};
        const char *getIPAddress(){return ipAddress;};
        bool isDummy(){return dummy;};
        bool isConnected(){return connected;};
        int connectToServer();
        void closeConnection();
        void rebootE6();
    protected:
    private:
        bool dummy;
        bool connected;
        int lasterror;
        const char *tmp_ecmd;
        char *tmp_ret;
        char ipAddress[18];
        int clientsocket;
        struct sockaddr_in e6server;
        char buffer[TCP_BUFFSIZE];
        int received;
        int sendTo(bool recOK, bool hasReturnString);
};

#endif // E6TCPCLIENT_H
