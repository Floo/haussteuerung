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
#ifndef SHAREDMEM_H
#define SHAREDMEM_H

#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <mqueue.h>
#include "../include/hs_config.h"
#include "../include/log.h"

extern int mem_sem_ID;
extern int mem_key;
extern struct sembuf LOCK;
extern struct sembuf UNLOCK;


using namespace std;



class MsgQueue
{
    private:
        mqd_t msg_ID; //ID der Message-Queue
        char *buffer;
        char *pcmd, *pvalue;
        char *pname;
        int msg_recv_buffer_length;
    public:
        MsgQueue(char *queueName);
        ~MsgQueue();
        int getMsgID(){return msg_ID;};
        void writeToQueue(mqu_entry *mqu);
        int readFromQueue(mqu_entry *mqu);
        void addQueueToFdSet(fd_set *fds, int *max);
};

class sharedMem{
  private:
    int memID;
    MsgQueue *msgqueue;
  public:
    char *strSHM;
    sharedMem();
    ~sharedMem();
    int updateHelper(const char *startTag, const char *strValue);
    void sendSharedMem();
};

//extern MsgQueue *msgQueue;
extern char *msg_name;

#endif


