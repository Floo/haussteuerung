
#include <stdio.h>

#include "e6tcpclient.h"
#include "threadsafe.h"
#include "hs_config.h"

#ifndef MAX7311_H
#define MAX7311_H

//-- Register ------------------------------------------------------------------
#define IN_L     0x0
#define IN_H     0x1
#define OUT_L    0x2
#define OUT_H    0x3
#define POLINV_L 0x4
#define POLINV_H 0x5
#define DDR_L    0x6
#define DDR_H    0x7
#define TO_REG   0x8

#define I2C_WRITE 0
#define I2C_READ 1





class MAX7311 : public Threadsafe{
	private:
		unsigned char adr;
		E6TcpClient *e6tcp;
	protected:

	public:
    MAX7311(unsigned char address, E6TcpClient *e6);
    ~MAX7311(){};
		int initOut(void);
		int getDDRw(void);
		int setDDRw(int data);
		int setw(unsigned int data);
		int setb(unsigned char port, unsigned char data);
		int set(unsigned char port, bool state);
		int pulse(unsigned char port, int time_ms);
};


#endif
