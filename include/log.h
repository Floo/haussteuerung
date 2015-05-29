/***************************************************************************
 *   Copyright (C) 2007 by Jens,,,   *
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


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <sys/sem.h>

extern bool logfile;
extern char *logpath;
extern bool verbose;
extern int verbose_level;

extern int log_sem_ID;

extern struct sembuf LOCK;
extern struct sembuf UNLOCK;

void printLog(const char *str, ...);
void printLogVerbose(int verblev, const char *text, ...);
void printErr(int err_no, const char *text, ...);
int countLines(char *logpath, long **zeilen);
void limitFileSize(char *logpath, int sem_ID);
