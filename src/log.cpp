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

#include "log.h"



//Log ausgeben
void printLog(const char *text, ...){
  FILE *stdlog;
  time_t aktzeit;
  va_list arg_ptr;
  char *strTime;
  //erstmal die Puffer in die Datei schreiben
  fflush( stdout );
  fflush( stderr );

  va_start(arg_ptr, text);
  if(logfile){
    semop(log_sem_ID, &LOCK, 1);
    stdlog = fopen(logpath, "a");
  }else{
    stdlog = stdout;
  }
  time(&aktzeit);
  strTime = ctime(&aktzeit);
  strTime[strlen(strTime) - 1] = '\0';
  fprintf(stdlog, "%s  :: ", strTime);
  vfprintf(stdlog, text, arg_ptr);
  fprintf(stdlog, "\n");
  va_end(arg_ptr);
  if(logfile){
    fflush(stdlog);
    fclose(stdlog);
    semop(log_sem_ID, &UNLOCK, 1);
  }
}

//Log ausgeben, in Abhaengigkeit vom Verbose-Level, 0 wird immer ausgegeben
void printLogVerbose(int verblev, const char *text, ...){
  FILE *stdlog;
  time_t aktzeit;
  va_list arg_ptr;
  char *strTime;

  if( verblev == 0 || ( verbose && ( verblev <= verbose_level ))){
    //erstmal die Puffer in die Datei schreiben
    fflush( stdout );
    fflush( stderr );
    va_start(arg_ptr, text);
    if(logfile){
      semop(log_sem_ID, &LOCK, 1);
      stdlog = fopen(logpath, "a");
    }else{
      stdlog = stdout;
    }
    time(&aktzeit);
    strTime = ctime(&aktzeit);
    strTime[strlen(strTime) - 1] = '\0';
    fprintf(stdlog, "%s  :: ", strTime);
    vfprintf(stdlog, text, arg_ptr);
    fprintf(stdlog, "\n");
    va_end(arg_ptr);
    if(logfile){
      fflush(stdlog);
      fclose(stdlog);
      semop(log_sem_ID, &UNLOCK, 1);
    }
  }
}

//Error ausgeben
void printErr(int err_no, const char *text, ...){
  FILE *stdlog;
  time_t aktzeit;
  va_list arg_ptr;
  char *strTime;

  //erstmal die Puffer in die Datei schreiben
  fflush( stdout );
  fflush( stderr );

  va_start(arg_ptr, text);
  if(logfile){
    semop(log_sem_ID, &LOCK, 1);
    stdlog = fopen(logpath, "a");
  }else{
    stdlog = stderr;
  }
  time(&aktzeit);
  strTime = ctime(&aktzeit);
  strTime[strlen(strTime) - 1] = '\0';
  fprintf(stdlog, "%s  || ", strTime);
  vfprintf(stdlog, text, arg_ptr);
  //fprintf(stdlog, "\n");
  fprintf(stdlog, " || Fehlermeldung: %s\n", strerror(err_no));
  va_end(arg_ptr);
  if(logfile){
    fflush(stdlog);
    fclose(stdlog);
    semop(log_sem_ID, &UNLOCK, 1);
  }
}

int countLines(char *logpath, long **zeilen){
    FILE *stdlog;
    int zeilenAnzahl;
    char strLine[256];
    //erstmal die Puffer in die Datei schreiben
    fflush( stdout );
    fflush( stderr );

    if(logfile){
        stdlog = fopen(logpath, "r");
    }else{
        return 0;
    }
    zeilenAnzahl = 0;
    while(!feof(stdlog)){
        fgets(strLine, 256, stdlog);
        zeilenAnzahl++;
    }
    zeilenAnzahl--;
    (*zeilen) = (long*)malloc(zeilenAnzahl * sizeof(long));
    fseek(stdlog, 0, SEEK_SET);
    (*zeilen)[0] = ftell(stdlog);
    for (int i = 1; i < zeilenAnzahl; i++)
    {
        fgets(strLine, 256, stdlog);
        (*zeilen)[i] = ftell(stdlog);
    }
    fclose(stdlog);
    return zeilenAnzahl;
    //free(zeilen); //Darf erst nach Verwendung freigegeben werden
}

void limitFileSize(char *logpath, int sem_ID){
    FILE *stdlog;
    FILE *oldlog;
    FILE *newlog;
    long anzBytes;
    char *tempFileNameOld;
    char *tempFileNameNew;
    char *tempFileExtensionOld;
    char *tempFileExtensionNew;
    char extention[5];
    char strLine[256];
    int i, zeilenAnzahl;
    int zeilenZuKopieren = 101;
    long *zeilen;
    //erstmal die Puffer in die Datei schreiben
    fflush( stdout );
    fflush( stderr );
    if(logfile){
        semop(log_sem_ID, &LOCK, 1);
        stdlog = fopen(logpath, "r");
        if(stdlog == NULL)
        {
            //Datei existiert nicht
            semop(log_sem_ID, &UNLOCK, 1);
            return;
        }
    }else{
        return;
    }
    fseek(stdlog, 0, SEEK_END);
    anzBytes = ftell(stdlog);
    fclose(stdlog);
    if(anzBytes > 200000)
    {
        tempFileNameOld = (char*)malloc(strlen(logpath) * sizeof(char));
        tempFileNameNew = (char*)malloc(strlen(logpath) * sizeof(char));
        strcpy(tempFileNameNew, logpath);
        strcpy(tempFileNameOld, logpath);
        strncpy(extention, &logpath[strlen(logpath) - 4], 4);
        tempFileExtensionOld = strstr(tempFileNameOld, extention);
        tempFileExtensionNew = strstr(tempFileNameNew, extention);
        strncpy (tempFileExtensionNew, "._10", 4);
        remove(tempFileNameNew);
        for(i = 9; i > 0; i--)
        {
            sprintf(extention, "._%02i", i);
            strncpy(tempFileExtensionOld, extention, 4);
            rename(tempFileNameOld, tempFileNameNew);
            strncpy(tempFileExtensionNew, extention, 4);
        }
        strncpy (tempFileExtensionNew, "._00", 4);
        rename(logpath, tempFileNameNew);
        zeilenAnzahl = countLines(tempFileNameNew, &zeilen);
        oldlog = fopen(tempFileNameNew, "r");
        stdlog = fopen(logpath, "w");
        newlog = fopen(tempFileNameOld, "w");
        //fseek(oldlog, zeilen[zeilenAnzahl - zeilenZuKopieren], SEEK_SET);
        while(ftell(oldlog) < zeilen[zeilenAnzahl - zeilenZuKopieren])
        {
            fgets(strLine, 256, oldlog);
            fputs(strLine, newlog);
        }
        while(!feof(oldlog))
        {
            fgets(strLine, 256, oldlog);
            fputs(strLine, stdlog);
        }
        free(zeilen);
        fclose(oldlog);
        fclose(newlog);
        fclose(stdlog);
        remove(tempFileNameNew);
    }
    semop(log_sem_ID, &UNLOCK, 1);
}
