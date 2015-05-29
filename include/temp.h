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
#include <sys/errno.h>
#include <unistd.h>
#include <math.h>


#ifndef TEMP_H
#define TEMP_H

#include "hs_config.h"
#include "threadsafe.h"
#include "ds1631.h"
#include "log.h"
#include "sharedMem.h"

extern bool verbose;
extern int verbose_level;

void *tempTimer(void *arg);

class Temp : public Threadsafe, protected sharedMem{
  private:
    double temperature;
    DS1631 *ds;
    pthread_t thr_id;
    char strTag[16];
  public:
    int init();
    Temp(DS1631 *io, const char* tag);
    double getTempDbl(void);
    int getTemp(void);
    void update(void);
    void updateSHM();
};

#endif


