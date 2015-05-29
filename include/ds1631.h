
#include <stdio.h>

#include "e6tcpclient.h"
#include "../include/threadsafe.h"

#define I2C_WRITE 0
#define I2C_READ 1


#ifndef DS1631_H
#define DS1631_H

class DS1631 : public Threadsafe{
	private:
		unsigned char adr;
		E6TcpClient *e6tcp;
	protected:

	public:
        DS1631(unsigned char address, E6TcpClient *e6);
		int init(void);
		double read(void);
		int stop(void);
};

#endif
