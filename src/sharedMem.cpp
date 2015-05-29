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

#include "sharedMem.h"
#include "hs_config.h"


sharedMem::sharedMem()
{
    memID = shmget(mem_key, MEM_MAX_SIZE, MEM_PERM);
    strSHM = (char *)shmat(memID, NULL, 0);
    msgqueue = new MsgQueue(msg_name);
}

int sharedMem::updateHelper(const char *startTag, const char *strValue)
{
    semop(mem_sem_ID, &LOCK, 1);
    string strSHMTemp(strSHM);
    string strStartTag;
    string strEndTag;
    //char foo[16];
    //char *foo1;
    strStartTag = startTag;
    strEndTag = startTag;
    strEndTag.insert(1, "/");
    string strNew(strValue);
    size_t posStart = strSHMTemp.find(strStartTag) + strStartTag.length();
    size_t posEnd = strSHMTemp.find(strEndTag);
    strSHMTemp.replace(posStart, posEnd - posStart, strNew);
    int length = strSHMTemp.copy(strSHM, MEM_MAX_SIZE);
    strSHM[length] = '\0';
    semop(mem_sem_ID, &UNLOCK, 1);
    return(0);
}

void sharedMem::sendSharedMem()
{
    mqu_entry msg;
    msg.mtyp = CMD_SEND_HSINFO;
    msg.mtext[0] ='\0';
    msgqueue->writeToQueue(&msg);
}

sharedMem::~ sharedMem()
{
    shmdt(strSHM);
}

MsgQueue::MsgQueue(char *queueName)
{
    mq_attr msgq_attr;
    int bytes;
    pname = (char*)malloc(strlen(queueName)+1);
    strcpy(pname, queueName);
    msg_ID = mq_open(queueName, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG, NULL);
    if (msg_ID == (mqd_t)-1)
    {
        printErr(errno, "mq_open failed, Fehler beim Erstellen oder Oeffnen der Message-Queue!");
        exit(1);
    }
    mq_getattr(msg_ID, &msgq_attr);
    printLogVerbose(1, "Queue \"%s\" initialisiert:\n\t- stores at most %ld messages\n\t- large at most %ld bytes each\n\t- currently holds %ld messages\n", pname, msgq_attr.mq_maxmsg, msgq_attr.mq_msgsize, msgq_attr.mq_curmsgs);
    msg_recv_buffer_length = msgq_attr.mq_msgsize;
    buffer = (char*)malloc(msg_recv_buffer_length); //Empfangsbuffer
    //Sicherstellen, dass Queue leer ist bzw. leeren
    while(msgq_attr.mq_curmsgs > 0)
    {
        bytes = mq_receive(msg_ID, buffer, msg_recv_buffer_length, NULL);
        if(bytes == -1)
        {
            printErr(errno, "Fehler beim Lesen aus MessageQueue.");
            exit(1);
        }
        mq_getattr(msg_ID, &msgq_attr);
    }
}

MsgQueue::~MsgQueue()
{
    mq_close(msg_ID);
    mq_unlink(pname);
    free(pname);
}

int MsgQueue::readFromQueue(mqu_entry *mqu)
{
    int bytes;
    mqu->mtyp = NO_CMD;
    bytes = mq_receive(msg_ID, buffer, msg_recv_buffer_length, NULL);
    if(bytes == -1)
    {
        printErr(errno, "Fehler beim Lesen aus MessageQueue.");
        return -1;
    }
    pcmd = strstr(buffer, "cmd=");
    pvalue = strstr(buffer, "value=");
    if (pcmd == NULL || pvalue == NULL)
    {
        return -1;
    }
    mqu->mtyp = strtol(pcmd + 4, &pvalue, 0);
    strcpy(mqu->mtext, pvalue);
    return 0;
}

void MsgQueue::writeToQueue(mqu_entry *mqu)
{
    if (strstr(mqu->mtext, "value=") != NULL)
    {
        sprintf(buffer, "cmd=%ld %s", mqu->mtyp, mqu->mtext);
    }
    else
    {
        sprintf(buffer, "cmd=%ld value=%s", mqu->mtyp, mqu->mtext);
    }
    mq_send(msg_ID, buffer, strlen(buffer), 1);
}

void MsgQueue::addQueueToFdSet(fd_set *fds, int *max)
{
    FD_SET(msg_ID, fds); //Posix Message Queue eintragen
    if (msg_ID > *max)
    {
        *max = msg_ID;
    }
}

