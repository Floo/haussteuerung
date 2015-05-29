#include "e6tcpclient.h"

E6TcpClient::E6TcpClient(char *ipAddr, int port)
{
    //ctor
    /* Construct the server sockaddr_in structure */
    lasterror = 0;
    received = 0;
    clientsocket = -1;
    connected = false;
    strcpy(ipAddress, ipAddr);
    memset(&e6server, 0, sizeof(e6server));       /* Clear struct */
    if((dummy = (!strcmp(ipAddr, "0.0.0.0"))))
    {
        return;
    }
    e6server.sin_family = AF_INET;                  /* Internet/IP */
    e6server.sin_addr.s_addr = inet_addr(const_cast<char*>(ipAddr));  /* IP address */
    e6server.sin_port = htons(port);       /* server port */

    connectToServer();
}

E6TcpClient::~E6TcpClient()
{
    //dtor
    closeConnection();
}

void E6TcpClient::closeConnection()
{
    if(!dummy)
    {
        close(clientsocket);
    }
    connected = false;
}

int E6TcpClient::connectToServer()
{
    if(!dummy)
    {
        if(clientsocket != -1)
        {
            closeConnection();
        }
        /* Create the TCP socket */
        if ((clientsocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        {
            printErr(errno, "Failed to create TCP-socket (E6)");
            lasterror = 1;
            return(RET_ERROR);
        }
        /* Establish connection */
        if (connect(clientsocket, (struct sockaddr *) &e6server, sizeof(e6server)) < 0)
        {
            printErr(errno, "Failed to connect with server %s.", ipAddress);
            lasterror = 2;
            return(RET_ERROR);
        }
        connected = true;
        printLog("Connected with server %s.", ipAddress);
    }else{
        connected = true;
    }
    return(RET_OK);
}

//liefert Anzahl der I2C-Geraete zurück und schreibt Adressen in Logfile
int E6TcpClient::i2cDetect()
{
    char i2cBuffer[TCP_BUFFSIZE];
    char *pch;
    char ecmd[32];
    int ret = 0;
    //SingleLock slock(this, true);
    sprintf(ecmd, "i2c detect\n");
    if(sendEcmdrecOK(ecmd, i2cBuffer) == RET_OK)
    {
        pch = strtok (i2cBuffer,"\n");
        while (pch != NULL)
        {
            if(strcmp(pch, "OK") != 0)
            {
                ret++;
                printLog("I2C-Device at %s %s", ipAddress, pch);
            }
            pch = strtok (NULL, "\n");
        }
    }
    return ret;
}

//Rueckgabe: 0 = OK (OK empfangen), -1 = Netzwerkfehler, 2 = Parser-Error, 1 = Dummy
int E6TcpClient::sendEcmd(char *ecmd)
{
    int ret = RET_ERROR;
    SingleLock slock(this, true);
    tmp_ecmd = ecmd;
    ret = sendTo(false, false);
    //wenn Senden fehlgeschlagen, Verbindung neu aufbauen und nochmal senden
    if(ret == RET_ERROR)
    {
        if(connectToServer() == RET_OK)
        {
          ret = sendTo(false, false);
        }
    }
    return(ret);
}

//Rueckgabe: 0 = OK (beliebige Rueckgabe empfangen), -1 = Netzwerkfehler, 2 = Parser-Error, 1 = Dummy
int E6TcpClient::sendEcmdrecOK(char *ecmd)
{
    int ret = RET_ERROR;
    SingleLock slock(this, true);
    tmp_ecmd = ecmd;
    ret = sendTo(true, false);
    //wenn Senden fehlgeschlagen, Verbindung neu aufbauen und nochmal senden
    if(ret == RET_ERROR)
    {
        if(connectToServer() == RET_OK)
        {
          ret = sendTo(true, false);
        }
    }
    return(ret);
}

//Rueckgabe: 0 = OK, -1 = Netzwerkfehler, 2 = Parser-Error,
// 3 = retString enthaelt gueltigen String
int E6TcpClient::sendEcmd(char *ecmd, char *retString)
{
    SingleLock slock(this, true);
    int ret = RET_ERROR;
    tmp_ecmd = ecmd;
    tmp_ret = retString;
    ret = sendTo(false, true);
    //wenn Senden fehlgeschlagen, Verbindung neu aufbauen und nochmal senden
    if(ret == RET_ERROR)
    {
        if(connectToServer() == RET_OK)
        {
          ret = sendTo(false, true);
        }
    }
    return(ret);
}

//Rueckgabe: 0 = OK, -1 = Netzwerkfehler, 2 = Parser-Error, 1 = Dummy
int E6TcpClient::sendEcmdrecOK(char *ecmd, char *retString)
{
    SingleLock slock(this, true);
    int ret = RET_ERROR;
    tmp_ecmd = ecmd;
    tmp_ret = retString;
    ret = sendTo(true, true);
    //wenn Senden fehlgeschlagen, Verbindung neu aufbauen und nochmal senden
    if(ret == RET_ERROR)
    {
        if(connectToServer() == RET_OK)
        {
          ret = sendTo(true, true);
        }
    }
    return(ret);
}

//sendet ECMD und empfaengt einen String bis letztes Byte 0x0A ist
//mit recOK == TRUE werden mehrere Strings empfangen bis 'OK' kommt
//'parser error' bricht den Empfang ab
int E6TcpClient::sendTo(bool recOK, bool hasReturnString)
{
    /* Send the ecmd to the server */
    int echolen;
    fd_set readfds, writefds;
    struct timeval timeout;
    lasterror = 0;
    if(dummy)
    {
        buffer[0] = '\0';
        return(RET_DUMMY) ;
    }
    if(!connected)
    {
        return(RET_ERROR);
    }
    echolen = strlen(tmp_ecmd);
    //FDSET vorbereiten
    FD_ZERO(&writefds);
    FD_SET(clientsocket, &writefds);
    timeout.tv_sec = 10; // Timeout fuer Empfang in sec
    timeout.tv_usec = 0;
    select(clientsocket+1, NULL, &writefds, NULL, &timeout);
    if (!FD_ISSET(clientsocket, &writefds))
    {
        printErr(EHOSTUNREACH, "Connection lost during write to %s.", ipAddress);
        buffer[0] = '\0';
        connected = false;
        return(RET_ERROR);
    }

    if (send(clientsocket, tmp_ecmd, echolen, 0) != echolen)
    {
        printErr(errno, "Mismatch in number of sent bytes to %s", ipAddress);
        lasterror = 3;
        connected = false;
        return(RET_ERROR);
    }
    /* Receive the word back from the server */
    buffer[0] = '\0';
    int received = 0;
    int bytes = 0;
    int ret = 255;
    char *buff_pointer;
    //FDSET vorbereiten
    FD_ZERO(&readfds);
    FD_SET(clientsocket, &readfds);
    timeout.tv_sec = 10; // Timeout fuer Empfang in sec
    timeout.tv_usec = 0;
    while(ret == 255)
    {
        buff_pointer = &buffer[received];
        select(clientsocket+1, &readfds, NULL, NULL, &timeout);
        if (FD_ISSET(clientsocket, &readfds))
        {
            bytes = read(clientsocket, buff_pointer, TCP_BUFFSIZE - received - 1);
        }
        else
        {
            printErr(EHOSTUNREACH, "Connection to %s lost.", ipAddress);
            lasterror = 4;
            ret = RET_ERROR;
            connected = false;
            break;
        }
        if (bytes < 1)
        {
            printErr(errno, "Failed to receive bytes from %s", ipAddress);
            lasterror = 4;
            connected = false;
            ret = RET_ERROR;
        }
        received += bytes;
        buffer[received] = '\0';  /* Assure null terminated string */
        if(!recOK && strchr(buffer, '\n') != NULL)
        {
            ret = RET_BUFFER; //im Buffer tmp_ret steht ein gueltiger Wert
        }
        if(recOK && (strstr(buffer, "OK\n") || strstr(buffer, "ok\n")))
        {
            ret = RET_OK;
        }
        if(strstr(buffer, "parser error\n") || strstr(buffer, "parse error\n"))
        {
            ret = RET_PARSER_ERROR;
        }
    }
    //buffer[received] = '\0';  /* Assure null terminated string */
    if(hasReturnString)
    {
        strcpy(tmp_ret, buffer);
    }
    return(ret);
}

void E6TcpClient::rebootE6()
{
    if(dummy) return;
    printLog("Reboot E6-Device, IP: %s", ipAddress);
    char ecmd[32];
    sprintf(ecmd, "reset\n");
    sendEcmdrecOK(ecmd);
}


