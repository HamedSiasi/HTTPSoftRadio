// Serial port driver for NB-IoT example application

#include "stdint.h"
#include "stdio.h"
#include "mbed-drivers/mbed.h"
#include "example-mbedos-blinky/serial_driver.h"




// (1)
SerialPort::SerialPort(
		PinName tx     /*UART1_TX*/,
		PinName rx     /*UART1_RX*/,
		int baudrate   /*9600*/)
{

	pgUart = new Serial(tx, rx);
	pgUart->baud(baudrate);
	pgUart->format(8, SerialBase::None, 1);

}


// (2)
SerialPort::~SerialPort()
{
	delete (pgUart);
}



// (3) Send lenBuf bytes from pBuf over the serial port, returning true, in the case of success.
// Transmit lenBuf characters from pBuf over the serial port.
// Returns TRUE on success, otherwise FALSE.

bool SerialPort::transmitBuffer(const char *pBuf, uint32_t lenBuf)
{
	unsigned long result = 0;
    if(pgUart->writeable()){
    	result = pgUart->printf(pBuf);
        if (!result){
            printf ("[serial->transmitBuffer]  Transmit failed !!! \r\n");
        }
    }
    return (bool) result;
}



// (4) Get up to lenBuf bytes into pBuf from the serial port, returning the number of characters actually read.
// Receive up to lenBuf characters into pBuf over the serial port.
// Returns the number of characters received.

uint32_t SerialPort::receiveBuffer (char *pBuf, uint32_t lenBuf)
{
	unsigned long result = 0;
    if(pgUart->readable()){
    	result = pgUart->scanf("%s",pBuf);
        if (!result){
            printf ("[serial->receiveBuffer] Receive failed !!!.\r\n");
        }
    }
    return (uint32_t) result;
}



// (5) Read a single character from the serial port, returning -1 if no character is read.
// Receive a single character from the serial port.
// Returns -1 if there are no characters, otherwise it
// returns the character (i.e. it can be cast to char).

int32_t SerialPort::receiveChar()
{
	int32_t returnChar = -1;
	if( pgUart->readable() ){
    	returnChar = (int32_t) pgUart->getc();
    }
    return returnChar;
}

// HamedSiasi









