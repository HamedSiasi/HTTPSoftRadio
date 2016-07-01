// Serial port driver for NB-IoT example application

#ifndef _SERIAL_DRIVER_H_
#define _SERIAL_DRIVER_H_

#include "stdint.h"
#include "stdio.h"
#include "mbed-drivers/mbed.h"

class SerialPort {
public:

    SerialPort(PinName tx = UART1_TX, PinName rx = UART1_RX, int baudrate = 9600);
    ~SerialPort();

    bool transmitBuffer(const char * pBuf);
    uint32_t receiveBuffer(char * pBuf, uint32_t lenBuf);
    int32_t receiveChar();

protected:
    Serial *pgUart = NULL;
};

#endif

// HamedSiasi
