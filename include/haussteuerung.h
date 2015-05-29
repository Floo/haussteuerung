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

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/errno.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>

#include "hs_config.h"
#include "iniparser.h"
#include "log.h"
#include "max7311.h"
#include "pcf8574.h"
#include "jalousie.h"
#include "abluft.h"
#include "temp.h"
#include "devices.h"
#include "socket.h"
#include "../include/e6udpserver.h"
#include "../include/e6tcpclient.h"
#include "../include/guitcpserver.h"



union semun {
  int val;                    /* value for SETVAL */
  struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
  unsigned short int *array;  /* array for GETALL, SETALL */
  struct seminfo *__buf;      /* buffer for IPC_INFO */
};

struct struct_jaltimerdata{
  struct timeval time;
  int iJalNr;
};

struct sun{
    int SA_hour;
    int SA_min;
    int SU_hour;
    int SU_min;
};

typedef void sigfunk(int);

struct sembuf LOCK = {0, -1, SEM_UNDO};
struct sembuf UNLOCK = {0, 1, SEM_UNDO};

//Funktionsprototypen
static void cleanup(void);
void *jalTimer(void *arg);
int jalousie_auf_zu(mqu_entry *msg);
void cleanup_jal_timer(void *arg);
void *cronTimer(void *arg);
void timeControl(struct tm *daytime);
void timeControlLicht(struct tm *daytime);
void sunTime(struct sun *SASU);
bool compareTimeString(char *strTime, struct sun *SASU, struct tm *daytime);
void msgsndAllJal(mqu_entry *message, bool jal_2_open);
void *abluftTimer(void *arg);
void tempControlHWR(void);
int jalControl(mqu_entry* msg);
static void progBeenden(int sig);
void show_help(void);
void writeUpdatesToSharedMem();
void init_E6TCPClient(in_addr_t ip);
void timestampAktualisieren();
int initSemaphore(int sem);
char* stringReplace(const char *charakter, const char *strReplace, char *strString);


