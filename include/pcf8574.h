
#include <stdio.h>

#include "e6tcpclient.h"
#include "../include/threadsafe.h"

#define I2C_WRITE 0
#define I2C_READ 1

#ifndef PCF8574_H
#define PCF8574_H

class PCF8574 : public Threadsafe{
	private:
		unsigned char adr; //Ger�eadresse
		unsigned char data; //aktuelller Schaltzustand der Ausg�ge
		E6TcpClient *e6tcp;
	protected:

	public:
    PCF8574(unsigned char address, E6TcpClient *e6);
    ~PCF8574(){};
		int init(void);
		int set(unsigned char port, bool state);
		int toggle(unsigned char port);
		int on(unsigned char port);
		int off(unsigned char port);
		int get(unsigned char port);
		unsigned char in(void);
		int out(unsigned char databyte);
};

#endif
