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

//hier stehen alle allgemeinen Definitionen drin

#ifndef HS_CONFIG_H
#define HS_CONFIG_H


// Konstanten fuer Klasse Jalousie u.a.
#define STOP 0
#define UNDEF 0 //Jalousie befindet sich irgendwo zwischen Oben und Unten
#define AUF 1
#define AB 2
#define AUS 0
#define AN 1
#define TIMER_AN 2
//Fahrzeiten fuer Jalousie
#define FAHRZEIT 70000000 //Fahrzeit in usec
#define IMPULSZEIT 150000 //Impulszeit in usec
#define SONNENZEIT 350000 //Impulszeit um Sonnenstellung zu erreichen, in usec
//Konstanten fuer Klasse Abluft
#define AUS 0
#define LOW 1
#define HIGH 2
//Intervall fuer Tempabfrage
#define TEMP_INTERVAL 360000000 // in usec

//Module aktivieren
//#define USE_GUI
//#define USE_REC868
//#define USE_UDP

//#define LOGFILE

//Messagetypen definieren
#define NO_CMD 0
#define CMD_JAL 1
#define CMD_CMD 2
#define CMD_FLOOR 3
#define CMD_ABLUFT 4
#define CMD_HWR 5
#define CMD_LICHT 6
#define CMD_SZENE 7
#define CMD_WETT 8
#define CMD_HELL 9
#define CMD_GET_HSINFO 10
#define CMD_GET_CONFIG 11
#define CMD_SEND_HSINFO 12
#define CMD_SET_CONFIG 13 //einen Wert in die haussteuerung.conf schreiben
#define CMD_DIMM 14 //Dimm-Befehl fuer eine Lampe Format: value=essen:DIMM_UP_START
#define CMD_LICHT_RESET_TIMER 15 //loescht alle Timer im Dimmer
#define CMD_E6_INIT 16 //E6-Device muss neu initialisiert werden
#define CMD_WATER 17 //value=VENTIL_1_START; value=VENTIL_2_STOP; value=VENTIL_1_TIMER; value=VENTIL_2_TIMER:10 (Dauer in min)
#define CMD_STECKDOSE_TERRASSE 18 //Schaltbefehl fuer Steckdose Terrasse: value=AN; value=AUS

#define DIMM_UP_START 1
#define DIMM_UP_STOP 2
#define DIMM_DOWN_START 3
#define DIMM_DOWN_STOP 4

#define CONFIG_HS 1
#define CONFIG_LIGHT 2
#define LOGFILE 3
#define LOGFILE_INVERS 4

#define MSG_MAX_LENGTH 1024
#define MEM_MAX_SIZE 1024
#define MSG_PERM 0666
#define MEM_PERM 0666

#define GEO_LAENGE_BERLIN 13.5
#define GEO_BREITE_BERLIN 52.5

#define REGENFAKTOR 0.2469

//Adressen der I2C-Geraete
#define I2C_DS_HWR 158
#define I2C_DS_INNEN 144
#define I2C_MAX 32 //HWR
#define I2C_PCF 64 //Main
#define I2C_PCF_T 64 //Terrasse

//Bits des PCF8574 (Abluft und Orientierungslicht)
#define ORIENT 4
#define ABLUFT_0 0
#define ABLUFT_1 1
#define HSGUI_POWER 3
#define HSGUI_RESET 2

//Bits des PCF8574 (Terrasse)
#define VENTIL_1 0
#define VENTIL_2 1
#define STECKDOSE 3


//FS20
#define FS20_LEFT 0
#define FS20_RIGHT 17
#define FS20_LEFT_CONT 20
#define FS20_RIGHT_CONT 19

#define ADR_JAL_0 5
#define ADR_JAL_1 6
#define ADR_JAL_2 7
#define ADR_JAL_3 8
#define ADR_JAL_ALL 4
#define ADR_ORIENT 10
#define ADR_LICHT 12

#define MAIN_IS_CONNECTED 1
#define HWR_IS_CONNECTED 2
#define TERRASSE_IS_CONNECTED 4

// FS20-Befehle
#define FS20_AN 0x11
    // "an, alter Wert"
#define FS20_DIMM_UP 0x13
    // "Dim Up"
#define FS20_DIMM_DOWN 0x14
    // "Dim Down"
#define FS20_AN_100 0x10
    // "100% an"
#define FS20_AN_93 0x0F
    // "93.75% an"
#define FS20_AN_87 0x0E
    // "87.5% an"
#define FS20_AN_81 0x0D
    // "81.25% an"
#define FS20_AN_75 0x0C
    // "75% an"
#define FS20_AN_68 0x0B
    // "68.75% an"
#define FS20_AN_62 0x0A
    // "62.5% an"
#define FS20_AN_56 0x09
    // "56.25% an"
#define FS20_AN_50 0x08
    // "50% an"
#define FS20_AN_43 0x07
    // "43.75% an"
#define FS20_AN_37 0x06
    // "37.5% an"
#define FS20_AN_31 0x05
    // "31.25% an"
#define FS20_AN_25 0x04
    // "25% an"
#define FS20_AN_18 0x03
    // "18.75% an"
#define FS20_AN_12 0x02
    // "12.5% an"
#define FS20_AN_06 0x01
    // "6.25% an"
#define FS20_AUS 0x00
    // "Aus"
#define FS20_TOGGLE 0x12
    // "Toggle"
#define FS20_RESET 0x1B
    // "Reset"
#define FS20_DEL_OFF_TIMER 0x3D
    // "Delete Off-Timer" 61d
#define FS20_DEL_ON_TIMER 0x3C
    // "Delete On-Timer" 60d

// Initwert fuer Dimmer des Orientierungslichts
#define ORIENT_DIMM_INIT 20

#define TCP_BUFFSIZE 1024

typedef struct{
  long mtyp;
  char mtext[MSG_MAX_LENGTH];
} mqu_entry;


#endif
